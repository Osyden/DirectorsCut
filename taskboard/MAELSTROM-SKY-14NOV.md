# Maelstrom cloud-shape correction — 14 November

The user rejected the cellular shapes in 76ff2cb as foam-like. The default finale now uses an 8K photographic storm sky with connected cloud banks, fine natural contours, and a composed warm opening. The old procedural material remains an explicit diagnostic option, not the default or approved direction.

Work is isolated in `E:/Projects/DirectorsCut/Maelstrom-Sky`, branch `codex/maelstrom-sky`. Parent sky commits are c8eca71 and 76ff2cb; the water baseline is 73300a6, cherry-picked locally as 558d457. No shared-main integration, push or packaged build is included. Water checkpoint 182f543 is not in these captures.

## Rendering and boundaries

`DCMaelstromSkyPlate.cpp` creates a 20 km, unlit sky sphere for Breach/Maelstrom and hides the native UDS sky meshes/cloud component during those phases. The HDR photograph is sampled by world view direction, with terrain excluded below 12 degrees and elevation remapped around a selected cloud opening. It stays fixed in the sky rather than rotating with the arena.

The background is photographic and static: it has no evolving volumetric cloud silhouettes or cloud parallax. The native directional sun, fog, surface shadows, photograph-derived light-function mask, directional shaft bloom and depth occlusion remain real time. The old spotlight shafts remain disabled. This is a deliberate change of technique to remove the repeating foam shapes within the jam schedule, not a claim that reference-level AAA lighting has been achieved. The broad light opening and atmospheric shafts still fall short of the reference's sharply separated beams.

The existing waterfall mist, spray, route, water geometry/materials, creatures, shared director and vendor assets are untouched. Phase-specific sun cues are applied from the atmosphere actor. The automatic helm camera crops most of the sky during deep descent; the cloud-detail capture uses a separate review camera.

All 11 native sky mesh visibility states, native cloud visibility, 29 UDS properties, sun/light-function settings and fog settings are captured and restored on exit or backward seek. Height fog stops before the sky sphere because the photograph already includes atmospheric haze; it still covers the arena. Volumetric fog remains active.

## Assets and reproduction

The source is [Quarry Cloudy](https://polyhaven.com/a/quarry_cloudy), photographed by Dimitrios Savva and processed by Jarod Guest, released by Poly Haven under [CC0](https://polyhaven.com/license). Full provenance, checksum and source URL are in `Content/DC/Maelstrom/Sky/ASSET_LICENSES.md`.

New assets are `T_MaelstromStormPlate`, `M_MaelstromSkyPlate` and `M_MaelstromSkyShadow` under `/Game/DC/Maelstrom/Sky`. The imported 8192 x 4096 HDR source is approximately 324 MB in Git LFS. BC6H runtime texture storage is approximately 43 MiB including mips; this is a size calculation, not a target-hardware profile. Existing `/Game/DC` always-cook settings include these assets. No download is required by the game.

To regenerate, run `Tools/fetch_maelstrom_sky_plate.ps1`, then run `Tools/make_maelstrom_sky_plate.py` through UE 5.8 `UnrealEditor-Cmd.exe -run=pythonscript -script=<absolute path> -unattended -AllowCommandletRendering -RenderOffscreen -DDC=InstalledNoZenLocalFallback`. Do not use `-nullrhi`. The generator checks shader compilation and emits `DC_RESULT Maelstrom photographic sky and shadow saved`. Raw EXR and experiments stay in ignored `Saved` folders.

Current defaults: source azimuth -45 degrees, elevation 20 degrees, sun energy 6.5, sky fill 1.7, exposure 0.05; photographic gain 0.18, anchor U 0.56 and source elevation 40 degrees. Directional bloom is 0.22 with threshold 1; sun volumetric scattering 2.4, fog extinction 0.35, scattering distribution 0.25, height-fog cutoff 15 km.

Useful look-development switches: `-DCMaelPlateGain=0.18`, `-DCMaelPlateAnchorU=0.56`, `-DCMaelPlateAnchorElevation=40`, `-DCMaelRayBloom=0.22`. `-DCMaelProceduralClouds` explicitly selects the rejected cellular experiment. `-DCOriginalMaelstromSky` bypasses the custom sky composition.

Review the full descent with:

```powershell
./Tools/review_maelstrom_sky.ps1 -Clean -Continuous -Wait -ReviewName plate-final-game -Start 268 -Shots '269,280,300,340,380,420,449,451' -Width 1920 -Height 1080
```

Omitting `-Native` exercises the game's DLSS setup. `-Native` uses 100% internal resolution with DLSS/frame generation disabled. `-EnvironmentOnly` removes encounter actors. Continuous captures use exact ride times; discrete seek captures wait 5–6 seconds for settling, so their actual times are later than filenames.

## Verification

- UE 5.8 Development Editor and Development Game builds pass with the final code. Both sky materials compile successfully; the generator completed and startup reports a complete shader map.
- A continuous 1920 x 1080 gameplay run from 268 to 451 seconds completed using normal DLSS Super Resolution at 67%, frame generation off. Reveal, descent, final approach and ending were captured. The ending restored 11 sky meshes/native cloud visibility and 29 UDS properties. This full run preceded the final correction that keeps the reduced fog extinction from being overwritten each frame.
- After that correction, a native 1920 x 1080 sky-focused render and a native gameplay seek regression 300 -> 150 -> 310 -> 451 both passed. Actual settled seek capture times were 306, 155, 315 and 456 seconds. Both exits restored the native sky, and re-entry created the photographic sky again. The earlier weather and the re-entry cloud shapes were visually inspected.
- No Maelstrom shader compilation failure, missing plate, assertion or fatal error occurred in these final reviews. `git diff --check` passes. Other Unreal sessions were running, so sampled frame timings are not presented as a performance benchmark.
- No packaged/cooked build, shared-main merge, target-hardware performance guarantee, water update or encounter balancing is included.

Unretouched captures: [final cloud shape, native review camera at 306](review/maelstrom-sky-photographic-14nov/sky-306.png), [final native helm after re-entry at 315](review/maelstrom-sky-photographic-14nov/helm-reentry-315.png), [DLSS deep descent at 420](review/maelstrom-sky-photographic-14nov/descent-420.png), [continuous ending at 451](review/maelstrom-sky-photographic-14nov/ending-451.png), [final seek ending at 456](review/maelstrom-sky-photographic-14nov/ending-seek-456.png).

For integration, cherry-pick this correction after c8eca71 and 76ff2cb if those commits are not already present. Retain the water owner's newer changes and check the combined scene before packaging.
