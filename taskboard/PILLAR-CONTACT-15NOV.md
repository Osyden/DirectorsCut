# Pillar passage and water contact

Branch: `codex/pillar-water-contact`, based on main `f11a89f`.

The repeated broken-column clusters spawned near the player's current lane, with only 5.6–10.4 seconds between obstacles. The entire rendered cluster could also exceed its nominal collision radius. Their bases followed the analytic channel rise, lifting stone out of the plugin water.

City obstacles now alternate between fixed sea-space positions 26–34 metres either side of the route. Their complete rendered footprint is fitted within a 9-metre radius, with 12.6–15.4 seconds between spawns. This leaves at least 12 metres beyond the 5-metre ship half-width on the central route; physical stone collision remains enabled.

City rocks and pillars stay at mean sea minus 3.5 metres and skip the analytic trough-height adjustment. Existing lateral travel and scene transforms remain. Upright pillars receive six-metre submerged foundations. A small component draws broken-edge foam around each actual foot and emits bounded, local-space spray bursts, following sampled DRW water height. It reuses installed wake/water assets; no asset generation is required.

Integration dependency: cherry-pick the steering/water agent's `DCOcean::WaveSamplePosition` API first. This helper is used only for water query coordinates; visible contact positions remain in stage space. DCTreadmill edits are confined to obstacle registration/spawn, one serial field and the tagged-anchor exclusion, outside steering integration/prediction.

Validation: standalone MSVC CPU check passed 566,100 footprint positions across both sides and orientations; minimum hull clearance 1,200 cm, minimum longitudinal footprint gap 17,100 cm at city speed. `git diff --check` passes. Unreal automation `DirectorsCut.Navigation.CityPillarClearance` is included but has not been run. No Unreal build, editor/GPU launch or native visual validation was performed in this worktree.

Root's next review: continuous traversal using `-DCStartAt=60 -DCRunThrough -DCPillarReview -DCPillarLog -DCAutoShots=90,100,110,120`. The review flag makes all city obstacles columns for reproducibility, leaving ordinary play's mix unchanged. Check the fixed stone waterline while steering, foam/spray at the feet, central-route clearance and physical collision when deliberately steering into a column. Logs prefixed `PILLAR_CONTACT` report foundation creation and active contact samples. Also test without the review flag before packaging.

## Cooked splash correction

The first packaged review exposed a Cascade SubUV serialization crash while duplicating the cooked vendor particle system. Runtime duplication is removed. Before the next cook, run `Tools/make_pillar_splash.py` in the editor commandlet: it preserves the original and authors `/Game/DC/FX/P_PillarWaterContact` with all emitter LODs in local space. Runtime loads this cooked copy; a missing copy safely falls back to the original and warns. `/Game/DC` is already always cooked. Verify the generator's `PILLAR_SPLASH ... local_space_modules=` success and the packaged `PILLAR_CONTACT ... splash=/Game/DC/FX/P_PillarWaterContact` path; an editor-only pass does not validate this crash fix.

City obstacles also cache their fitted stone radius before adding the absolute-space foam mesh. This prevents its initially empty bounds at world origin from making a 9m pillar report a 650m collision-activation reach. Native `Strikes: ... (reach ...)` should now report at most 900cm for these obstacle actors. No engine or packaged run was performed by the subagent for this correction.

## Rendered water height and exposed columns

Branch `codex/pillar-height-correction`, based on `ca18f29`. The 17:39 package survived creation of the cooked local-space splash; its close-up revealed another concrete problem. DRW's sampled height excludes the city's material-only trough displacement. At the first pillar's sea Y=-2814cm this missing offset is about 255cm. Foam and spray were below the rendered water, while the old 350cm base submergence also hid short column meshes.

`DCOcean::RenderedHeightOffset(Context, StagePosition)` now exposes exactly `SeaRise(StagePosition.Y + LaneOffset) - SeaRise(LaneOffset)`, matching the material WPO. Contact sampling adds it once after raw DRW sampling, including fallback heights. This helper takes unshifted stage coordinates. Raw hull sample heights remain unchanged.

Bases now sit 100cm below mean sea, with eight-metre foundations. Authored upright pieces are explicitly tagged and kept vertical before XY footprint fitting. At creation, only their Z scale is increased as needed, with the transformed bottom held fixed. Their minimum top accounts for the fully developed future trough across the entire 9m footprint, 250cm wave allowance and at least 400cm exposed stone. The future trough is used because early pillars spawn before it develops. No height changes occur while the pillar passes the ship; XY fitting, spacing and collision remain unchanged. `PILLAR_FIT` logs old/new height, foot, actual top and required top for native verification.

Suggested close-up after continuous start at60: `-DCShotCam=0,-4800,2200,-41,90 -DCTenths -DCAutoShots=107.0,107.5,108.0 -DCPillarReview -DCPillarLog`. This looks down at the first port cluster with less foreground-wave occlusion. Confirm visible tall columns, attached waterline wash/spray, fixed foot and no exposed foundation bottom. No new assets or generators are required for this correction.
