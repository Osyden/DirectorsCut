[CmdletBinding()]
param(
    [ValidateSet('Both','Baseline','ACE')][string]$Mode = 'Both',
    [string]$Clip = 'finch_confirmation',
    [string]$Provider = 'LocalA2F-Mark-AR',
    [string]$Character = '/Game/PirateCast/Assembled/Finch/BP_Finch.BP_Finch_C',
    [string]$EngineRoot = 'E:\Utilities\UE_5.8',
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$Executable = '',
    [switch]$Packaged,
    [ValidateRange(1,9)][int]$Repeats = 2,
    [switch]$CurvesOnly,
    [switch]$Show,
    [switch]$AllowLocalInference
)
$ErrorActionPreference = 'Stop'
if ($Mode -ne 'Baseline' -and !$AllowLocalInference) {
    throw 'GPU comparison requires -AllowLocalInference. The test uses a separate CUDA context; see taskboard/ACE-GPU-CRASH-15NOV.md.'
}
& (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
$root = [IO.Path]::GetFullPath($ProjectRoot)
if (!$Executable) { $Executable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
if ($Packaged -and $Executable -like '*UnrealEditor.exe') { throw 'Select the packaged game executable.' }
if ($Clip -notmatch '^[a-z_]+$') { throw 'Clip must be a fixture name such as finch_confirmation.' }
if (-not $Provider.StartsWith('LocalA2F-')) { throw 'Only explicit local Audio2Face providers are allowed.' }
$wav = Join-Path $root "Saved/ACE/Fixtures/$Clip.wav"
if (-not (Test-Path -LiteralPath $wav)) { throw "Generate fixtures first: $wav" }
$runRoot = Join-Path $root ('Saved/ACE/Runs/' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss_fff') + '_' + $Clip)
New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
$modes = if ($Mode -eq 'Both') { @('Baseline','ACE') } else { @($Mode) }
$summary = @()
foreach ($current in $modes) {
    $output = Join-Path $runRoot $current.ToLowerInvariant()
    New-Item -ItemType Directory -Force -Path $output | Out-Null
    $map = if ($Packaged) { '/Game/DC/Maps/L_Ride' } else { '/Engine/Maps/Entry' }
    $launchArgs = @(
        ($map + '?game=/Script/DirectorsCut.DCAceCompareGameMode'),
        '-unattended', '-nosplash', '-windowed', '-ResX=1280', '-ResY=720',
        '-ACEAutoQuit', "-ACEWav=$wav", "-ACEProvider=$Provider", "-ACEOutput=$output", "-ACERepeats=$Repeats",
        ('-abslog=' + (Join-Path $output 'runtime.log'))
    )
    if (!$Packaged) { $launchArgs = @((Join-Path $root 'DirectorsCut.uproject'),'-game') + $launchArgs }
    if ($current -eq 'Baseline') { $launchArgs += '-ACEBaseline' }
    else { $launchArgs += @('-DCEnableACE', '-useCIGforAI=0') }
    if ($CurvesOnly) { $launchArgs += '-ACECurvesOnly' }
    if ($Character) { $launchArgs += "-ACECharacter=$Character" }
    if (-not $Show) { $launchArgs += '-RenderOffscreen' }
    # Start-Process takes a command-line string, so quote each argument explicitly.
    if (($launchArgs | Where-Object { $_.Contains('"') }).Count) { throw 'Quotes are not supported in paths or arguments.' }
    $quotedArgs = ($launchArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $windowStyle = if ($Show) { 'Normal' } else { 'Hidden' }
    $proc = Start-Process -FilePath $Executable `
        -ArgumentList $quotedArgs -WindowStyle $windowStyle -PassThru
    $clock = [Diagnostics.Stopwatch]::StartNew()
    'elapsed_seconds,gpu_memory_used_mib,gpu_utilization_percent' | Set-Content -LiteralPath (Join-Path $output 'gpu.csv')
    while (-not $proc.WaitForExit(1000)) {
        $gpu = & nvidia-smi --query-gpu=memory.used,utilization.gpu --format=csv,noheader,nounits
        foreach ($line in $gpu) { ('{0:F3},{1}' -f $clock.Elapsed.TotalSeconds,$line) | Add-Content -LiteralPath (Join-Path $output 'gpu.csv') }
        if ($clock.Elapsed.TotalSeconds -gt 240) {
            # This process was started by this invocation; never terminate another editor.
            Stop-Process -InputObject $proc
            throw "Comparison timed out. Inspect $output/runtime.log"
        }
    }
    $report = Join-Path $output 'result.csv'
    if (-not (Test-Path -LiteralPath $report)) { throw "No test result (exit $($proc.ExitCode)). Inspect $output/runtime.log" }
    $row = Import-Csv -LiteralPath $report
    $summary += $row
    $row | Format-Table -AutoSize
    if ($proc.ExitCode -ne 0 -or $row.result -ne 'success') { throw "Comparison failed. Inspect $output/runtime.log" }
}
$summary | Export-Csv -LiteralPath (Join-Path $runRoot 'comparison.csv') -NoTypeInformation
Write-Output "Comparison saved to $runRoot"
Write-Output 'GPU readings cover the whole workstation. Baseline start delay is a playback request; ACE delay is its playback-start event.'
