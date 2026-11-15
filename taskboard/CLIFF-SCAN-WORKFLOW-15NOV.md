# Physical rock scan and isolated material workflow

Branch `codex/maelstrom-saturday`. This adds reproducible source acquisition,
linear height decoding and an isolated Unreal material. It does not replace
normal-game cliffs or establish final visual quality.

The selected source is [Marble Cliff 01](https://polyhaven.com/a/marble_cliff_01)
by Amal Kumar, supplied by Poly Haven under [CC0](https://polyhaven.com/license).
The reported scan footprint is 4.299351692 m square. Its source Blender material
uses displacement scale 0.600000024 and midlevel 0.524999976. These are the
source author's material settings, not an independently calibrated survey.
The scan is not enlarged to generate hundreds-of-metres rock features.

`Tools/fetch_cliff_scan.py` downloads the selected material and its maps from
the public API into ignored Saved storage. Each file is checked against the
API MD5 and recorded with SHA256. Different existing source files are not
silently replaced. `Tools/extract_cliff_scan.py` loads only material datablocks
in background Blender with auto-execution disabled and decodes the height as
linear data. The raw height array is bottom-to-top, matching Blender image
pixels. Its current SHA256 is
`b1f91b981d6b4ab1e4f9584f2ffdf061c0888c67c5e9729d43cb41229ce7f7ef`.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/fetch_cliff_scan.py --asset marble_cliff_01 --output Saved/CliffSources/PolyHaven/marble_cliff_01
& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --disable-autoexec --python Tools/extract_cliff_scan.py -- --manifest Saved/CliffSources/PolyHaven/marble_cliff_01/source_manifest.json
```

For Unreal, run `Tools/import_cliff_scan_textures.py` in the Python commandlet
with `DC_CLIFF_SCAN_MANIFEST` naming that source manifest. Assets go exclusively
under `/Game/DC/Maelstrom/GeneratedCliffPreview/Scan`. Albedo is sRGB; roughness
is linear; the GL normal is imported with normal compression and green-channel
flip. The generated `Saved/CliffScanImport.json` supplies references to
`Tools/make_cliff_preview_material.py` with `DC_CLIFF_STONE=scan`.

Exposed faces sample the original chart U/Z at the source footprint. The native
shader accounts for the imported Y reflection and image V convention. Upper
terrain blends to a horizontal projection. Roughness comes from the real map;
colour has a restrained authored cool tint. These remain registered periodic
samples: visible repetition is unresolved, and a future anti-repeat method
must change geometry and material sampling together. No shader displacement
or emissive lighting is used.

The actual source downloads, linear extraction, Unreal texture import and
material construction ran successfully. NullRHI does not establish a final
GPU shader/render pass. Native graphics review is still required on the next
useful macro geometry. Original scans and generated Unreal assets stay in
ignored authoring/preview directories; only tools and this source note are
committed.

Actual clay and registered material reviews of the first displaced cut remain
visually rejected. Native-scale surface relief cannot repair the underlying
planar excavation. The stronger second cut creates capped slots because its
original upper seam is frozen. Continue with the existing eroded parent shaped
into an escarpment; do not spend another pass increasing scan amplitude or
adding material detail to the rejected plane.
