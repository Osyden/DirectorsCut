param(
    [ValidateSet('ACE','LlamaCpp','Both')][string]$Backend = 'Both',
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$Executable = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    [switch]$Packaged
)
$ErrorActionPreference = 'Stop'
$out = Join-Path $ProjectRoot ('Saved/ACEDialogue/' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss'))
New-Item -ItemType Directory -Force -Path $out | Out-Null
$backends = if ($Backend -eq 'Both') { @('ACE','LlamaCpp') } else { @($Backend) }
foreach ($selected in $backends) {
    & (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
    $log = Join-Path $out "$selected-runtime.log"
    $stdout = Join-Path $out "$selected-stdout.log"
    $stderr = Join-Path $out "$selected-stderr.log"
    $report = Join-Path $out "$selected-comparison.json"
    $launch = @('/Engine/Maps/Entry?game=/Script/DirectorsCut.DCDialogueReviewGameMode',
        '-unattended','-RenderOffscreen','-windowed','-forceres','-ResX=512','-ResY=512','-nosound','-nosplash',
        '-DCNoACE','-DCDialogueProbe','-DCLLMPort=8797','-useCIGforAI=0',"-DCDialogueBackend=$selected",
        "-DCDialogueReport=$report",'-ExecCmds=t.MaxFPS 30',"-abslog=$log")
    if (!$Packaged) { $launch = @((Join-Path $ProjectRoot 'DirectorsCut.uproject'),'-game') + $launch }
    $quoted = ($launch | ForEach-Object { if ($_.Contains('"')) { throw 'Unexpected quote in path' }; '"' + $_ + '"' }) -join ' '
    $proc = Start-Process -FilePath $Executable -ArgumentList $quoted -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    Write-Output "Dialogue $selected probe PID=$($proc.Id); report=$report"
    if (!$proc.WaitForExit(180000)) {
        # Bound only this launched process and its direct game/sidecar children.
        $children = @(Get-CimInstance Win32_Process | Where-Object { $_.ParentProcessId -eq $proc.Id })
        foreach ($child in $children) { Stop-Process -Id $child.ProcessId -Force -ErrorAction SilentlyContinue }
        Stop-Process -InputObject $proc -Force -ErrorAction SilentlyContinue
        throw "Dialogue $selected probe timed out; inspect $out"
    }
    $text = Get-Content -LiteralPath $log -Raw
    if ($proc.ExitCode -ne 0 -or $text -notmatch "DIALOGUE_REVIEW_SUCCESS backend=$selected") {
        throw "Dialogue $selected comparison failed (exit $($proc.ExitCode)); inspect $out"
    }
    if ($text -match 'GPU timeout|Fatal error|EXCEPTION_ACCESS_VIOLATION') { throw "Runtime fault in $log" }
    if ($selected -eq 'ACE') {
        $evidence = $text + (Get-Content -LiteralPath $stdout -Raw) + (Get-Content -LiteralPath $stderr -Raw)
        if ($evidence -notmatch 'offloaded [1-9][0-9]*/[0-9]+ layers to GPU') {
            throw "No actual GPU-layer-offload evidence; do not count this as an ACE GPU pass: $out"
        }
    }
    Write-Output "PASS: $selected comparison; $report"
}
