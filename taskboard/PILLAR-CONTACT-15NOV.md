# Pillar passage and water contact

Branch: `codex/pillar-water-contact`, based on main `f11a89f`.

The repeated broken-column clusters spawned near the player's current lane, with only 5.6–10.4 seconds between obstacles. The entire rendered cluster could also exceed its nominal collision radius. Their bases followed the analytic channel rise, lifting stone out of the plugin water.

City obstacles now alternate between fixed sea-space positions 26–34 metres either side of the route. Their complete rendered footprint is fitted within a 9-metre radius, with 12.6–15.4 seconds between spawns. This leaves at least 12 metres beyond the 5-metre ship half-width on the central route; physical stone collision remains enabled.

City rocks and pillars stay at mean sea minus 3.5 metres and skip the analytic trough-height adjustment. Existing lateral travel and scene transforms remain. Upright pillars receive six-metre submerged foundations. A small component draws broken-edge foam around each actual foot and emits bounded, local-space spray bursts, following sampled DRW water height. It reuses installed wake/water assets; no asset generation is required.

Integration dependency: cherry-pick the steering/water agent's `DCOcean::WaveSamplePosition` API first. This helper is used only for water query coordinates; visible contact positions remain in stage space. DCTreadmill edits are confined to obstacle registration/spawn, one serial field and the tagged-anchor exclusion, outside steering integration/prediction.

Validation: standalone MSVC CPU check passed 566,100 footprint positions across both sides and orientations; minimum hull clearance 1,200 cm, minimum longitudinal footprint gap 17,100 cm at city speed. `git diff --check` passes. Unreal automation `DirectorsCut.Navigation.CityPillarClearance` is included but has not been run. No Unreal build, editor/GPU launch or native visual validation was performed in this worktree.

Root's next review: continuous traversal using `-DCStartAt=60 -DCRunThrough -DCPillarReview -DCPillarLog -DCAutoShots=90,100,110,120`. The review flag makes all city obstacles columns for reproducibility, leaving ordinary play's mix unchanged. Check the fixed stone waterline while steering, foam/spray at the feet, central-route clearance and physical collision when deliberately steering into a column. Logs prefixed `PILLAR_CONTACT` report foundation creation and active contact samples. Also test without the review flag before packaging.
