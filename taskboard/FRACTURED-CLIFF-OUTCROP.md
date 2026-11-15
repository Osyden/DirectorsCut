# True 3D outcrop: bounded prototype, realism gate not passed

15 November, owner Codex cliff worker, `codex/cliff-fractured-outcrop`, based on
`2b80091`. Own changes: new `Tools/generate_fractured_cliff_outcrop.py` and this
handoff. No existing generator, runtime, material, UE, cloud or water edits.

## Actual result and visual verdict

The tool produces a closed arbitrary-triangle rock volume from the immutable
eroded massif and subtracts finite volumes using Blender exact booleans. The
result contains actual roof undersides and multiple separated solid intervals
under a vertical ray. It is **not projected back into a heightfield**.

The first thin-joint candidate was visually rejected: long straight kerfs and
crosses were engraved into an otherwise smooth slab. Its initial six degenerate
boolean triangles were caught; a 10-micrometre cleanup removes those slivers.
That technical repair did not solve the visual failure.

One authorized revision replaces all standalone slots with 18 overlapping,
unequal convex wedge detachments. It creates clear exposed planes, projections
and undercuts. However, actual wall/oblique/side clay views still show pristine
polygonal pockets, similarly sized angular cutouts and straight shelves. It reads
as a quarried blockout, **not a realistic cliff**. Parent and independent reviewer
received this verdict. No FBX export, Nanite import or whole-arena expansion is
approved by this handoff. Stop iteration here pending the next design decision.

## Physical construction

- Source crop: row 89, column 64, 72m tangent by 40m outward; source surface has
  65.13m vertical span and about 54.5-degree median slope. Its closest shoreline
  is 36.50m away. It contains no protected samples and remains at least 12m from
  a temple pad. Current measured boundary/temple metadata stays in provenance.
- The original source's exact ABD/ADC triangles are sampled every 0.5m. This
  refines topology without inventing extra surface noise. A 24m vertical backing
  closes the volume for booleans; the backing is an artificial authoring aid,
  not claimed geological thickness. Total closed bounds are 72x40x89.13m.
- A 4m crop inset and a 12m-deep surface skin constrain all subtraction volumes.
  Crop-edge geometry and the connected backing remain intact. These guards
  preserve the current crop, not a yet-unimplemented full-sector join.
- Wedges have nominal 15–21m by 14–20m extents and 2.5–5.5m normal recessions.
  Their intersections create smaller exposed faces; the six-corner primitive
  footprints remain too recognizable. Unequal spacing, strike/dip and tapered
  rear corners are recorded explicitly in `outcrop.json`. No claim is made that
  this is physically simulated fracture or final 4–12m block geology.
- Output coordinates are absolute arena/design-world XYZ metres, before the
  ship-centred runtime pose. NPZ stores `vertices_m`, `triangles`, common
  `frame_origin_world_m`, tangent/outward/up bases and `metadata_json`. PLY keeps
  the same original triangles. The blend uses the equivalent centred local frame
  and contains `Baseline` and `Fractured` objects. No material masks are invented.

## Reproduction and retained artifacts

Run from `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams` or a checkout of this
source with the recorded immutable input. Blender 5.3.0, `02f0899d78e3`; CPU only,
eight threads. Include `--python-exit-code 1` so a failed geometry check is also a
failed process. The tool refuses to overwrite an earlier output directory.

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python-exit-code 1 --python Tools/generate_fractured_cliff_outcrop.py -- `
  --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz `
  --output-dir Saved/FracturedOutcrop/wedges --mode wedges --seed 81515
```

Input SHA256:
`1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.
The [massif handoff](MASSIF-CLIFF-LANDFORM.md) records its measured-source/anchor
dependencies. Do not regenerate those from a different current formula silently.

Final generated folder:
`E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/FracturedOutcrop/wedges/`.

| Original artifact | SHA256 |
| --- | --- |
| baseline.npz | `6be60bcbdfa6d2ce9520fd7baadb8dfa0eadcc472578c76ae61192b80d7132ee` |
| fractured.npz | `117868b62c7422e50d6360ef18023f92a56c67387f40826f9abe3f54fe12c46f` |
| fractured.ply | `aef655447e4580f980e55ee5bc5dc1a5ae2e3a839c08474613446e78f123161a` |
| outcrop.blend | `66be4206b455456cf25edf7ade1a7012c9895bb2db9de6537df04c32bac884a1` |

Sibling `first/` retains the six-degenerate-triangle failed slot candidate;
`clean/` retains its valid but visually rejected form. `--mode slots` reproduces
that old shape with cleanup. `wedges_replay/` is an actual identical-input replay:
all original vertices and triangle indices match exactly. Archive timing and
path metadata can change byte hashes on reproduction.

Actual locked CPU renders are under:
`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/OutcropReview/Pass9-WedgesFlat/`.
View `wall_fractured.png`, `oblique_fractured.png`, `side_fractured.png` against
their matching `*_baseline.png`. Both use the same rigid camera/frame, physical
scale and flat normals. The geometry is original, not a smooth-normal illusion.
The reviewer owns the generic review tool and retained review evidence.

## Geometry verification, not visual acceptance

Final original mesh: 18,290 vertices, 36,576 triangles, no boundary/nonmanifold
edges, no degenerate triangles, all finite. Signed volume is 56,602.098m3 versus
69,120m3 baseline. Original crop boundary remains exactly on the candidate mesh;
independent review also verifies all 896 border vertices.

The generator's one-metre probe grid finds 353 rays with four or six intersections
and 800 interior downward roof triangles, excluding the artificial backing.
Independent original-triangle C++ BVH checks find 242 multi-interval rays in a
different 64x64 probe grid; baseline has none. Its 256 brute-force-oracle rays
have zero mismatch and maximum distance error 3.09e-11cm. Different sampling
grids explain the different counts. These establish genuine 3D cavities and
query correctness, not successful geology. The audit utility belongs to a
separate worker (`4b00812`).

Next visual work would require unequal geological grouping, smaller edge damage
and weathering across the exposed faces, and a less repetitive detachment
primitive. Adding more identical holes or stronger erosion to the old heightfield
is not an approved next step. The existing live maelstrom remains unchanged.
