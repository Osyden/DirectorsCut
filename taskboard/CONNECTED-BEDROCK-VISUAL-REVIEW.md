# Connected-bedrock visual review, 15 November

Independent CPU review after merging root `235a0fa`. No geometry, runtime,
Unreal, material or generic renderer edits by the reviewer. Both source shape
candidates were rejected; no native preview is recommended from these images.

## Fixed comparison

Used existing `Tools/review_cliff_outcrop.py` on original triangle NPZs, explicit
open surfaces, Cycles CPU/eight threads, 24 samples, angle-15 normal shading,
identical neutral rough clay and raking light. Added no displacement, texture,
geometry smoothing or vertical exaggeration. Root's separate measured mean
receiving-water mesh supplies coast context without animated waves.

This worker's ignored `Saved/ImplicitCliffReview/ConnectedBedrock/cameras.json`
preserves prior wall, oblique and 1.5 km cameras. At the parent's request it also
adds `opposite_rim`, 3.2 km horizontal distance with unchanged 35 mm focal length,
36 mm sensor and eye height. The whole 500 m sector occupies approximately
230 pixels across a 1440-pixel image at that distance. These are isolated
physical-scale views, not actual ride-camera or combined-atmosphere captures.

## First connected body: rejected

Input `Flow-Seams/Saved/ContinuousBedrock/First/candidate.npz`, SHA256
`fe8a08a7253de3e6f8019744780fa615dd2214dcb6d9ef5ee8797ab3ad1ea9c5`.
Paired wall/oblique images are in `ConnectedBedrock/First/`; paired 1.5 and
3.2 km images are in `ConnectedBedrock/FirstDistance/`. All candidate views
were inspected. Their matching `_baseline.png` files use the supplied unchanged
source; candidate filenames retain the renderer's `_fractured.png` suffix.

The central excavated-and-refilled scar is gone, but the 26 releases produce
scattered polygonal quarry pockets in the same soft massif. Broad smooth floors
and straight dark mouths remain visible from 1.5 km and still mark the sector
at 3.2 km. There is no convincing connected rock hierarchy. This is a repeat
of the earlier finite-slot/quarry-pocket visual failure, not a missing-texture
problem. The root independently rejected these images.

The first export is also numerically invalid: 50,963 vertices / 100,932
triangles, three components, 12 nonmanifold edges and 58 degenerate triangles.
It was explicitly rendered diagnostically before the author's cleanup. Its
992 outer boundary edges are preserved; this does not certify the full mesh.

## One permitted corner-release revision: rejected

Input `Flow-Seams/Saved/ContinuousBedrock/CornerRelease/candidate.npz`, SHA256
`cd41b13b28ef1056fe84cb54101b1e82fbcef90c04d18ac0bb36b9eb440c0b54`.
Paired wall/oblique captures are in `ConnectedBedrock/CornerRelease/`, again
with unchanged cameras, material, light, water context and physical dimensions.

Ten selected convex-shoulder releases reduce visual clutter, but the massif's
main silhouettes remain essentially unchanged. The surviving central and right
cuts still read as isolated flat-floored polygonal mouths. It fails the requested
coherent-massing improvement; fewer holes alone is insufficient. No further
distance renders were run because the parent explicitly conditioned those on
meaningful improvement in the wall/oblique views. No third morphology trial.

This export has 52,207 vertices / 103,420 triangles, one component, preserved
992 outer boundary edges, zero degenerate triangles, one nonmanifold edge and
one duplicate triangle flag. The independent auditor identifies a zero-thickness
opposite duplicate flap: cancellation of two faces and one orphan vertex
produces a clean open manifold in its in-memory check. All 16,359 protected
triangles are exact. That cleanup was not performed by this reviewer, and no
new source hash is implied. It does not resolve the independently observed
appearance failure.

No sector rollout, visual completion claim, engine import or runtime terrain
replacement follows from either candidate. The next direction must change
coherent exposed massing, not repeat isolated recess operations on the same
soft face or rely on stone textures to hide them.
