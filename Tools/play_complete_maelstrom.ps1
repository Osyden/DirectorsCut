# User-invoked interactive game using the installed complete arena and sky.
# Examples: ./Tools/play_complete_maelstrom.ps1
#           ./Tools/play_complete_maelstrom.ps1 -FullRun
#           ./Tools/play_complete_maelstrom.ps1 -Start 0 -Mute
param(
    [ValidateRange(0,449)][float]$Start = 265,
    [switch]$FullRun,
    [switch]$Mute,
    [ValidateRange(640,7680)][int]$Width = 1600,
    [ValidateRange(360,4320)][int]$Height = 900,
    [string]$Engine = 'E:/Utilities/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'DirectorsCut.uproject'
$editor = Join-Path $Engine 'Engine/Binaries/Win64/UnrealEditor.exe'
$runtime = Join-Path $projectRoot 'Content/DC/Maelstrom/GeneratedCliffPreview/Runtime'
foreach ($required in @('CompleteArena.json', 'CliffLandformPreviewImport.json',
                        'CliffPreviewSourceAudit.json', 'CliffPreviewSupports.json',
                        'CliffPreviewSurface.bin')) {
    if (!(Test-Path -LiteralPath (Join-Path $runtime $required) -PathType Leaf)) {
        throw "Complete arena installation is missing $required in $runtime."
    }
}
$arenaMarker = Get-Content -LiteralPath (Join-Path $runtime 'CompleteArena.json') -Raw | ConvertFrom-Json
if (!$arenaMarker.caldera_lake -and !(Test-Path -LiteralPath (Join-Path $runtime 'CliffPreviewWaterfall.json') -PathType Leaf)) {
    throw "Complete vortex installation is missing CliffPreviewWaterfall.json in $runtime."
}
if (!(Test-Path -LiteralPath $project -PathType Leaf)) { throw "Project is missing: $project" }
if (!(Test-Path -LiteralPath $editor -PathType Leaf)) { throw "Unreal Editor is missing: $editor" }
if ($FullRun) { $Start = 0 }
$startText = $Start.ToString([Globalization.CultureInfo]::InvariantCulture)
$saved = Join-Path $projectRoot 'Saved'
New-Item -ItemType Directory -Path $saved -Force | Out-Null
$commands = 'r.ScreenPercentage 100,r.NGX.DLSS.Enable 0,r.Streamline.DLSSG.Enable 0'
$arguments = @(
    ('"{0}"' -f $project), '-game', '-windowed',
    ('-ResX={0}' -f $Width), ('-ResY={0}' -f $Height),
    '-DCNoVoice', '-DCStation=helm', ('-DCMaelstromStart={0}' -f $startText),
    '-DCNoDLSS', ('-ExecCmds="{0}"' -f $commands),
    '-DDC=InstalledNoZenLocalFallback',
    ('-abslog="{0}"' -f (Join-Path $saved 'CompleteMaelstromInteractive.log'))
)
if ($Mute) { $arguments += '-nosound' }
# This launcher is explicitly invoked by the user to open a playable window.
Start-Process -FilePath $editor -WorkingDirectory $projectRoot -ArgumentList $arguments -WindowStyle Normal
