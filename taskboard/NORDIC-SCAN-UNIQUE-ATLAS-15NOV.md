# Unique Nordic scan atlas — native review pending

Owner: cliff material worker, `codex/cliff-bedding-resolved`.

The first native continuous-cliff preview was rejected: a 7.9875 × 5.5125 m tile with roughly 5.5 m depth range repeated across the pilot as a waffle. Its constant-color material fallback also left a bare roof.

The replacement is one 4096 × 2048 atlas covering 497.418836818384 m of arc and Z −200..80 m. Albedo, roughness, fine normals and depth use the same three-way stochastic source sampling, bounded ±14.9° orientation and smooth coordinate warp. Source features retain their physical size; the depth multiplier is explicitly reduced to 0.19, giving −0.59645..+0.45303 m. This supplies subordinate scanned relief, not the whole cliff's macro geology.

The geometry worker owns the matching unique UV0, broad shoulders and a radial UV continuation across the roof. The importer uses the supplied UV0 unchanged; actual scan albedo/roughness cover the roof and outside pilot too. Vertex R only attenuates fine normals. The optional `M_CliffScanSurface` is selected by `-DCCliffPreviewScan`; normal defaults remain separate.

## Reproduce

Run with the installed Blender Python (NumPy; no Blender or GPU process needed):

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' 'Tools/bake_unique_owned_cliff.py' --source 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/OwnedCliffProjection/vfmjadufa/Tile' --output 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/OwnedCliffProjection/vfmjadufa/UniquePilotReplay'
```

The measured input tile is an explicit ignored dependency, SHA256 `3c98018f3b8cf02e5e51e897ee7fe927c1d8ac6209e12621e249fe16c010c714` for `tile.npz`. It was ray-projected from owned Massive Nordic Coastal Cliff `vfmjadufa`, X −7.2..3.45 m/Z 3..10.35 m, using actual mesh UVs and original 8K maps. A 25% periodic overlap was applied jointly to the original small tile. The original projection recipes remain in `Saved/project_owned_cliff_faces.py` and `Saved/bake_owned_cliff_tile.py`; this new CLI does not silently regenerate that measured input from a different source.

Original unique output: `Saved/OwnedCliffProjection/vfmjadufa/UniquePilot/`. The manifest records source paths/hashes, physical dimensions and amplitudes. Derived TGA files use bottom-left origin, +V toward source +Z. Depth texels have the same centres. The full-pilot geometry samples the finite field; there is no geometry-side repeating 8 m UV lookup.

Root runs `Tools/import_owned_cliff_atlas.py` with `DC_OWNED_CLIFF_ATLAS` set to that directory. It checks map hashes and imports GL normals with green inversion. Texture inputs default to source UV0, avoiding the UE5.8 `Coordinates` pin issue. Scalar parameters use `set_editor_property`. All vendor/derived assets remain ignored; only source and this note are committed.

The portable CLI replay completed in 22.2 seconds: all three TGA files were byte-identical to the original delivered candidate, and the complete depth array was bitwise identical. Container metadata differs by explicit generator hash/output provenance.

The fine normal atlas currently transfers original texture perturbation, not every geometric frequency removed by the geometry worker's low-pass. Native visual acceptance is pending; successful baking and correspondence are not an art-quality claim.
