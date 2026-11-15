# App-local VC runtime deployment: no machine-wide installation or admin rights.
param(
    [Parameter(Mandatory=$true)][string]$WindowsRoot,
    [string]$RuntimeDirectory,
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $WindowsRoot).Path
if (!(Test-Path -LiteralPath (Join-Path $root 'DirectorsCut/Binaries/Win64') -PathType Container)) {
    throw 'Expected the packaged Windows root containing DirectorsCut/Binaries/Win64.'
}
if (!$RuntimeDirectory) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Specify the official Visual Studio x64 CRT redistributable directory.' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'No Visual Studio C++ installation found.' }
    $versions = Get-ChildItem -LiteralPath (Join-Path $vs 'VC/Redist/MSVC') -Directory |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
        Sort-Object { [version]$_.Name } -Descending
    foreach ($version in $versions) {
        $candidate = Join-Path $version.FullName 'x64/Microsoft.VC143.CRT'
        if (Test-Path -LiteralPath $candidate -PathType Container) { $RuntimeDirectory = $candidate; break }
    }
}
if (!$RuntimeDirectory) { throw 'Official x64 VC143 CRT redistributable was not found.' }
$runtime = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
if ($runtime -notmatch '[\\/]x64[\\/]Microsoft\.VC143\.CRT$' -or $runtime -match 'debug_nonredist') {
    throw 'Use the release x64/Microsoft.VC143.CRT redistributable, never debug or System32 DLLs.'
}
$dlls = @(Get-ChildItem -LiteralPath $runtime -File -Filter '*.dll')
$openmp = Join-Path (Split-Path -Parent $runtime) 'Microsoft.VC143.OpenMP/vcomp140.dll'
if (!(Test-Path -LiteralPath $openmp -PathType Leaf)) { throw 'Official x64 OpenMP redistributable is required by ACE and Whisper.' }
$dlls += Get-Item -LiteralPath $openmp
foreach ($required in @('msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')) {
    if ($required -notin $dlls.Name) { throw "Incomplete CRT source: $required" }
}
$sourceRecords = @($dlls | ForEach-Object {
    $sig = Get-AuthenticodeSignature -LiteralPath $_.FullName
    if ($sig.Status -ne 'Valid' -or $sig.SignerCertificate.Subject -notmatch 'Microsoft Corporation') {
        throw "Invalid Microsoft runtime signature: $($_.Name)"
    }
    [pscustomobject]@{ Name=$_.Name; Source=$_.FullName; Version=$_.VersionInfo.FileVersion;
        Bytes=$_.Length; SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
})
$executables = @(Get-ChildItem -LiteralPath $root -File -Recurse -Filter '*.exe')
if (!$executables) { throw 'No packaged executables found.' }
$destinations = @($executables.DirectoryName | Sort-Object -Unique)
$running = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $_.ExecutablePath.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)
})
if (!$ValidateOnly -and $running) { throw 'Close processes from this package before staging runtime files.' }
$records = @()
# Preflight all destinations before copying. Preserve existing pinned Phonon files.
foreach ($destination in $destinations) {
    foreach ($dll in $sourceRecords) {
        $target = Join-Path $destination $dll.Name
        if (Test-Path -LiteralPath $target) {
            if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $dll.SHA256) {
                throw "Existing CRT differs from selected redistributable; reconcile the pinned payload first: $target"
            }
        } elseif ($ValidateOnly) { throw "Missing app-local CRT: $target" }
        $records += [pscustomobject]@{ Path=$target.Substring($root.Length+1).Replace('\','/');
            Version=$dll.Version; Bytes=$dll.Bytes; SHA256=$dll.SHA256 }
    }
}
if (!$ValidateOnly) {
    foreach ($destination in $destinations) {
        foreach ($dll in $sourceRecords) {
            $target = Join-Path $destination $dll.Name
            if (!(Test-Path -LiteralPath $target)) { Copy-Item -LiteralPath $dll.Source -Destination $target }
        }
    }
}
foreach ($record in $records) {
    if ((Get-FileHash -LiteralPath (Join-Path $root $record.Path) -Algorithm SHA256).Hash -ne $record.SHA256) {
        throw "Staged CRT hash mismatch: $($record.Path)"
    }
}
if (!$ValidateOnly) {
    $notices = Join-Path $root 'DirectorsCut/ThirdPartyNotices/MicrosoftVCRuntime'
    New-Item -ItemType Directory -Path $notices -Force | Out-Null
    @{
        source='Visual Studio release x64 Microsoft.VC143.CRT and Microsoft.VC143.OpenMP redistributables';
        deployment='App-local beside each executable; no system installation';
        files=$records
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $notices 'app-local-manifest.json') -Encoding UTF8
    @'
Microsoft Visual C++ Runtime (x64)
Copyright Microsoft Corporation. All rights reserved.
Unmodified release DLLs from the licensed Visual Studio redistributable.
These files are governed by the applicable Microsoft software license terms.
Deployment documentation and distributable-code reference:
https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files
https://learn.microsoft.com/visualstudio/releases/2022/redistribution
Windows system libraries and GPU drivers remain operating-system prerequisites.
'@ | Set-Content -LiteralPath (Join-Path $notices 'NOTICE.txt') -Encoding UTF8
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../SYSTEM_REQUIREMENTS.txt') -Destination $root -Force
}
Write-Output "Verified $($records.Count) app-local CRT files across $($destinations.Count) executable directories."
