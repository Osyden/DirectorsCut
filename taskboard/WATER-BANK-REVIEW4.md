# Water-bank differential normal pass

Owner: Codex water worker on `codex/crater-bank-review4`, based on `24c0fcb`, isolated `Crater-Workers/Flow-Seams`.

Claim: basin normal construction in `Tools/make_maelstrom_materials.py` only. Route, bank heights, breakers, foam coverage, roughness, sky, cliffs, falling water and volume caches remain unchanged. Root owns material generation and visual acceptance.

Implemented: correct the missing flow-map chain rule and the missing physical surface metric when turning arena-XY wave derivatives into a tangent normal. The previous code interpreted planar derivatives directly as orthonormal tangent slopes, which is only correct on a flat surface with an identity flow map.

## Evidence and change

Viewed the actual root `Saved/SceneReview/crater-contact3-wide/shot_0300.png` and `shot_0420.png`. The far bank has coherent bright small ripples across a very smooth large profile; the nearby surface also carries large white/metallic areas. Prior foam-mask review established that broad pale regions can be reflection rather than actual foam. This pass does not add foam, turn down specular, change roughness, increase wave amplitudes or alter the route to disguise that finding.

The material samples two rotating/scaling advection maps, `q` and `q2`. Their wave derivatives previously got averaged and immediately treated as unit tangent-frame slopes. The change keeps those derivatives separate, applies each map's screen-space chain rule, and reconstructs the physical surface differential from world-position derivatives in metres. That differential is projected into the existing orthonormal mesh tangent frame. Rotated arenas and steep banks therefore retain the correct gradient orientation and surface stretch. Unreliable degenerate pixel derivatives resolve to the base normal instead of a specular spike. The entrance tunnel retains its previous mapping.

## Geometry finding for a separate pass

Default ride time425 corresponds to maelstrom elapsed155 because the disabled hell phase shifts breach420 to270. At that section the route radius is413.1m and width scale0.7728. The outer channel-to-bowl recovery spans offsets131.4 to255.0m and rises85.4m over123.6m; maximum grade is46.9 degrees at offset214m. At ride420/elapsed150, the same recovery rises98.5m over132.4m and reaches50.6 degrees. These are calculated from the current production equations in the inner-bowl regime, with waves disabled.

Correct shading cannot remove that smooth mean silhouette. Keeping the same playable route, surrounding bowl elevations and recovery span requires taking up that height difference somewhere. Do not claim that changing a blend constant alone can make it shallow without affecting another contract. If the matched normal pass still leaves it terrain-like, the next bounded geometry change should add a measured population of short, broken physical wave packets to this bank outside the steering corridor, preserving mean height and shoreline clearance. Existing sparse `BreakingWaves` occupy only small patches, while the background CPU relief is just40cm +20cm noise. Use spatially indexed finite packets rather than multiplying the current all-waves-per-vertex loop; review physical12–40m wavelengths and roughly0.5–2m relief against the mesh spacing before increasing density. No route/geometry change was made here.

## Verification and integration

Python AST parsing and `git diff --check` pass. A CPU comparison over256 tilted planes, rotated arenas and two independent affine flow maps matches the analytic displaced-plane normal to maximum vector error3.55e-16. The old direct tangent shortcut differs by up to32.95 degrees in that fixture. This checks the chain-rule/metric reconstruction, not Unreal compilation or visual quality.

Root must regenerate `Tools/make_maelstrom_materials.py` and compare matching beauty/normal views at300 and420/425, plus adjacent frames to check specular stability. Source target compilation is not required for this script-only candidate. Inspect the entrance tunnel once because the material is shared. No Unreal, GPU, material generation or engine build ran in the worker, and visual acceptance remains pending.
