"""Publish verified full-ring local assets/data to the normal playable scene.

Run after mesh/material/fall imports and actual-source audit. Generated owned
assets remain ignored; Runtime data is staged with the game by DefaultGame.ini.
The completion marker is written last so half-imported scenes are never enabled.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--engine',type=Path,default=Path('E:/Utilities/UE_5.8'))
    parser.add_argument('--caldera',action='store_true',help='Install the single-inlet level lake; retire bowl-bound sidefalls')
    args=parser.parse_args()
    project=args.project.resolve(); saved=project/'Saved'
    target=project/'Content/DC/Maelstrom/GeneratedCliffPreview/Runtime'
    if subprocess.run(['git','check-ignore','--quiet','--',str(target/'CompleteArena.json')],cwd=project).returncode:
        raise RuntimeError('Generated runtime data must remain outside public Git')
    imported=read(saved/'CliffLandformPreviewImport.json')
    audit=read(saved/'CliffPreviewSourceAudit.json')
    supports=read(saved/'CliffPreviewSupports.json')
    falls=None if args.caldera else read(saved/'CliffPreviewWaterfall.json')
    surface=read(imported['export_sidecar'])
    geology=read(saved/'CliffGeologyImport.json')
    source=Path(surface['input_path'])
    shape=read(source.parent/'manifest.json')
    fbx_hash=imported['source_sha256']; source_hash=surface['input_sha256']
    if args.caldera:
        entry=shape.get('entry_cut',{})
        if not entry.get('original_topology_and_uvs_retained') or shape.get('vertices',0)<800000:
            raise RuntimeError('Caldera requires the complete ring with an excavated inlet')
        material_source=entry['source_sha256']
    else:
        if shape['schema']!='directorscut.complete_cliff_ring.v1' or shape['angular_segments']<1536:
            raise RuntimeError('Full circumference cliff required, not a pilot')
        material_source=source_hash
    if shape['seam_position_error_m']>1.e-5 or shape['exported_collapsed_triangles']:
        raise RuntimeError('Cliff seam/native triangles failed')
    if not audit.get('geometry_passed') or not audit.get('mask_fidelity_passed') or audit['source_fbx_sha256']!=fbx_hash:
        raise RuntimeError('Actual imported cliff audit must match current mesh')
    if supports['source_fbx_sha256']!=fbx_hash or supports['surface_mesh']['source_sha256']!=source_hash:
        raise RuntimeError('Ruin support source does not match')
    if not args.caldera:
        if falls['cliff_fbx_sha256']!=fbx_hash or falls['source_candidate_sha256']!=source_hash:
            raise RuntimeError('Waterfalls must fit this full cliff source')
        from export_cliff_erosion_seed import source_snapshot
        _, water_source=source_snapshot(project,None,args.engine)
        if falls.get('water_source',{}).get('position_functions_sha256')!=water_source['position_functions_sha256']:
            raise RuntimeError('Water shape changed after waterfall contacts were fitted')
    if geology['context_source'].get('full_ring_geometry_source')!=material_source:
        raise RuntimeError('Full-ring material weathering belongs to a different mesh')
    binary=(saved/'CliffPreviewSurface.bin').read_bytes()
    if hashlib.md5(binary).hexdigest()!=supports['surface_mesh']['md5']:
        raise RuntimeError('Surface binary changed after verification')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=source_hash:
        raise RuntimeError('Cliff source changed after export')
    shore_points=shape.get('shore_mist_centers_m',[]) if args.caldera else []
    shore_report=None
    shore_path=saved/'CalderaShoreMist.json'
    if args.caldera and shore_path.exists():
        shore_report=read(shore_path)
        if (shore_report.get('schema')!='directorscut.caldera_shore_mist.v1'
            or shore_report.get('source_candidate_sha256')!=source_hash
            or shore_report.get('cliff_fbx_sha256')!=fbx_hash):
            raise RuntimeError('Regenerate shoreline mist contacts for this exact cliff source')
        shore_points=shore_report.get('shore_mist_centers_m',[])
        metrics=shore_report.get('metrics',{})
        if (shore_report.get('count')!=160 or len(shore_points)!=160
            or shore_report.get('radius_m')!=46 or shore_report.get('height_offset_m')!=2
            or not all(len(p)==3 and all(math.isfinite(v) for v in p) and abs(p[2]+398)<1e-6 for p in shore_points)
            or not 0<=metrics.get('conservative_continuous_coverage_bound_m',math.inf)<=46
            or not 0<=metrics.get('source_reconstruction_error_m',math.inf)<1e-6):
            raise RuntimeError('Shoreline fog footprint/coverage does not match the runtime contract')
    assets=[imported['asset'],geology['material']]+([] if args.caldera else [falls['core']['asset']])
    for asset in assets:
        package=asset.split('.')[0]
        if not package.startswith('/Game/'):
            raise RuntimeError(f'Unexpected complete-scene package: {package}')
        if not (project/'Content'/package[len('/Game/'):]).with_suffix('.uasset').is_file():
            raise RuntimeError(f'Complete-scene asset is missing: {package}')
    target.mkdir(parents=True,exist_ok=True)
    runtime_files=['CliffPreviewSourceAudit.json','CliffPreviewSupports.json','CliffPreviewSurface.bin']
    if not args.caldera:
        runtime_files.append('CliffPreviewWaterfall.json')
    if shore_report is not None:
        runtime_files.append('CalderaShoreMist.json')
    for name in runtime_files:
        shutil.copy2(saved/name,target/name)
    # Packaged execution checks these stable identities, not author-machine
    # absolute source paths or files in Saved.
    imported['source_candidate_sha256']=source_hash
    (target/'CliffLandformPreviewImport.json').write_text(json.dumps(imported,indent=2)+'\n')
    marker=dict(schema='directorscut.complete_crater.v1',cliff_fbx_sha256=fbx_hash,
                source_candidate_sha256=source_hash,full_circumference=True,
                material=geology['material'],waterfalls=None if args.caldera else falls.get('core',{}).get('asset'),
                normal_game_enabled=True,caldera_lake=args.caldera,
                lake_height_m=-400 if args.caldera else None,
                # Older parallel checkouts still expect the angular128 field.
                # Never feed their interpolation an unordered coverage set.
                shore_mist_centers_m=shape.get('shore_mist_centers_m',[]) if args.caldera else [],
                shore_mist_coverage_centers_m=shore_points if shore_report is not None else [],
                shore_mist_contact_report_sha256=hashlib.sha256(shore_path.read_bytes()).hexdigest() if shore_report is not None else None,
                integrated_inlet_banks=shape.get('integrated_inlet_banks',False),
                bank_boulder_anchors_m=shape.get('bank_boulder_anchors_m',[]))
    temporary=target/'CompleteArena.pending'
    temporary.write_text(json.dumps(marker,indent=2)+'\n')
    temporary.replace(target/'CompleteArena.json')
    print(json.dumps(dict(installed=str(target),**marker),indent=2))


if __name__=='__main__':
    main()
