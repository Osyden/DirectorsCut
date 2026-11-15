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

## Revision after actual combined review A

The retained `Saved/Screenshots/Auto/shot_0300.png` and `shot_0340.png` from the rendering checkout were inspected. **The first pass failed visually**: reducing talus glare revealed a huge smooth concave curtain. Its shared cross-section overwhelmed the small fractures. Passing continuity tests did not establish that it looked like a cliff.

This revision replaces the common smooth shoulder with jittered formation profiles. Different rock masses retreat at separate elevations, with steep fronts between their broken shoulders. Asymmetric protruding buttresses have unequal flanks, leaning axes and different termination heights. Narrow irregular drainage cleaves cut between them; the shared waterfall drainage mask also recesses its contact surface more strongly. Toe, lip, cap, temple pads and overall crater diameter are unchanged. Material mineral variation now samples irregular three-dimensional volumes rather than height-biased bedding, with restrained cool/warm rock contrast. No vendor rock enlargement or new regular joint pattern is used.

Offline numerical checks again sampled the wide section, 50 m inlet and 20 m stress case. Results: no shoreline intrusion beyond floating-point error, seam error below 1e-8 cm, maximum section join discrepancy 0.315 cm, all sampled face normals inward. In the wide section, 28.2% of face samples have a radial slope below 0.25 (near vertical), and the midwall spans 451.5 m of radial variation across distinct formations. New Unreal test assertions guard against returning to the rejected shared smooth curtain. Python material syntax and `git diff --check` pass. **Unreal compilation, actual automation and visual review remain pending with root.**

Regenerate `M_CraterRock` after this revision, run the same automation and recapture the same deck angles. Also inspect waterfalls against the new recessed shoulders, since their sampler follows the changed geometry. The stronger geometry is deliberately visible at scene scale; it may need local reduction if an actual frame shows repeated fins, abrupt benches or cramped water space. It is not signed off as convincing geology until the renders and motion support that judgment.

## Revision after actual combined review B

Inspected `Saved/SceneReview/crater-iteration-b/shot_0300.png`, `shot_0340.png` and the close waterfall frame `waterfall-close-b/shot_03060.png`. **B also failed:** its 300–450 m radial retreats became giant rounded shelves. The close-up shows working fine texture normals, but essentially no convincing middle-scale rock structure. The nonlinear relief safety limiter rounded the generated protrusions further. This is a shape failure, not proof of a disconnected normal output.

The new bounded revision removes the staggered terrace steps. A continuous escarpment now carries unions of oblique intersecting rock planes at three sizes: major formations, fractured blocks and smaller chips. Plane intersections retain arrises rather than smoothing into pillows. A piecewise-linear shoreline safety bound replaces the nonlinear limiter. Rim diameter, receiving-water toe, temple supports and the waterfall sampler interface remain unchanged. The global lean required by those fixed endpoints remains; this does not claim to make the whole wall vertical.

The material now adds analytical 22 m fracture normals and matching restrained fissure darkening. The existing 1.6 m/12 m ground normals remain fine detail; they are not magnified to stand in for rock formations. Sampling uses UE's `UnpackNormalMap`, equivalent to the old RG decoding on this Windows target but respecting other normal encodings. Material generation validates base-color, world-normal and roughness output connections and logs the installed textures' compression and sRGB metadata. Dry rock roughness is now 0.70–0.81; wet rock remains 0.56. Root must regenerate the material and check shader compilation.

CPU numerical probe: finite positions, shoreline exclusion, seam closure, section connections and inward normals pass on the wide cliff and both narrow-inlet fixtures. Maximum join discrepancy 0.424 cm; seam below 1e-8 cm. Near-vertical local samples are 13.0%, and midwall radial variation is 203.2 m. The previous assertion demanding at least 220 m of radial variation rewarded the rejected terraces; it is replaced with a bounded range and a minimum local-steep-face guard. Neither assertion establishes visual quality. Python syntax and `git diff --check` pass; no Unreal build, shader compilation or GPU render was run here.

Required visual checks: fractured faces versus new low-poly-looking facets, waterfall contact through the changed surface, fracture-normal aliasing at distance, and shader/GPU cost of the extra material layer. Root also observed VSM non-Nanite marking overflow. This pass does not suppress it; baking the static basin geometry into a Nanite mesh or adding spatial LOD/section culling remains separate work. Pass C requires the same actual deck and close-up review before sign-off.
