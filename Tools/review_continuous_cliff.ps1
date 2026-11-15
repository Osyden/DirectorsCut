# Actual UE captures of the current local continuous-cliff pilot.
# Owned-derived assets must first be imported; this does not install a pack.
param(
    [switch]$PlayerView,
    [switch]$DeckView,
    [ValidateSet('helm','port','starboard')][string]$Station = 'helm',
    [ValidateRange(50,4000)][float]$Distance = 665,
    [ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$ReviewName = 'ContinuousCliff',
    [string]$Shots = '300,250,451',
    [ValidateRange(640,7680)][int]$Width = 1600,
    [ValidateRange(360,4320)][int]$Height = 900
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$saved = Join-Path $projectRoot 'Saved'
foreach ($required in @('CliffLandformPreviewImport.json','OwnedCliffAtlasImport.json',
                        'CliffPreviewSupports.json','CliffPreviewSurface.bin','CliffPreviewSourceAudit.json')) {
    if (!(Test-Path -LiteralPath (Join-Path $saved $required))) {
        throw "Missing local cliff demo data: $required. See taskboard/CONTINUOUS-SCAN-CLIFF-15NOV.md."
    }
}
$import = Get-Content -LiteralPath (Join-Path $saved 'CliffLandformPreviewImport.json') -Raw | ConvertFrom-Json
$supports = Get-Content -LiteralPath (Join-Path $saved 'CliffPreviewSupports.json') -Raw | ConvertFrom-Json
if ($import.source_sha256 -ne $supports.source_fbx_sha256) {
    throw 'Cliff support data belongs to another mesh. Re-run prepare_cliff_preview_surface.py.'
}
$reviewArgs = @('-DCCliffPreview','-DCCliffPreviewScan','-DCCliffPreviewSupports',
                '-DCMaelSunNeutral','-DCMaelSunGroundBalance')
if ($PlayerView -or $DeckView) {
    $reviewArgs += '-DCCliffPreviewPlayerView'
    if ($DeckView) { $reviewArgs += '-DCShotCam=-1100,0,1000,18,80' }
} else {
    $reviewArgs += @(
        ('-DCCliffPreviewDistance={0}' -f $Distance.ToString([Globalization.CultureInfo]::InvariantCulture)),
        '-DCCliffPreviewTargetX=-95.392','-DCCliffPreviewTargetY=42.3076',
        '-DCCliffPreviewTargetZ=-5','-DCCliffPreviewAzimuth=39.624'
    )
}
# Old fitted waterfalls and scan instances are deliberately absent: neither is
# bound to the current source mesh. The demo is a cliff-material/shape review.
& (Join-Path $PSScriptRoot 'review_maelstrom_sky.ps1') -EnvironmentOnly -Native -Clean -Wait `
    -ReviewName $ReviewName -Shots $Shots -Width $Width -Height $Height `
    -Station $Station -ExtraArgs $reviewArgs
