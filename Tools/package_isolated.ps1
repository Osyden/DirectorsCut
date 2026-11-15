# Package an isolated, built project without overwriting earlier release candidates.
param(
    [Parameter(Mandatory=$true)][string]$ProjectPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [Parameter(Mandatory=$true)][string]$SidecarsRoot,
    [string]$EngineRoot = 'E:\Utilities\UE_5.8'
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectPath).Path
$projectRoot = Split-Path -Parent $project
& (Join-Path $PSScriptRoot 'verify_release_content.ps1') -ProjectRoot $projectRoot -EngineRoot $EngineRoot
$output = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $output) { throw "Output already exists; choose a new candidate folder: $output" }
foreach ($part in @('bin','models')) {
    if (!(Test-Path -LiteralPath (Join-Path $SidecarsRoot $part) -PathType Container)) { throw "Missing sidecars: $part" }
}
$running = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -in @('UnrealEditor.exe','UnrealEditor-Cmd.exe') -and $_.CommandLine -and $_.CommandLine.Contains($project)
}
if ($running) { throw 'The selected runtime is busy. Finish its playtest/cook first.' }
& (Join-Path $PSScriptRoot 'patch_drw_legacy_calls.ps1') -EngineRoot $EngineRoot
function Get-SourceManifest {
    $files = @($project)
    foreach ($area in @('Source','Config')) {
        $files += Get-ChildItem -LiteralPath (Join-Path $projectRoot $area) -Recurse -File | Select-Object -ExpandProperty FullName
    }
    @($files | Sort-Object | ForEach-Object {
        [pscustomobject]@{ Path=$_.Substring($projectRoot.Length+1); SHA256=(Get-FileHash -LiteralPath $_).Hash }
    })
}
$manifest = Get-SourceManifest
New-Item -ItemType Directory -Path $output | Out-Null
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'source-manifest.json')
$log = Join-Path $output 'package.log'
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$uatArgs = @('BuildCookRun',"-project=$project",'-platform=Win64','-clientconfig=Development',
    '-build','-ubtargs=-WaitMutex','-cook','-stage','-pak','-archive',"-archivedirectory=$output",'-nop4','-utf8output','-unattended',
    # UE 5.8 TryGetUsedTexturesInCookCache indexes past the cached texture table
    # when stripping M_DeckFire. Retain default textures during cooking instead.
    '-AdditionalCookerOptions=-NullRHI -ini:Engine:[SystemSettings]:r.Material.StripUnusedDefaultTextures=0')
Write-Output "Packaging started; log: $log"
& $uat @uatArgs *> $log
$code = $LASTEXITCODE
if ($code -ne 0) { throw "Packaging failed with exit $code; inspect $log" }
if (Select-String -LiteralPath $log -Pattern 'Failed to compile Material|doesn.t have a valid ShaderMap' -Quiet) {
    throw "Cooked material compilation failed; reject this candidate and inspect $log"
}
if (($manifest | ConvertTo-Json -Compress) -ne ((Get-SourceManifest) | ConvertTo-Json -Compress)) {
    throw 'Source/config changed during packaging; do not accept this candidate.'
}
$gameRoot = Join-Path $output 'Windows\DirectorsCut'
$bootstrap = Join-Path $output 'Windows\DirectorsCut.exe'
$launcher = Join-Path $output 'Windows\Atlantean Odyssey.exe'
$leaf = Join-Path $gameRoot 'Binaries/Win64/Atlantean Odyssey.exe'
if (!(Test-Path -LiteralPath $bootstrap -PathType Leaf) -or !(Test-Path -LiteralPath $leaf -PathType Leaf)) {
    throw 'Packaging did not produce the bootstrap and branded game executable. Inspect the target receipt/output path.'
}
# UAT embeds its receipt-derived leaf path in resource 201. Renaming the
# bootstrap is safe; renaming the leaf afterwards would break that resource.
$bootstrapText = [Text.Encoding]::Unicode.GetString([IO.File]::ReadAllBytes($bootstrap))
if (!$bootstrapText.Contains('DirectorsCut\Binaries\Win64\Atlantean Odyssey.exe')) {
    throw 'Bootstrap does not reference the branded game executable; reject this package.'
}
if ([Diagnostics.FileVersionInfo]::GetVersionInfo($leaf).ProductName -ne 'Atlantean Odyssey') {
    throw 'Game executable has stale Windows product branding; rebuild its resources before packaging.'
}
Move-Item -LiteralPath $bootstrap -Destination $launcher
& (Join-Path $PSScriptRoot 'ACE/stage_sidecars.ps1') -SourceRoot $SidecarsRoot -GameRoot $gameRoot
Write-Output "Packaged candidate: $launcher. Run and inspect it before accepting the build."
