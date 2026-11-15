"""Author a connected structural escarpment before optional erosion.

Uses the existing physical sector and semantic targets verbatim. The raw field
already spans the measured coast-to-plateau drop: unequal plane-bounded rock
masses, coherent oblique joint traces and finite recessed catchments. Variant B
retains the existing fractal hinterland; neither variant generates new noise,
shader displacement, cube tiles or an erosion-as-macro-structure claim.
This remains a 2 m heightfield authoring proof, not true 3D overhang geometry.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0,str(Path(__file__).resolve().parent))
from generate_massif_cliff_seed import screened_fit
from generate_weathered_cliff_mesh import original_mesh


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def structural_field(source, variant):
    xy=source['world_xy_m'].astype(np.float64)
    delta=xy-source['frame_origin_world_m'][:2]
    u=delta@source['basis_u_world'][:2]
    v=delta@source['basis_v_world'][:2]
    outlets=source['outflow_mask']>0
    if not np.all(outlets.sum(axis=0)==1):
        raise ValueError('This sector proof requires one measured shoreline outlet per source column')
    rows=np.argmax(outlets,axis=0);cols=np.arange(u.shape[1])
    # The first valid/outlet row is a rasterized submerged toe, not a smooth
    # shore coordinate. Propagating its 2 m row jumps through a 5:1 face makes
    # 10 m artificial stair steps throughout the scarp. Interpolate the actual
    # measured receiving-water boundary instead. Hard toe targets stay exact.
    clearance=source['shore_clearance_m'].astype(float)
    crossings=(clearance[:-1]>=0.)&(clearance[1:]<0.)
    if not np.all(crossings.sum(axis=0)==1):
        raise ValueError('Expected one measured receiving-water boundary crossing per column')
    lower=np.argmax(crossings,axis=0)
    a=clearance[lower,cols];b=clearance[lower+1,cols]
    coast_v=v[lower,cols]+a/(a-b)*(v[lower+1,cols]-v[lower,cols])
    coast_actual=source['constraint_target_height_m'][rows,cols].astype(float)
    tangential=u[0]
    knots=np.linspace(float(tangential.min()),float(tangential.max()),11)
    levels=np.array([np.median(coast_actual[np.abs(tangential-k)<=30.]) for k in knots])
    coast_smooth=np.interp(tangential,knots,levels)
    distance=np.maximum(0.,v-coast_v[None,:])
    coast=coast_smooth[None,:]+(coast_actual-coast_smooth)[None,:]*np.clip(1-distance/20.,0.,1.)
    if variant=='massed':
        # Reuse the established two-dimensional mountain interior as the
        # hinterland only. It no longer has to acquire a 200 m scarp through
        # harmonic boundary fitting. This is not new noise on the old result.
        mountain=source['unconstrained_massif_height_m'].astype(float)
        plateau=24.+.40*(mountain-24.)+.012*(u-250.)
        front_knots=np.array([0.,35.,86.,126.,169.,215.,262.,307.,353.,404.,451.,500.])
        front_values=np.array([34.,49.,31.,52.,43.,72.,47.,67.,38.,56.,31.,44.])
        toe=np.interp(u,front_knots,front_values)
        basal=coast+.10*np.minimum(distance,70.)
        raw=np.maximum(basal,np.minimum(plateau,coast+3.25*(distance-toe)))
        # Different cap levels and overlapping widths expose finite masses
        # along the face instead of extruding four prisms to the skyline.
        # Strike families are shared; spacing, size and termination are not.
        masses=[(38.,18.,.48,2.5,.20),(83.,7.,.81,2.2,-.22),
                (122.,30.,.38,3.4,.20),(157.,14.,.64,2.7,.20),
                (201.,37.,.90,2.1,-.22),(238.,22.,.47,3.2,-.22),
                (282.,31.,.71,2.6,.20),(328.,13.,.36,3.0,-.22),
                (367.,18.,.87,2.3,.20),(416.,35.,.57,3.4,-.22),
                (464.,13.,.75,2.9,.20)]
        for center,front,level,flank,strike in masses:
            lateral=u-center+strike*(distance-90.)
            cap=coast+(plateau-coast)*level-flank*np.abs(lateral)
            face=coast+4.1*(distance-front-.08*lateral)
            raw=np.maximum(raw,np.minimum(face,cap))
        before_catchments=raw.copy()
        # Bounded incision, never an unbounded V-plane through the entire
        # plateau. Unequal trunks with one short joining tributary.
        channels=[(132.,.20,22.,165.,20.,17.),(309.,-.22,38.,193.,29.,23.)]
        for center,strike,start,end,depth,width in channels:
            t=np.clip((distance-start)/(end-start),0.,1.)
            length=np.minimum(np.clip((distance-start)/22.,0.,1.),np.clip((end-distance)/40.,0.,1.))
            trace=center+strike*(distance-90.)
            lateral=np.clip(1.-np.abs(u-trace)/(width+8.*t),0.,1.)
            raw-=depth*length*lateral
        trace=132.+.20*(118.-90.)-.30*(distance-118.)
        along=np.clip((distance-118.)/18.,0.,1.)*np.clip((190.-distance)/35.,0.,1.)
        raw-=10.*along*np.clip(1.-np.abs(u-trace)/15.,0.,1.)
        raw=np.maximum(basal,raw)
        valid=source['heightfield_valid_mask']>0
        raw[~valid]=source['constraint_target_height_m'][~valid]
        return raw,dict(plateau=plateau,coast=coast,distance=distance,toe=toe,
                        catchment_cut=np.maximum(0.,before_catchments-raw)),dict(
            variant=variant,primary_scarp_slope=3.25,shared_joint_strikes=[.20,-.22],
            front_tangential_knots_m=front_knots.tolist(),front_inland_positions_m=front_values.tolist(),
            masses=[dict(center_u_m=a,front_inland_m=b,relative_cap=c,flank_slope=d,strike=e) for a,b,c,d,e in masses],
            channels=[dict(center_u_m=a,strike=b,start_m=c,end_m=d,max_depth_m=e,width_m=f) for a,b,c,d,e,f in channels],
            hinterland='Existing v3 unconstrained heterogeneous/ridged field compressed to40% relief about24m; no fresh noise',
            shoreline_detail_decay_m=20.,shoreline_coordinate='measured clearance zero crossing',
            noise_used=True,new_noise_generated=False,erosion_used=False)
    # One inclined structural datum continues through the scarp and hinterland.
    # Broad unequal ridges are intersections of rock planes, not noise fields.
    plateau=28.+.045*(u-250.)+.018*(v-230.)
    plateau+=np.maximum(0.,15.-.13*np.abs(u-155.+.22*(v-250.)))
    plateau+=np.maximum(0.,10.-.20*np.abs(u-390.-.28*(v-250.)))
    front_knots=np.array([0.,70.,145.,220.,300.,390.,500.])
    front_values=np.array([24.,39.,19.,58.,28.,47.,22.])
    toe=np.interp(u,front_knots,front_values)
    basal=coast+.11*np.minimum(distance,60.)
    raw=np.maximum(basal,np.minimum(plateau,coast+3.6*(distance-toe)))
    # Unequal buttresses remain connected to the plateau; lateral planes make
    # finite promontories, with oblique traces shared by the catchment system.
    buttresses=[(72.,9.,4.5,14.,1.25,.20),
                (216.,17.,4.8,18.,1.00,-.22),
                (364.,8.,5.1,12.,1.45,.20),
                (458.,12.,4.3,7.,1.15,-.22)]
    for center,front,slope,boost,flank,strike in buttresses:
        lateral=u-center+strike*(distance-120.)
        cap=plateau+boost-flank*np.abs(lateral)
        face=coast+slope*(distance-front-.12*lateral)
        raw=np.maximum(raw,np.minimum(face,cap))
    before_catchments=raw.copy()
    # Two finite V-shaped drainage recesses follow the same joint directions.
    # Their floors rise into the plateau, so they naturally terminate instead
    # of engraving identical grooves across the full height/whole sector.
    catchments=[(170.,.22,80.,.62,2.8),(333.,-.22,120.,.70,3.1)]
    for center,strike,pivot,grade,bank in catchments:
        trace=center+strike*(distance-pivot)
        floor=coast+grade*distance+bank*np.abs(u-trace)
        raw=np.minimum(raw,np.maximum(basal,floor))
    # One unequal upper tributary meets the first main recess at160m inland.
    trace=170.+.22*(160.-80.)-.38*(distance-160.)
    branch=coast+.64*distance+3.3*np.abs(u-trace)
    raw=np.where(distance>=160.,np.minimum(raw,np.maximum(basal,branch)),raw)
    valid=source['heightfield_valid_mask']>0
    raw[~valid]=source['constraint_target_height_m'][~valid]
    return raw,dict(plateau=plateau,coast=coast,distance=distance,toe=toe,
                    catchment_cut=np.maximum(0.,before_catchments-raw)),dict(
        variant=variant,geological_datum_m=28.,bedding_dip_dz_du_dv=[.045,.018],
        primary_scarp_slope=3.6,primary_scarp_angle_degrees=float(np.degrees(np.arctan(3.6))),
        front_tangential_knots_m=front_knots.tolist(),front_inland_positions_m=front_values.tolist(),
        buttresses=[dict(center_u_m=a,front_inland_m=b,slope=c,cap_boost_m=d,flank_slope=e,strike=f) for a,b,c,d,e,f in buttresses],
        catchments=[dict(center_u_m=a,strike=b,pivot_inland_m=c,floor_grade=d,bank_slope=e) for a,b,c,d,e in catchments],
        shoreline_detail_decay_m=20.,
        shoreline_coordinate='linear zero crossing of measured shore_clearance_m; not first valid raster row',
        noise_used=False,erosion_used=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--variant',choices=('escarpment','massed'),default='escarpment')
    args=parser.parse_args()
    if args.output.exists() or 'saved' not in [p.lower() for p in args.output.resolve().parts]:
        parser.error('Use a new ignored Saved output directory')
    with np.load(args.input,allow_pickle=False) as z:source={k:z[k].copy() for k in z.files}
    old_meta=json.loads(str(source['metadata_json']))
    valid=source['heightfield_valid_mask']>0;fixed=source['protected_mask']>=1
    target=source['constraint_target_height_m'].astype(np.float32)
    raw,fields,settings=structural_field(source,args.variant)
    height,correction,solver=screened_fit(raw,target,valid,fixed,float(source['cell_size_m']),fit_length_m=18.,max_iterations=1600)
    active=valid&~fixed
    if not np.array_equal(height[fixed],target[fixed]) or not np.isfinite(height).all():
        raise ValueError('Structural field violated fixed finite targets')
    erodibility=np.where(fields['catchment_cut']>6.,.55,.28).astype(np.float32)
    erodibility[fixed]=0.
    result={k:source[k].copy() for k in ('cell_size_m','frame_origin_world_m','basis_u_world','basis_v_world','basis_up_world',
        'world_xy_m','u_m','v_m','shore_clearance_m','heightfield_valid_mask','outflow_mask','protected_mask','constraint_type',
        'constraint_target_height_m','coast_distance_m','anchors_json')}
    result.update(height_m=height,bedrock_m=(height-(4.+8.*erodibility)).astype(np.float32),erodibility=erodibility,
        loose_m=(.04*erodibility).astype(np.float32),unconstrained_massif_height_m=raw.astype(np.float32),
        constraint_correction_m=correction.astype(np.float32),structural_catchment_cut_m=fields['catchment_cut'].astype(np.float32),
        structural_front_inland_m=fields['toe'].astype(np.float32))
    result['bedrock_m'][fixed]=height[fixed]
    gy,gx=np.gradient(height.astype(float),float(source['cell_size_m']))
    slope=np.degrees(np.arctan(np.hypot(gx,gy)))
    metrics=dict(valid_cells=int(valid.sum()),fixed_land_cells=int((fixed&valid).sum()),
        fixed_targets_bitwise_preserved=True,valid_domain_unchanged=True,
        raw_height_range_m=[float(raw[valid].min()),float(raw[valid].max())],height_range_m=[float(height[valid].min()),float(height[valid].max())],
        free_correction_abs_p50_p90_p99_m=np.percentile(np.abs(correction[active]),[50,90,99]).tolist(),
        free_correction_over25m_fraction=float((np.abs(correction[active])>25).mean()),
        free_correction_over50m_fraction=float((np.abs(correction[active])>50).mean()),
        slope_degrees_p50_p90_p99=np.percentile(slope[active],[50,90,99]).tolist(),solver=solver)
    meta=dict(old_meta)
    meta.update(schema='directorscut.structural_cliff_sector.v1',source_kind='structural_cliff_macro_landform',
        source_seed_path=str(args.input.resolve()),source_seed_sha256=sha(args.input),generator_sha256=sha(__file__),
        settings=settings,metrics=metrics,erosion_applied=False,transport_masks_valid=False,
        limitations='Art-directed 2 m heightfield macro structure; no overhangs, true 3D joint exposure, calibrated tectonics or live integration. Fixed inherited submerged toe needs water-context review; partial temple coverage remains partial.',
        inherited_constraint_metrics=old_meta.get('metrics',{}),arena_integrated=False,visual_accepted=False)
    result['metadata_json']=np.asarray(json.dumps(meta));result['seed_metrics_json']=np.asarray(json.dumps(metrics))
    args.output.mkdir(parents=True)
    np.savez_compressed(args.output/'seed.npz',**result)
    (args.output/'seed.json').write_text(json.dumps(meta,indent=2)+'\n')
    frame=dict(frame_origin_world_m=source['frame_origin_world_m'],basis_u_world=source['basis_u_world'],
        basis_v_world=source['basis_v_world'],basis_z_world=source['basis_up_world'])
    for name,data in (('baseline',source),('candidate',result)):
        vertices,triangles,_,_=original_mesh(data)
        colors=np.tile(np.array([0.,0.,0.,1.],np.float32),(len(vertices),1))
        mesh_meta=dict(old_meta if name=='baseline' else meta)
        mesh_meta.update(variant=name,diagnostic_clay_only=True,transport_masks_valid=False,
                         mesh_source_seed_path=str(args.input.resolve()) if name=='baseline' else str((args.output/'seed.npz').resolve()),
                         mesh_source_seed_sha256=sha(args.input) if name=='baseline' else sha(args.output/'seed.npz'))
        np.savez_compressed(args.output/(name+'.npz'),vertices_m=vertices,triangles=triangles,vertex_rgba=colors,
            **frame,metadata_json=np.asarray(json.dumps(mesh_meta)))
    print(json.dumps(dict(output=str(args.output.resolve()),metrics=metrics),indent=2))


if __name__=='__main__':main()
