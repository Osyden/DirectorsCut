# Maelstrom photographic sky

`T_MaelstromStormPlate.uasset` contains the 8K EXR version of **Quarry Cloudy**, photographed by **Dimitrios Savva** and processed by **Jarod Guest**, published by Poly Haven.

- Asset: https://polyhaven.com/a/quarry_cloudy
- License: **CC0 1.0**, https://polyhaven.com/license and https://creativecommons.org/publicdomain/zero/1.0/
- Source: https://dl.polyhaven.org/file/ph-assets/HDRIs/exr/8k/quarry_cloudy_8k.exr
- Source MD5: `de7277fd2a08cf2da23f66749a68bfd1`
- Source size: 383,679,030 bytes; source dimensions: 8192 x 4096.

The source photograph is imported intact. The runtime material excludes terrain below 12 degrees elevation, remaps elevation and azimuth to the scene sun, grades the sky, and derives a light-function mask. These material changes are project work; they do not change the source asset's CC0 status. Only the imported asset is needed by the game. The raw EXR stays in ignored `Saved/SkyAssetReview` and can be fetched again with `Tools/fetch_maelstrom_sky_plate.ps1`.

The older `M_MaelstromCumulus` / `T_MaelstromErosion` assets are an experimental procedural fallback, available explicitly through `-DCMaelProceduralClouds`. Their foam-like cloud-shape pass was rejected and is not the default finale sky.
