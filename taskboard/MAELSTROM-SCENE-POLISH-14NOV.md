# Maelstrom environment checkpoint � 14 November

Owner: Codex. Branch: `codex/maelstrom-saturday`, following `0a7beef`. Isolated project: `E:/Projects/DirectorsCut/Maelstrom-Saturday`.

The user prioritizes scene, water, lighting, clouds and VFX. **Boss iteration is deferred. This is a visual-work checkpoint, not a finalized or reference-quality scene.** Crater diameter remains 3.2 km and depth 900 m. Early-game, crew, gunnery, HUD, treadmill, ruins placement and purchased sky-pack code are unchanged.

## Changes

- Localized rim-channel variation near the falls instead of carrying it down the entire bowl as long mountain-like ridges. Broader irregular swells and asymmetric standing crests are part of the actual water mesh; surface queries use the same geometry. Removed a rejected separate-sheet breaker experiment that looked detached from the water.
- Fixed an introduced foam-coordinate defect: dotting absolute position against its rotating radial basis reduced the texture to radius and produced concentric contour stripes. Foam now samples planar coordinates advected by the circulating current.
- Generate an original transient 768-square crest map from the crater. Foam can travel away from its source instead of remaining painted on a ridge. No purchased texture is copied into this map. Optional installed DRW detail and live FFT normals remain supported; this is not FFT-driven geometric displacement or fluid simulation.
- Added lit, torn spray at crests and small airborne bursts. Reduced distant ripple normals and FFT contribution to limit metallic high-frequency highlights. Enabled the material's pixel-animation flag for temporal reconstruction.
- Reduced the blanket of height fog while retaining localized mist at waterfall feet. Adjusted maelstrom daylight direction, neutral sun color, sky fill, cloud coverage and shaft scattering. These are stage cues; no global sky-pack source was edited. The cloud-height override tested in a diagnostic run is **not** part of the implementation.
- Reworked the exit tube's lip and larger wall ripples, increased its tessellation near the mouth, corrected inward triangle winding and calculated normals from the actual surface. Increased water displacement is confined to the basin instance; the portal retains its explicit amplitude.
- Added `-DCSceneReview` to suppress encounter actors for environment inspection. Normal gameplay still has the existing boss and hazards. Review tooling now supports named capture sets, native rendering, resolution, tenths of seconds, waiting and stale-image detection.

## Validation

UE 5.8 `DirectorsCutEditor Win64 Development` builds. The existing `DirectorsCut.Maelstrom.RouteAndSteering` automation test passes with exit code 0 (`Saved/ScenePolishRouteTests.log`), covering continuity, containment and 30/120 fps steering consistency. The final material generator completed without shader errors. The scene-only continuous run `Saved/art8.log` travelled from 268 to 451 seconds and returned to open sea; subsequent changes concern foam, tunnel normals and atmosphere, not route scheduling.

Final-source run `Saved/scene-polish-native.log` captured 272, 276, 280, 287 and 325 seconds at **native 2560x1440**, without DLSS or frame generation. Sampled frame rates were 64�83 fps. These are individual offscreen samples on this workstation, with the boss disabled; they are not a packaged-game or 4K performance guarantee. Smaller DLSS previews showed more unstable distant edges. Separate reflection-mode tests did not justify changing the game's reflection mode.

Review images: [post-reveal](review/maelstrom-scene-14nov/reveal-280.png), [descent](review/maelstrom-scene-14nov/descent-287.png), [orbit](review/maelstrom-scene-14nov/orbit-325.png). These are unretouched game captures. Still-frame inspection and consecutive captures do not replace watching the sequence in motion.

Replay:

```powershell
./Tools/review_maelstrom.ps1 -Continuous -EnvironmentOnly -Native -Width 2560 -Height 1440 -Wait -ReviewName scene-review -Start 268 -Shots '272,276,280,287,325'
```

## Remaining visual work

1. Water still has overly regular swell groupings, soft surface detail and conspicuous pale patches. Integrate the installed water system's actual displacement with the curved bowl, paired normals and safe hull region; do not simply increase normal intensity. Current large crests are standing geometry with animated surface detail.
2. Foam needs finer edge breakup and more convincing propagation/decay; crest spray is a foundation, not the reference's airborne white water. Preserve dark troughs and test from the helm and both cannon cameras in motion.
3. Cloud composition remains the existing sky pack with adjusted phase lighting. It needs a dedicated stage composition and cloud-shadow pass before calling it cinematic. Avoid adding a second competing cloud layer or hiding weak water under uniform fog.
4. Validate the brief bright exit, mist density and temporal edge stability in an interactive playthrough. Audio was muted in these tests. Existing editor-toolset Python startup errors remain unrelated to this visual pass.

No new packaged build or main-branch merge was made. The old overnight executable does not contain this work. Keep boss design and balance deferred until the environment passes the owner's visual review.
