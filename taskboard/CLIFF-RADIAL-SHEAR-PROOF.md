# Radial-shear crop: stronger relief, appearance still rejected

15 November, separately authorized after the two rejected weathered-field trials
(`a5ad314`). New `Tools/probe_cliff_radial_shear.py` only; no full-sector generation,
runtime, material, UE or asset import. This is one 72m crop proof, then stop.

Mapping in original tangent/outward/up coordinates:
`U'=U; V'=V+G(U,V)*F(U,Z); Z'=Z`.

F uses the same existing Blender Manhattan Voronoi family, now strictly independent
of V: rotated U/Z coordinates,14/8m plus5.5/3.5m scales,0.7m warp at42m,0.12m small
noise, clamped to[-2,2]m. There are no V-dependent slope/hardness/erodibility
multipliers. G is smoothstep of Euclidean XY distance to source-protected vertices
over3..15m, with no crop-edge override. Distance is1-Lipschitz and smoothstep's
maximum derivative is1.5/12=.125, so the continuous determinant
`1+F*dG/dV >= .75`. This continuous bound is **not** a discrete mesh certificate.

Actual crop has19,481 vertices/38,496 triangles. U/Z change<=1.60e-13m; scalar
offset range[-1.423,+2]m. Actual source-normal relief median/P95/maximum is
0.311/0.978/1.662m, roughly twice the upward-biased candidate. There are136 XY
projection reversals, zero degenerates and zero relative3D normal reversals.
Projected reversals alone do not prove visible overhangs or self-intersection.
All crop guards happen to equal1: zero fixed vertices means this crop does not
empirically prove support preservation; that remains a formula/source-domain
contract. Original source correspondence and transported masks remain attached.

Actual matched clay images still show puffy ridges, mottling and repeating
diagonal/horizontal marks. They do not establish convincing broken rock planes.
Root, generator worker and independent reviewer reject the appearance. The
mapping can be useful technically, but this field is not accepted for a full
sector or import. No further shape trial is authorized by this note.

## Reproduce

```powershell
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python-exit-code 1 --python Tools/probe_cliff_radial_shear.py -- `
  --input Saved/WeatheredCliff/faceted/crop_baseline.npz `
  --output-dir Saved/WeatheredCliff/radial_shear --seed 81517
```

Use a new output directory. Input comes from the immutable v3 massif and
`generate_weathered_cliff_mesh.py --relief-mode faceted`; see
[preceding handoff](WEATHERED-CLIFF-MESH.md). The probe verifies the original
massif hash stored in that mesh before generation. Original geometry/frame/masks
are kept; no XY re-projection is used after deformation.

Artifacts beneath `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/WeatheredCliff/radial_shear/`:

- `baseline.npz`: `96df07c4c8c69de0d6b4cc7c819f5176b56ed2f74c1098cf0754cc1a3c0c1ca1`
- `candidate.npz`: `efe9c799778c9da5925fe9b5c4ec97323f0890e61aa1e7a3f0572aeb6a185c58`
- `radial_shear.json`: exact function/guard definitions, physical frame, source
  paths/hashes, geometry metrics and explicit unverified discrete claims.
- `generator_source.py`: exact executed source; final tracked source replaces a
  deprecated NumPy2D cross call with equivalent scalar arithmetic. Actual replay
  under `radial_shear_replay/` matches every numeric array exactly.

Actual review:
`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/WeatheredReview/RadialShearClose/close_wall_fractured.png`
and `close_oblique_fractured.png`, with exact previous close camera/clay/normals.
The independent original-triangle auditor owns any further ray/contact report;
the generator deliberately leaves overhang/self-intersection verification false.
