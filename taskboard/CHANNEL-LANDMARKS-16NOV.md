# Larger water hazards and varied shorelines

Owner: tunnel_boss, codex/channel-landmarks, based on main ac37b0c. CPU/source only while human tests. No engine, build, package or asset changes.

## History and scope

Compared DCTreadmill before/after8e52bc8 (the route-clearance fix) and the older random obstacle spawner. Earlier visual variation came from irregular complex/arch/crag footprints. The clearance fix correctly moved rotated architecture outside the hull corridor, then added two ruler-straight banks to support walkers. Reverting that would restore floating/overlapping masonry and unsupported enemies.

This change restores irregular shore shapes and rock landmarks within explicit clearance bounds. The voyage centreline and water/ship transforms stay the same; this is shoreline and encounter variation, not a new curved navigation route.

## Changes

- Pillar minimum visible height above the conservative wave envelope rises6m to24m. Spawned cluster scale rises0.55-0.75 to1.1-1.35; actual full XY footprint remains fitted, capped at15m radius instead of9m. Clear opposite lane moves33m to42m.
- Existing five staggered26s encounters now alternate three large pillar clusters with two restored crag hazards. Crags request35-45m source height and receive the same submerged datum, minimum visibility fit, wave-contact foam/splash and one-shot hull impact/pushaway. No vendor asset changes. DCPillarReview still forces all columns for comparisons.
- Each400m bank becomes eight50m segments. The spare inboard strip recedes0-9m with different port/starboard profiles; outer terraces extend7-43m. Every visible section has a matching collision proxy. Continuous fixed-height enemy support is preserved.
- Eight broad rock shoulders alternate along the banks, outside the fully rotated hull envelope but inboard of every existing enemy footprint. They are36-64m along-shore,9m across-shore, with varying28-42m source heights and deeply submerged feet. Existing crag meshes/materials are reused.

## CPU evidence

Tools/probe_water_ruins.py passes129,340 checks. Production helm response at120/30/10Hz,2s reaction and full/35% authority clears all five enlarged hazards; worst sampled margin6.69m. Full-yaw passing-lane and longitudinal spacing checks pass. Water-datum and wave-top tests remain valid. Shoreline tests cover both banks over4km of route samples, all actual imported Rampage footprints for3/4-creature waves, statue pedestal margin, and non-overlap with rock shoulders. One-shot damage policy unchanged. CityPillarClearance native test expectations updated to24m exposed height.

The damaged-rigging simulation starts near the centre with a2s reaction; it is not a promise that every late steering decision can recover.

## Root visual/native acceptance after human releases GPU

Use current harness, continuous -DCStartAt=60, DCPillarLog and DCLaneLog. First pillar pass near107; first rock near133; later pillars/rock near159/185/211.

1. Normal helm view at96/101/105/108/126/130/133/159: columns must be visibly much larger and rock silhouettes readable early enough to choose a side. No giant wall across the route.
2. Side view -DCShotCam=-4000,-6000,1700,-18,37 at106/107/108 and132/133/134: solid submerged bases, foam meeting stone, no cylinder foundation exposed above the actual waves.
3. Collision: DCStation=helm DCSteer=0 DCNoCurrent; require one CITY_WATER_IMPACT per actor, splash/jolt/damage and continuous separation. Existing cooked splash only.
4. Held starboard DCSteer=1 and port DCSteer=-1 passes: no bank/shoulder overlap; no floating walker or statue bases. Inspect bank silhouettes around135/165/195 from ordinary helm plus an elevated overview if needed. Scalloped banks are structural outlines, not moving waves.
5. CPU native automation DirectorsCut.Navigation.CityPillarClearance, and existing DCNavigationVerify for engine sweeps.

Runtime log CITY_WATER_RUIN now includes kind=rock or kind=pillars. PILLAR_FIT logs the actual new top heights for both. Native art/performance and exact collision acceptance remain pending; larger bank component count should be checked in representative captures.
