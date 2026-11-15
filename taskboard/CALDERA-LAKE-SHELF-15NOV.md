# Caldera descent water correction — 15 November

Review branch: `codex/maelstrom-saturday`, based on combined main `3368536`. This patch is isolated from the packaged local candidate and does not publish or change main.

## Cause and change

The flat lake inherited the ship's hull-clearance displacement while the ship was still hundreds of metres above it on the inlet. The shader could therefore pull nearby lake vertices up to the ship's height beneath the descent. CPU route samples put the lake 338.30 m below the ship at encounter second 5 and 244.52 m below it at second 10. This is a separate displacement defect: the native comparison below does **not** establish it as the cause of the visible dark triangle beside the ship.

`make_maelstrom_materials.py` now applies hull flattening to the lake only when the undisplaced water is near the ship's waterline: full strength within 3 m, smoothly fading to zero at 8 m. The cataract retains its separate hull protection and existing terminal-overlap gate. The shared gate feeds displacement, FFT normals and fallback normals; the normal derivative already includes the gate gradient. World position excludes shader displacement but includes the arena transform, so the check uses the actual vertical separation in centimetres.

The accepted lake size, inlet shape, route, terrain, bank fog, dense waterfall impact fog and selected sky are unchanged. This does not address the separate exposed river-bed slivers caused by flattening steep cataract water against the shallow bed.

## Validation

The review checkout was fast-forwarded to combined main before this change. Its missing licensed pirate content and ACE plugins were copied locally from main, with independent plugin binaries. The release-content preflight passes all 37 required assets and configured cook directories. Editor and Game builds pass in `Saved/BuildCalderaLakeShelfEditor.log` and `Saved/BuildCalderaLakeShelfGame.log`.

The water-only commandlet passes in `Saved/ImportCalderaLakeShelf.log`, including `optional live FFT graph=True`. It rebuilt only `M_MaelstromWater`; the four independently modified cascade/impact-mist/spray/whitewater assets remain uncommitted and untouched by this pass. Baseline native captures are in `Saved/SceneReview/CalderaLakeShelfBefore`.

Native post-change review passes in `CalderaLakeShelfAfter`: actual ride times 281, 285, 289, 320, 463 (open sea), then 275 (backward seek). `CalderaLakeShelfOverview` covers actual times 276, 285 and 315. Both processes exited normally; logs confirm eight live DRW FFT cascades, no failed water/cliff material compilation and restored sky settings. The existing experimental editor-toolset Python startup errors and a non-Nanite shadow-queue warning remain; this is not a clean release/performance certification.

Screenshot filenames are seek requests, not exact capture times: the harness advances about five to six seconds while settling. Use the logged `Auto shot` times when comparing images or describing route positions.

## Visual result and next issue

- [Before at ride 285](review/caldera-lake-shelf-15nov/before.png) and [after at ride 285](review/caldera-lake-shelf-15nov/after.png) are nearly identical from the deck. The exposed dark river-bed sliver remains. Do **not** describe this patch as fixing that visible defect or as a major visual improvement.
- [Current inlet overview at ride 315](review/caldera-lake-shelf-15nov/overview.png) confirms the low shoreline haze, dense waterfall-foot fog, level lake and accepted banks remain intact.
- The steep early inlet still creates a water trough around the nearly level ship, and the outgoing tunnel remains visibly stylized in the external diagnostic at ride 276. Those are unresolved geometry/composition issues. More fog is not the primary remedy for the bed intersection.

The safe next investigation is the cataract's own hull-flatten footprint versus the actual bed and ship envelope. Any bed change must retain bank contacts, the full steering envelope and matching regenerated source/support hashes. No terrain change was made in this patch.

### Actual-triangle diagnosis after the native review

`Saved/CalderaHullBedProbe/report.json` and its sibling `probe.py` reconstruct the production 240x64 water mesh, flatten each vertex, then query its resulting triangles and the accepted source rock triangles. They include the route's arena pitch/yaw/heave. At encounter seconds 5/10/14, the bed protrudes 7.74/10.22/11.03 m above flattened water **inside the recorded HullShip bounding footprint**, and 10.38/13.50/14.89 m outside it. The original sloped water covers the sampled bed by at least 3.99 m.

A tighter ellipse still leaves 6.61-9.68 m exposure inside the footprint and allows water up to world Z=11.25 m there, above the recorded hull maximum Z=10.39 m. Do not ship a mask-only reduction without checking flooding. Keeping the horizontal ship requires deeper upstream navigation-bed clearance; the sampled hull-bottom bound suggests approximately 16.2 m additional lowering for a 1 m margin. That is a starting bound, not a finished geometry prescription: the probe uses a static bounding box, omits dynamic waves/transient jolts, and does not certify exact hull collision or the full steering route. Prefer an underwater-only correction retaining the visible banks, then regenerate/audit all source supports and test actual descent motion.

## Integration

Cherry-pick only this scoped patch after local visual review. The material is tracked; alternatively regenerate it with `Tools/make_maelstrom_water_only.py` using the normal Unreal commandlet flags and `-DDC=InstalledNoZenLocalFallback`. Do not run the full multi-material builder. Preserve main's packaging shader shim, mandatory caldera texture checks and `r.Material.StripUnusedDefaultTextures=0`.

No new package is produced here. The combined main playable remains the user's local testing candidate; do not overwrite its frozen content or upload it automatically.
