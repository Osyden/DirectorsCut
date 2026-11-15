# Unique cliff context atlas

Owner: Codex on `codex/cliff-context-map`, based on `1872b00`. CPU material authoring only; accepted geometry, runtime code, lighting, and the root-owned Unreal session are unchanged.

`Tools/bake_cliff_context.py` makes an 8192 x 2048 unique context atlas covering existing UV0 `[-7,-1.25]` to `[13.3,2.35]`. Sample with `(UV0-min)/(max-min)` and clamp. Use as the outside-pilot/roof layer beneath the separately masked unique pilot. The only periodicity is the complete 10,053 m circumference to preserve its angular seam, with no repeated 497 m panels.

The palette matches the pilot worker's linear base .222 and RGB multipliers [.981,1.008,1.027], with deliberately restrained broad variation, strata, and mineral staining. Roughness centers on .79. The GL normal encodes small authored height gradients and requires one green flip on Unreal import. These masks are authored context, not measured geology, simulated erosion, or actual slope data. This coarse context is not a replacement for close-range stone detail.

Generate with bundled NumPy Python:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/bake_cliff_context.py --output Saved/CliffContext/UniqueRing --width 8192 --height 2048
```

Outputs are ignored: three standard `T_NordicFace_*.tga` maps, `tile_manifest.json` with SHA-256 hashes/UV transform/palette and continuity audit, `context_preview.png`, and `context_pilot_crop.png`. No owned source pixels are used. The full-resolution bake checks full-circumference field continuity and a nonzero 497 m shift difference; native material integration and rendered acceptance remain with root.

Completed full-resolution bake in 23.36 seconds. The actual PNG preview was inspected. Linear RGB range .1860–.2619, roughness .7412–.8275, maximum normal perturbation 1.034 degrees. The complete-ring noise seam error was 1.03e-14 and the 497 m shift RMS difference .6197. `git diff --check` passed and output exclusion was verified. No editor or GPU work was performed.

## V2 after native context review

Root's actual `CliffGeologyFirst/shot_0300.png` showed the subtle context reading as blank chalk. The authorized material-only V2 adds `--fractured`, producing irregular 3–15 m planar fracture normals and interrupted shallow bedding, without randomized cell colours. Linear base .170 matches the revised pilot palette. The source mesh is untouched.

`Saved/CliffContext/UniqueRingV2` retains the standard filenames and UV extent. Its full 8192 x 2048 bake took 60.87 seconds. Actual colour and normal crops were inspected. Normal median 16.80 degrees and maximum 35 degrees; linear RGB .1267–.2085; roughness .7412–.8431. This is a stronger shading candidate, requiring root's native visual review.

The manifest bounds describe original source UVs. Unreal's legacy FBX importer flips V and its TGA loader flips bottom-origin rows. Context sampling must therefore use runtime V minimum `1-sourceMaxV=-1.35` and maximum `1-sourceMinV=2.25`. Root applied this correction. Existing symmetric pilot bounds remain valid. An inherited source-mesh angular seam spans 320 triangles far from the pilot; their interpolated UVs can still streak through the context despite its periodic full-ring endpoints.
