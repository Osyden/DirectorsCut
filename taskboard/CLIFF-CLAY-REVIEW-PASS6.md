# Clean cliff clay review, pass 6

Owner: waterfall/review worker, `codex/crater-clay-review-pass6`, starting at `540c059`. No runtime geometry, shader, cloud or lighting changes. The cliff worker owns landform generation and the erosion worker owns transport.

## Actual result: reject this sector before FBX import

Reviewed the clean source-8 seed and its bounded 256-step open-outflow erosion result in matched CPU clay, at physical scale without detail shaders. Both remain a continuous broad wall with rounded, almost full-height slots. The clean seed removes the folded old wall's projection artifacts, which makes it a more honest simulation input, but does not yet establish a convincing natural cliff.

The deck view has a nearly continuous rim line and little hierarchy between large projecting buttresses, recessed catchments and smaller joints. The oblique view exposes a smooth broad cap and an almost uniform tall face. The plan view shows a few simple longitudinal grooves rather than branching drainage. The lower cut boundary has visible triangulated teeth; these are mesh/padding boundary artifacts, not geological detail.

The eroded result mainly changes a narrow band near the foot, sharpening the base of the rounded slots and placing small deposits beneath them. It leaves the large silhouette and slab shape essentially unchanged. Measured maximum net cut is 2.261 m and fill 4.583 m across the 500 x 504 m sector. The maps agree with the erosion worker's concern: gross erosion/deposition concentrate in a slope band, while flow across the upper protected area does not produce convincing incised catchments. A stronger run on this same seed would amplify the wrong shape.

Next geometry change should establish unequal projecting ridges/buttresses, genuinely recessed and converging catchments, discontinuous escarpment setbacks and varied major face heights while preserving the protected shoreline and temple pads. Then evaluate whether erosion reinforces those features. This is a design recommendation, not a claim that erosion alone can generate overhangs or credible rock fracture.

## Reproduction and evidence

- Seed: `Crater-Workers/Cliff-Contact/Saved/CliffLandform/source08_clean_seed.npz`; metadata `source_kind=clean_macro_landform`, seed 71421. The old `4bb5b0c` reference supplies boundary constraints only; the interior is newly authored.
- Result: `Crater-Workers/Flow-Seams/Saved/Erosion-Outflow/Clean-Source08/clean_source08_open256.npz`.
- Seed-only views: this worker's `Saved/CliffLandform/CleanSeedReview`.
- Matched wall, oblique, deck and plan before/after images plus maps/manifest: this worker's `Saved/CliffLandform/CleanOpen256Review`. The manifest includes input hashes, physical frame and exact cameras.
- Actual render: installed Blender 5.3.0, Cycles CPU, eight threads, 12 samples, flat triangle shading, fixed raking light. The paired run took approximately ten seconds. There are 104,078 valid triangles. No GPU or Unreal was launched.

`Tools/review_eroded_cliff_sector.py` now makes raw measured profiles and discarded-overhang maps optional, supports a seed-only review, and labels clean landforms distinctly from projected old geometry. Frame/protection equality, unchanged fixed heights and bedrock limits remain checked before rendering. The existing measured-seed format still loads.

## FBX path, withheld for the rejected initial geometry

`Tools/export_eroded_cliff_fbx.py` can export a clean accepted tile in centred local U/V metres, with the median elevation as local Z origin and an explicit arena origin/basis in its JSON sidecar. Standard Blender settings are forward `-Y`, up `Z`, scene unit 1 m and applied unit scale. It writes linear vertex masks R=normalized log cumulative flow, G=normalized gross deposition, B=normalized gross erosion, A=1; normalization divisors and raw maxima are recorded. A roundtrip option checks physical bounds, triangle count and mask values at matching physical corners. Runtime colour quantization and Unreal import remain unverified.

The installed exporter exposes linear colour export and active-colour priority. Epic documents [static-mesh vertex colours through FBX](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine); these are suitable data channels for a later Nanite static-mesh preview. Nanite does not repair weak landform shape or stitch an open tile to the live cliff. No FBX was produced for the rejected initial or branched seeds. The later ridged candidate below passed only the isolated-preview gate; no worker import or gameplay integration occurred.

## Branched-face revision

The cliff worker supplied `source08_clean_branched_seed.npz`; its untextured views are in `Saved/CliffLandform/BranchedSeedReview`. This makes the recesses deeper and asymmetric, but several now resemble pointed openings below a continuous smooth cap. Repeated full-height partitions remain. The plan view shows isolated pockets below the protected upper plateau rather than tributaries feeding real mouths. It is also rejected before erosion/FBX. The next proposed revision permits explicit preview-only catchment cuts behind the rim; land-placement/support integration must be revalidated before any runtime use.

The renderer additionally accepts `--camera-manifest <previous review.json>` to lock cameras across different authored seeds, not only within each seed/erosion pair.

## Catchment revision and independent ridged candidate

`source08_catchment_seed.npz` extends three explicit trunk/tributary cuts behind the rim. Its locked clay views are under `Saved/CliffLandform/CatchmentSeedReview`. The network connectivity is clearer, but similar Y-shaped cuts appear stamped onto a smooth cap and terminate in repeated dark slot mouths. It remains rejected for landform quality. Its proposed changes behind the rim also require later temple-footing validation.

The independent Blender ridged/heterogeneous XY terrain is a stronger macro starting point: unequal diagonal ridges, projecting masses and varied recess heights replace the repeated full-height slots. Two-octave before/after views are in `RidgedOpen256Review`. The actual bounded erosion preserves those masses and concentrates cut/deposition into several local areas instead of the earlier broad foot band. This is a meaningful change in geometry, though still a coarse heightfield with a large smooth protected cap, soft fins and unresolved rock fracture. It qualifies for an isolated authoring/material preview, not replacement of the live cliff.

The raw four-octave candidate in `Ridged4SeedReview` is diagnostic only. Its extra octaves raised the median height by 5.44 m, so the stronger relief could not honestly be attributed to detail alone. The erosion worker corrected this by removing the spatial low-frequency mean from the added-octave residual while preserving the two-octave macro signal. The corrected seed differs by median -0.0015 m, with P5/P95 about -3.13/+3.30 m, according to its generator audit.

## Preferred preview deliverable

Corrected four-octave seed and its actual 256-step erosion are reviewed with the original locked cameras in `Saved/CliffLandform/Ridged4DetailOpen256Review`. Smaller breaks now interrupt ridge shoulders and face planes without replacing the unequal macro masses. The result is preferable to the smooth two-octave seed for an isolated material/Nanite test. It still lacks real overhangs and geological layer/joint structure, retains a broad flat cap, and exposes coarse grid facets in places. Nothing here establishes AAA quality, live terrain compatibility or performance.

- Seed: `Crater-Workers/Flow-Seams/Saved/Ridged-Cliff/source08_ridged4_detail_seed.npz`.
- Actual solver result: `Crater-Workers/Flow-Seams/Saved/Erosion-Outflow/Ridged4-Source08/ridged4_detail_open256.npz`.
- Preview FBX and frame/mask metadata: this worker's `Saved/CliffLandform/RidgedPreview/source08_ridged4_detail_open256.fbx` and matching `.json`.
- The two-octave FBX `source08_ridged_open256.fbx` remains a valid comparison. The seed-only export has zero RGB masks and must not be used to evaluate erosion-mask quality.

The preferred preview contains 52,536 vertices and 104,078 triangles. Blender FBX export and import roundtrip preserve bounds within 0.0000153 m and individual physical corner positions within 0.0000229 m; all real RGBA values match exactly at their corresponding physical corners. The actual erosion has maximum net cut 2.269 m and fill 4.337 m. These transport masks are solver outputs, not painted placeholder data. Unreal's import units, axis conversion, vertex-colour quantization and Nanite settings remain root's explicit preview validation. All generated bulk remains under ignored Saved.
