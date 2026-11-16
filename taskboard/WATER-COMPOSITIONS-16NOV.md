# Distinct water-obstacle compositions

User requested more varied in-water compositions quickly after0049. Root owns this source-only pass; existing playtest remains untouched. Five encounters now use five deliberately different assemblies: unequal column crescent, three-peak rock reef, sunken arch and broken neighbouring pier, split rock ridge with a ruined column, and diagonal fallen arcade. Each includes bottom-anchored fallen masonry and uses existing cooked AncientRuins meshes.

New DCRuins::SpawnWaterComposition builds each assembly with explicit bottom placement, size and yaw. The existing treadmill registration fits the complete footprint to15m before placement, then applies existing submerged water contact/foundations. Timing, committed lanes, clear passing lanes, warning lifecycle and one-shot collision/pushaway remain unchanged. DCPillarReview and unavailable-pack fallbacks are retained. No new assets or recook needed.

Editor compile passed22.43s; existing129,340 lane/water/support checks pass and diffcheck is clean. This is not visual acceptance of the new assemblies. Native normal-helm/side views of the five passes (about107/133/159/185/211) and a new isolated package are queued behind the active human playtest. Keep0049 selected until accepted; root asked whether the human is finished. Do not terminate their game.
