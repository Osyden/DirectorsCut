param(
    [switch]$Continuous,
    [string]$Shots = '282,300,325,350,380,408,430,440,451',
    [string]$Station = 'helm',
    [string]$Orders = 'port_battery_fire;starboard_battery_fire',
    [float]$Start = 279,
    [float]$Steer = 2,
    [switch]$Sound,
    [switch]$NoFFT,
    [string]$Engine = 'E:/Utilities/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$projectFile = Join-Path $projectRoot 'DirectorsCut.uproject'
$editorFile = Join-Path $Engine 'Engine/Binaries/Win64/UnrealEditor.exe'
$logFile = Join-Path $projectRoot 'Saved/MaelstromReview.log'
$arguments = @(
    ('"{0}"' -f $projectFile), '-game', '-unattended', '-RenderOffscreen',
    '-windowed', '-forceres', '-ResX=1600', '-ResY=900',
    '-ExecCmds="r.SetRes 1600x900w"', '-DCNoVoice', '-DDC=InstalledNoZenLocalFallback', '-DCMaelstromRouteLog',
    ('-DCStation={0}' -f $Station), ('-DCAutoShots={0}' -f $Shots),
    ('-abslog="{0}"' -f $logFile)
)
if ($Orders) { $arguments += ('-DCOrders={0}' -f $Orders) }
if ($Continuous) { $arguments += @('-DCRunThrough', ('-DCMaelstromStart={0}' -f $Start)) }
if ([Math]::Abs($Steer) -le 1) { $arguments += ('-DCMaelstromSteer={0}' -f $Steer) }
if (!$Sound) { $arguments += '-nosound' }
if ($NoFFT) { $arguments += '-DCMaelstromNoFFT' }
$reviewProcess = Start-Process $editorFile -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output ('Review process {0}. Screenshots: {1}/Saved/Screenshots/Auto; log: {2}' -f $reviewProcess.Id, $projectRoot, $logFile)
