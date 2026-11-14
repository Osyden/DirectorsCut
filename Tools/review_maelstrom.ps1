param(
    [switch]$Continuous,
    [switch]$EnvironmentOnly,
    [switch]$Tenths,
    [switch]$Wait,
    [switch]$Native,
    [switch]$Clean,
    [ValidateRange(0,4)][int]$WaterReview = 0,
    [ValidateRange(640,7680)][int]$Width = 1600,
    [ValidateRange(360,4320)][int]$Height = 900,
    [ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$ReviewName = 'MaelstromReview',
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
$logFile = Join-Path $projectRoot ('Saved/{0}.log' -f $ReviewName)
$arguments = @(
    ('"{0}"' -f $projectFile), '-game', '-unattended', '-RenderOffscreen',
    '-windowed', '-forceres', ('-ResX={0}' -f $Width), ('-ResY={0}' -f $Height),
    '-DCNoVoice', '-DDC=InstalledNoZenLocalFallback', '-DCMaelstromRouteLog',
    ('-DCStation={0}' -f $Station), ('-DCAutoShots={0}' -f $Shots),
    ('-abslog="{0}"' -f $logFile)
)
$commands = 'r.SetRes {0}x{1}w' -f $Width,$Height
if ($Native) {
    $arguments += '-DCNoDLSS'
    $commands += ',r.ScreenPercentage 100,r.NGX.DLSS.Enable 0,r.Streamline.DLSSG.Enable 0'
}
$arguments += ('-ExecCmds="{0}"' -f $commands)
if ($Orders -and !$EnvironmentOnly) { $arguments += ('-DCOrders={0}' -f $Orders) }
if ($EnvironmentOnly) { $arguments += '-DCSceneReview' }
if ($Clean) { $arguments += '-DCNoHUD' }
if ($WaterReview -gt 0) { $arguments += ('-DCWaterReview={0}' -f $WaterReview) }
if ($Tenths) { $arguments += '-DCTenths' }
if ($Continuous) { $arguments += @('-DCRunThrough', ('-DCMaelstromStart={0}' -f $Start)) }
if ([Math]::Abs($Steer) -le 1) { $arguments += ('-DCMaelstromSteer={0}' -f $Steer) }
if (!$Sound) { $arguments += '-nosound' }
if ($NoFFT) { $arguments += '-DCMaelstromNoFFT' }
$reviewProcess = Start-Process $editorFile -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output ('Review process {0}. Screenshots: {1}/Saved/Screenshots/Auto; log: {2}' -f $reviewProcess.Id, $projectRoot, $logFile)

if ($Wait) {
    $reviewProcess.WaitForExit()
    if ($reviewProcess.ExitCode -ne 0) { throw "Review failed; inspect $logFile" }
    if (Select-String -LiteralPath $logFile -Pattern 'Failed to compile Material /Game/DC/Maelstrom/' -Quiet) {
        throw "Maelstrom shader compilation failed; captures would show a fallback material. Inspect $logFile"
    }
    $destination = Join-Path $projectRoot ("Saved/SceneReview/$ReviewName")
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($shot in $Shots.Split(',')) {
        $time = [double]::Parse($shot, [Globalization.CultureInfo]::InvariantCulture)
        $file = if ($Tenths) { 'shot_{0:D5}.png' -f [int][Math]::Round($time*10) } else { 'shot_{0:D4}.png' -f [int][Math]::Round($time) }
        $source = Join-Path $projectRoot "Saved/Screenshots/Auto/$file"
        if ((Get-Item -LiteralPath $source).LastWriteTime -lt $reviewProcess.StartTime) { throw "Stale screenshot: $source" }
        Copy-Item -LiteralPath $source -Destination (Join-Path $destination $file)
    }
    Write-Output "Saved review: $destination"
}
