"""Export an accepted clean cliff tile as a physical-metre FBX preview only.

Run in installed Blender --background --factory-startup --threads 8 --python
THIS_FILE -- --seed clean.npz --eroded eroded.npz --output Saved/.../sector.fbx
No Unreal import/runtime change. An open triangulated tile, not a stitched world.
Vertex RGBA = normalized log flow / gross deposition / gross erosion / 1.
The JSON sidecar records physical frame, normalizations and export conventions.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

sys.dont_write_bytecode=True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from review_eroded_cliff_sector import load_pair, triangles, source_info, configure_mesh_normals


def main():
    import bpy
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed",type=Path,required=True)
    parser.add_argument("--eroded",type=Path)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--roundtrip",action="store_true")
    parser.add_argument("--normal-mode",choices=("flat","angle"),default="flat",help="Shading only, unchanged geometry; flat preserves prior exports")
    parser.add_argument("--normal-angle-deg",type=float,default=35.)
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    if not 0.<args.normal_angle_deg<180.:parser.error("Normal angle must be strictly between0 and180 degrees")
    output=args.output.resolve()
    if output.suffix.lower()!=".fbx" or "saved" not in [p.lower() for p in output.parts]:
        parser.error("Output must be an FBX under ignored Saved")
    seed,result=load_pair(args.seed,args.eroded)
    if not source_info(seed)["clean"]:
        parser.error("Export requires a clean macro landform; projected folded-wall data is diagnostic only")
    rows,cols=seed["height_m"].shape
    cell=float(seed["cell_size_m"])
    width,depth=(cols-1)*cell,(rows-1)*cell
    x,y=np.meshgrid(np.arange(cols)*cell-width*.5,np.arange(rows)*cell-depth*.5)
    height=result["height_m"]
    valid=seed["heightfield_valid_mask"]>0
    z_origin=float(np.median(height[valid]))
    vertices=np.stack((x,y,height-z_origin),axis=-1).reshape(-1,3)
    faces=triangles(valid)
    # Remove unused padding vertices; physical surface and boundary stay exact.
    used=np.unique(faces)
    remap=np.full(len(vertices),-1,dtype=np.int64);remap[used]=np.arange(len(used))
    vertices=vertices[used];faces=remap[faces]
    channels=[];normalization={}
    for key,logarithmic in (("flow_accumulation",True),("deposited_m",False),("eroded_m",False)):
        raw=result[key].astype(np.float64)
        field=np.log1p(np.maximum(raw,0)) if logarithmic else np.maximum(raw,0)
        maximum=float(field[valid].max())
        channels.append((field/max(maximum,1.e-12)).ravel()[used])
        normalization[key]={"transform":"log1p" if logarithmic else "identity","divisor":maximum,
                            "raw_max":float(raw[valid].max()),"units":"m3" if logarithmic else "m"}
    colors=np.stack((*channels,np.ones(len(used))),axis=-1).astype(np.float32)
    bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
    scene=bpy.context.scene
    scene.unit_settings.system="METRIC";scene.unit_settings.scale_length=1.0
    scene.unit_settings.length_unit="METERS"
    mesh=bpy.data.meshes.new("CleanCliffPreview")
    mesh.from_pydata(vertices.tolist(),[],faces.tolist());mesh.update()
    obj=bpy.data.objects.new("CleanCliffPreview",mesh);scene.collection.objects.link(obj)
    normal_report=configure_mesh_normals(mesh,args.normal_mode,args.normal_angle_deg)
    uv=mesh.uv_layers.new(name="Physical100mUV")
    corners=np.array([loop.vertex_index for loop in mesh.loops],dtype=np.int64)
    uv.data.foreach_set("uv",(vertices[corners,:2]/100.).astype(np.float32).ravel())
    attribute=mesh.color_attributes.new(name="ErosionMasks",type="FLOAT_COLOR",domain="CORNER")
    attribute.data.foreach_set("color",colors[corners].ravel())
    mesh.color_attributes.active_color=attribute
    expected_corner_normals=np.array([normal.vector[:] for normal in mesh.corner_normals],dtype=float) if args.roundtrip else None
    obj.select_set(True);bpy.context.view_layer.objects.active=obj
    output.parent.mkdir(parents=True,exist_ok=True)
    settings=dict(use_selection=True,object_types={"MESH"},axis_forward="-Y",axis_up="Z",
                  apply_unit_scale=True,apply_scale_options="FBX_SCALE_NONE",global_scale=1.0,
                  use_mesh_modifiers=True,mesh_smooth_type="OFF",use_triangles=True,
                  colors_type="LINEAR",prioritize_active_color=True,add_leaf_bones=False,
                  bake_anim=False,bake_space_transform=False,path_mode="AUTO")
    bpy.ops.export_scene.fbx(filepath=str(output),**settings)
    center=np.asarray(seed["frame_origin_world_m"],dtype=float).copy()
    center+=np.asarray(seed["basis_u_world"])*width*.5+np.asarray(seed["basis_v_world"])*depth*.5
    center[2]=z_origin
    expected_bounds=np.stack((vertices.min(axis=0),vertices.max(axis=0)))
    report=dict(schema="directorscut.cliff_preview_fbx.v1",source_kind=source_info(seed)["kind"],
                seed_path=str(args.seed.resolve()),seed_sha256=hashlib.sha256(args.seed.read_bytes()).hexdigest(),
                result_path=str(args.eroded.resolve()) if args.eroded else None,
                result_sha256=hashlib.sha256(args.eroded.read_bytes()).hexdigest() if args.eroded else None,
                fbx_path=str(output),fbx_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                units="metres; Blender scene unit1; apply_unit_scale=True",blender_version=bpy.app.version_string,
                axis_forward="-Y",axis_up="Z",bake_space_transform=False,global_scale=1.0,
                geometry_frame="local X=U clockwise tangent, Y=V outward radial, Z=up, centered on tile and median elevation",
                local_bounds_m=expected_bounds.tolist(),local_dimensions_m=np.ptp(vertices,axis=0).tolist(),
                local_origin_arena_m=center.tolist(),basis_u_arena=np.asarray(seed["basis_u_world"]).tolist(),
                basis_v_arena=np.asarray(seed["basis_v_world"]).tolist(),basis_z_arena=[0,0,1],
                vertices=len(vertices),triangles=len(faces),open_boundary=True,flat_normals=args.normal_mode=="flat",normals=normal_report,
                color_encoding="LINEAR FLOAT_COLOR ErosionMasks; R=log flow,G=deposition,B=erosion,A=1",
                color_normalization=normalization,uv0="local physical XY metres /100",unreal_imported=False,
                nanite_enabled=False,mask_runtime_quantization_unverified=True)
    if args.roundtrip:
        bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
        bpy.ops.import_scene.fbx(filepath=str(output),colors_type="LINEAR",use_custom_normals=True)
        imported=[o for o in scene.objects if o.type=="MESH"]
        if len(imported)!=1:raise RuntimeError("FBX roundtrip must contain one mesh")
        check=imported[0]
        xyz=np.array([check.matrix_world@v.co for v in check.data.vertices],dtype=float)
        actual_bounds=np.stack((xyz.min(axis=0),xyz.max(axis=0)))
        bounds_error=float(np.max(np.abs(actual_bounds-expected_bounds)))
        imported_color=check.data.color_attributes.active_color
        if imported_color is None:raise RuntimeError("FBX lost vertex color masks")
        actual_color=np.array([entry.color for entry in imported_color.data],dtype=float)
        imported_corners=np.array([loop.vertex_index for loop in check.data.loops],dtype=np.int64)
        if actual_color.shape!=colors[corners].shape:
            raise RuntimeError("FBX changed per-corner mask count/domain")
        # Compare masks at their physical vertex positions, not merely their
        # histograms: a reordered/reversed field can preserve every histogram.
        expected_xyz=vertices[corners];actual_xyz=xyz[imported_corners]
        expected_order=np.lexsort(tuple(np.round(expected_xyz[:,axis],4) for axis in (2,1,0)))
        actual_order=np.lexsort(tuple(np.round(actual_xyz[:,axis],4) for axis in (2,1,0)))
        corner_error=float(np.max(np.abs(expected_xyz[expected_order]-actual_xyz[actual_order])))
        color_error=float(np.max(np.abs(actual_color[actual_order]-colors[corners][expected_order])))
        # A position can have several split normals. Match the exact triangle
        # plus its corner, not just position or a histogram of normal values.
        source_xy={tuple(np.round(v[:2],3)):i for i,v in enumerate(vertices)}
        imported_to_source=np.array([source_xy[tuple(np.round(v[:2],3))] for v in xyz])
        actual_source_corners=imported_to_source[imported_corners]
        source_keys=np.column_stack((np.repeat(np.sort(faces,axis=1),3,axis=0),corners))
        actual_keys=np.column_stack((np.repeat(np.sort(actual_source_corners.reshape(-1,3),axis=1),3,axis=0),actual_source_corners))
        source_normal_order=np.lexsort(tuple(source_keys[:,i] for i in (3,2,1,0)))
        actual_normal_order=np.lexsort(tuple(actual_keys[:,i] for i in (3,2,1,0)))
        if not np.array_equal(source_keys[source_normal_order],actual_keys[actual_normal_order]):
            raise RuntimeError("FBX changed triangle-corner identity for normal verification")
        normal_matrix=check.matrix_world.inverted().transposed().to_3x3()
        actual_normals=np.array([(normal_matrix@normal.vector).normalized()[:] for normal in check.data.corner_normals])
        normal_error=float(np.max(np.abs(actual_normals[actual_normal_order]-expected_corner_normals[source_normal_order])))
        n0=actual_normals[actual_normal_order];n1=expected_corner_normals[source_normal_order]
        cosine=np.sum(n0*n1,axis=1)/(np.linalg.norm(n0,axis=1)*np.linalg.norm(n1,axis=1))
        normal_angle_error=float(np.degrees(np.arccos(np.clip(cosine,-1,1))).max())
        report["blender_roundtrip"]={"bounds_max_error_m":bounds_error,"vertices":len(check.data.vertices),
                                     "triangles":len(check.data.polygons),"color_max_error":color_error,
                                     "corner_position_max_error_m":corner_error,"corner_normal_max_error":normal_error,
                                     "corner_normal_max_angle_degrees":normal_angle_error,
                                     "normal_tolerance_degrees":.1,"imported_custom_normals":check.data.has_custom_normals}
        # Blender encodes imported custom split normals; allow sub-tenth-degree
        # storage quantization while still rejecting lost smooth/sharp splits.
        if bounds_error>.001 or corner_error>.001 or color_error>1.e-5 or normal_angle_error>.1 or len(check.data.polygons)!=len(faces):
            raise RuntimeError("FBX roundtrip changed dimensions, masks, corner normals or triangle count: "+json.dumps(report["blender_roundtrip"]))
    output.with_suffix(".json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(report,indent=2))

if __name__=="__main__":main()
