# Measures how far the picture moved between consecutive screenshots in Saved/Screenshots/Auto
# (best match over small shifts of a downsampled image). Used to check which way the sea streams.
# usage: powershell -File Tools/flow.ps1
Add-Type -AssemblyName System.Drawing
$dir = Join-Path $PSScriptRoot "..\Saved\Screenshots\Auto"
$files = Get-ChildItem $dir -Filter *.png | Sort-Object Name
$W = 96; $H = 54
function Load($path) {
    $img = [System.Drawing.Image]::FromFile($path)
    $small = New-Object System.Drawing.Bitmap $W, $H
    $g = [System.Drawing.Graphics]::FromImage($small)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBilinear
    $g.DrawImage($img, 0, 0, $W, $H); $g.Dispose(); $img.Dispose()
    $a = New-Object 'double[,]' $W, $H
    for ($x = 0; $x -lt $W; $x++) { for ($y = 0; $y -lt $H; $y++) { $c = $small.GetPixel($x, $y); $a[$x, $y] = 0.3 * $c.R + 0.6 * $c.G + 0.1 * $c.B } }
    $small.Dispose()
    return ,$a
}
$prev = $null
foreach ($f in $files) {
    $cur = Load $f.FullName
    if ($prev -ne $null) {
        $best = 1e30; $bx = 0; $by = 0
        for ($dx = -10; $dx -le 10; $dx++) { for ($dy = -10; $dy -le 10; $dy++) {
            $sum = 0.0; $n = 0
            for ($x = 12; $x -lt $W - 12; $x += 2) { for ($y = 12; $y -lt $H - 12; $y += 2) {
                $d = $cur[($x + $dx), ($y + $dy)] - $prev[$x, $y]; $sum += $d * $d; $n++ } }
            $e = $sum / $n
            if ($e -lt $best) { $best = $e; $bx = $dx; $by = $dy }
        } }
        Write-Output ("{0}: moved {1} right, {2} down (of {3}x{4})" -f $f.Name, $bx, $by, $W, $H)
    }
    $prev = $cur
}
