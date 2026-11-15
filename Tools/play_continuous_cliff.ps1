# Run explicitly to inspect the local cliff demo from the helm.
param([switch]$Sound, [string]$Engine = 'E:/Utilities/UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$saved = Join-Path $projectRoot 'Saved'
foreach ($required in @('CliffLandformPreviewImport.json','OwnedCliffAtlasImport.json',
                        'CliffPreviewSupports.json','CliffPreviewSurface.bin','CliffPreviewSourceAudit.json')) {
    if (!(Test-Path -LiteralPath (Join-Path $saved $required))) {
        throw "Missing local cliff demo data: $required. See taskboard/CONTINUOUS-SCAN-CLIFF-15NOV.md."
    }
}
$mesh = Get-Content -LiteralPath (Join-Path $saved 'CliffLandformPreviewImport.json') -Raw | ConvertFrom-Json
$support = Get-Content -LiteralPath (Join-Path $saved 'CliffPreviewSupports.json') -Raw | ConvertFrom-Json
if ($mesh.source_sha256 -ne $support.source_fbx_sha256) { throw 'Cliff support/mesh versions differ.' }
$project = Join-Path $projectRoot 'DirectorsCut.uproject'
$editor = Join-Path $Engine 'Engine/Binaries/Win64/UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editor)) { throw "Unreal Editor is missing: $editor" }
$arguments = @(
    ('"{0}"' -f $project), '-game','-windowed','-ResX=1600','-ResY=900',
    '-DCNoVoice','-DCSceneReview','-DCStation=helm','-DCMaelstromStart=295',
    '-DCCliffPreview','-DCCliffPreviewScan','-DCCliffPreviewSupports',
    '-DCCliffPreviewPlayerView','-DCMaelSunNeutral','-DCMaelSunGroundBalance',
    '-DDC=InstalledNoZenLocalFallback',
    ('-abslog="{0}"' -f (Join-Path $saved 'ContinuousCliffInteractive.log'))
)
if (!$Sound) { $arguments += '-nosound' }
# This launcher is user-invoked and opens an interactive game window.
Start-Process -FilePath $editor -WorkingDirectory $projectRoot -ArgumentList $arguments -WindowStyle Normal
