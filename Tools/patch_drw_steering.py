"""Anchor DRW's wave field to the travelling world, across every ocean LOD.

Root-owned native workflow (close native jobs first):
  bundled-python Tools/patch_drw_steering.py --backup
  UnrealEditor-Cmd project.uproject -run=pythonscript -script=<this file> -nullrhi -unattended
  recook the game; preserve Saved/OceanSteeringPatch/manifest.json with build evidence.
Restore before another vendor-material patch with --restore (plain Python, editor closed).
Only wave coordinates change. Shore/depth/mask/stitch geometry stays ship-stage local.
"""
from pathlib import Path
import hashlib
import json
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = Path('E:/Utilities/UE_5.8/Engine/Plugins/Marketplace/DynamicR1d9938e98fdbV5/Content')
EVIDENCE = ROOT / 'Saved/OceanSteeringPatch'
MANIFEST = EVIDENCE / 'manifest.json'
FUNCTIONS = ['Core/MF_GetUV', 'MF_GetWPO', 'MF_GetBaseNormal', 'MF_GetHQFoam']
MATERIALS = ['M_DRW_High', 'M_DRW_Med', 'M_DRW_Low', 'M_DRW_Far', 'M_DRW_UltraLow', 'M_DRW_Billboard']
PATHS = ['Materials/MaterialFunctions/'+n+'.uasset' for n in FUNCTIONS] + ['Materials/OceanLOD/'+n+'.uasset' for n in MATERIALS]
MARKER = 'DC_STEERING_SAMPLE_V1'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if '--backup' in sys.argv:
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    data = json.loads(MANIFEST.read_text()) if MANIFEST.exists() else {'schema':'dc.ocean-steering.v1','files':{}}
    for rel in PATHS:
        source = PLUGIN / rel
        if not source.exists():
            raise RuntimeError('Required installed material missing: '+str(source))
        if rel in data['files']:
            row=data['files'][rel]
            if sha(source) not in (row['original_sha256'],row.get('patched_sha256')):
                raise RuntimeError('Asset changed since steering backup; reconcile before patching: '+rel)
        else:
            dest=EVIDENCE/'originals'/rel
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,dest)
            data['files'][rel]={'original_sha256':sha(source),'original':str(dest)}
    MANIFEST.write_text(json.dumps(data,indent=2)+'\n')
    print('DC_RESULT steering backups verified: '+str(MANIFEST))
    raise SystemExit(0)

if '--restore' in sys.argv:
    data=json.loads(MANIFEST.read_text())
    for rel,row in data['files'].items():
        original=Path(row['original'])
        if sha(original)!=row['original_sha256']: raise RuntimeError('Corrupt original: '+rel)
        shutil.copy2(original,PLUGIN/rel)
    print('DC_RESULT restored original water assets; recook to restore a package')
    raise SystemExit(0)

import unreal
mel=unreal.MaterialEditingLibrary
if not MANIFEST.exists(): raise RuntimeError('Run plain Python --backup with Unreal closed first')
manifest=json.loads(MANIFEST.read_text())
for rel,row in manifest['files'].items():
    if sha(PLUGIN/rel) not in (row['original_sha256'],row.get('patched_sha256')):
        raise RuntimeError('Installed water changed since backup: '+rel)
collection=unreal.load_asset('/Game/DC/Materials/MPC_Sea')
if not collection: raise RuntimeError('MPC_Sea missing')
audit=[]
reroute_declarations={}

def inputs(fn,node):
    return dict(zip(mel.get_material_expression_input_names(node),mel.get_inputs_for_material_function_expression(fn,node)))

def add_input(node,name):
    pins=list(node.get_editor_property('inputs'))
    if any(str(p.get_editor_property('input_name'))==name for p in pins): return
    pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    node.set_editor_property('inputs',pins)

def trough_node(fn):
    for node in mel.get_material_function_expressions(fn):
        if isinstance(node,unreal.MaterialExpressionCollectionParameter) and str(node.get_editor_property('parameter_name'))=='Trough':
            return node
    node=mel.create_material_expression_in_function(fn,unreal.MaterialExpressionCollectionParameter)
    node.set_editor_property('collection',collection);node.set_editor_property('parameter_name','Trough')
    return node

def reroute_declaration(fn,node):
    # Declaration is not editor-readable in UE 5.8 and has no reflected getter.
    # Named usages also expose no ordinary input pin. Export the serialized
    # reference through the public exporter API instead of assuming unshifted UVs.
    key=node.get_path_name()
    if key in reroute_declarations: return reroute_declarations[key]
    destination=EVIDENCE/'reroutes'/(fn.get_name()+'_'+node.get_name()+'.t3d')
    destination.parent.mkdir(parents=True,exist_ok=True)
    task=unreal.AssetExportTask()
    task.set_editor_property('object',node)
    task.set_editor_property('exporter',unreal.ObjectExporterT3D())
    task.set_editor_property('filename',str(destination))
    task.set_editor_property('automated',True)
    task.set_editor_property('prompt',False)
    task.set_editor_property('replace_identical',True)
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError('Cannot export named reroute: '+key)
    raw=destination.read_bytes()
    serialized=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
    match=re.search(r'^\s*Declaration=(.+)$',serialized,re.MULTILINE)
    if not match:
        raise RuntimeError('Named reroute has no serialized Declaration: '+key)
    reference=match.group(1).strip()
    candidates=[n for n in mel.get_material_function_expressions(fn)
        if isinstance(n,unreal.MaterialExpressionNamedRerouteDeclaration)
        and re.search(r'(?<![\w])'+re.escape(n.get_name())+r'(?![\w])',reference)]
    if len(candidates)!=1:
        raise RuntimeError('Cannot uniquely resolve named reroute '+key+': '+reference)
    reroute_declarations[key]=candidates[0]
    return candidates[0]

def uses_shifted_uv(fn,node,visited=None):
    if not node: return False
    visited=visited or set()
    if node.get_path_name() in visited: return False
    visited.add(node.get_path_name())
    if isinstance(node,unreal.MaterialExpressionMaterialFunctionCall):
        called=node.get_editor_property('material_function')
        if called and called.get_name()=='MF_GetUV': return True
    if isinstance(node,unreal.MaterialExpressionNamedRerouteUsage):
        declaration=reroute_declaration(fn,node)
        if declaration and uses_shifted_uv(fn,declaration,visited): return True
    return any(uses_shifted_uv(fn,parent,visited) for parent in inputs(fn,node).values())

for short in FUNCTIONS:
    path='/DynamicRealWater/Materials/MaterialFunctions/'+short
    fn=unreal.load_asset(path)
    nodes=list(mel.get_material_function_expressions(fn))
    existing=[n for n in nodes if isinstance(n,unreal.MaterialExpressionCustom) and MARKER in n.get_editor_property('code')]
    expected={'Core/MF_GetUV':1,'MF_GetWPO':2,'MF_GetBaseNormal':1,'MF_GetHQFoam':2}[short]
    if existing:
        if len(existing)!=expected: raise RuntimeError('Partial previous patch: '+short)
        audit.append({'function':short,'status':'already patched','nodes':len(existing)})
        continue
    trough=trough_node(fn)
    count=0
    if short=='Core/MF_GetUV':
        coordinates=[n for n in nodes if isinstance(n,unreal.MaterialExpressionTextureCoordinate)]
        if len(coordinates)!=1: raise RuntimeError('Unexpected shared wave UV graph; inspect before patching')
        source=coordinates[0]
        consumers=[(n,name) for n in nodes for name,parent in inputs(fn,n).items() if parent==source]
        if not consumers: raise RuntimeError('No UV consumers')
        offset=mel.create_material_expression_in_function(fn,unreal.MaterialExpressionCustom)
        offset.set_editor_property('description',MARKER)
        offset.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        add_input(offset,'BaseUV');add_input(offset,'DCTrough')
        offset.set_editor_property('code','// '+MARKER+'\nreturn BaseUV + float2(0,-DCTrough.x/6400.0);')
        assert mel.connect_material_expressions(source,'',offset,'BaseUV')
        assert mel.connect_material_expressions(trough,'',offset,'DCTrough')
        for node,name in consumers:
            assert mel.connect_material_expressions(offset,'',node,name),(short,name)
        count=1
    else:
        for node in nodes:
            if not isinstance(node,unreal.MaterialExpressionCustom): continue
            code=node.get_editor_property('code').replace('\r\n','\n').replace('\r','')
            updated=code
            if short=='MF_GetWPO' and 'W.Displacement(' in code:
                for point in ('EdgeStart','EdgeEnd'):
                    old='CascadesSampler, '+point+', InvTileA'
                    if old not in updated: raise RuntimeError('Unexpected displacement call')
                    updated=updated.replace(old,'CascadesSampler, '+point+'+float3(0,-DCTrough.x,0), InvTileA')
                # Older FFT fallback takes stitched UVs. Shift only if its input
                # is not already supplied by the shared MF_GetUV patched above.
                for uv in ('UVStart','UVEnd'):
                    if not uses_shifted_uv(fn,inputs(fn,node).get(uv)):
                        updated=updated.replace(', '+uv+', 0.0)',', '+uv+'+float2(0,-DCTrough.x/6400.0), 0.0)')
            elif short=='MF_GetWPO' and 'W.FarFoam(' in code:
                updated=updated.replace('W.WindowWeight(WorldPosition)','W.WindowWeight(WorldPosition+float3(0,-DCTrough.x,0))')
                updated=updated.replace('FoamLevelsSampler, WorldPosition,','FoamLevelsSampler, WorldPosition+float3(0,-DCTrough.x,0),')
            elif short=='MF_GetBaseNormal' and 'W.Surface(' in code:
                updated=updated.replace('CascadesSampler, WorldPosition,','CascadesSampler, WorldPosition+float3(0,-DCTrough.x,0),')
            elif short=='MF_GetHQFoam' and ('return T.Color(' in code or 'return T.WorldNormal(' in code):
                updated=updated.replace('WorldPosition.xy / -abs(Size)','(WorldPosition.xy+float2(0,-DCTrough.x)) / -abs(Size)')
            if updated!=code:
                add_input(node,'DCTrough')
                assert mel.connect_material_expressions(trough,'',node,'DCTrough')
                node.set_editor_property('code','// '+MARKER+'\n'+updated)
                count+=1
    if count!=expected: raise RuntimeError(f'Unexpected patch coverage {short}: {count}, expected {expected}; nothing saved for this function')
    mel.update_material_function(fn)
    assert unreal.EditorAssetLibrary.save_asset(path,only_if_is_dirty=False)
    audit.append({'function':short,'status':'patched','nodes':count})

for name in MATERIALS:
    path='/DynamicRealWater/Materials/OceanLOD/'+name
    material=unreal.load_asset(path)
    mel.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_asset(path,only_if_is_dirty=False)
for rel,row in manifest['files'].items(): row['patched_sha256']=sha(PLUGIN/rel)
manifest['audit']=audit
manifest['sampling']='wave world Y = stage Y - MPC_Sea.Trough.x; CPU DCOcean::WaveSamplePosition adds LaneOffset'
MANIFEST.write_text(json.dumps(manifest,indent=2)+'\n')
unreal.log_warning('DC_RESULT steering wave mapping patched across all ocean LODs: '+json.dumps(audit))
