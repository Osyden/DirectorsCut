"""CPU-only view of the actual compressed-cliff core and generated VDB frame.

This reads native float VDB densities, not Unreal's imported UNorm8 SVT. It is
an attachment/lighting diagnostic; it does not predict the native renderer.
"""
import argparse
import json
import math
from pathlib import Path
import sys
sys.dont_write_bytecode=True

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0,str(Path(__file__).resolve().parent))
from attach_compressed_cliff_waterfall import load,sha
from review_eroded_cliff_sector import reset_scene,camera_at,label_camera


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--attachment',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--frame',type=int,default=12);p.add_argument('--samples',type=int,default=32)
    p.add_argument('--without-volumes',action='store_true')
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
    if a.output.exists() or 'saved' not in [s.lower() for s in a.output.resolve().parts]:p.error('Choose new Saved output')
    report=json.loads((a.attachment/'attachment.json').read_text());profile=json.loads((a.attachment/'waterfall_profile.json').read_text())
    cliff=load(report['source_path']);water=load(report['water_path'])
    if sha(report['source_path'])!=report['source_sha256'] or sha(report['water_path'])!=report['water_sha256']:raise ValueError('Source changed since attachment')
    origin=np.asarray(report['frame_origin_arena_m']);B=np.stack([report[k] for k in ('basis_u_arena','basis_v_arena','basis_z_arena')],axis=1)
    scene=reset_scene(1440,960,a.samples)
    scene.cycles.volume_bounces=1
    def material(name,color,rough):
        m=bpy.data.materials.new(name);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF')
        bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
        return m
    mats=[material('WetCore',(.095,.20,.22),.22),material('AeratedCore',(.68,.76,.77),.55),material('ReceivingFoam',(.75,.80,.80),.75)]
    def mesh(name,data,materials,indices=None):
        me=bpy.data.meshes.new(name);q=(data['vertices_m']-origin)@B
        me.from_pydata(q.tolist(),[],data['triangles'].tolist());me.update()
        ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
        for m in materials:me.materials.append(m)
        for i,f in enumerate(me.polygons):f.use_smooth=True;f.material_index=int(indices[i]) if indices is not None else 0
    mesh('UnchangedSigma6',cliff,[material('NeutralClay',(.33,.33,.33),.85)])
    mesh('MeasuredMeanWater',water,[material('MeanWater',(.075,.12,.14),.8)])
    core=load(a.attachment/'core.npz');mesh('ActualCore',core,mats,core['face_material_index'])
    foam=load(a.attachment/'receiving_foam.npz');mesh('ActualReceivingFoam',foam,[mats[2]])
    frames=[]
    if not a.without_volumes:
        for name,extinction in [('Falling',1.6),('Impact',.4)]:
            path=a.attachment/'Volumes'/name/f'{name}_{a.frame:04d}.vdb'
            if not path.is_file():raise ValueError(f'Missing actual VDB frame {path}')
            volume=bpy.data.volumes.new(name);volume.filepath=str(path.resolve());volume.grids.load()
            ob=bpy.data.objects.new(name,volume);bpy.context.collection.objects.link(ob)
            ob.location=(np.asarray(profile['local_origin_arena_cm'])*.01-origin)@B
            ob.rotation_euler=(0,0,math.pi);ob.scale=(.01,.01,.01)
            m=bpy.data.materials.new(name+' physical scattering');m.use_nodes=True;nodes=m.node_tree.nodes;nodes.clear()
            out=nodes.new('ShaderNodeOutputMaterial');vol=nodes.new('ShaderNodeVolumePrincipled')
            attr=nodes.new('ShaderNodeAttribute');attr.attribute_name='density'
            multiply=nodes.new('ShaderNodeMath');multiply.operation='MULTIPLY';multiply.inputs[1].default_value=extinction
            vol.inputs['Color'].default_value=(.85,.90,.92,1);vol.inputs['Anisotropy'].default_value=.25
            vol.inputs['Density Attribute'].default_value=''
            m.node_tree.links.new(attr.outputs['Fac'],multiply.inputs[0]);m.node_tree.links.new(multiply.outputs[0],vol.inputs['Density']);m.node_tree.links.new(vol.outputs['Volume'],out.inputs['Volume']);volume.materials.append(m)
            frames.append(dict(path=str(path.resolve()),sha256=sha(path),density_extinction_per_m=extinction,object_scale=.01,
                               grids=[g.name for g in volume.grids]))
    sun=bpy.data.objects.new('Fixed review light',bpy.data.lights.new('Fixed review light','SUN'));scene.collection.objects.link(sun)
    sun.data.energy=2.;sun.data.angle=.08;sun.rotation_euler=Vector((.7,.45,-.6)).to_track_quat('-Z','Y').to_euler()
    source=np.asarray(report['source_chart_m']);foot=np.asarray(report['foot_chart_m']);center=(source+foot)*.5
    a.output.mkdir(parents=True)
    cameras=[('close',center+np.array((-80,-150,35)),center,110),('sector',np.array((700,-642,370.754114151)),np.array((250,258,-54.245885849)),900)]
    for name,location,target,width in cameras:
        title='ACTUAL CORE + VDB ON RING PILOT' if report.get('template_seed_only') else 'ACTUAL CORE + VDB ON SIGMA6'
        cam=camera_at(scene,location,target,width);label_camera(scene,cam,title,f'{report["drop_m"]:.1f} m drop | native physical width | float-density CPU review','Measured mean water | no emission | separate falling/impact volumes | native UE result unverified')
        scene.render.filepath=str((a.output/(name+'.png')).resolve());bpy.ops.render.render(write_still=True)
        for ch in list(cam.children):bpy.data.objects.remove(ch,do_unlink=True)
        bpy.data.objects.remove(cam,do_unlink=True)
    (a.output/'review.json').write_text(json.dumps(dict(attachment=str(a.attachment.resolve()),source_sha256=report['source_sha256'],frames=frames,
        frame=a.frame,without_volumes=a.without_volumes,cpu_threads=8,uses_emission=False,
        limitations='Original float VDB, not native SVT quantization; static mean water; same fixed review sun, no scene lighting changes.'),indent=2))


if __name__=='__main__':main()
