"""Import the owned Nordic projected face tile into the optional cliff preview.

Set DC_OWNED_CLIFF_ATLAS to the ignored Tile directory or pass --atlas.
Reads original-derived TGA maps and tile_manifest.json. Geometry supplies UV0
and vertex R=atlas blend weight. No pixels, geometry, levels or defaults change.
Only /Game/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface is replaced;
this is selected by the explicit DCCliffPreviewScan runtime flag.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import unreal

sys.path.insert(0,str(Path(__file__).resolve().parent))
from import_owned_cliff_scans import import_task, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--atlas',type=Path,default=os.environ.get('DC_OWNED_CLIFF_ATLAS'))
    args=parser.parse_args()
    if not args.atlas or not (args.atlas/'tile_manifest.json').is_file():
        raise RuntimeError('Provide the generated owned scan Tile directory')
    manifest=json.loads((args.atlas/'tile_manifest.json').read_text())
    project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    for relative in ('Content/LocalScans/nordic_face_tile/owned.uasset',
                     'Content/DC/Maelstrom/GeneratedCliffPreview/M_CliffScanSurface.uasset'):
        if subprocess.run(['git','check-ignore','--quiet','--',str(project/relative)],cwd=project).returncode:
            raise RuntimeError('Owned-derived assets must remain ignored: '+relative)
    texture_root='/Game/LocalScans/nordic_face_tile'
    maps={}
    for role,name in [('albedo','T_NordicFace_BaseColor.tga'),('normal','T_NordicFace_NormalGL.tga'),('roughness','T_NordicFace_Roughness.tga')]:
        path=args.atlas/name
        digest=sha(path)
        if manifest['files'].get(name)!=digest:
            raise RuntimeError('Atlas source hash mismatch: '+name)
        asset_name=path.stem+'_'+digest[:10]
        target=texture_root+'/'+asset_name
        texture=unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
        if texture is None:
            texture=next((a for a in import_task(path,texture_root,asset_name) if isinstance(a,unreal.Texture2D)),None)
        if texture is None: raise RuntimeError('Texture import failed: '+name)
        texture.set_editor_property('srgb',role=='albedo')
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if role=='normal' else unreal.TextureCompressionSettings.TC_DEFAULT)
        texture.set_editor_property('address_x',unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property('address_y',unreal.TextureAddress.TA_WRAP)
        if role=='normal': texture.set_editor_property('flip_green_channel',True)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False): raise RuntimeError('Texture save failed')
        maps[role]=texture
    destination='/Game/DC/Maelstrom/GeneratedCliffPreview'
    name='M_CliffScanSurface'; target=destination+'/'+name
    material=unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
    if material is None:
        material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,destination,unreal.Material,unreal.MaterialFactoryNew())
    mel=unreal.MaterialEditingLibrary
    mel.delete_all_material_expressions(material)
    material.set_editor_property('used_with_nanite',True)
    material.set_editor_property('used_with_instanced_static_meshes',True)
    material.set_editor_property('tangent_space_normal',True)
    material.set_editor_property('two_sided',False)
    material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    def node(kind):return mel.create_material_expression(material,kind)
    def link(a,pin,b,inp):
        if not mel.connect_material_expressions(a,pin,b,inp):raise RuntimeError('Expression connection failed: '+inp)
    def prop(a,pin,p):
        if not mel.connect_material_property(a,pin,p):raise RuntimeError('Property connection failed: '+str(p))
    def scalar(value):
        a=node(unreal.MaterialExpressionConstant);a.r=value;return a
    def color(value):
        a=node(unreal.MaterialExpressionConstant3Vector);a.constant=unreal.LinearColor(*value,1.);return a
    mask=node(unreal.MaterialExpressionVertexColor)
    scale=node(unreal.MaterialExpressionScalarParameter)
    scale.set_editor_property('parameter_name','AlbedoScale')
    scale.set_editor_property('default_value',1.)
    for role,property_name,background in [('albedo',unreal.MaterialProperty.MP_BASE_COLOR,color((.20,.21,.205))),
                                          ('roughness',unreal.MaterialProperty.MP_ROUGHNESS,scalar(.88)),
                                          ('normal',unreal.MaterialProperty.MP_NORMAL,color((0.,0.,1.)))]:
        tex=node(unreal.MaterialExpressionTextureSample);tex.texture=maps[role]
        tex.sampler_type=(unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role=='albedo' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        # Unconnected texture input is source UV0. UE5.8 Python rejects the
        # display-name Coordinates pin; no explicit UV expression is needed.
        value=tex;pin='R' if role=='roughness' else ''
        if role=='albedo':
            value=node(unreal.MaterialExpressionMultiply);link(tex,'',value,'A');link(scale,'',value,'B')
        if role=='normal':
            blend=node(unreal.MaterialExpressionLinearInterpolate)
            link(background,'',blend,'A');link(value,pin,blend,'B');link(mask,'R',blend,'Alpha')
            prop(blend,'',property_name)
        else:
            # The complete mesh supplies source-chart UV0, including a radial
            # continuation across the roof. Keep actual stone color/roughness
            # there and outside the pilot instead of a bare constant-gray lid.
            prop(value,pin,property_name)
    prop(scalar(0.),'',unreal.MaterialProperty.MP_METALLIC)
    prop(scalar(.35),'',unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Optional material save failed')
    saved=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
    report=dict(material=material.get_path_name(),textures={k:v.get_path_name() for k,v in maps.items()},
                atlas_manifest=str((args.atlas/'tile_manifest.json').resolve()),atlas_source=manifest,
                mapping='Exact geometry UV0, vertexR blend; no extra texture scale or phase',
                normal='Projected fine GL normal; imported green flip; full measured relief stays in geometry',
                outside_pilot='Same scan-derived color/roughness on supplied UV0; vertexR attenuates fine normal only',runtime_opt_in='DCCliffPreviewScan')
    (saved/'OwnedCliffAtlasImport.json').write_text(json.dumps(report,indent=2))
    unreal.log_warning('DC_OWNED_CLIFF_ATLAS '+json.dumps(report))


if __name__=='__main__':main()
