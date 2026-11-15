# True 3D outcrop review, 15 November

Owner: Codex waterfall/review worker, `codex/cliff-outcrop-review-pass9`.
Scope: offline CPU clay and independent mesh audit only. No Unreal assets, live
terrain, waterfall runtime, lighting or sky changed. No FBX exported.

## Result

The cleaned Boolean candidate is a valid closed-mesh prototype, but **fails the
visual gate**. Actual wall and oblique views show straight narrow saw slots and
cross-like intersections cut into the same smooth, faceted slope. The side view
shows little useful change in exposed relief. Internal downward roof triangles
and multiple ray hits establish true 3D geometry; they do not establish
convincing visible rock fractures.

The cut widths are about 0.54–0.91 m, with long straight runs and capped ends.
Their crisp darkness makes the artificial pattern more legible. Angle-based
normal smoothing could reduce sample facets but would not repair this shape.
The next geometry attempt needs finite, irregular wedge/block removals that
expose different rock planes and projecting broken edges. Avoid merely widening
the same slots into caves or a rectangular brick grid.

## Evidence and physical scale

Source: `Flow-Seams/Saved/FracturedOutcrop/clean/{baseline.npz,fractured.npz}`.
Underlying massif hash:
`1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.

Review outputs are under
`Fitted-Waterfall/Saved/OutcropReview/Pass9-CleanFlat/`, with paired
`wall`, `oblique`, and `side` baseline/fractured PNGs and `review.json`.
Separate latest audit: `Saved/OutcropReview/Pass9-CleanAudit/review.json`.
All six images use identical cameras and light, neutral rough clay, flat
normals, Cycles CPU, eight threads, 16 samples. Render completed in about eight
seconds. The first invalid candidate remains separately under
`Saved/OutcropReview/Pass9-FirstFlat/` and is not silently repaired by the reviewer.

The XY crop is 72 × 40 m and its exposed source height span is 65.13 m. The
reported full closed-mesh height is 89.13 m because it includes 24 m of artificial
backing below the original heightfield. Those closure faces are authoring
geometry, not a new visible cliff intended for the game. No vertical exaggeration
or heightfield reprojection was used.

## Independent checks

Clean candidate: 25,352 vertices, 50,708 triangles, one connected component,
67,811.24 m³ positive signed volume. Zero boundary edges, nonmanifold edges,
inconsistent edge windings, duplicate triangles, degenerate triangles or unused
vertices. Baseline: 23,490 vertices, 46,976 triangles, 69,120.00 m³.
All 896 vertices on the rectangular XY crop planes match at 10 µm quantization;
moving one boundary vertex 1 cm makes the independent boundary test fail.

These checks do not exhaustively detect self-intersections or nested-shell
containment, nor do they prove full game-sector seams or temple support. The
normal-selected downward area includes internal cavity faces; it is explicitly
labelled a diagnostic rather than proof of useful visible overhangs. The current
candidate does not claim erosion/flow vertex masks.

## Reusable tool

`Tools/review_cliff_outcrop.py` reads full triangle NPZ files with absolute
arena-metre vertices and a shared orthonormal crop frame. It applies only a rigid
frame transform, audits topology and crop boundary vertices, then renders matched
views. Optional angle normals use the already-tested shared normal helper;
default remains flat. `--camera-manifest` fixes cameras across subsequent
candidates; `--audit-only` needs only the bundled Python and NumPy.

Synthetic audit checks cover a valid cube, missing face, one reversed face,
inside-out mesh, and boundary preservation/mutation. Bad meshes can be rendered
for diagnosis, but their report remains failed. Generated bulk stays in ignored
`Saved`; export and Unreal integration remain gated on actual shape acceptance.
