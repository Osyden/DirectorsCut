# Curved fractured bedrock: useful structure, unfinished cliff

15 November, continuation from `b43ba15`, branch `codex/maelstrom-saturday`.
**No production arena, water, clouds, lighting, boss or gameplay changes.**
All new geometry is an offline original-triangle proof; no new Unreal import,
native build, motion test or performance claim is made this pass.

## Actual progress and rejection

We moved from soft terrain displacement to fractured volumes generated inside
the measured curved cliff. The latest candidate has larger coherent faces and
real ledges, with fewer uniform upright shards. Root and two independent
reviewers agree this is useful structural progress, **not final visual approval**.
The exposure still ends too abruptly in the old smooth slope. Thin fins,
pristine planes and some deep straight cuts remain. Do not extend it around the
rim or add material noise to conceal those defects yet.

The earlier corrected 30m cube and backed face both failed. Rigidly embedding
the backed face into the real slope still looked like an attached plaque;
6.88m2 of its artificial front/side cap was exposed. It is retained as evidence,
not the construction to continue. A first curved-skin attempt then produced
too many equally sized angular fragments. The final revision allows broader
rock masses, retains a shallower core, and removes shallow fragments in two
connected downhill regions rather than throughout the entire face.

## Reference corrections and provenance

The inspected `b91965b` reference has more defects than the original note knew:
`Sphere::RandomSurface` returns zero, disabling its polyhedral fracture discs;
its matrix/plane reconstruction cannot reliably provide block bounds. The
adapter bypasses both with explicit unit-normal fracture families and bounds
from actual hull points. It also fixes grid-to-physical coordinate export and
bypasses the image warp/loader entirely. This is a **derived corrected method**,
not reproduction of the supplied example models. See [implementation proof](IMPLICIT-CLIFF-FACE-PROOF.md),
[source-skin export](IMPLICIT-CLIFF-SOURCE-SKIN.md) and
[independent audit](CLIFF-IMPLICIT-FACE-AUDIT.md).

MIT attribution accompanies the wrapper in `Tools/third_party_notices/`.
Original and bundled notices are retained in ignored source snapshots. No
upstream textures, sample models or compiled binaries were added to Git.

## Current physical construction

- Original source remains the measured `source08_measured_boundary_massif_v3_open256.npz`
  in `Crater-Workers/Flow-Seams/Saved/Massif-Cliff`, SHA256
  `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.
- `generate_cliff_skin_samples.py` samples 15,539 deterministic points in a
  curved 60x28m interior region, 0.15–14m vertically below the actual source.
  Physical lattice spacing1.15m, bounded jitter gives at least0.805m separation.
  Depth below the source normal is explicitly an approximation for selection.
- `run_implicit_cliff_reference.py --mode source-skin --planes-only` exports
  original cluster IDs, physical planes, point membership and36 finite discs
  in three coherent joint families. The final max-span18m export has177 clusters;
  root retains155 and removes22 shallow clusters within two release regions.
  No rectangular rock stamps, upscaling or smooth-front clipping is used.
- `assemble_cliff_skin_blocks.py` reconstructs each convex hull from its original
  physical members. Four deepest members anchor it within a retained core.
  Final core depth7m and anchors12m are vertical; the original back lies24m down.
  A graded inner opening preserves the original outer crop guard.
- A failed exact-boolean result had one tiny triangle below the existing area
  threshold. The final result collapses only its0.393mm shortest edge; endpoint
  displacement is about0.1965mm. The acceptance threshold was not relaxed.
- Final closed solid:26,426 vertices,52,872 triangles, one connected component,
  no boundary/nonmanifold/winding/degenerate/duplicate defects. All7,488 original
  guard vertices are preserved. Candidate SHA256
  `0ced0f0d9e3140ae98e058c037126984bc517a903bb2280fea52793775d9ab1f`.

## Honest surface review

The initial closed-volume renders include a24m artificial backing used only
for booleans. `extract_cliff_authoring_surface.py` removes precisely those
explicit side/back caps; it does not alter positions, hide the rock transition,
or project overhangs to a heightfield. The open surface is72x40x65.133m with
14,681 vertices/28,936 triangles. It has one connected component,448 boundary
edges identical to the original source boundary, and no topology defects.

Actual final flat-clay wall/oblique/1500m views are under
`Saved/ImplicitCliffFit/LargeMassSurfaceReview`. The1500m view uses a35mm lens
on a36mm sensor and a100m reference bar; this is an isolated outcrop, not the
full crater. Flat and angle35 comparisons show that smoothing softened some
face changes, while the thin fins and abrupt transition are real geometry.
Representative original renders are retained in `taskboard/review/crater-pass11-15nov`.

## Reproduction and next step

Use installed Blender5.3/Python with NumPy, and new ignored output directories.
The final artifacts are under `Maelstrom-Saturday/Saved/ImplicitCliffFit`:

1. Generate `SourceSkin/points.json` with `Tools/generate_cliff_skin_samples.py`.
2. Run the reference wrapper with the inspected upstream clone, `--mode source-skin
   --sample-points <points.json> --planes-only --skin-max-block-m 18`.
   This produces `LargeMassPlanes/Geometry/block_planes_m.json`.
3. Run `Tools/assemble_cliff_skin_blocks.py` with those points/blocks and
   `--core-depth-m 7 --rear-depth-m 12 --release-patches`, producing `LargeMassClean`.
4. Extract both baseline/candidate authoring surfaces, then use the existing
   `review_cliff_outcrop.py --open-surface --normal-mode flat` with matched views.

Next: keep the source-informed volume approach and larger mass hierarchy.
Resolve the rectangular exposure transition with an irregular, fracture-aligned
transition that retains the real guard, and remove mechanically implausible
thin fins without smoothing away the broad faces. Review that actual surface
before native preview/500m extension. Do not return to free cube placement,
uniform surface noise, or another round of generic audit infrastructure.
Full sector seams, all temple supports, material-mask correspondence, waterfall
contacts and runtime integration remain outstanding. No native build was
required for this offline-only pass; every new generator was actually executed.
