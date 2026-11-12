# Publishes the current screenshots for remote review: JPEG copies in taskboard/review/<Label>/ plus a contact sheet.
# usage: powershell -File Tools/review.ps1 -Label maelstrom-01 [-Columns 3]
param([string]$Label = "review", [int]$Columns = 3)

Add-Type -AssemblyName System.Drawing
$src = Join-Path $PSScriptRoot "..\Saved\Screenshots\Auto"
$dst = Join-Path $PSScriptRoot "..\taskboard\review\$Label"
New-Item -ItemType Directory -Force $dst | Out-Null

$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$params = New-Object System.Drawing.Imaging.EncoderParameters 1
$params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), 88L

$files = Get-ChildItem $src -Filter *.png | Sort-Object Name
foreach ($f in $files) {
    $img = [System.Drawing.Image]::FromFile($f.FullName)
    $img.Save((Join-Path $dst ($f.BaseName + ".jpg")), $codec, $params)
    $img.Dispose()
}

# Contact sheet of the same set.
$tileW = 640; $tileH = 360
$rows = [int][math]::Ceiling($files.Count / $Columns)
$sheet = New-Object System.Drawing.Bitmap ($Columns * $tileW), ($rows * $tileH)
$g = [System.Drawing.Graphics]::FromImage($sheet)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$i = 0
foreach ($f in $files) {
    $img = [System.Drawing.Image]::FromFile($f.FullName)
    $g.DrawImage($img, ($i % $Columns) * $tileW, [int][math]::Floor($i / $Columns) * $tileH, $tileW, $tileH)
    $img.Dispose(); $i++
}
$g.Dispose()
$sheet.Save((Join-Path $dst "_contact.jpg"), $codec, $params)
$sheet.Dispose()
Write-Output "review set '$Label': $($files.Count) screenshots and a contact sheet in taskboard/review/$Label"
