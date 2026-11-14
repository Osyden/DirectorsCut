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

## Prepared FBX path, deliberately not run on rejected geometry

`Tools/export_eroded_cliff_fbx.py` can export a clean accepted tile in centred local U/V metres, with the median elevation as local Z origin and an explicit arena origin/basis in its JSON sidecar. Standard Blender settings are forward `-Y`, up `Z`, scene unit 1 m and applied unit scale. It writes linear vertex masks R=normalized log cumulative flow, G=normalized gross deposition, B=normalized gross erosion, A=1; normalization divisors and raw maxima are recorded. A roundtrip option checks physical bounds, triangle count and mask values at matching physical corners. Runtime colour quantization and Unreal import remain unverified.

The installed exporter exposes linear colour export and active-colour priority. Epic documents [static-mesh vertex colours through FBX](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine); these are suitable data channels for a later Nanite static-mesh preview. Nanite does not repair weak landform shape or stitch an open tile to the live cliff. No FBX was produced, imported or enabled in gameplay because this candidate failed the clay acceptance gate.

## Branched-face revision

The cliff worker supplied `source08_clean_branched_seed.npz`; its untextured views are in `Saved/CliffLandform/BranchedSeedReview`. This makes the recesses deeper and asymmetric, but several now resemble pointed openings below a continuous smooth cap. Repeated full-height partitions remain. The plan view shows isolated pockets below the protected upper plateau rather than tributaries feeding real mouths. It is also rejected before erosion/FBX. The next proposed revision permits explicit preview-only catchment cuts behind the rim; land-placement/support integration must be revalidated before any runtime use.

The renderer additionally accepts `--camera-manifest <previous review.json>` to lock cameras across different authored seeds, not only within each seed/erosion pair.
