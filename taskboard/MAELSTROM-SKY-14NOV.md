# Maelstrom sky and lighting

Sky work is isolated in `E:/Projects/DirectorsCut/Maelstrom-Sky`, branch `codex/maelstrom-sky`. It includes the water chat's spiral checkpoint `73300a6` as local cherry-pick `558d457`. The sky change can be cherry-picked separately onto that water checkpoint or a later descendant. Main and the packaged build are not updated by this branch.

## Result and boundaries

The finale replaces the UDS cloud material with an original volumetric cumulus material. Overlapping towers, three scales of density erosion and slow wind produce real cloud silhouettes and self-shadowing. An irregular opening follows the actual sun. Cooler cloud albedo and sea-coloured ground bounce separate blue-grey undersides from the warm light around the opening. The sun is composed for the port-facing helm view, at source azimuth -45 degrees and elevation 18 degrees.

Native cloud shadows provide the atmospheric shafts and surface lighting. The old three spotlight shafts are disabled while the bespoke cloud volume is active. The UDS light function is also temporarily removed because its shadow density does not match the new clouds. Existing waterfall mist and spray are retained.

Cloud density rises with the arena rim during the 900 m descent, inside a fixed tracing envelope. Updating the density offset instead of the cloud component's height every frame avoids repeatedly rebuilding its render proxy. Ground albedo is also set once on entry; reasserting it every frame caused the cloud pass to disappear in review.

All sky actor overrides, the original cloud material/component settings, light function and render-target mode are restored on exit or backward seek. The shared director, water/route source, vendor assets and global rendering config are not edited. The only phase cues changed are Breach and Maelstrom, applied from the atmosphere actor.

This is a substantial real-time lighting pass, not a claim of matching the reference's final AAA finish. Cloud silhouettes still have some softness at close inspection. The water surface, repeated rim landmarks, spray/contact effects and gameplay staging remain separate workstreams.

## Reproduce

Build `DirectorsCutEditor Win64 Development` with UE 5.8, then run:

```powershell
./Tools/review_maelstrom_sky.ps1 -Continuous -EnvironmentOnly -Native -Clean -Wait -ReviewName sky-review -Start 268 -Shots '269,280,300,340,380,420,449,451' -Width 1920 -Height 1080
```

Use continuous mode for exact ride-time screenshots. Discrete shots intentionally wait for settling after a seek, so their logged actual ride times are later than the filenames; a discrete 449-second shot has already passed the ending.

`-EnvironmentOnly` suppresses encounter actors. Omit it to check gameplay visibility. `-Native` disables DLSS and frame generation for image/performance comparison; omit it to exercise the game's normal DLSS setup. `-OriginalSky` restores the previous finale composition for comparison.

Look-development overrides can be passed through `-ExtraArgs`: `-DCMaelCloudDensity=0.025`, `-DCMaelCloudErosion=0.18`, `-DCMaelCloudOpening=0.23`, `-DCMaelSkyFill=1.7`, `-DCMaelSunEnergy=6.5`, `-DCMaelSkyExposure=0.05`, `-DCMaelCloudRenderMode=3`. Mode 3 is the clean full-resolution default. Mode 2 is a cheaper option with visibly rougher bright cloud edges. Mode 1 produced objectionable outlines on the distant rim and should not be used for this scene. Target-hardware profiling remains necessary; this workstation has an RTX PRO 6000 Blackwell.

## Material authoring

`Tools/make_maelstrom_sky.py` reproduces `Content/DC/Maelstrom/Sky/M_MaelstromCumulus.uasset`. It uses stock engine volume noise and contains no copied vendor material. The existing `/Game/DC` always-cook directory includes the asset and its engine texture dependency.

Run the generator using `UnrealEditor-Cmd.exe` with `-run=pythonscript`, `-script=<absolute generator path>`, `-unattended -AllowCommandletRendering -RenderOffscreen -DDC=InstalledNoZenLocalFallback`. Do **not** use `-nullrhi`. The authoring utility waits for shader compilation and checks the material resource before saving. Check the final `DC_RESULT Maelstrom cumulus material saved` marker and inspect a fresh game render after any regeneration.

The scene controller and generator contain diagnostic stock/engine material and shader-compilation switches for reproducible investigations; these are not player-facing options.

## Validation

- UE 5.8 Development Editor and Development Game targets both compile successfully, including the non-editor material-authoring guard. `git diff --check` passes.
- A normal-speed, native 1920x1080 run from 268 to 451 seconds completed through the revised spiral. The clouds remained above the rising rim at the deep viewpoints, and the 29 UDS overrides were restored at the 450-second exit. The final sun/fill/exposure balance was applied afterward and checked again with encounter actors enabled at actual ride times 286, 305, 385 and 425 seconds.
- A final gameplay run from 438 to 451 seconds completed using the normal DLSS Super Resolution setup at 67%, with frame generation off. The custom sky restored at the ending. Native and DLSS captures have been visually inspected; these are offscreen game renders, not generated paintovers.
- A native 1280x720 seek regression ran 300 -> 150 -> 310 -> 451. Both exits restored all 29 UDS properties, the second entry created a fresh custom cloud MID, and the re-entry image showed the intended sky. No restart was required.
- Native scene-only GPU readings at the capture points were roughly 11-14 ms on this RTX PRO 6000 workstation; the sky-focused view was 15.6 ms. These are sampled readings, not a benchmark or a minimum-spec guarantee. Game/CPU readings also include the rest of the prototype. No maelstrom shader compilation failure or fatal error occurred in the final reviews. Existing experimental editor Python/plugin startup warnings remain outside this change.
- No cooked/package run or shared-main integration has been performed. The water branch's route tests were not repeated because this pass does not change the route. Full encounter balance is outside the sky validation.

Matching camera comparison at actual ride time 306 seconds: [before](review/maelstrom-sky-14nov/sky-before.png), [after](review/maelstrom-sky-14nov/sky-after.png). Final gameplay lighting: [helm at 305 seconds](review/maelstrom-sky-14nov/helm-305.png), [deep view at 425 seconds](review/maelstrom-sky-14nov/helm-425.png), [DLSS final approach](review/maelstrom-sky-14nov/dlss-449.png), [ending restored](review/maelstrom-sky-14nov/ending-restored-451.png).

The current automatic helm camera looks down into the spiral, so the sky occupies only a narrow band during the deepest stretch. The sky-focused comparison deliberately uses a separate review camera to expose cloud shape and lighting. Camera changes remain with the water/route owner.
