param(
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$Executable = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    [switch]$Packaged
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
$out = Join-Path $ProjectRoot ('Saved/ACEDialogue/combined_' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss'))
New-Item -ItemType Directory -Force -Path $out | Out-Null
$log = Join-Path $out 'runtime.log'
$stdout = Join-Path $out 'stdout.log'
$stderr = Join-Path $out 'stderr.log'
$report = Join-Path $out 'integration.json'
$launch = @('/Game/DC/Maps/L_Ride','-unattended','-RenderOffscreen','-windowed','-forceres',
    '-ResX=1600','-ResY=900','-nosplash','-DCNoMenu','-DCACEIntegrationTest','-DCDialogueBackend=ACE',
    '-DCEnableACE','-useCIGforAI=0','-DCRunThrough','-DCAutoShots=0,60,9999',
    "-DCACEIntegrationReport=$report",'-ExecCmds=t.MaxFPS 60',"-abslog=$log")
if (!$Packaged) { $launch = @((Join-Path $ProjectRoot 'DirectorsCut.uproject'),'-game') + $launch }
$quoted = ($launch | ForEach-Object { if ($_.Contains('"')) { throw 'Unexpected quote in path' }; '"' + $_ + '"' }) -join ' '
$proc = Start-Process -FilePath $Executable -ArgumentList $quoted -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Write-Output "Combined ACE test PID=$($proc.Id); report=$report"
if (!$proc.WaitForExit(270000)) {
    $snapshot = @(Get-CimInstance Win32_Process)
    $owned = [Collections.Generic.List[int]]::new()
    $owned.Add($proc.Id)
    for ($index = 0; $index -lt $owned.Count; $index++) {
        foreach ($child in $snapshot | Where-Object { $_.ParentProcessId -eq $owned[$index] }) {
            if (!$owned.Contains([int]$child.ProcessId)) { $owned.Add([int]$child.ProcessId) }
        }
    }
    for ($index = $owned.Count - 1; $index -ge 0; $index--) {
        Stop-Process -Id $owned[$index] -Force -ErrorAction SilentlyContinue
    }
    throw "Combined ACE test timed out; inspect $out"
}
$runtime = Get-Content -LiteralPath $log -Raw
if ($proc.ExitCode -ne 0 -or $runtime -notmatch 'ACE_INTEGRATION_SUCCESS generated_lines=6 crews=3 world_cycles=2 scene_cancel=1 restart=1') {
    throw "Combined ACE test failed (exit $($proc.ExitCode)); inspect $out"
}
if ($runtime -match 'GPU timeout|Fatal error|EXCEPTION_ACCESS_VIOLATION|disabling ACE for this run') {
    throw "Runtime fault/fallback in $out"
}
$samples = @{}
$avatars = @{ BP_Finch_C='Rex'; BP_Marlow_C='Duncan'; BP_Odessa_C='Ruby' }
$verifiedClips = 0
foreach ($line in $runtime -split "`n") {
    if ($line -match 'Voice: ACE queued (Rex|Duncan|Ruby) ') { $samples[$Matches[1]] = $false }
    if ($line -match 'received ([1-9][0-9]*) animation samples, ([1-9][0-9]*) audio samples for clip on (BP_Finch_C|BP_Marlow_C|BP_Odessa_C)') {
        $samples[$avatars[$Matches[3]]] = $true
    }
    if ($line -match 'ACE_INTEGRATION_LINE cycle=[12] speaker=(Rex|Duncan|Ruby) ') {
        if (!$samples[$Matches[1]]) { throw "Generated clip has no positive facial/audio samples: $line" }
        $verifiedClips++
    }
}
if ($verifiedClips -ne 6) { throw "Expected six generated clips with positive A2F samples, found $verifiedClips" }
$evidence = $runtime + (Get-Content -LiteralPath $stdout -Raw) + (Get-Content -LiteralPath $stderr -Raw)
if ($evidence -notmatch 'offloaded [1-9][0-9]*/[0-9]+ layers to GPU') {
    throw "Missing actual GPU layer-offload evidence: $out"
}
Write-Output "PASS: combined ACE dialogue, Piper, Audio2Face, scene cancellation and level reload; $report"
