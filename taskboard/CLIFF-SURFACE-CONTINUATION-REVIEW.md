# Cliff surface continuation: actual CPU evidence

15 November. Reviewer branch `codex/implicit-exposure-review`; optional material
review tool committed as `e9ca6b4`. No Unreal/runtime changes by this worker.

All images below are under this worker's ignored
`Saved/ImplicitCliffReview/HistoricalCraterCut/`. Existing source-frame cameras,
neutral fixed light, CPU/eight threads, 24 samples and angle-179 continuous
normals were retained unless explicitly identified as the flat diagnostic.
Source/camera/material hashes and topology results remain in each `review.json`.

## Actual comparisons

| Candidate | Candidate SHA256 | Captures and observation |
| --- | --- | --- |
| DisplacedFaceA | `f9138107bece82bc6b47dfb87607fed90a5a2d57a8a428ba63da2955f5b1c848` | `DisplacedFaceAClay/{oblique,wall,close_wall}_fractured.png`: real native-scale relief, but fine repeated stucco on the same broad cut. `DisplacedFaceAMaterial/{oblique,close_wall}_fractured.png` exposes regular texture repetition. Rejected for appearance. |
| DisplacedFaceB | `4b26b17a78dafc9f24805fda7c76cbd662840d16b9a5880bb7e39487f41b72e1` | `DisplacedFaceBClay/{oblique,wall,close_wall}_fractured.png`: eight stronger recesses register clearly, but read as capped vertical slots beneath horizontal lintels. Rejected; no material or distant follow-up. |
| ParentCompressionA | `d4902107e275836ea1d10ff6470615d8763a2a0c51166e1376573f23774a5992` | `ParentCompressionA/{oblique,wall}_fractured.png`: original parent massing continues through the face, eliminating the blank cut and capped slots. Compression also creates thin folded-looking ridges and dark diagonal clefts. |
| ParentCompressionSigma6 | `0753f94e7c54646dc2c2136cfc3a2296af241910dab5e7d699c6fd57f97d80f6` | `ParentCompressionSigma6/{oblique,wall}_fractured.png`: source-height smoothing reduces fine serration/folds modestly; continuous branching masses remain. Dark diagonal clefts and soft folded surfaces persist. Useful for root's isolated native-context evaluation, not final cliff/scene approval. |

`ParentCompressionAFlatDiagnostic/oblique_fractured.png` uses the same positions
and triangles with flat normals. The black diagonal slashes persist, so they
are not the previous angle-15 split-normal artifact. They appear as thin deep
compressed valleys/creases and shadows; this review does not claim mesh holes
or self-intersections from the image alone.

Compression is explicitly authored radial deformation of the original parent,
not a new erosion solve or a physical-scale-preserving terrain simulation.
U spans 400 m. The selected uncompressed baseline is about 807 m deep; candidate
A spans 203.04 m radially. Sigma6 applies normalized source-height smoothing
with sigma 6 m/support 18 m before the same compression. Source Z changes by
−7.92..+8.13 m; its final dimensions are 400 × 206.878 × 204.628 m. No renderer
zoom, actor-scale magnification or claim of exact original heights hides this.

Measured mean-water overlays use the exact original frame. The independent
worker/root verified DisplacedFaceA's fixed contact guard and fresh sampled
dry clearance of 35.0037 m. Compression candidates recompute contour/toe joins;
their modern contact/route conclusions require their own fresh root query.
An overlay alone is not contact certification. Temple supports, old sector
seams and full animated-route compatibility remain separate, unapproved work.

## Real map and rendering contract

The installed Paragon slate D/N assets are verified albedo/native normal maps;
no installed natural-cliff displacement was established. AncientRuins baked
cliff instances reference D/N/M through the common material. Its architectural
wall/floor H maps are used with height blending, not proof of a natural cliff
displacement source; the Paragon packed rock mask's channel roles remain unclear.

Root obtained verified CC0 Poly Haven `marble_cliff_01` maps. Read-only inspection
of the original blend confirms a 4.299351692 m square source plane, unit mapping,
normal strength 1, and authored displacement scale 0.6/midlevel 0.525. Those
authoring settings are not independent survey calibration.

Optional `review_cliff_outcrop.py --rock-source <source-directory>` applies its
actual diffuse, GL normal and roughness maps only to explicit `face_kind=1` cut
triangles. All share U/Z coordinates at the native tile size; diffuse is sRGB,
normal/roughness Non-Color. No renderer displacement is added. The mesh worker
confirmed U/Z remain unchanged by its V-only displacement, so height and surface
maps stay registered. Original source files are immutable and all bulk stays
under Saved. Neutral clay remains the default and the primary geometry control.

`MaterialContract/close_wall_fractured.png` and DisplacedFaceA material images
verify the maps load and align, while honestly exposing periodic repetition.
Completed A distance images exist in `DisplacedFaceAMaterial/`, but were not
independently viewed after root halted further A review; no distance-quality
conclusion is made from those files. Root handles native material variation and
the actual Unreal context test of the latest geometry.
