param([ValidateSet('Baseline','NoShafts')][string[]]$Cases = @('Baseline','NoShafts'))
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$editor = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
foreach ($case in $Cases) {
    $busy = Get-CimInstance Win32_Process | Where-Object { $_.Name -in @('Atlantean Odyssey.exe','DirectorsCut.exe','UnrealEditor.exe','UnrealEditor-Cmd.exe') }
    if ($busy) { throw 'Game/editor is running; leave the human playtest undisturbed.' }
    $folder = Join-Path $root ('Saved/TunnelLightingReview/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '/' + $case)
    New-Item -ItemType Directory -Force $folder | Out-Null
    $commands = 't.MaxFPS 60,r.Shadow.Virtual.AllowScreenOverflowMessages 0'
    if ($case -eq 'NoShafts') { $commands += ',ShowFlag.LightShafts 0' }
    $arguments = @(('"' + $root + '/DirectorsCut.uproject"'), '/Game/DC/Maps/L_Ride',
        '-game','-unattended','-RenderOffscreen','-nosound','-DCNoVoice','-windowed','-forceres',
        '-ResX=1600','-ResY=900','-nosplash','-DCRunThrough','-DCTimeScale=3',
        '-DCAutoShots=232,238,240,242,246,250','-DCOrders=Hold_fire','-DCLightningLog',
        ('-ExecCmds="' + $commands + '"'), ('-abslog="' + $folder + '/runtime.log"'))
    $since = Get-Date
    $process = Start-Process $editor -ArgumentList $arguments -WorkingDirectory $root -WindowStyle Hidden -PassThru
    $handle = $process.Handle
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "$case failed: $($process.ExitCode)" }
    $shots = @(Get-ChildItem (Join-Path $root 'Saved/Screenshots/Auto') -Filter 'shot_*.png' | Where-Object LastWriteTime -ge $since)
    if ($shots.Count -ne 6) { throw "$case produced $($shots.Count) screenshots, expected6. See $folder/runtime.log" }
    $shots | Copy-Item -Destination $folder
    Write-Output "Completed $case at $folder"
}
