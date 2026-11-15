# Caldera helm, current boundary and bow landing

Source ready on `codex/caldera-helm-boundary`; native generation, build and visual review belong to root. No editor/GPU was launched during the human playtest.

## Behaviour

- Replaces the hidden lane-centering spring with carried radial velocity and predictive braking only near the real limits. Maximum radial speed is 12.5m/s, acceleration is bounded at 19m/s², and neutral coasts to rest in the chosen lane. The existing moving inlet-side guard remains authoritative.
- Visible heading follows travel up to 40°, preserving the fixed slam joint's minimum cannon angle. Boss timing, committed obstacle positions, hit shapes and damage are unchanged.
- Caldera rigging authority has a 0.75 minimum. Torn rigging still reduces steering speed by 25%; the previous 65% reduction made committed tentacles unavoidable from some lane/momentum states. Fog retains its existing rigging penalty.
- Two narrow, broken foam crests mark the inner no-go region. Their outer radius is lake orbit minus the enforced 110m inward limit minus the actual hull corner radius. With the measured hull this is about 794m, enclosing Kraken's inner orbit. They fade in at caldera elapsed34–42s, use existing basin coordinates and the same live FFT displacement/material, and do not rotate water textures or create a separate floating overlay.
- Landing crash, spray, pitch impulse and compression now trigger at first bow-waterline contact with the flat lake. The bow's height uses the same smoothed physical deck pitch, measured hull half-length and production inlet height. The former centre-height trigger was late by 3.73–3.75s in the neutral replay. One-shot and seek suppression remain in place; no landing damage is added.

## CPU evidence

`Tools/test_boss_caldera.ps1`: **47,839 checks pass** at15/30/60/120fps. This compiles only the standalone replay, not Unreal.

- Full counter-rudder reverses carried velocity within0.734s; neutral stopping distance is at most10.99m. Step displacement remains bounded by speed, rather than teleporting the ship.
- 384 actual committed-obstacle sweep scenarios pass, covering both obstacle directions, 2s and3s reaction, intact/torn rigging, lane positions from−95m to+95m where the inlet permits, and initial radial momentum−9/0/+9m/s.
- With torn rigging and2s reaction, the first obstacle's age9s sweep sits80.5m to starboard. The second sits51.4m to port even when starting at−95m with−9m/s wrong-way momentum. Its collider half-width is41m. At strike, those clearances increase to85.7m and58.1m.
- Neutral landing occurs at elapsed25.408–25.467s, instead of29.158–29.200s. With default breach270 this is ride295.4s. Held-port/held-starboard trajectories also contact bow-first. Cross-rate contact variance is less than0.06s.
- Native route automation gains physical bow-contact assertions and preserves its inlet, continuous-route, steering and heave checks. It has not been run by this agent.

## Integration and visual review

Regenerate `Tools/make_maelstrom_materials.py` in the root-owned editor/commandlet. It changes the existing water material's foam branch only; preserve all current FFT, displacement, normals, trough/query and reflection work. Defaults leave portal/tunnel boundary strength zero. No binary assets accompany this source commit.

After integration, use continuous traversal across ride290–301 with `-DCMaelstromRouteLog`; expect exactly one `MAEL_LANDING` near295.4 on neutral helm. Its log includes bow clearance, hull reach and actual deck pitch. Seeks after landing must not replay the crash.

Inspect the inner boundary from the helm near ride315–340, then held inward input and a reversal, using `-DCMaelstromSteer=-1` only for focused review if desired. Confirm both crests remain subdued but visible, remain attached to moving water, enclose the central Kraken area, and coincide with the hull's minimum approach. Visually review40° heading from cannon cameras and verify the fixed slam joint remains reachable. These are remaining visual checks, not claimed completed tests.
