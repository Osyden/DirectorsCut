# Caldera steering water, descent and landing — 15 November

Owner: voice_audio subagent, `codex/caldera-motion-15nov`, based on main `9c3ecd3`. Root owns builds/material generation/GPU review. Source changes were made in the isolated Crew-Deck-Routes worktree; no engine or GPU process was launched here.

The live FFT branch of `M_MaelstromWater` sampled stage-world XY, while the authored foam and shoreline used basin-local UVs. Because the arena rotates around a stationary ship, this made the dominant wave field remain aligned to the ship as the banks turned. The generator now samples displacement, surface derivatives and accumulated foam in the basin chart. It rotates the sampled horizontal normals/displacements back to stage world. Camera distance, filtering footprints, foam clipmap coverage and the near-hull protection stay in actual stage world. The independent tunnel mapping is unchanged. This requires regenerating the water material, not just compiling C++.

The stage explicitly set pitch to zero to keep the sky/rim level, and generic ship motion supplied no slide pitch. The actual physical ship now follows the steered inlet's sampled grade (smoothed, capped at 34 degrees bow-down). The arena/sky alignment remains unchanged. Crew/player positions continue to use the shared deck transform. The water's protected hull patch and CPU surface query follow that pitched deck plane instead of creating a horizontal shelf through the inclined hull.

`UpdateView` also cleared the camera additive offset after the pawn had computed its deck lean and impact shake. It now preserves those and adds only the existing reveal framing. Order typing skips that addition because the pawn suspends its normal camera update while typing, preventing offset accumulation.

Landing remains one event when the actual steered route reaches lake height; seeks initialize the event state without replaying it. Its existing 65-degree/sec pitch impulse and trauma now remain visible. Added a bounded 90 cm damped compression/rebound, settled at three seconds, and bow-first spray ahead of the rail bursts. No landing damage is added. `MAEL_LANDING` logs the event and spray count; `MAEL_ROUTE` now includes physical `deckPitch`.

Root-requested sound integration: `EmitWaterContact` accepts a fourth optional `bPlaySound=true` argument; sound cooldown is 1.2 seconds and volume uses `DCAudio::GetSpeechDuck()`. Root adds that getter and makes repeated boss water pulses silent. Landing spray is silent; the existing single `Boom(1)` carries its impact. No DCAudio files were changed here.

## Checks and required integration

- `Tools/probe_caldera_motion.py` compiles the actual production route helpers and inlet height with MSVC. All 26,718 sampled assertions pass: finite/bounded smoothed pitch, substantial descent, one landing for each held offset, level lake orbit, bounded compression and settled heave.
- At unguarded fixed offsets -90/0/+90 m, maximum pitch is -32.95/-31.47/-34 degrees; lake contacts occur at elapsed 22.12/29.16/49.68 seconds. These remain explicit control cases in the probe; the inlet guard below prevents the unsafe outer control from occurring in live steering.
- Added six focused assertions to the existing `DirectorsCut.Maelstrom.RouteAndSteering` automation test. Python generator syntax and `git diff --check` pass.
- Root must regenerate only `Tools/make_maelstrom_water_only.py` in the main checkout, compile the combined source (including the root-owned audio getter), then inspect a normal-speed descent and landing with `-DCMaelstromRouteLog -DCRunThrough` and shots spanning ride 275–305. Compare steering both ways on the lake with fixed shore references, including water review normal/foam modes if needed.
- Confirm normal and displacement axes turn together, foam stays registered to the lake, inclined hull does not intersect the protected water patch, helm/deck camera follows the bow-down ship, impact occurs once, typing does not accumulate camera offsets, and the horizon still matches the sky. Test seek/restart without a spurious impact. Native material compilation and rendered acceptance remain pending; CPU checks do not establish those results.

## Inlet containment follow-up

The audit of actual held-starboard production steering established a real failure: at elapsed 42.98 seconds the hull left the cataract's visible 160 m half-width (including 12 m hull margin), but the CPU height remained 13.32 m above the lake. At 51 seconds it dropped that entire distance in one frame. The route's broad angular height test outlived the rendered inlet sheet. This was not just delayed impact timing.

`InletOutwardLimit` now progressively closes the outward part of the steering corridor over elapsed 16–30 seconds. The existing hydraulic restoring force follows that boundary, with a final bounded projection for safety. Inward steering remains available. Full outward steering is progressively restored only after the centreline has passed the inlet's angular footprint plus bow margin (250–350 m across that footprint, approximately elapsed 58–70 s). This changes no enemy timers, cannon arcs or attack windows, and leaves accepted terrain/sky geometry unchanged.

The expanded actual-source CPU probe passes 26,754 sampled assertions. Held port/neutral/starboard all remain within the visible inlet's hull margin at 30/60/120 fps. Port lands at 23.20 seconds; neutral/starboard land at 29.16–29.20. At 60 fps the greatest descent steps are 51.1/37.5/38.7 cm respectively, replacing starboard's former 1332 cm snap. Both directions recover their normal ~70.9 m held-steering balance after clearing the inlet. Native held-starboard descent remains a required acceptance view.

The hull-plane origin was also checked: `DCGame` constructs `Deck(Lean, Pivot-Lean.RotateVector(Pivot)+(0,0,Heave))` for `Pivot=(0,0,SeaLevelZ)`. Therefore `Deck.TransformPosition(Pivot).XY` is identically zero. The CPU/shader plane equations correctly use zero XY under that shared invariant; no extra origin parameter is needed.
