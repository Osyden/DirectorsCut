# Regional cliff exposure: construction works, appearance rejected

Branch: `codex/maelstrom-saturday`, following `4a16933`. This pass generated and inspected one contiguous 160 x 96 m fractured exposure within the measured 500 m cliff sector. It is still a rubble-like patch within soft terrain. **Do not roll it around the crater or describe it as finished geology.** No runtime arena, water, waterfall, sky, lighting, early-game or boss changes were made, and no new Unreal import/build was needed for the rejected shape.

## What was actually built

The sampler, assembler and exterior extractor now consume the physical crop descriptor. One new source point cloud feeds the corrected implicit-fracture reference; this is not a scaled or tiled copy of the old outcrop. Root added an optional finite fracture count to the reference runner and generalized the existing exact-triangle context join. The previous 72 x 40 m join reproduces all seven saved geometry/correspondence arrays bitwise.

The new crop spans about 106 m vertically, with a 4 m unchanged perimeter and no protected shoreline or temple-pad samples. It uses 21,516 points at a 2 m physical lattice, 456 clusters with maximum span under 18 m, and 160 finite fracture discs. Assembly retains 424 hulls; 409 need no extension and 15 have short measured anchors. See [construction and reproduction](REGIONAL-CLIFF-EXPOSURE-15NOV.md), [independent constraints](CLIFF-REGIONAL-EXPOSURE-AUDIT.md), and [visual review](REGIONAL-CLIFF-EXPOSURE-REVIEW.md).

Two tiny closed inward cavity shells were found and separately classified. The final exterior removes only their 66 hidden triangles, totaling 0.0711064 cubic metres of enclosed void. All exterior faces remain unchanged. This fixes the component-count issue, not the appearance.

Final source exterior: `Crater-Workers/Flow-Seams/Saved/RegionalCliff/JointExposure/visible_candidate_exterior.npz`, SHA256 `e4f9d41f09851035ecce30f6afb18107436df32786741b76bb80ef8619798764`.

Root rebuilt the context at `Saved/RegionalCliff/FullContextExterior/context.npz`. Its 113,104 vertices / 225,258 triangles form one valid open manifold in the existing review checks, with all 992 original outer boundary edges preserved. The seam adjustment is at most 3.568 micrometres; retained source positions and masks are bitwise unchanged. `Saved/RegionalCliff/FullContextExteriorAudit/review.json` records the final check. These checks do not establish exhaustive intersection correctness, runtime contacts, material quality or performance.

## Actual images and diagnosis

- [Close original geometry](review/cliff-regional-15nov/near.png)
- [Full 500 m sector with measured mean water](review/cliff-regional-15nov/context.png)
- [The same sector from 1.5 km](review/cliff-regional-15nov/far.png)

These are actual CPU clay renders, not generated concepts or in-game screenshots. They show the first visible export; the final cleanup only removes the two proved hidden cavity shells and does not alter the visible shape. Water is a static measured context surface, not the animated spiral.

The new exposure has genuine recessed planes and remains readable at distance, but many clean, similarly sized polygon faces are separated by dark gaps. Its abrupt contrast with the smooth surrounding face remains obvious. Both independent visual reviewers and root reject whole-cliff acceptance.

The assembler explains this result: it selects a central footprint, excavates a graded 7 m cavity and unions point-cluster hulls back into that opening. Increasing the crop size preserves that scar-and-refill construction. Three coherent fracture families already exist, so adding more families alone will not solve it. Finite point spacing may also contribute to overly wide gaps between hulls; that cause remains a hypothesis to measure, not a verified fracture-width model.

## Next geometry experiment

Keep a connected bedrock body and partition it by a shared geological field, rather than excavating a window and replacing it with separated hulls. Start with unequal primary masses across the sector; keep most interfaces closed, expose selected terminating joints, and remove only a few surface-reaching cells to reveal ledges and recesses. Fracture aperture must be independent of point spacing. Do not turn every plane into a saw slot: the earlier straight-slot and quarry-pocket trials already failed.

The next bounded comparison must also change the coherent escarpment shape. A perfect small fracture pattern on the same soft fluted body is not sufficient. Use the full physical drop, connected drainage and selected large rock masses across the face while retaining the measured shore, outer seam and actual foundation volumes. Avoid the previously rejected giant planar prisms, regular triangular buttresses and isolated repeated patches. Keep one common arena-coordinate structural field across any processing chunks.

Acceptance is visual first: at the existing wall, oblique and 1.5 km cameras, the sector should read as a continuous weathered escarpment with irregular large masses, smaller physical joints and connected gullies. There should be no patch boundary, dark gap around every polygon or repetitive support geometry. Then derive masks on the accepted final surface, apply physically scaled stone, and inspect the native preview. Only after that regenerate waterfall contacts and resume water integration.

## Mountain and texture algorithms

The established workflow remains appropriate: ridged/heterogeneous terrain for the large landform, hydraulic and thermal erosion for drainage and debris, separate detailed 3D cliff surfaces for steep faces and undercuts, and material masks derived from slope, deposition and actual surface structure. Adapt the crater shape before the final erosion pass; stretching a finished mountain field can distort both features and drainage.

[SideFX's erosion documentation](https://www.sidefx.com/docs/houdini/nodes/sop/heightfield_erode.html) describes physical feature sizes and height, sediment, debris and flow outputs. [Project Pegasus](https://www.sidefx.com/tutorials/project-pegasus-cliff-generation/) demonstrates extracting, refining and displacing cliff geometry from terrain, then importing tiled Nanite meshes. The corrected local implementation of [Modeling Rocky Scenery using Implicit Blocks](https://aparis69.github.io/public_html/projects/paris2020_Blocks.html) supplies useful fracture construction, but its use does not guarantee the paper's visual result.

For materials, reuse suitable installed tileable stone albedo/normal maps, add authored roughness and blend exposed rock, deposits and current wetness with separate masks. Keep dimensions in metres and map in the moving arena's coordinates; Epic documents [world-aligned texturing](https://dev.epicgames.com/documentation/unreal-engine/texturing-material-functions-in-unreal-engine) and its texture-swimming limitation on moving objects. The current new-face RGB zeros are diagnostic missing data, not valid erosion/wetness. Existing optional slate proof is still dark/mottled and cannot cure the geometry.

No asset purchase is required for the next construction experiment. A suitable scan could later improve surface detail, but it cannot supply the missing whole-cliff structure.
