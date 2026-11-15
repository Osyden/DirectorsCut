# Isolated cliff import source audit

Worker branch: `codex/cliff-import-audit-pass7`. Scope: new read-only audit tool; no importer, runtime, asset or map changes.

`Tools/audit_cliff_preview_import.py` prepares a hashed reference from the existing FBX JSON/NPZ authoring files, then runs inside a parent-owned Unreal Python commandlet with `-EnablePlugins=GeometryScripting`. `DC_CLIFF_AUDIT_SOURCE` names the generated gzip. Default report: `Saved/CliffPreviewSourceAudit.json`. It reads explicit SourceModel LOD0 into a transient DynamicMesh with build settings and build scale disabled. Missing source data fails; it never substitutes a Nanite fallback. It compares every position and triangle and all triangle-corner RGBA, without averaging masks.

## Actual parent-run evidence

On the original `source08_ridged4_detail_open256.fbx` (SHA256 `143c4a6b78e87c2ca0bab1cfccafc1f054997490662eeb996d83135b7d90ec57`) and `/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector`:

- All **52,536 vertices, 104,078 triangles and 312,234 corner colors** are accounted for.
- Maximum position residual: **0.00244140625cm**. Triangle connectivity matches.
- Actual signed mapping is **imported cm = 100 × (author X, −author Y, author Z)**. It was established from asymmetric vertex positions and checked over the whole mesh, not inferred from absolute bounds.
- Arena reconstruction uses the sidecar origin in cm, **+basisU for imported X, −basisV for imported Y, +Z for imported Z**. The audit writes that exact basis and four physical anchors. This includes a reflection; an actor rotation alone cannot replace the signed transform.
- Maximum numeric RGBA differences are `[0.003921508789, 0.003921192139, 0.003921538591, 0]`, within one truncated byte. All masks stay attached to the correct physical vertices. No exporter color-space change is warranted.

## Why the source color path initially looked suspicious

Installed UE5.8 legacy import code (`FbxStaticMeshImport.cpp:944–952`) truncates FBX colors to `FColor`, then constructs `FLinearColor(FColor)`, decoding RGB from sRGB. Reading that step alone suggests masks were darkened. The complete path reverses it: `FMeshDescriptionToDynamicMesh::bTransformVertexColorsLinearToSRGB` defaults true (`MeshDescriptionToDynamicMesh.h:43`), and its implementation explicitly converts back (`.cpp:1113–1127`). GeometryScript returns numeric byte-normalized masks. The corrected audit expects that complete route and retains the decoded-storage alternative only as a diagnostic.

The render build separately re-encodes using `ToFColor(true)` (`StaticMeshBuilder.cpp:1708`). Nanite uses normalized byte values through `Cluster.cpp:90`, `NaniteAttributeDecode.ush:848`, and `NaniteVertexFactory.ush:841`. This source trace supports the chosen numeric masks; it is not a measurement of final GPU material sampling or Nanite LOD interpolation.

## Verification and limits

Offline full-source mirror/float32-unit conversion test passed. Deliberate 1cm vertex movement and changed triangle connectivity were rejected. Parent ran the actual UE commandlet successfully. Generated source payload and reports remain under ignored `Saved`.

This validates authoring geometry, signed frame and numeric masks for an isolated preview. It does not validate runtime cliff stitching, temple support, waterfalls, GPU appearance, or distant Nanite color interpolation. The terrain remains preview-only.
