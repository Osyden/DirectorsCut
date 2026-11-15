"""Prepare measured shore weather data, or apply it to the isolated UE preview.

Standalone: --source <compressed-parent.npz> --output <new Saved directory>.
UE Python: set DC_CLIFF_WEATHER_CONFIG=<prepared weather.json>, then run this
script after make_cliff_preview_material.py. Root owns all UE execution.
DC_CLIFF_WEATHER_COORDINATES=arena transforms an identity-arena ring back to
the stored source chart; default sourcechart keeps the isolated sector mapping.
All added weather is bounded to the measured lateral span and near-shore zone.
Only preview base colour/roughness change; scan UV, normals, AO and lights stay.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import zlib


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def prepare():
    import numpy as np
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    if a.output.exists() or 'saved' not in [v.lower() for v in a.output.resolve().parts]:
        p.error('Use a new ignored Saved directory')
    with np.load(a.source, allow_pickle=False) as z:
        src = {k: z[k].copy() for k in z.files}
    meta = json.loads(str(src['metadata_json']))
    if meta.get('schema') != 'directorscut.compressed_escarpment.v1':
        raise ValueError('This weather profile requires the audited compressed-parent frame')
    u = src['contact_uv_m'][:, 0]
    values = np.c_[src['contact_uv_m'][:, 1], src['contact_water_height_m']]
    if not np.allclose(np.diff(u), np.diff(u)[0], atol=1e-10):
        raise ValueError('Uniform physical shore profile expected')
    low, high = values.min(axis=0), values.max(axis=0)
    span = np.maximum(high - low, 1e-9)
    data = np.rint(np.clip((values - low) / span, 0, 1) * 65535).astype(np.uint16)
    rgba = np.stack((data[:, 0] >> 8, data[:, 0] & 255, data[:, 1] >> 8, data[:, 1] & 255), axis=1).astype(np.uint8)
    # Two linear16-bit scalars packed into RG and BA. Bilinear channel sampling
    # and linear unpacking preserve scalar interpolation, including byte carries.
    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', len(u), 1, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(b'\0' + rgba.tobytes())) + chunk(b'IEND', b'')
    parent_path = Path(meta['parent_path'])
    if sha(parent_path) != meta['parent_sha256']:
        raise ValueError('Historical parent changed')
    with np.load(parent_path, allow_pickle=False) as parent:
        flow = parent['flow_accumulation'].ravel()
    ids, weights = src['main_source_grid_indices'], src['main_source_barycentric']
    sampled = (flow[ids] * weights).sum(axis=1)
    basis = np.stack([src['basis_u_world'], src['basis_v_world'], src['basis_z_world']], axis=1)
    local = (src['vertices_m'] - src['frame_origin_world_m']) @ basis
    main = local[int(src['main_vertex_start']):].reshape(-1, len(u), 3)
    sampled = sampled.reshape(-1, len(u))
    eligible = main[:, :, 2] > values[None, :, 1] + 14.
    filtered_flow = np.where(eligible, sampled, 0.)
    row = filtered_flow.argmax(axis=0)
    strength = filtered_flow[row, np.arange(len(u))]
    emitters = []
    for col in np.argsort(strength)[::-1]:
        if strength[col] < 512.:
            break
        if col < 5 or col >= len(u) - 5 or any(abs(u[col] - e['u_m']) < 32. for e in emitters):
            continue
        top = float(main[row[col], col, 2])
        length = min(100., top - float(values[col, 1]) - 2.)
        if length < 14.:
            continue
        emitters.append(dict(u_m=float(u[col]), top_z_m=top, length_m=length,
                             half_width_m=float(np.clip(.5 + .14 * np.log2(1. + strength[col] / 512.), .7, 1.8)),
                             source_drainage_m2=float(strength[col]), source_row=int(row[col])))
        if len(emitters) == 6:
            break
    a.output.mkdir(parents=True)
    profile = a.output / 'shore_profile.png'
    profile.write_bytes(png)
    reconstructed = low + span * data.astype(float) / 65535.
    report = dict(schema='directorscut.cliff_preview_weather.v1', source_path=str(a.source.resolve()),
                  source_sha256=sha(a.source), parent_path=str(parent_path), parent_sha256=sha(parent_path),
                  profile_path=str(profile.resolve()), profile_sha256=sha(profile), profile_samples=len(u),
                  profile_u_range_m=[float(u[0]), float(u[-1])], profile_low_m=low.tolist(), profile_span_m=span.tolist(),
                  profile_decode_max_error_m=float(abs(reconstructed - values).max()),
                  frame_origin_world_m=src['frame_origin_world_m'].tolist(), frame_basis_columns=basis.tolist(),
                  wet_band_vertical_m=[.7, 7.], wet_band_landward_m=[4., 20.],
                  runoff_emitters=emitters, generator_sha256=sha(__file__),
                  defaults=dict(dry_tint=[.72, .78, .84], retained_chroma=.65,
                                wet_color_factor=.55, stain_color_factor=.86, wet_roughness_factor=.70),
                  uv_contract='UE local cm: original U=X*.01, V=-Y*.01, Z=Z*.01; original frame and unit actor scale required',
                  limitations='Static measured shoreline; no dynamic waves or rain simulation. Sparse authored stains seeded by historical drainage are not measured mineral deposits or current saturation. No AO, emissive, normal or light changes.')
    (a.output / 'weather.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(output=str(a.output.resolve()), profile_error_m=report['profile_decode_max_error_m'],
                          runoff_emitters=len(emitters)), indent=2))


def apply_unreal():
    import unreal
    config = Path(os.environ['DC_CLIFF_WEATHER_CONFIG']).resolve()
    data = json.loads(config.read_text(encoding='utf-8'))
    if data['schema'] != 'directorscut.cliff_preview_weather.v1':
        raise ValueError('Unexpected weather config')
    coordinates = os.environ.get('DC_CLIFF_WEATHER_COORDINATES', 'sourcechart')
    if coordinates not in ('sourcechart', 'arena'):
        raise ValueError('DC_CLIFF_WEATHER_COORDINATES must be sourcechart or arena')
    if sha(data['profile_path']) != data['profile_sha256'] or sha(data['source_path']) != data['source_sha256']:
        raise ValueError('Prepared weather data or source geometry changed')
    root = '/Game/DC/Maelstrom/GeneratedCliffPreview'
    saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
    base_report = json.loads((saved / 'CliffRockMaterialPreview.json').read_text(encoding='utf-8'))
    if base_report.get('preset') not in ('scan','slate'):
        raise ValueError('Weather calibration requires the scan or installed slate preview preset')
    if base_report.get('preset') == 'slate' and base_report.get('transport_masks') != 'none':
        raise ValueError('Slate shore comparison requires transport_masks=none to avoid unverified vertex-colour wetness')
    material = unreal.load_asset(root + '/M_CliffRock')
    if not isinstance(material, unreal.Material):
        raise ValueError('Regenerate the isolated scan material first')
    M = unreal.MaterialEditingLibrary
    color_prop, rough_prop = unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS
    color_base = M.get_material_property_input_node(material, color_prop)
    rough_base = M.get_material_property_input_node(material, rough_prop)
    if color_base is None or rough_base is None:
        raise ValueError('Missing base material outputs')
    if isinstance(color_base, unreal.MaterialExpressionCustom) and str(color_base.get_editor_property('description')).startswith('DCCliffWeather'):
        raise ValueError('Weather already applied; regenerate base material before another application')
    texture_path = root + '/Weather/T_Shore_' + data['profile_sha256'][:12]
    texture = unreal.load_asset(texture_path)
    if texture is None:
        task = unreal.AssetImportTask()
        task.filename = data['profile_path']
        task.destination_path = root + '/Weather'
        task.destination_name = texture_path.rsplit('/', 1)[1]
        task.automated = True
        task.replace_existing = False
        task.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(texture_path)
    if not isinstance(texture, unreal.Texture2D):
        raise ValueError('Shore profile import failed')
    texture.set_editor_property('srgb', False)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('filter', unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('never_stream', True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError('Cannot save shore profile')
    def node(cls):
        return M.create_material_expression(material, cls)
    def connect(src, output, dst, pin):
        if not M.connect_material_expressions(src, output, dst, pin):
            raise RuntimeError('Cannot connect weather input ' + pin)
    def custom(code, output_type, inputs, description):
        out = node(unreal.MaterialExpressionCustom)
        out.set_editor_property('code', code)
        out.set_editor_property('description', description)
        out.set_editor_property('output_type', output_type)
        pins = []
        for name in inputs:
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', name)
            pins.append(pin)
        out.set_editor_property('inputs', pins)
        for name, (src, output) in inputs.items():
            connect(src, output, out, name)
        return out
    wp = node(unreal.MaterialExpressionWorldPosition)
    position = node(unreal.MaterialExpressionTransformPosition)
    position.set_editor_property('transform_source_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    position.set_editor_property('transform_type', unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    connect(wp, '', position, '')
    wn = node(unreal.MaterialExpressionVertexNormalWS)
    normal = node(unreal.MaterialExpressionTransform)
    normal.set_editor_property('transform_source_type', unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    normal.set_editor_property('transform_type', unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    connect(wn, '', normal, '')
    profile = node(unreal.MaterialExpressionTextureObject)
    profile.set_editor_property('texture', texture)
    profile.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    u0, u1 = data['profile_u_range_m']
    n = data['profile_samples']
    low, span = data['profile_low_m'], data['profile_span_m']
    wet_z0, wet_z1 = data['wet_band_vertical_m']
    wet_v0, wet_v1 = data['wet_band_landward_m']
    coordinate_code = 'float3 p=P*.01;\n'
    if coordinates == 'arena':
        # Imported identity-frame UE coordinates reflect source arena Y.
        # Recover arena metres, then the original source chart (also UE Y-reflected).
        origin = data['frame_origin_world_m']
        basis = data['frame_basis_columns']
        def vector(values):
            return 'float3(' + ','.join(f'{float(v):.16g}' for v in values) + ')'
        u_axis, v_axis, z_axis = [[basis[r][c] for r in range(3)] for c in range(3)]
        coordinate_code = f'''float3 arena=float3(P.x,-P.y,P.z)*.01;
float3 q=arena-{vector(origin)};
float3 p=float3(dot(q,{vector(u_axis)}),-dot(q,{vector(v_axis)}),dot(q,{vector(z_axis)}));
'''
    code = coordinate_code + f'''
float u=saturate((p.x-({u0:.12g}))/({u1-u0:.12g}));
float4 encoded=Profile.SampleLevel(ProfileSampler,float2((u*({n-1}.0)+.5)/{n}.0,.5),0);
float2 packed=float2(dot(encoded.rg,float2(65280,255)),dot(encoded.ba,float2(65280,255)))/65535;
float2 shore=float2({low[0]:.12g},{low[1]:.12g})+packed*float2({span[0]:.12g},{span[1]:.12g});
float height=p.z-shore.y,landward=-p.y-shore.x;
// A clamped lookup is not a shoreline extension around the rest of the ring.
// Fade wholly inside the measured 400 m span; only the local near-shore rock
// gets weather. Wetness itself uses the tighter 4..20 m band below.
float lateral=smoothstep({u0:.12g},{u0+4:.12g},p.x)*(1-smoothstep({u1-4:.12g},{u1:.12g},p.x));
float zone=lateral*smoothstep(-8,-4,landward)*(1-smoothstep(100,140,landward));
float wet=(1-smoothstep({wet_z0:.12g},{wet_z1:.12g},height))*(1-smoothstep({wet_v0:.12g},{wet_v1:.12g},landward));
float cliff=1-smoothstep(.45,.85,abs(normalize(N).z));
float stain=0;
'''
    for e in data['runoff_emitters']:
        code += f'''{{float down=({e['top_z_m']:.12g})-p.z;
float across=abs(p.x-({e['u_m']:.12g}))/({e['half_width_m']:.12g});
float strand=(1-smoothstep(.35,1,across))*smoothstep(0,4,down)*(1-smoothstep({e['length_m']*.65:.12g},{e['length_m']:.12g},down));
stain=max(stain,strand);}}
'''
    code += 'return float3(saturate(wet)*zone,saturate(stain*cliff)*(1-saturate(wet))*zone,zone);'
    masks = custom(code, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                   {'P': (position, ''), 'N': (normal, ''), 'Profile': (profile, '')}, 'DCCliffWeatherMasks')
    color_out = M.get_material_property_input_node_output_name(material, color_prop)
    rough_out = M.get_material_property_input_node_output_name(material, rough_prop)
    settings = dict(data['defaults'])
    if base_report.get('preset') == 'slate':
        # The selected installed slate already has the desired cool grey colour.
        # Keep that source colour; only actual local wet/stain masks alter it.
        settings.update(dry_tint=[1.,1.,1.],retained_chroma=1.)
    tint = settings['dry_tint']
    color = custom(f'''float luma=dot(Base,float3(.2126,.7152,.0722));
float3 stone=lerp(luma.xxx,Base,{settings['retained_chroma']:.12g})*float3({tint[0]:.12g},{tint[1]:.12g},{tint[2]:.12g});
return lerp(Base,stone,Masks.z)*lerp(1,{settings['wet_color_factor']:.12g},Masks.x)*lerp(1,{settings['stain_color_factor']:.12g},Masks.y);''', unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                   {'Base': (color_base, color_out), 'Masks': (masks, '')}, 'DCCliffWeatherColor')
    rough = custom(f'''float wet=clamp(Base*{settings['wet_roughness_factor']:.12g},.28,.60);
return clamp(lerp(Base,wet,Masks.x)-.035*Masks.y,.28,.94);''', unreal.CustomMaterialOutputType.CMOT_FLOAT1,
                   {'Base': (rough_base, rough_out), 'Masks': (masks, '')}, 'DCCliffWeatherRoughness')
    if not M.connect_material_property(color, '', color_prop) or not M.connect_material_property(rough, '', rough_prop):
        raise RuntimeError('Cannot connect weather outputs')
    M.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Cannot save preview weather material')
    (saved / 'CliffPreviewWeather.json').write_text(json.dumps(dict(config=str(config), config_sha256=sha(config),
        material=material.get_path_name(), profile=texture.get_path_name(), data=data, applied=True,
        coordinates=coordinates, material_preset=base_report['preset'],effective_settings=settings,
        finite_lateral_range_m=[u0,u1], lateral_inside_fade_m=4.,
        near_shore_landward_zone_m=[-8.,-4.,100.,140.],
        note='All weather including dry tint is local; no profile extrapolation to other ring sectors.'), indent=2), encoding='utf-8')
    unreal.log_warning('DC_CLIFF_PREVIEW_WEATHER applied to isolated colour/roughness outputs')


if __name__ == '__main__':
    if os.environ.get('DC_CLIFF_WEATHER_CONFIG'):
        apply_unreal()
    else:
        prepare()
