# Deep erosion response: visual rejection, 15 November

Read-only review of the root's separate existing-solver experiment, not a third
cutting-geometry variant. Source:
`Maelstrom-Saturday/Saved/MacroErosionResponse/Deep640/{baseline,candidate}.npz`.
Candidate SHA256:
`d87ba287494eeb480131cd1355ddfaddaee43b4c275ffa6d851d31c82ba362c3`.

Actual matched wall/oblique captures are in this review worker's ignored
`Saved/ImplicitCliffReview/MacroErosion/Deep640/`, with `_baseline.png` and
`_fractured.png` pairs. Used the existing fixed 500 m cameras, original triangle
surface, angle-15 shading, neutral clay, raking light, measured mean-water
context, Cycles CPU/eight threads/24 samples. No source-frame changes,
exaggeration, added geometry detail or material tricks. Both candidate images
were inspected.

**Rejected for native visual preview.** The stronger erosion changes the
landform substantially, but broad areas become smooth planar aprons bounded by
sharp grid-like flutes and spines. Locked temple pads become conspicuous
isolated vertical pedestals as surrounding ground erodes. The result does not
produce convincing connected valleys and unequal rock masses; it is visually
worse than the source. Preservation of support triangles is separate from a
believable transition around those supports. The original cropped submerged
toe is hidden by the measured water context and is not the reason for rejection.

The root reports 640 model steps, 35–55 m authored erodible depth, maximum
erosion 52.43 m and deposition 17.17 m, with all 16,359 protected triangles exact.
These accelerated model steps are not a claim about realistic geological time.
The existing review confirms one valid open surface with 52,536 vertices /
104,078 triangles, all 992 outer boundary edges unchanged, and physical chart
bounds 500 × 492 × 221.15 m. Those numerical passes do not establish visual
quality. No geometry, solver, asset, Unreal or runtime changes were performed
by this reviewer, and no further render or tuning is implied by this note.
