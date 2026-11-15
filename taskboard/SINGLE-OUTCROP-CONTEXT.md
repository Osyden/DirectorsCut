# One original3D outcrop in the unchanged full massif

Owner: Codex `cliff_material`; helper `Tools/compose_cliff_context_patch.py`.
This is a **clay context proof only**. It does not expand/regenerate geology across the500 m sector, change the macro silhouette outside one crop, or integrate anything into Unreal.

## Construction and measured result

Input source is the immutable `Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`.
Patch is the approved-for-context original open surface `Saved/ImplicitFaceProof/JointBoundaryUnion/visible_candidate.npz`, SHA256 `b889d3a18a74dcf0c7b2b5b4d9bb271c7907a16ae795ae571bff979fe6c48c4c`. Its one open component and absence of internal cavity shells were independently confirmed before this task.

The helper obtains the source's original unrefined mesh through `generate_weathered_cliff_mesh.original_mesh`, removes only the exact72 ×40 m crop at row89/column64, then joins the existing patch:

- 1,440 original source triangles removed, matching the grid-aligned crop exactly.
- Only112 immediately adjacent outside triangles split. Their existing edges gain336 interpolated points to match the patch's448 boundary vertices at0.5 m spacing. No full-mesh subdivision occurs.
- Joining uses the actual original3D source edges, not XY-only snapping. Maximum adjustment is **1.8169994e-6 m**, below the1e-5 m tolerance. Interior patch positions/triangles remain original3D, including overhangs.
- Final context: **66,598 vertices /132,214 triangles**. The full source's992 external boundary edges remain; no crop seam boundary, nonmanifold edge, inconsistent winding or degenerate triangle is detected.
- All retained original vertex positions and normalized source mask values are bitwise unchanged. New seam masks interpolate original edge values. New patch-interior RGB is explicitly0, alpha1: **diagnostic missing data**, not invented flow, sediment or erosion. The helper does not authorize using those diagnostic zeros as final shading masks.

The output retains original-source vertex/face indices, patch vertex indices and seam indices for independent correspondence checks. Split outside triangles remain in their original planar triangles; every other outside face is retained verbatim. The source's full frame is shared by baseline/context outputs; this is not a scaled-up outcrop.

## Reproduce

From `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/compose_cliff_context_patch.py --source Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz --patch Saved/ImplicitFaceProof/JointBoundaryUnion/visible_candidate.npz --output Saved/ImplicitFaceProof/FullMassifContextReplay
```

Actual outputs are `Saved/ImplicitFaceProof/FullMassifContext/baseline.npz`, `context.npz` and `context.json`. New output paths are required. Only Python/NumPy and existing source helpers ran; no Blender render, GPU, engine or asset import occurred in this task.

The source/masks and conforming seam checks pass. Independent audit and the physical full-sector clay review are separate gates. This places one locally fractured exposure in its original macro context; it is not a claim that the surrounding sector or the new rock has reached final visual quality.
