# Descending spiral geometry prototype

Owner: Codex water chat, `codex/maelstrom-saturday`, isolated checkout `E:/Projects/DirectorsCut/Maelstrom-Saturday`. The user authorized this prototype after proposing a shaped descending current, connected side waterfalls, and an eventual underwater exit. This pass proves geometry and movement; boss choreography and the underwater transition remain subsequent work.

## Ownership

The user asked for coordination with **Improve Maelstrom sky visuals**. Sent the sky chat the branch, checkout, changed-function boundaries and the need to review lower viewpoints. Water chat edits route, basin geometry, breaker placement and the automatic helm view. Sky chat retains clouds/lighting; this pass does not change atmosphere code or phase-lighting values. Shared `DCMaelstrom.cpp` edits must be merged by function, not copied wholesale.

## Prototype

- Preserve the nominal 3.2 km diameter and 900 m depth. Existing irregular rim bays remain.
- Replace the short outer orbit with a leftward arc of roughly half a revolution. Radius contracts from about 1.55 km at the breach to 192 m at the final approach. The route already has tangential speed inside the exit tunnel.
- Shape a continuous banked current into the same basin surface. A broad higher outer shoulder and low inner spill frame the path. Blend the shoulders back into the basin without overlapping water surfaces. Vary bank height along the arc and narrow both geometry and steering range near the throat.
- During the final approach, dissolve the banks into the central bowl. The first full run exposed a tall folded wall when the wide cross-section persisted at a 200 m radius; the final geometry removes that convergence artifact.
- Use a common lower bowl with a continuous connection to the irregular rim; keep the join below the lip to prevent cracks at outward bays.
- Keep the ship at the world origin and apply inverse route travel to the arena. All arena-attached landmarks follow the same frame. Local waves still animate independently. The sky remains under the sky chat's control.
- Aim the helm view toward the approaching spiral with an inward bias. Remove trajectory-derived whole-arena pitch to protect the horizon; this is a ship-centric motion approximation, not a full vessel physics simulation.
- Distribute small crest packets along the longer route. Reduce the visibly card-like crest spray opacity. No new foam or lighting assets are introduced by this geometry pass.

## Review requirements

Build, route/steering automation, a continuous normal-speed scene-only run from the tunnel to open sea, and real deck-camera captures at entry, mid-descent and the inner approach. Inspect the route telemetry for height reversals or jumps. Do not infer convincing motion or final water quality from the passing automation alone.

The cloud chat should evaluate the later deep viewpoints as well as the reveal. Whitewater injection at side-fall contact points, downstream foam trails, the boss splash, and the underwater exit are not implemented by this prototype. Existing combat targets will require a separate placement/balance review against the new route.

## Reproduce the prototype

Run from the isolated checkout, after building the Editor target:

```powershell
./Tools/review_maelstrom.ps1 -Continuous -EnvironmentOnly -Clean -Native -Width 1920 -Height 1080 -Wait -ReviewName spiral-review -Start 268 -Shots '280,300,340,380,420,440,449,451'
```

This is a normal-speed offscreen scene-only review. The previously packaged game does not contain this prototype. Existing sky/light settings are intentionally retained so the sky chat can integrate separately. The end currently uses the existing transition to open sea; it is not the requested future underwater sequence.

## Validation

- UE 5.8 Development Editor build succeeded. Extended `DirectorsCut.Maelstrom.RouteAndSteering` automation passed, covering route continuity, heading, handedness, angle wrapping, inward progression, asymmetric banks, narrowing width and 30/120 fps steering stability.
- A continuous normal-speed 268–451 second scene review completed. All 36 route telemetry samples descended monotonically; the stage exited at 450.01 seconds. No maelstrom material compilation failure occurred. Existing plugin/editor-utility startup warnings remain outside this pass.
- A full held-port run through the narrowing final approach completed and exited at 450.02 seconds. Physical offset settled from roughly 55 m to 43 m as the lane narrowed, with no runaway lateral velocity or crossing through the crater centre. That run preceded only the final geometric bank fade; route and steering code did not change afterward.
- Real unretouched 1920×1080 images are in `review/maelstrom-spiral-14nov`. Early and middle captures come from `spiral-prototype-b`; late captures are replaced by the final approach recheck after the convergence fix. These are geometry-prototype evidence with the old sky, not generated paintovers or finished lighting.

The water still needs a more convincing breaking surface and contact-driven whitewater. Some crest spray reads as cards, and the far waterfall/mist pattern remains repetitive. The camera shows the curling corridor more clearly, but a human motion/playability review is still needed. Boss placement, underwater travel and final cloud integration are not validated here.

The final 410�451 second recheck completed after the central-bank fix, exited at 450.01 seconds, and showed the folded wall removed in the 449-second frame. No water material changes were needed.

Review frames: [exit, 280 s](review/maelstrom-spiral-14nov/shot_0280.png), [early descent, 300 s](review/maelstrom-spiral-14nov/shot_0300.png), [mid-descent, 340 s](review/maelstrom-spiral-14nov/shot_0340.png), [lower arc, 380 s](review/maelstrom-spiral-14nov/shot_0380.png), [inner approach, 420 s](review/maelstrom-spiral-14nov/shot_0420.png), [final approach, 449 s](review/maelstrom-spiral-14nov/shot_0449.png), [open sea restored, 451 s](review/maelstrom-spiral-14nov/shot_0451.png).
