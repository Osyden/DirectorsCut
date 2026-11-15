# Original weathered mesh clay review, 15 November

Worker: `codex/cliff-weathered-review-pass10`, based on `10b1fdc`.
Scope: independent topology/boundary review and CPU clay only. No Unreal run,
asset import, FBX export, or live scene change.

## First candidate: visual gate not passed

The first full mesh adds modest surface relief while retaining the connected
massif's large shape. Close views still read as rounded, smeared terrain with a
lumpy surface, rather than differentiated bedrock planes or fractures. Full wall
and deck views retain long similar sloping faces and inflated folds. At 1,500 m
the new detail contributes only subtle breakup; it does not fundamentally change
the massing. This is not reference quality and is not approved for export.

The flat-normal close diagnostic exposes sample facets and fine ridges along
some original triangles. Angle35 shading reduces those artifacts, but does not
create missing geology. Neither view justifies a claim of genuine fracture or
overhangs. The generator deliberately biases displacement toward vertical to
avoid XY triangle folds; its first accepted horizontal-normal factor is0.125.
Relief amplitude is unchanged. This is authored noise, not fresh erosion.

Sawtooth vertices along the isolated tile's lower edge, abrupt side strips, and
some flat support regions are already present in the baseline. They remain
integration problems; the new detail should not be credited or blamed for
creating them. Temple/support checks are owned by the independent audit worker.

## Reproduction and images

Inputs:
`Flow-Seams/Saved/WeatheredCliff/first/{sector_baseline.npz,sector_weathered.npz,crop_baseline.npz,crop_weathered.npz}`.

Full baseline SHA256:
`425b2222c5d312e6cad545bbc634107824603b30c1b41ac858eb6fd6b1345d54`.
Full candidate SHA256:
`a0d335212d57ba7c152e8874b1b9048754b69249500e7ad681136e6e48cbb270`.

Outputs in this worker's ignored `Saved/WeatheredReview/`:

- `FirstFull/`: matched wall, deck, oblique and far-crater pairs.
- `FirstClose/`: matched close wall and oblique pairs for the72×40m source crop.
- `FirstCloseFlatDiagnostic/`: matched close oblique with flat normals, geometry unchanged.

Files retain `_baseline` and `_fractured` stage suffixes from the existing
reviewer; open-surface image titles explicitly say `CANDIDATE`, not fractured
rock. `review.json` records input hashes, camera coordinates, scale and audits.

All views use original3D positions/triangles with only a rigid frame transform,
neutral rough clay, the same raking sun, zero shader detail, no fog/atmosphere,
no vertical exaggeration, Cycles CPU/eight threads/16 samples. Full eight-image
review finished at42.55s; close four-image run took about8s. Main comparisons use
identical angle35 normals; a separate flat diagnostic prevents shading changes
being confused with geometry changes. The footer's no-displacement statement
refers to the renderer, not to the generator's authored surface displacement.

The far view uses a35mm lens,36mm horizontal sensor and1,500m horizontal range
to the tile center. Its reference bar is100m long with10m upright posts. It is
an isolated sector perspective, not a simulated full crater or gameplay frame.

## Independent numerical checks

Full mesh:1,025,013 vertices,2,045,576 triangles,500×492×230.627m. Both inputs
are a single connected **open** manifold surface. No degenerate/duplicate
triangles, nonmanifold edges, inconsistent adjacent winding or unused vertices.
All4,448 actual open-boundary edges are preserved at10µm quantization. Closed
solid status is false and enclosed volume is explicitly absent.

The cropped visual comparison is also valid open topology, but its464 perimeter
edges move because the crop was cut from the interior of the globally displaced
surface. That diagnostic boundary comparison correctly reports failure; this
crop is not a separately protected integration tile. It does not contradict
the exact full-sector boundary result.

## Second and final candidate: visual gate not passed

The bounded second trial is under `Flow-Seams/Saved/WeatheredCliff/faceted/`,
using the same four file names. Actual original baseline vertices, triangles and
all frame arrays are bitwise identical between the two trials. The second
generator uses angular Voronoi distances in geological axes at two scales,
with modest warp and fine noise. It retains the0.125 horizontal direction bias
and does not claim overhangs.

Matched images are `Saved/WeatheredReview/FacetedClose/` and `FacetedFull/`,
reusing the first trial's camera manifests without recentering. The full
eight-image set again includes wall, deck, oblique and1,500m perspective. The
second trial adds shallow horizontal/crosshatched markings to the500m face;
these read as a repeated texture-like pattern over the same soft folds. The
close crop remains lumpy rather than clearly broken into geological planes.
The far view does not acquire a stronger cliff hierarchy. **This candidate is
also rejected for export/integration.** Neither extra noise amplitude nor
additional triangles would address the demonstrated large-form problem.

Second candidate SHA256:
`157769fb9a8c8dedbf83f177034538532627aa5c510ba3f77e77bd95f40273d3`.
The same independent topology checks pass and all4,448 full-sector boundary
edges are preserved. The two-trial limit is reached; no third candidate,
FBX exporter, Unreal import or shader tuning was started during this pass.

The reviewer does not prove arbitrary self-intersection absence, absolute
outward orientation, temple support, or game contact correctness. Vertex colors
in the source describe interpolated old v3 erosion/transport masks; no solver
was rerun after displacement. They are ignored by clay.

## Tool changes

`Tools/review_cliff_outcrop.py --open-surface` adds explicit open topology checks
and compares every actual boundary edge rather than assuming a rectangular
closed crop. It adds full, close and far cameras; `--camera-manifest` locks
subsequent candidate views. `--views` permits focused review first. Default
closed-solid behavior is retained.

Synthetic tests verify valid open surface acceptance, no closed/volume claim,
unchanged boundary after interior movement, boundary failure after edge movement,
and winding failure after reversing one face. A synthetic CPU perspective render
verified label/reference sizing before the actual candidate was available.
