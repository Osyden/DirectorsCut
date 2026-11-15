# One measured lower waterfall attachment

Owner: Codex waterfall worker, `codex/implicit-exposure-review`. Offline CPU authoring only; no live actor, sky, cloud, water or Unreal asset changed.

## Current output for isolated native preview

Use `Crater-Workers/Fitted-Waterfall/Saved/CompressedWaterfall/RingPilotB`, not the earlier Sigma6 folders. The actual source is `Cliff-Contact/Saved/CompressedEscarpment/RingPilotB/candidate.npz`, SHA256 `18e7b77ce3540d52ebb459f40571e2980a6632dcba84073d6f6de502e2112046`. All support/clearance queries use those ring triangles. `compressed_chart_uvz_m` only locates the seed; it is not used as replacement geometry. Receiving water is the finite measured `Maelstrom-Saturday/Saved/StructuralCliff/mean_water.npz`, SHA256 `bd4514e69d97799d52a78c2100d39a5d85be6b02662036a2866d818333177ff2`.

- Actual lower outlet: 32.46 m drop, 34.35 m centreline, 2.60–3.78 m width. Source ring vertex 12860. Source arena metres `(-1058.7471,1168.1472,-89.7581)`; receiving point `(-1052.6556,1161.6619,-122.2193)`.
- Last 4.32 m is an authored ballistic jet with 6 m/s horizontal launch and 4 m/s initial downward speed, gravity 9.81 m/s². Thirteen separately supported lateral tracks are tested against the actual rock and finite water. They have different contact times, approximately 0.60–0.66 s.
- `support.npz` records original triangle IDs/barycentrics, path and lateral sections. Attached core triangles that bridge a sharp crease are subdivided and reprojected; a bounded clearance failure aborts generation.
- `core.npz` has 2,736 triangles; `receiving_foam.npz` has 590. Foam filaments sample actual sloping water, following the same prescribed 18 m/s route-tangent direction as the runtime exporter. These are static geometry/UV anchors, not a completed foam shader.
- `waterfall_core.foam.fbx` contains the two meshes and three diagnostic material slots: `WetCore`, `AeratedCore`, `ReceivingFoam`. UV0 is across-fall fraction / physical arc metres. Replace diagnostic stripes with the native animated water/whitewater materials; the stripes are visibly unsuitable as final art.
- `waterfall_profile.json` uses the existing `directorscut.waterfall_profile.v1` schema and explicit preview source ID **800**, not runtime feed 8. `attachment.json` records exact source hashes and physical transforms. No old feed-8 cache fits this source.

## Transform contract

The FBX is in the existing sector chart, metres, exported `axis_forward=-Y`, `axis_up=Z`, unit scale 1. Its documented chart origin/bases are in `attachment.json`. Account for the already-verified native FBX Y reflection before reconstructing arena coordinates; do not simply set its actor transform to the identity used by the full-ring mesh. The `.npz` mesh positions themselves are absolute arena metres.

VDB uses its own yaw-only physical centimetre frame. The exact `local_origin_arena_cm` and basis vectors are in the profile. X is minus sector U, Y is minus sector V (outward), Z is up. Keep native SVT voxel size 50 cm and `pivot_at_centroid=false`, component scale 1. Do **not** apply the FBX Y reflection to a VDB frame. The foot is locally `(0,1000,0)`; source/impact locations are in the profile, not inferred from volume bounds.

The initial cache is 48 frames at 12 fps, four seconds, 0.5 m voxels, FloatGrid density. `Volumes/manifest.json` records actual fixed bounds and per-frame hashes. Both sequences total 14,626,967 bytes (13.95 MiB), generated in 6.25 s offline. Falling resolution is 25×40×84; impact is 69×78×59. All 20,000 generated strands reach covered water; maximum measured sampled entry residual is 0.000023 m. Exact t=0 versus period and written/read VDB density differences are zero. These are CPU checks, not native rendering or GPU-cost claims.

The fitted generator retains its existing mass/voxel normalization, submerged-density mask and entry-synchronized impact. No whole new fluid solver was added. The profile's central guide and separate lateral tracks are not identical clocks; the VDB generator integrates travel speed on that guide, so the recorded static jet shape does not claim a validated native velocity field.

## Actual visual result and limitations

`Saved/CompressedWaterfall/RingPilotBVolumeReview/close.png` and `sector.png` render actual ring triangles, actual core, and VDB frame 12 using CPU Blender. The correctly labelled volume render supersedes the geometry-only picture's stale Sigma6 caption. No image generation or geometry exaggeration was used.

The lower fall is visible and correctly placed, but **not visually accepted**: the bare core reads as a straight striped ribbon, and the CPU volume is dark/soft under the fixed review sun. The volume uses actual float density and no emission; it does not predict Unreal's UNorm8 import or its lighting. The foam filaments are too thin/regular to count as finished receiving foam. Native material/lighting evaluation is required, followed by deliberate sheet breakup and splash detail.

The first longer ravine path was mostly hidden by its own walls. An attempted longer jet there failed whole-rock clearance and was rejected rather than forced through terrain. Horizontal projected widths also produced tall jagged strips; the current tool instead uses surface-tangent widths and nearest original triangles. Earlier Sigma6 caches and meshes are diagnostic failures, not integration candidates.

The current lower outlet does not provide the reference's epic rim waterfall. Root has requested a separate supported high-rim freefall with a 100–180 m drop and 10–20 m/s feeder momentum on the current ring. That is the next authoring task. The existing lower cache is useful for testing attachment and native rendering while the tall source is selected.

Static/local limits: no upstream feeder mesh yet; no complete route or animated-wave collision guarantee; receiving coverage is resampled at 1 m for the VDB. Any ring geometry change invalidates the attachment and fitted cache. No terrain, support, or whole-scene acceptance is implied.

## Reproduce

Run Blender background CPU with `Tools/attach_compressed_cliff_waterfall.py -- --cliff <RingPilotB/candidate.npz> --water <mean_water.npz> --output <new Saved directory> --template-source-index 939 --width-m 4 --render`. The output directory must be new.

Then run bundled Python with `Tools/generate_waterfall_vdb.py --profile <output/waterfall_profile.json> --output <output/Volumes> --width-m 4 --height-m 40 --voxel-m .5 --fps 12 --duration 4 --emission-rate 5000`.

For an actual combined CPU frame: Blender background, eight threads, `Tools/review_compressed_waterfall.py -- --attachment <output> --output <new Saved review directory> --frame 12 --samples 32`. All generated meshes, caches and images stay ignored under Saved.
