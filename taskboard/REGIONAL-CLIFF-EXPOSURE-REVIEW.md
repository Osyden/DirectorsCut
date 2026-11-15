# Regional original-3D cliff exposure review, 15 November

Scope: independent matched CPU clay review only. The root owns composition and
Unreal; the geometry worker owns generation. This review changed no geometry,
runtime, materials, source frame, water or engine assets.

## Evidence

Regional source: `Flow-Seams/Saved/RegionalCliff/JointExposure/`, original
`visible_baseline.npz` and `visible_candidate.npz`. Candidate SHA256:
`8e04e6981e0ba3b14795205aae7e7598daca62e016da52a8b4cf0080f523de1f`.
Its actual chart bounds are 160 × 96 × 105.61 m. This is one continuous larger
exposure, not a repeated array of the previous small patch.

Full context: root `Maelstrom-Saturday/Saved/RegionalCliff/FullContext/`,
`baseline.npz` and `context.npz`. Candidate SHA256:
`1d105f9d199c3046d10b21e35347948ba6c7a961e840096295678c7d7ed7a916`.
Actual chart bounds remain 500 × 492 × 230.63 m.

Captures are under this worker's ignored
`Saved/ImplicitCliffReview/RegionalExposure/`:

- `NearAngle15`: regional wall, oblique and 1.5 km perspective pairs.
- `FullContextWithWater`: full-sector wall, oblique and 1.5 km perspective
  pairs, with the root's measured mean receiving-water mesh.
- `BaselineWithWater`: previous full-sector control in the same cameras.

Each paired view has `_baseline.png` and `_fractured.png` filenames. The latter
is the existing renderer's candidate label, not an additional processing step.
The full-context cameras are exactly those from the prior 500 m comparison,
stored in `Saved/ImplicitCliffReview/StructuralMacro/cameras.json`. The far
perspective uses 35 mm focal length / 36 mm sensor, 1,500 m horizontal range,
100 m reference bar and 10 m posts. It is physical-scale isolated context, not
an actual ride-camera or final atmosphere capture.

Used the root's existing updated `Tools/review_cliff_outcrop.py`, Cycles CPU,
eight threads, 24 samples, identical neutral rough clay and raking light,
15-degree normal shading. No vertical exaggeration, geometry smoothing,
texture or displacement was added. The mean-water mesh is separate measured
context only; it hides the cropped submerged toe and has no animated waves.

## Actual visual decision

The exposure creates real recessed planes and an unequal block hierarchy over
a broader area. It is materially more promising than the rejected giant
concrete-like escarpment prisms. The larger region stays legible at 1.5 km.

It is still not convincing whole-cliff geology. At close range, very clean
polygonal slabs and dark gaps make the central region read as shattered rubble
or a jigsaw. Several thin upper-left splinters look mechanically dubious in
the image, without this constituting a structural simulation finding. In full
context it remains an isolated angular scar inside soft, fluted terrain. The
massif silhouette and surrounding geological structure have not become coherent
merely because the exposure is larger.

Keep this as a bounded original-3D technique proof, not final art acceptance or
authorization to repeat it across the sector. The margins need to share the
same mass/joint hierarchy as the surrounding cliff; additional isolated detail
or blanket noise would not resolve the current contrast. Root decisions about
technical native previews remain separate from this whole-cliff rejection.

## Topology distinction

The initial regional visible surface has 64,574 vertices / 128,158 triangles;
the composed context has 113,141 vertices / 225,324 triangles. Both preserve
their original outer boundary edge geometry (1,024 and 992 edges respectively)
at the reviewer's 10-micrometre comparison tolerance. No nonmanifold,
inconsistent-winding, degenerate, duplicate or unused-vertex flags were found.

The generic one-component open-surface gate reports three components. The
independent audit worker verified that the extra two are trapped inward-facing
cavity shells, 30 + 36 triangles and total signed volume -0.0711064 m³. All 103
tested cavity vertex/face-centre probes were inside the main shell on two
independent oblique rays. They are not positive floating rock solids. The
recommended 66-triangle cleanup preserves every external visible face, so it
does not resolve or invalidate the appearance criticism. No cleanup was made
by this reviewer and no revised hash is implied here.
