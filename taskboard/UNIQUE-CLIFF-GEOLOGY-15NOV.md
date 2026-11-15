# Unique cliff geology material

Owner: Codex `terrain_texture_strategy`, branch `codex/cliff-bedding-resolved`.

`Tools/bake_cliff_geology.py` replaces repeated scan albedo with one original
497.4188 by 280 metre geological field. It reads the frozen Faceted mesh's
UV-rasterized actual positions, normals, slope, curvature and drainage-start
mask. The mesh remains unchanged. Beds form an ordered irregular sequence with
a broad dip and fold; mineral regions span tens to hundreds of metres. Narrow
broken joints and shallow unique fine normals supply local surface variation.
Runoff follows gravity projected onto the real surface in UV coordinates.
Curvature affects moisture retention; it is not baked as black cavity shading.

Generate with the existing NumPy-capable Python:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/bake_cliff_geology.py `
  --fields 'E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CliffGeology/FacetedFields/fields.npz' `
  --output Saved/CliffGeology/FacetedUnique
```

Outputs are ignored: three 4096 by 2048 TGA roles with the existing importer
filenames, `tile_manifest.json` with source/output hashes and physical mapping,
small diagnostic masks, and a 1500-pixel albedo preview. Albedo is sRGB encoded
from neutral cool gray linear reflectance. Roughness is linear, dry around .79
and the strongest wet runnels around .4. Normals are OpenGL tangent space with
+V-up rows; the importer must perform its existing single green-channel flip.

The flat atlas was inspected. Output file hashes, dimensions and untouched
source geometry are checked at handoff. Root owns Unreal integration and the
actual camera comparison. This is art-directed geology and approximate UV
drainage; it is not a calibrated sedimentary/erosion reconstruction.

This atlas covers the pilot only. The existing full-ring UV0 extends outside
[0,1], including roof vertices above V=1 even where vertex R=1. The importer
must clamp these textures and blend all pilot layers using both vertex R and
an in-domain UV mask. A separately generated unique context atlas supplies the
remaining full-ring material; wrapping this pilot would repeat it.
