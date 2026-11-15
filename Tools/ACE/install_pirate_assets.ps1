param([Parameter(Mandatory=$true)][string]$TargetProject, [switch]$UpdateVerifiedAssets)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$targetRoot = (Resolve-Path $TargetProject).Path
if (!(Test-Path (Join-Path $targetRoot 'DirectorsCut.uproject'))) { throw 'Target must be a DirectorsCut project directory.' }
if ($sourceRoot -eq $targetRoot) { throw 'Source and target must differ.' }
$report = Get-Content (Join-Path $sourceRoot 'Saved/PirateCast/validated-cast.json') -Raw | ConvertFrom-Json
if (@($report).Count -ne 4 -or @($report | Where-Object { @($_.missing_packages).Count -gt 0 }).Count -gt 0) { throw 'Four validated characters are required.' }
$manifest = @()
$previous = @{}
$previousPath = Join-Path $targetRoot 'Saved/PirateCast/installed-assets.json'
if (Test-Path -LiteralPath $previousPath) {
    foreach ($entry in (Get-Content -LiteralPath $previousPath -Raw | ConvertFrom-Json)) { $previous[$entry.Path] = $entry.SHA256 }
}
foreach ($folder in @('PirateCast', 'PirateCastVendor')) {
    $base = Join-Path $sourceRoot "Content/$folder"
    foreach ($file in Get-ChildItem -LiteralPath $base -File -Recurse) {
        $relative = [System.IO.Path]::GetRelativePath($sourceRoot, $file.FullName)
        $destination = Join-Path $targetRoot $relative
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        if (Test-Path -LiteralPath $destination) {
            $targetHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
            if ($targetHash -ne $hash) {
                if (!$UpdateVerifiedAssets -or $previous[$relative] -ne $targetHash) { throw "Target asset differs; refusing to overwrite: $relative" }
                $backup = Join-Path $targetRoot ("Saved/PirateCast/AssetBackups/$targetHash/" + $relative)
                New-Item -ItemType Directory -Force -Path (Split-Path $backup) | Out-Null
                Copy-Item -LiteralPath $destination -Destination $backup
                Copy-Item -LiteralPath $file.FullName -Destination $destination
            }
        } else {
            New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination
        }
        if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $hash) { throw "Copy verification failed: $relative" }
        $manifest += [PSCustomObject]@{ Path=$relative; Bytes=$file.Length; SHA256=$hash }
    }
}
$evidence = Join-Path $targetRoot 'Saved/PirateCast'
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'installed-assets.json') -Encoding utf8
"Verified $($manifest.Count) purchased/generated assets in $targetRoot. Keep Content/PirateCast and Content/PirateCastVendor ignored by Git."
