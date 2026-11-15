# Implicit exposure transition review, 15 November

Worker branch `codex/implicit-exposure-review`, based on `c44dba5`.
Scope: matched CPU clay review using the existing renderer. No geometry,
renderer, Unreal, asset, lighting or gameplay source changes.

## Control

Control inputs are the original exposed surfaces under
`Maelstrom-Saturday/Saved/ImplicitCliffFit/LargeMassClean/`:
`visible_baseline.npz` and `visible_candidate.npz`. The artificial authoring
backing is excluded; physical surface bounds are 72 × 40 × 65.13 m.

Control and comparisons are under this worker's ignored
`Saved/ImplicitCliffReview/ExposureComparison/`. `CleanFlat` and `CleanAngle15`
show the original surface against the cleaned large-mass exposure. The wall and
oblique cameras are exactly those in the root's `LargeMassSurfaceReview`; a
fixed side camera was added in the local `cameras.json` manifest. Subsequent
candidates reuse that manifest without reframing. Each set contains wall,
oblique and side views at 24 samples, Cycles CPU/eight threads, identical rough
clay and raking light, no atmosphere, textures, displacement or geometry
smoothing. Normals are either flat or 15-degree shading, explicitly labelled.

Angle 15 preserves the major planes better than the earlier angle 35 setting.
The hard exposure border and narrow upper/left fins remain visible in both
control shading modes; they are geometry issues.

## First transition candidate: rejected

Input: `Flow-Seams/Saved/ImplicitFaceProof/JointBoundary/visible_candidate.npz`.
SHA256: `e99939b37fc542bf32b76af796199c2acb402402958e1ab219776208b854cdfe`.
Matched old-exposure-versus-new-exposure outputs are `JointBoundaryFlat` and
`JointBoundaryAngle15` under the comparison directory. In these pairs,
`_baseline` is the previous cleaned exposure, not the original smooth terrain.

The new footprint is less rectangular and the upper transition is quieter, but
the grading creates conspicuous thin vertical sawtooth skirts along several
central diagonal joins and the lower-left edge. These are visible under flat
and 15-degree shading. They read as generated curtains rather than plausible
rock contacts and are a regression from the cleaned control. This candidate is
**not credible enough for a native bounded proof**.

The existing reviewer also finds four connected visible components, compared
with one in the old cleaned exposure. The 448 outer boundary edges remain
unchanged, with no other edge/winding/degenerate/duplicate/unused-vertex flags.
The independent audit worker subsequently verified that the three extra
components are negatively oriented enclosed cavity shells (532 faces), with
centroid containment parity 1, not positively oriented floating rock pieces.
The generic open-surface review's one-component requirement therefore does not
diagnose a disconnected source solid here. The appearance rejection stands.

## Corrected exterior union: bounded preview gate passed

Input: `Flow-Seams/Saved/ImplicitFaceProof/JointBoundaryUnion/visible_candidate.npz`.
SHA256: `b889d3a18a74dcf0c7b2b5b4d9bb271c7907a16ae795ae571bff979fe6c48c4c`.
Outputs: `JointBoundaryUnionFlat` and `JointBoundaryUnionAngle15`, again paired
against the previous cleaned exposure with identical cameras and light.

The corrected union perimeter removes the central sawtooth curtains in both
shading modes. Large coherent fracture planes remain, the exposure boundary is
less rectangular, and the side view retains real depth instead of relying on
shading. This is a meaningful repair. It is **credible enough for a native
bounded context proof**, subject to the independent full-solid/support audit.

It is not a finished natural cliff. The slabs are unusually clean and angular,
several joints are deep and dark, and the contrast between the broken exposure
and soft surrounding terrain is still conspicuous. Do not repeat this patch
across the sector or claim final visual acceptance on the strength of this
small clay view. A game-distance comparison should establish whether these
planes and recesses read as rock at the actual lighting and scale.

The corrected visible surface has 14,839 vertices and 29,240 triangles, one
connected component, and the same 448 actual outer boundary edges at the
reviewer's 10-micrometre comparison tolerance. No nonmanifold, inconsistent
winding, degenerate, duplicate or unused-vertex flags were found. Its explicit
open-surface audit passes; it remains an open surface and the reviewer does not
claim enclosed-volume, temple-support or exhaustive self-intersection proof.

All six corrected views were inspected. The angle-15 pass changes shading only;
the renderer verifies positions and topology unchanged. No new geometry,
engine import, runtime mutation, FBX export or sector acceptance was performed
by this review worker.

## Single patch in the full massif

At the parent's request, rendered the unchanged full-sector baseline and one
substituted patch from `Flow-Seams/Saved/ImplicitFaceProof/FullMassifContext/`.
The actual chart dimensions are 500 × 492 × 230.63 m. Inputs are `baseline.npz`
(SHA256 `15583c3d1a9cd6ba6db5b2053db7386a816e1e3753c490824154d8d9487c6809`)
and `context.npz`
(`bea887a211dd15ded0fb73f8d1880e331c668bd6ed4ec3cd3ef4bfc169ab03ff`).
Matched wall and oblique outputs are under this worker's ignored
`Saved/ImplicitCliffReview/FullMassifContextAngle15/`; each camera has
`_baseline.png` and `_fractured.png`. Both use the same original sector frame,
15-degree shading and neutral CPU clay setup, with no vertical exaggeration.

The 72 × 40 m patch has the right subordinate scale, preserves the massif
silhouette, and supplies legible planes and recesses where the source had a
smooth face. It nevertheless reads as a sharp fractured scar in broad soft,
fluted terrain. The surrounding slopes lack a common geological joint/strata
hierarchy. The regular triangular fringe and stepped diagonal seams are already
visible in the baseline; they were not introduced by this patch substitution.

This preserves the recommendation for a **bounded native local proof**, but
does **not** justify repeating the patch across the sector. The full context
has not become a coherent realistic cliff merely by adding this exposure.
The macro landform, drainage and joint orientations need to agree before a
sector-wide fracture pass. The existing open-surface audit passes, with all
992 outer boundary edges unchanged; topology success is separate from this
visual limitation. No source geometry was mutated by the reviewer.
