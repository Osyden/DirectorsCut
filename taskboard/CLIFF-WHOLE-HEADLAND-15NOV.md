# Whole headland: bounded native comparison

Owned by cliff_design_audit. No runtime, cloud, water, build or import changes.

The H2 native frame exposed the central problem: a small rock patch could not
replace the rest of the soft 500 m wall. The new generator replaces the full
source-8 pilot, canonical columns 537–601. One common bedrock mass supports
222 unequal primary joint-plane volumes and 57 smaller shoulder volumes.
Selective 0.6–1.8 m arris truncations and finite corner fractures preserve broad
quiet faces. It is authored 3D geometry, not simulated erosion or copied props.

The actual CPU D and F renders show a useful change from clay to connected
angular rock. They remain too evenly covered in clean planar slabs, and the
back plateau and side joins need native review. This is **not final art
acceptance**. Root approved the bounded native comparison; no further shape
changes are included after F.

## Frozen deliverables

All paths below are relative to this worktree's `Saved` directory.

- `StructuralWholeFace/JointedFExterior/full_ring_candidate.npz`
  SHA256 `a3ec491c5faf3fd3fdbdf7f907cce21b802535a47bdfaab264664e08d3d2e826`.
- `JointedHeadlandNative/jointed_headland.fbx`, plus `.json` and `.npz`.
  FBX SHA256 `42ba5fa822e2b5ebb8d8e006bc1ad494748d2bcb1d47cfb28731c148dc99fafb`.
- `JointedHeadlandNative/original_mesh_reference.json.gz` for the existing
  original-source import audit. Full ring: 251,775 vertices, 500,168 triangles.
- Export frame is identity basis with origin `[-1083,1147,-25]` arena metres.
  Use the sidecar's existing FBX reflection convention; do not place it at zero.
- `StructuralWholeFace/JointedFReview/{wall,oblique,close_wall}_fractured.png`
  are actual CPU triangle renders, 35° normals. The final exterior omits only
  invisible buried shells, so its visible geometry matches these frames.

## Geometry and contact evidence

The original shoreline collar, first eight radial rows and complete 448-edge
outer perimeter remain exact. The folded historical collar is kept outside
the Boolean operand; a fresh monotone interior core gets the solid backing.
This fixes the actual source-collar self-intersections from the earlier flat
vertical closure, instead of weakening import tolerances.

`JointedFExterior/final_geometry_check.json`: one connected open exterior,
29,128 triangles; no degenerate/duplicate faces, nonmanifold edges or inconsistent
winding. All 448 original boundary edges match. The closed Boolean core also
has zero boundary/nonmanifold/degenerate errors. Exported float32 centimetre
coordinates have zero collapsed triangles.

Two isolated closed internal shells (44 and 16 faces, total 0.1345257 m³) were
omitted only after every shell vertex had outward-facing exits in all six axis
directions against the original closed main solid. Minimum exit distances were
3.713 m and 2.252 m. `closed_diagnostic.npz` preserves the pre-omission source;
the manifest records this limited containment proof. No exterior geometry was
removed by face area or size thresholds.

`JointedFExteriorContact/report.json` samples 14,783 vertices and 29,128 triangle
centroids against the current production water and full-steer route functions.
Minimum dry route margin: **27.4951 m**; all samples including submerged toe:
**26.8978 m**. Elevated rock overhangs extend up to 7.5049 m waterward of the
mean-water boundary. These are static samples, not continuous collision or
animated-wave validation. The position-functions hash remains
`b0349251bec80a6ffa3677afcb742dc73eddb456d63dc820a0a73c32f2b3a524`.

## Integration

The original ring arrays remain in the NPZ for provenance only. All new
interior contacts must use actual triangles. Old 550/552/554 outlet positions
are no longer crests; the waterfall worker has the frozen source hash and is
refitting unequal supported outlets. The new plateau is roughly 40–55 m with
some rock tops near 64 m. Root's generic actual-triangle support sampler must
reposition temple bases; no old interior support islands are frozen.

## Reproduction

Run CPU Blender with at most eight threads:

```powershell
& E:/Utilities/Blender/blender.exe --background --factory-startup --threads 8 --python-exit-code 1 --python Tools/generate_jointed_headland.py -- --input Saved/CompressedEscarpment/RingPilotB/candidate.npz --output Saved/StructuralWholeFace/Reproduction
```

The source input SHA is
`18e7b77ce3540d52ebb459f40571e2980a6632dcba84073d6f6de502e2112046`.
Existing `generate_rock_buttress.py` supplies full-ring splicing and the import
precision gate; `generate_fractured_cliff_outcrop.py` supplies Blender helpers.
The exact artifact-generating script snapshot is retained in
`JointedFExterior/Source`. The committed generator subsequently inlines its
unchanged ten-line canonical sampler and corrects the primary-count metadata:
249 sites were considered; 222 primary volumes survived the boundary guard.
Those two source cleanups change no geometry decisions. Rejected A/B radial
face graphs and their tool are retained only under ignored `Saved` evidence.
