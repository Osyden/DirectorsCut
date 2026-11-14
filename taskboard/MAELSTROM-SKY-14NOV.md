# Maelstrom volumetric clouds — 14 November

The user explicitly selected the first volumetric pass (c8eca71, `sky-after.png`) as the baseline and rejected the photographic skybox. This revision restores actual ray-marched volumetric clouds, removes the photographic runtime path/assets/tools, and removes the cellular erosion texture/baker that produced the foam-like regression.

The accepted bank layout, layered engine volume noise, slow wind and irregular sun opening are retained. Medium-scale contour variation increases from 0.26 to 0.30, the density boundary tightens from 4 to 4.5, and fine edge erosion increases from 0.18 to 0.24. Larger smooth-union banks and a narrow sun aperture were evaluated and discarded because they flattened the silhouette or looked carved. This is a restrained refinement of the chosen version, not another replacement of its composition.

## Lighting and quality

Clouds self-shadow through native volume ray marching and cast native shadow maps into the atmosphere, fog and scene. Cloud shadow strength on surfaces/fog is now 1.0 instead of 0.7. The cloud shadow map retains its 4 km extent, resolution scale 2 and ray-sample scale 2.

The default weather sometimes gives volumetric fog only a 700 m view distance after a seek. During the finale, the view distance is now at least 4 km so cloud shadows can be visible through the crater. Extinction is 0.55, scattering distribution 0.15, and sun volumetric scattering is at least 3. The original fog distance, extinction, scattering and enabled state restore on exit/backward seek; restoration values are logged.

Directional shaft bloom/depth occlusion supplements the native volume shadows. Bloom is 0.18 with threshold 0.85, maximum brightness 2 and the existing warm tint. It is screen-space and depends on the visible sun; do not describe the entire visible fan as volumetric scattering. A bloom-disabled control render was used to inspect cloud shape and native lighting independently. The old spotlight shafts remain disabled.

Full-resolution cloud rendering retains view/shadow sample scales 4/3. Distance to maximum sample count is now 3 km instead of 6 km, giving denser tracing through the cloud contours. This costs GPU time; `-DCMaelCloudSampleDistance=6` is a lower-cost review option. Rendering mode/sample-distance overrides restore when leaving the stage.

Implementation references: [Epic's volumetric cloud shadowing documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine), plus the installed UE 5.8 `VolumetricFog.usf` cloud-shadow-map sampling path. No photographic cloud backdrop or photographic shadow mask remains. The cloud shapes come from the volumetric material.

## Scope and integration

Worktree: `E:/Projects/DirectorsCut/Maelstrom-Sky`, branch `codex/maelstrom-sky`. Water baseline remains 73300a6 (local cherry-pick 558d457). The water chat's newer 182f543/e09fae6 and structural cliff/basin rebuild are separate and are not represented in these renders. No DCMaelstrom.cpp/h, water, route, terrain, boss, shared director or vendor asset changes are included. No main merge, push or packaged/cooked build is performed.

This commit follows c8eca71, 76ff2cb and e7dee7a, and removes e7dee7a's photographic work. If integrating onto a branch without those commits, apply the series or transfer the final sky-owned files carefully; do not leave the photographic default active. Recheck lighting against the water owner's new terrain before packaging.

## Reproduce

Build `DirectorsCutEditor Win64 Development` with UE 5.8. `Tools/make_maelstrom_sky.py` rebuilds `/Game/DC/Maelstrom/Sky/M_MaelstromCumulus` using `/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseShape128`. Run through `UnrealEditor-Cmd.exe -run=pythonscript -script=<absolute path> -unattended -AllowCommandletRendering -RenderOffscreen -DDC=InstalledNoZenLocalFallback`; do not use `-nullrhi`. It waits for compilation and checks the material before saving. Existing `/Game/DC` always-cook settings and the material's engine-texture dependency cover the assets.

The exact camera used by the user's chosen screenshot is `-DCShotCam=0,-3500,3500,10,-45` at actual ride time 306, not the later -65-degree look-development view:

```powershell
./Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Native -Clean -Wait -ReviewName volume-hero -Shots '300' -Width 1920 -Height 1080 -ExtraArgs '-DCShotCam=0,-3500,3500,10,-45'
./Tools/review_maelstrom_sky.ps1 -Clean -Continuous -Wait -ReviewName volume-game -Start 268 -Shots '269,280,300,340,380,420,449,451' -Width 1920 -Height 1080
```

Current defaults: density 0.025, erosion 0.24, opening 0.23, sun source azimuth -45 degrees/elevation 18 degrees, sky fill 1.7, sun energy 6.5, exposure 0.05. Look-development switches: `-DCMaelCloudDensity`, `-DCMaelCloudErosion`, `-DCMaelCloudOpening`, `-DCMaelCloudSampleDistance`, `-DCMaelRayBloom`, `-DCMaelRayThreshold`, `-DCMaelSunScatter`. `-DCMaelRayBloom=0` isolates the native lighting. These are development controls, not player-facing UI.

## Validation and remaining gaps

- Final UE 5.8 Development Editor and Development Game targets pass. The material generator completes shader compilation successfully; final reviews report a complete startup shader map. `git diff --check` passes.
- Native 1920 x 1080 hero review at actual ride time 306 uses the same camera as the user's selected c8eca71 screenshot. The saved first-pass material was also rendered as a control before retaining the smaller contour refinements.
- Final continuous 1920 x 1080 gameplay run from 268 to 451 completed with normal DLSS Super Resolution at 67%, frame generation off, and encounter actors enabled. Reveal, helm, deep descent, final approach and ending were inspected. No Maelstrom compile failure, missing legacy-texture load, assertion or fatal error occurred.
- Final native 1920 x 1080 seek regression 300 -> 150 -> 310 -> 451 completed. Actual settled capture times were 306, 155, 315 and 456. Re-entry restored the custom volume; both exits restored 29 UDS properties and logged the original fog view distance 70,000 cm, extinction 2.5 and scattering distribution 0.20. Active finale logs show 400,000 cm, 0.55 and 0.15 respectively.
- Hero sampled GPU time was about 20 ms; gameplay capture samples varied with viewpoint and other concurrent Unreal sessions. These readings are not a target-hardware benchmark. No packaged/cooked build or combined-terrain review is included.

Unretouched renders: [final sky at the matching 306-second camera](review/maelstrom-volumetric-return-14nov/sky-306.png), [DLSS helm at 300](review/maelstrom-volumetric-return-14nov/helm-300.png), [deep descent at 420](review/maelstrom-volumetric-return-14nov/descent-420.png), [native re-entry at 315](review/maelstrom-volumetric-return-14nov/helm-reentry-315.png), [restored ending at 451](review/maelstrom-volumetric-return-14nov/ending-451.png). The user's chosen first pass remains [here](review/maelstrom-sky-14nov/sky-after.png).

Cloud contours still have softness and some bright-edge grain; the reference has more intricate internal cloud structure and more sharply separated shafts. This pass restores and refines the requested volumetric direction without claiming AAA parity. Deep-descent helm views show little sky because the route camera looks into the crater. Target-hardware profiling and review with the new terrain remain necessary.
