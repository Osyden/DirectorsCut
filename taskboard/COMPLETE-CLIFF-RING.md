# Complete connected cliff ring

Owner: Codex cliff-design worker, `codex/cliff-scan-demo`. The full circumference replaces the isolated pilot; water, runtime and material code remain owned by root and the waterfall worker.

`Tools/generate_complete_cliff_ring.py` reads the accepted Faceted source's preserved canonical ring. It builds one continuous 360-degree surface with unequal support platforms, broad connected wall/apron massing and scan-derived planar faces. The scan field is evaluated continuously around a cylinder; no 500-metre sector is copied around the ring. This is authored geometry, not a physical erosion simulation.

Final source: `Saved/CompleteCliffRing/FirstJoined/candidate.npz`, SHA-256 `7af36bbdd621ef5dcf1e220cf8419703295609d51978a4ce01891c3360cd239c`.

Final native bundle: `Saved/CompleteCliffRing/NativeJoined/complete_cliff_ring_joined.fbx`, matching JSON/NPZ and `original_mesh_reference.json.gz`. The earlier `First` / `Native` outputs are superseded by a front-to-plateau join correction and remain immutable.

Reproduction from this worker checkout:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_complete_cliff_ring.py --source Saved/ContinuousScanCliff/Faceted/full_ring_candidate.npz --tile E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/OwnedCliffProjection/vfmjadufa/UniquePilot/tile.npz --output Saved/CompleteCliffRing/FirstJoined
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 --python-exit-code 1 --python Tools/export_cliff_surface_fbx.py -- --input Saved/CompleteCliffRing/FirstJoined/candidate.npz --output Saved/CompleteCliffRing/NativeJoined/complete_cliff_ring_joined.fbx --normal-angle-deg 60
```

Use new output directories for replay. Geometry has 863,513 vertices and 1,720,320 triangles; its structured source is 281×3073 with crest row 248. Arena coordinates are metres with identity basis; the existing export origin is retained. Every angular column is rebuilt. All vertex R values are zero, selecting the full-context material rather than the former hero atlas.

The original toe rows 0–8 and outer boundary are preserved. The source already had a submillimetre discrepancy between its duplicate angular endpoints: closing that seam changes the final duplicate toe by at most 0.191 mm and outer edge by 0.378 mm. All other canonical toe vertices are unchanged. The new seam is exactly coincident, with split UV endpoints. Native float32 export has zero collapsed triangles; minimum doubled area is 0.531 m².

UVs retain the existing global context convention and lie inside its configured bounds. Material bakers must select the full UV domain, not the old `vertexR>0` pilot filter. Old waterfalls require actual-source fitting; temples require the current arbitrary-triangle support query. No full dynamic route validation or visual-quality acceptance is claimed. Root owns native import and review.
