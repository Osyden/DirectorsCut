# Upstream escarpment method proof: visual review, 15 November

Independent CPU review of the geometry worker's unchanged official Fastscapelib
0.3.0 escarpment reproduction. This is not crater geometry, a calibrated real
landscape, a fracture model or an AAA asset. No Unreal or game files changed.

## Exact input and rendering

Inputs: `Flow-Seams/Saved/FastscapeReference/OfficialEscarpment/`, with
`baseline.npz`, `candidate.npz`, `terrain_arrays.npz` and `manifest.json`.
Candidate SHA256:
`1c2265b9afeecf4428aa60dc0ded0c5f28833f272634dcc8f9413421d6463fa2`.
The manifest records upstream revision
`707fcc10976b0e729f1415fd16b0a161e1f746c3`, notebook
`doc/source/examples/escarpment_py.ipynb`, Fastscapelib 0.3.0 and 50 model steps
of 2,000 years. See the generation worker's manifest for executed cells and
dependency provenance; this review does not independently rerun the solver.

Original source dimensions are **20 × 10 km**, a **100 m grid** with
101 × 201 samples, and an initial step of about **400 m**. The final height
range is approximately 0–400.12 m. The model's identity XYZ frame is retained.
There is no rescaling, vertical exaggeration, crater frame, water context or
temple-support constraint in these images.

Correct captures are under this worker's ignored
`Saved/ImplicitCliffReview/UpstreamFastscape/OfficialEscarpmentLongClip/`:

- `UPSTREAM REFERENCE 20x10km - plan_fractured.png`
- `UPSTREAM REFERENCE 20x10km - oblique_fractured.png`
- `UPSTREAM REFERENCE - 7km detail_fractured.png`

Matching `_baseline.png` files show the initial step in the same cameras.
The three candidate views and the initial full oblique were inspected.
Used existing neutral-clay renderer, CPU/eight threads, 24 samples, angle-15
shading, unchanged original triangles. Camera widths are 22.5 km (plan),
27 km (full oblique) and 7 km (detail); relief is never exaggerated.

The initial `OfficialEscarpment/` full plan/oblique captures are **invalid blank
camera tests**: the existing generic camera has a 10 km far clip. The ignored
one-run `run_reference_review.py` wrapper changes only that camera far clip to
100 km, records it in the corrected report and invokes the existing renderer.
No generic source-tool edit or geometry change was made. Do not use the initial
blank full-domain images as terrain evidence.

## What the actual geometry shows

The initially straight step develops an irregular retreating scarp, branching
ravines and interlocking spurs. This is a more coherent macro-process response
than the isolated quarry pockets and rubble patches of the recent custom tests.
The full view is naturally low in aspect because 400 m relief spans a 20 km
domain; the camera does not artificially steepen it.

The 7 km detail also exposes conspicuous 100 m raster facets, stepped channel
edges and large smooth slope faces. This result is useful as an established
process proof, **not** a game-ready cliff or evidence that the crater has been
solved. A crater adaptation would need its own physical domain/resolution,
boundary conditions and protected support treatment; simply squeezing this
finished 20 km field into a 500 m sector would invalidate its feature scale.

Both meshes are valid open surfaces in the existing reviewer (20,301 vertices,
40,000 triangles, one component, no invalid edge/winding/degenerate flags).
The generic pair-preservation gate is false because 500 of 600 outer edges
change between the initial/final model. That is compatible with the documented
upstream mixed boundary conditions: only the left base level is fixed, while
other boundaries evolve or loop. It is not an arena-boundary preservation claim
or evidence of mesh corruption. No new topology framework was introduced.
