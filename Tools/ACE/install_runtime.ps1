param([Parameter(Mandatory=$true)][string]$TargetProject)
$ErrorActionPreference='Stop'
$source=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$target=(Resolve-Path -LiteralPath $TargetProject).Path
if ($source -eq $target -or !(Test-Path (Join-Path $target 'DirectorsCut.uproject'))) { throw 'Choose a different DirectorsCut project.' }
$manifest=@()
function Copy-Verified([string]$Relative) {
    $from=Join-Path $source $Relative
    $to=Join-Path $target $Relative
    $hash=(Get-FileHash -LiteralPath $from -Algorithm SHA256).Hash
    if (Test-Path -LiteralPath $to) {
        if ((Get-FileHash -LiteralPath $to -Algorithm SHA256).Hash -ne $hash) { throw "Refusing to overwrite a different existing file: $Relative" }
    } else {
        New-Item -ItemType Directory -Force -Path (Split-Path $to) | Out-Null
        Copy-Item -LiteralPath $from -Destination $to
    }
    if ((Get-FileHash -LiteralPath $to -Algorithm SHA256).Hash -ne $hash) { throw "Copy verification failed: $Relative" }
    $script:manifest += [pscustomobject]@{Path=$Relative; SHA256=$hash}
}
foreach ($plugin in @('NV_ACE_Reference','NvAudio2FaceMarkRegressive')) {
    $base=Join-Path $source "Plugins/$plugin"
    foreach ($file in Get-ChildItem -LiteralPath $base -File -Recurse) {
        $relative=[IO.Path]::GetRelativePath($source,$file.FullName)
        if ($relative -match '^Plugins[\\/][^\\/]+[\\/](Binaries|Intermediate)[\\/]') { continue }
        Copy-Verified $relative
    }
}
foreach ($package in (Get-Content (Join-Path $source 'Saved/ACE/face-dependencies.json') -Raw | ConvertFrom-Json)) {
    if (!$package.StartsWith('/Game/MetaHumans/')) { throw "Unexpected dependency scope: $package" }
    Copy-Verified ('Content/' + $package.Substring(6) + '.uasset')
}
$saved=Join-Path $target 'Saved/ACE'
New-Item -ItemType Directory -Force -Path $saved | Out-Null
$manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $saved 'installed-runtime.json')
Write-Output "Verified $($manifest.Count) local runtime files; keep vendor plugin/model/MetaHuman assets ignored by Git."
