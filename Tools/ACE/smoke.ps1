[CmdletBinding()]
param(
    [ValidateSet('Audio','ACE')][string]$Mode = 'Audio',
    [ValidateRange(1,10)][int]$Repeats = 3,
    [string]$Executable = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [switch]$Packaged
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
$out = Join-Path $ProjectRoot ('Saved/ACE/Smoke/' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss') + '_' + $Mode)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$log = Join-Path $out 'runtime.log'
$startedUtc = [DateTime]::UtcNow
$launch = @('/Game/DC/Maps/L_Ride','-unattended','-nop4','-RenderOffscreen','-nosplash','-windowed',
    '-ResX=1920','-ResY=1080','-DCVoiceSmokeTest',"-DCVoiceSmokeRepeats=$Repeats",'-useCIGforAI=0',
    '-DCSkipIntro','-DCRunThrough','-DCAutoShots=10,1000','-ExecCmds=t.MaxFPS 60',"-abslog=$log")
if (!$Packaged) { $launch = @((Join-Path $ProjectRoot 'DirectorsCut.uproject'),'-game') + $launch }
if ($Mode -eq 'ACE') { $launch += '-DCEnableACE' } else { $launch += '-DCNoACE' }
$quoted = ($launch | ForEach-Object { if ($_.Contains('"')) { throw 'Unexpected quote in argument' }; '"' + $_ + '"' }) -join ' '
$proc = Start-Process -FilePath $Executable -ArgumentList $quoted -WindowStyle Hidden -PassThru
Write-Output "Started $Mode test PID=$($proc.Id), log=$log"
$timer = [Diagnostics.Stopwatch]::StartNew()
while (!$proc.WaitForExit(1000)) {
    if ($timer.Elapsed.TotalSeconds -gt (90 + 30*$Repeats)) {
        Stop-Process -InputObject $proc -Force
        throw "Test timed out; inspect $log"
    }
}
$expected = if ($Mode -eq 'ACE') { 'ACE' } else { 'audio' }
$text = Get-Content -LiteralPath $log -Raw
if ($proc.ExitCode -ne 0 -or !$text.Contains("VOICE_SMOKE_SUCCESS mode=$expected cycles=$Repeats")) {
    throw "Voice smoke failed (exit $($proc.ExitCode)); inspect $log"
}
if ($text -match 'GPU timeout|Fatal error|EXCEPTION_ACCESS_VIOLATION') { throw "Runtime fault in $log" }
if ($text -match 'Crew HUD portrait missing|running without a ship|SkipPackage: /Game/(Brigantine|UltraDynamicSky|AncientRuins|PirateCast|MetaHumans|Fab/Claw_Kaiju_Monster)/') {
    throw "Missing core voyage content; voice playback alone is not a valid game test. Inspect $log"
}
$shot = Join-Path $ProjectRoot 'Saved/Screenshots/Auto/shot_0010.png'
if (!(Test-Path -LiteralPath $shot) -or (Get-Item -LiteralPath $shot).LastWriteTimeUtc -lt $startedUtc) {
    throw "No fresh voyage screenshot was captured; inspect $log"
}
Copy-Item -LiteralPath $shot -Destination (Join-Path $out 'voyage.png')
Write-Output "Inspect the actual voyage image before accepting this candidate: $out/voyage.png"
Write-Output "PASS: $Mode, $Repeats cycles, $($Repeats*3) crew lines; $log"
