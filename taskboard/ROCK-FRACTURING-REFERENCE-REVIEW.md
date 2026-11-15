# Rock-fracturing reference review, 15 November

**Useful starting algorithm, not a ready-made cliff.** The provided polyhedral
model is the strongest starting family for our arena: unequal angled faces,
deep joints and visible block projections provide a more useful rock structure
than our rejected soft surface-noise variants. Rhombohedral is the next useful
family for inclined joints. This judgement comes from actual neutral-clay
renders of the upstream meshes, not from the publication's showcase images.

## Source and limitations

Source: [Axel Paris, Rock-fracturing](https://github.com/aparis69/Rock-fracturing),
commit `b91965b3011ed269cbc3a051b00c9b284aaa2e36`.
The [author's project page](https://aparis69.github.io/public_html/projects/paris2020_Blocks.html)
describes fracture-derived implicit blocks for rocky scenery. The
[repository README](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/README.md)
states that this is a rewritten subset, not the original implementation behind
all published scenes; implicit replication, tiling and some scenes are absent.
Good individual blocks therefore do not solve continuous arena placement.

The repository carries [MIT licensing, copyright 2019 Axel Paris](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/LICENSE).
Retain that notice when distributing substantial source portions, and credit
Paris, Peytavie, Guérin, Dischler and Galin, *Modeling Rocky Scenery using Implicit
Blocks*, The Visual Computer 36, 2251–2261 (2020), as requested by the README.
Only this review note is committed here; reference code, models, textures and
rendered bulk remain in ignored `Saved` folders. No Unreal asset was imported.

## Actual clay review

Inputs: `Flow-Seams/Saved/ReferenceCode/Rock-fracturing/Objs/tile_*.obj`.
Outputs: `Fitted-Waterfall/Saved/ReferenceRockReview/ProvidedFitted/` with front
and oblique images for all four families and `review.json` containing hashes,
dimensions, counts and cameras. The local rendering harness remains in ignored
`Saved/ReferenceRockReview/render_reference_rocks.py`.

Cycles CPU, eight threads, 24 samples; rough neutral clay with no texture or
shader displacement. Original OBJ normals and geometry are preserved. Only a
rigid axis rotation and centering translation are applied. No decimation,
resculpting, smoothing or physical scale assignment. Source Y is treated as up,
consistent with the explicit Y-axis tabular-joint code. Camera distances fit
actual source bounds; they are not a shared metre-scale comparison with our
72m crop. An initial framing assumption based on tileSize20 put a camera inside
the supplied larger grid-coordinate mesh; that failed image is not evidence.
Use only `ProvidedFitted`, not the earlier `Provided` folder.

| Family | Actual appearance | Triangles | Source-unit bounds after axis rotation |
|---|---|---:|---|
| Polyhedral | Best starting family: unequal large faces and oblique joints. Still a cubic assembly with wide gaps and pitted surfaces. |839,408|173.85×173.67×174.31|
| Rhombohedral | Useful slanted joint relationships; projections read more naturally than orthogonal masonry, but similar block sizes remain obvious. |1,280,872|173.41×174.41×173.70|
| Equidimensional | Clear fracture blocks but predominantly orthogonal, masonry-like stacked forms. |1,245,136|174.12×173.68×173.25|
| Tabular | Distinct thin plates, but the supplied stack looks like separated pancakes. Use as a limited bedding reference, not the whole cliff. |444,936|98.45×98.42×98.10|

The first three meshes contain uniformly pitted/bulbous surface detail even
under plain clay, so that detail is in geometry. Some face regions show
concentric or stair-step extraction artifacts. These are not polished AAA
assets. Their deep joints help, but simply enlarging and repeating these cubes
would produce another artificial wall. The best next use is a bounded section
of unequal polyhedral/rhombohedral blocks embedded into the connected massif,
with fewer exposed gaps and consistent geological direction. Coast, temple
support, continuous seams, waterfall contact and performance remain unresolved.

## Critical coordinate finding

**OBJ units are not documented metres.** The current main function constructs a
tileSize20 SDF domain, but the output pipeline does not restore that domain's
coordinates after polygonization:

- [MC.h](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/Code/Include/MC.h)
  creates edge vertices from integer grid indices plus interpolated fractions.
- [PolygonizeSDF in blocks.cpp](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/Code/Source/blocks.cpp)
  samples the SDF into a200³ field, then returns the marching-cube mesh without
  transforming its positions back into the sampled box.
- [main.cpp](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/Code/Source/main.cpp)
  writes those returned positions directly to OBJ.

A new authored export must explicitly map each grid position through
`box_min + grid_position * cell_diagonal`, with corresponding inverse-transpose
normal handling if the spacing is anisotropic. Capture the exact generation
box and resolution alongside it. The supplied meshes' different98/174-unit
extents cannot be used as evidence of a shared physical size. Physical block
size should be chosen deliberately only after that coordinate contract is
resolved. No further geometry tests or engine integration were performed in
this review.
