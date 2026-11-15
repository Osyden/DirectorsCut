"""Export the accepted original 3D surface for an isolated clay engine preview.

No heightfield reconstruction, geometry refinement, displacement or scaling.
Missing scientific masks are explicitly diagnostic zeros, never invented flow
or erosion data. The established source audit validates the actual import.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from review_cliff_outcrop import load_mesh
from review_eroded_cliff_sector import configure_mesh_normals


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--normal-angle-deg', type=float, default=15.)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
    if args.output.exists() or args.output.suffix.lower() != '.fbx' or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new FBX under ignored Saved')
    import bpy
    data = load_mesh(args.input)
    vertices = data['chart'].astype(np.float32)
    triangles = data['triangles']
    rgba = np.zeros((len(vertices), 4), dtype=np.float32)
    rgba[:, 3] = 1.
    args.output.parent.mkdir(parents=True, exist_ok=True)
    original_path = args.output.with_suffix('.npz')
    np.savez_compressed(original_path, vertices_m=data['vertices'], triangles=triangles,
                        vertex_rgba=rgba, metadata_json=np.asarray(json.dumps(dict(
                            input_path=str(args.input.resolve()), input_sha256=sha(args.input),
                            masks='Constant diagnostic RGBA(0,0,0,1); not erosion/flow data',
                            source_metadata=data['metadata']))))
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.
    mesh = bpy.data.meshes.new('OriginalCliffSurface')
    mesh.from_pydata(vertices.tolist(), [], triangles.tolist())
    mesh.update()
    obj = bpy.data.objects.new('OriginalCliffSurface', mesh)
    scene.collection.objects.link(obj)
    normals = configure_mesh_normals(mesh, 'angle', args.normal_angle_deg)
    corners = np.array([loop.vertex_index for loop in mesh.loops], dtype=np.int64)
    uv = mesh.uv_layers.new(name='Physical100mUV')
    uv.data.foreach_set('uv', (vertices[corners, :2]/100.).ravel())
    color = mesh.color_attributes.new(name='DiagnosticZeros', type='FLOAT_COLOR', domain='CORNER')
    color.data.foreach_set('color', rgba[corners].ravel())
    mesh.color_attributes.active_color = color
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(args.output.resolve()), use_selection=True, object_types={'MESH'},
                            axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
                            apply_scale_options='FBX_SCALE_NONE', global_scale=1.,
                            use_mesh_modifiers=True, mesh_smooth_type='OFF', use_triangles=True,
                            colors_type='LINEAR', prioritize_active_color=True, add_leaf_bones=False,
                            bake_anim=False, bake_space_transform=False, path_mode='AUTO')
    sidecar = dict(schema='directorscut.cliff_preview_fbx.v1', source_kind='original_cliff_exposure_geometry_only',
                   mesh_path=str(original_path.resolve()), mesh_sha256=sha(original_path),
                   input_path=str(args.input.resolve()), input_sha256=sha(args.input),
                   fbx_path=str(args.output.resolve()), fbx_sha256=sha(args.output),
                   local_origin_arena_m=data['origin'].tolist(), basis_u_arena=data['basis'][:, 0].tolist(),
                   basis_v_arena=data['basis'][:, 1].tolist(), basis_z_arena=data['basis'][:, 2].tolist(),
                   local_bounds_m=[vertices.min(axis=0).tolist(), vertices.max(axis=0).tolist()],
                   local_dimensions_m=np.ptp(vertices, axis=0).tolist(), vertices=len(vertices),
                   triangles=len(triangles), units='metres; scene unit1; apply_unit_scale=True',
                   axis_forward='-Y', axis_up='Z', normals=normals,
                   color_encoding='LINEAR FLOAT_COLOR; constant diagnostic(0,0,0,1)',
                   scientific_masks_available=False, unreal_imported=False, runtime_integrated=False)
    args.output.with_suffix('.json').write_text(json.dumps(sidecar, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(fbx=str(args.output), vertices=len(vertices), triangles=len(triangles))), flush=True)


if __name__ == '__main__':
    main()
