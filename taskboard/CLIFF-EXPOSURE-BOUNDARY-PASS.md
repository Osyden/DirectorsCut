# Source-informed exposure boundary and fin correction

Owner: Codex `cliff_material`, branch `codex/cliff-skin-boundary-fins`, based on `c44dba5`.
Only `Tools/assemble_cliff_skin_blocks.py` and this handoff change. The source points, large-mass fracture seed, terrain, materials and Unreal remain untouched. This is a bounded original-mesh authoring proof, not a full-sector integration.

## Diagnosed source defects

The previous `LargeMassClean` carved a graded 60 × 28 m rectangular window. Its shape remains visible even after the artificial crop closure is removed. More importantly, every retained hull received four anchors at the same 12 m vertical depth whether it needed support or not.

Independent audit found 134 of155 retained original clusters already had at least four points more than0.5 m inside the old core. Of620 manufactured anchor points, **325 moved upward** from their original source positions. Twenty-two clusters acquired extensions over6 m, with a maximum11.1766 m. This changes source plane sets and creates long unsupported wedges. Original thin fragments also exist; indiscriminately deleting thin faces would remove valid buried geology.

## Bounded change

Opt-in `--joint-transition` preserves legacy reproduction when omitted:

- Original source-cluster XY footprints define the exposure, selected through one central connected authoring region. Their union determines the outer boundary rather than the rectangular cavity bounds. A0.75 m halfspace margin seals gaps at the point-lattice scale; this is a polygon-plane offset, not a maximum Euclidean corner expansion. Exterior flood fill prevents a projection hole from becoming an artificial unfractured pillar. A0.25 m crop-XY Euclidean distance grid gives a4 m inward transition; the actual4 m crop guard stays fixed. This is an exposure mask only; it does not smooth or displace the block faces.
- An original hull with four spatially spread points at least0.5 m inside the local core is retained **without any new anchors**. Otherwise only a downward extension toward that actual core is permitted, at most2.5 m vertically. A fixed-depth target can never raise a source point.
- Only an unsupported, exposed, physically thin whole fragment is detached: original PCA thickness below1 m, thickness/longest-span below0.12, longest span above3 m. Already-supported/buried thin blocks stay. A thick fragment requiring a longer unsupported anchor is also detached rather than made into a fin. The previous two coherent release patches are retained.

The final candidate keeps139 original hulls unchanged and uses9 short local-core anchors, maximum2.426592 m. Seven additional unsupported fragments detach (IDs26,75,76,84,88,93,100), plus the previous22 shallow-release clusters. All decisions and footprint IDs are recorded in `assembly.json`.

## Two actual candidates, preserved separately

1. `Saved/ImplicitFaceProof/JointBoundary` is **rejected**. Maximum per-footprint clearance treated internal overlap edges as transition edges. With a1.5 m falloff this generated visible serrated skirts. Numerical manifold checks did not reveal that appearance failure; matched clay did.
2. `Saved/ImplicitFaceProof/JointBoundaryUnion` corrects the same cause using only the exterior union distance and4 m falloff. It retains26,584 vertices /53,176 triangles in the closed source, with zero reported boundary/nonmanifold/degenerate edges/faces; minimum triangle area3.33e-8 m². The existing closure-only extractor gives29,240 original open-face triangles without changing positions or projecting overhangs.

Author and independent reviewer viewed the actual corrected flat wall/oblique clay: the sawtooth curtains are gone, larger planes remain, and the perimeter is quieter and less rectangular. This passes only the bounded native-context-preview gate, subject to the independent guard/component audit. Clean angular slabs, deep narrow joints and the contrast with soft surrounding terrain still fail the finished natural-cliff target. No further geometry iteration was made after this review.

Corrected closed-source SHA256: `0c4814c37e34f9e4c1892a04705496c8b28b0115bd0eb5e0e468b2de86d37e3e`.
The first candidate's extra negative surface components were enclosed cavity shells, not proven floating positive solids. Closed-edge counts alone are not a single-component guarantee. Preserve that distinction when reviewing either output.

## Reproduce and validate

From `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`, with a new output directory:

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 --python Tools/assemble_cliff_skin_blocks.py -- --points E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/ImplicitCliffFit/SourceSkin/points.json --blocks E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/ImplicitCliffFit/LargeMassPlanes/Geometry/block_planes_m.json --output Saved/ImplicitFaceProof/JointBoundaryReplay --core-depth-m 7 --rear-depth-m 12 --release-patches --joint-transition
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/extract_cliff_authoring_surface.py --input Saved/ImplicitFaceProof/JointBoundaryReplay/candidate.npz --output Saved/ImplicitFaceProof/JointBoundaryReplay/visible_candidate.npz
```

The legacy `--rear-depth-m12` argument is unused in joint-transition mode; current metadata makes that explicit. No runtime/GPU work occurred. Syntax and `git diff --check` pass. A focused regression using overlapping rectangular footprints verifies no false transition at their internal overlap, finite distances, and exactly zero exposure at/outside the4 m guard. The final small input-dtype normalization accepts integer-coordinate test footprints and does not change the float-coordinate generated candidate.

Review compares against root's `LargeMassClean/visible_candidate.npz`, using the original open-face geometry. Parent/reviewer owns acceptance and any native export. Do not equate this source correction or a successful closed mesh check with photorealistic cliff approval.
