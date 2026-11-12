# Tiles every screenshot in Saved/Screenshots/Auto into one contact sheet (Saved/Screenshots/contact.png).
# usage: powershell -File Tools/contact.ps1 [-Columns 4] [-TileWidth 480]
param([int]$Columns = 4, [int]$TileWidth = 480)

Add-Type -AssemblyName System.Drawing
$dir = Join-Path $PSScriptRoot "..\Saved\Screenshots\Auto"
$files = Get-ChildItem $dir -Filter *.png | Sort-Object Name
if ($files.Count -eq 0) { Write-Output "no screenshots"; exit 1 }

$tileHeight = [int]($TileWidth * 9 / 16)
$rows = [int][math]::Ceiling($files.Count / $Columns)
$sheet = New-Object System.Drawing.Bitmap ($Columns * $TileWidth), ($rows * $tileHeight)
$g = [System.Drawing.Graphics]::FromImage($sheet)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$i = 0
foreach ($f in $files) {
    $img = [System.Drawing.Image]::FromFile($f.FullName)
    $x = ($i % $Columns) * $TileWidth
    $y = [int][math]::Floor($i / $Columns) * $tileHeight
    $g.DrawImage($img, $x, $y, $TileWidth, $tileHeight)
    $img.Dispose()
    $i++
}
$g.Dispose()
$out = Join-Path $PSScriptRoot "..\Saved\Screenshots\contact.png"
$sheet.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$sheet.Dispose()
Write-Output "contact sheet: $($files.Count) tiles"
