# Dedicated cliff generator and exposed temple skyline

Scope: cliff geometry, its material, temple support and the existing falls' attachment to that geometry. Water shading/foam, route, sky/light settings and boss gameplay remain as at the integrated checkpoint. Isolated `codex/maelstrom-saturday` branch.

## Reference reading

Both supplied references use a broad low rim with small architectural silhouettes against clouds. Deep faces establish the vertical scale below it. Rounded mountain peaks behind temples and giant enlarged rock props reverse that hierarchy. The old scene still had two different rock generators: Terrain instance scatter and legacy `BuildLandmarks` crags/buttresses. Both are removed from the maelstrom.

## Implementation

- `DCCliffGenerator.h` generates a deterministic parametric cross-section: submerged toe, lower debris slope, tall steep wall and low plateau. Face height and radial position are independent, supporting leaning/recessed faces without stretching a landscape heightfield. Physical detail comes from multiple scales of angular fracture geometry, not scaled low-resolution vendor rocks.
- Circumferential cell hashes wrap, and the wall/plateau share endpoints. The connected mesh has247,457 vertices /491,520 triangles. Geometry normals are computed from parametric derivatives; UE clockwise winding faces into the basin and up from the plateau.
- Large fractured masses, smaller breaks and chips use separate physical scales. A restrained chipped rim replaces the previous rounded peaks. Deep regular joint lines and continuous bedding stripes were rejected after actual renders; the final version uses interrupted, nonuniform fracture relief.
- Temple pads are flattened against the generated cap. Nine existing ruin complexes sit behind the lip, with two42m statues rather than110m figures. The scene's cliff geometry contains no scaled rock props. Existing temple architecture remains built from the installed modular ruins.
- Rock material uses unit-scale local triplanar coordinates:4m colour,1.6m fine normals,12m intermediate normals. Vertex R carries wetness; G carries recess shading. Mineral colour, runoff, roughness and cavity shading are restrained rather than broad painted blobs. Installed textures are referenced, not copied.
- Existing waterfall cores now sample the same parametric wall and receiving-water contacts, avoiding the invalid heightfield-only query for a leaning face. This only maintains their attachment; it is not the new volumetric waterfall implementation.
- The waterfall worker identified the user's example pack and prepared the [VDB handoff](WATERFALL-VDB-14NOV.md) and a read-only audit script. The pack is not installed, so no speculative VDB adapter or cloud-volume substitute is enabled.

## Verification and actual captures

Editor and Game Development builds pass. The rock material generator exits successfully. `DirectorsCut.Maelstrom.CliffContinuity` passes: finite bounded positions, circumferential seam closure, toe/face/cap continuity, inward wall normals and exclusion from the receiving-water region, including the narrow inlet section. Runtime shoreline clearance remains35m beyond the full-steer allowance.

Final geometry/material review `cliff-wall-c` uses native1920x1080, environment-only captures requested282/300/340/420/451, settling at288/305/345/425/456. The helper completes with no maelstrom shader failure, fatal error or assertion. Exit restores fog distance70000 and29sky properties. GPU samples at the four cliff views were12.8/9.8/10.1/15.4ms; these are sparse samples, not a boss-fight or target-machine performance guarantee. Existing startup VSM/toolset warnings remain.

- [Cliffs and temple skyline at305s](review/maelstrom-cliffs-14nov/cliffs_0300.png).
- [Later descent at345s](review/maelstrom-cliffs-14nov/cliffs_0340.png).
- [Deep view at425s](review/maelstrom-cliffs-14nov/cliffs_0420.png).
- [Restored open sea at456s](review/maelstrom-cliffs-14nov/cliffs_0451.png).

Review A was rejected for continuous horizontal strata. B was rejected for regular vertical joints resembling wall panels. C is the retained implementation. Temples now read against the sky and the hillside/crag obstruction is gone, but this is **not AAA visual sign-off**: the backlit faces still need stronger convincing mineral/weathering detail and local art-directed fracture shapes. The broad talus under the wall is conspicuous under current light; review its relationship with volumetric fall impacts once the real pack is installed. No lighting changes were made to conceal those issues.

These are actual offscreen engine renders, not paintovers. There was no packaged build, main merge or manual motion playtest this pass.

## Reproduce

Build with UBT source discovery (`-gather`) because new generator/test source was added. Regenerate `M_CraterRock` through `Tools/make_maelstrom_rock.py` if needed. The generated original material is checked in; its installed AncientRuins texture references must resolve.

Run `Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Clean -Native -Width 1920 -Height 1080 -Wait -ReviewName cliff-wall-c -Shots '282,300,340,420,451'`. No vendor meshes/materials are modified.
