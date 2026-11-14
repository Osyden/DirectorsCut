# Cliff worker pass 2

Owner: Codex cliff worker, `codex/crater-cliffs-pass2`, isolated checkout `E:/Projects/DirectorsCut/Crater-Workers/Cliffs`. Claim is limited to `DCCliffGenerator.h`, `DCCliffGeneratorTests.cpp`, `DCMaelstromTerrain.cpp`, `Tools/make_maelstrom_rock.py` and this note. Root owns builds, generated assets and rendered review.

Observed in actual `maelstrom-cliffs-14nov/cliffs_0300.png`: the low temple skyline is now readable, but the lower debris slope is hundreds of metres wide and forms a bright continuous shelf. The reference relies on tall faces and fine distant detail; it does not need larger rock props. This pass narrows and varies that shelf while keeping the toe, rim lip, plateau endpoints and waterfall sampling contract.

## Changes

- The old lower debris slope occupied almost the entire toe-to-lip gap (roughly500m on a wide section). It now occupies42–127m, capped at26% of the available radial gap, with unequal fan widths and heights. Its endpoints still connect to the same receiving-water toe and existing top lip.
- The face has steep lower and upper regions connected by a recessed shoulder whose height varies around the crater. This moves the required horizontal gap into the wall's larger geological profile rather than displaying a continuous bright flat apron.
- Large formation, fault, fracture and chip scales remain independent. Sparse oblique cleaves terminate across formations; no new periodic rings or full-height joint panels are added. Relief is smoothly bounded above the shoreline, including the narrow inlet.
- Vertex cavity shading now follows the same geometric cleaves instead of an unrelated noise field.
- Wetness fades over1.5–18m above receiving water instead of5–45m. Wet-rock roughness increases from.43 to.56 so the lower rock does not behave as a second glossy water sheet under the low sun.
- Rim/plateau endpoint, temple pads, shared waterfall sampler and V=.20/.84 section boundaries are preserved. No temple placement patch is needed for this bounded pass: the retained frame already puts supported temple roofs above the low skyline.

## Validation and integration

- `git diff --check` passes. Rock-generator Python syntax passes using `ast.parse`.
- A numerical Python port sampled three sections (wide cliff,50m inlet,20m stress case),192 angles and161 vertical samples. Finite positions, receiving-water exclusion, seam closure, section continuity and inward face normals all passed. Max section-boundary separation was0.332cm for epsilon1e-6; max seam error was below1e-9cm; wide-section talus spans42–127m. This is an offline numerical check, **not a substitute for the Unreal automation test**.
- Extended `DirectorsCut.Maelstrom.CliffContinuity` with geometric acceptance checks: talus remains below30% of footprint and18% of face height, debris fans vary, and plateau stays within3.5m of its low cap. Existing finite/seam/normal/shoreline checks remain.
- No Unreal build, material generation, GPU render or generated `.uasset` mutation was performed in this worker checkout.

After cherry-pick into the rendering checkout:

1. Build `DirectorsCutEditor Win64 Development` and `DirectorsCut Win64 Development` with the existing UE5.8 UBT command.
2. Run the existing Unreal Python generator `Tools/make_maelstrom_rock.py` to update `/Game/DC/Maelstrom/M_CraterRock` (AncientRuins tile textures must be installed in that checkout).
3. Run Unreal automation `DirectorsCut.Maelstrom.CliffContinuity`, for example with `-unattended -NullRHI -ExecCmds="Automation RunTests DirectorsCut.Maelstrom.CliffContinuity" -TestExit="Automation Test Queue Empty"` on the Editor executable and project.
4. Match the retained deck frames: `Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Clean -Native -Width 1920 -Height 1080 -Wait -ReviewName cliff-pass2 -Shots '282,300,340,420,451'`.

Expected benefit: less bright foreground rock shelf, more visible vertical rock mass, broken geology and wetter regions that do not dominate the lighting. Remaining limits: the preserved toe and rim are hundreds of metres apart, so the overall escarpment still leans across that gap; it cannot be wholly vertical without moving the water or top lip. Actual renders must decide whether the new recessed shoulder reads as geology or another broad bench. Material detail and temple contrast still need root's visual acceptance. This note does not claim AAA parity.
