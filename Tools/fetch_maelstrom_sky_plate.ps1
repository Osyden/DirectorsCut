$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$folder = Join-Path $projectRoot 'Saved/SkyAssetReview'
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$target = Join-Path $folder 'quarry_cloudy_8k.exr'
$expected = 'DE7277FD2A08CF2DA23F66749A68BFD1'
if (!(Test-Path -LiteralPath $target) -or (Get-FileHash -LiteralPath $target -Algorithm MD5).Hash -ne $expected) {
    Invoke-WebRequest -Uri 'https://dl.polyhaven.org/file/ph-assets/HDRIs/exr/8k/quarry_cloudy_8k.exr' -OutFile $target
}
if ((Get-FileHash -LiteralPath $target -Algorithm MD5).Hash -ne $expected) { throw 'Sky HDRI checksum mismatch' }
Write-Output 'Verified Quarry Cloudy 8K EXR (CC0, Poly Haven). Run make_maelstrom_sky_plate.py in Unreal.'
