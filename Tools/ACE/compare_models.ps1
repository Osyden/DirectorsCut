<#
Runs existing native reviews against LOCAL artifacts only. Never downloads models.
Example (after an idle GPU window is agreed):
  ./compare_models.ps1 -GemmaModel 'E:/Models/gemma-4-E4B-it-Q4_K_M.gguf' -BoltModel 'E:/Models/Bolt-Q4_K_M.gguf'
Use -Models Gemma -Phase Conversation to run only one missing comparison arm.
Gemma/Bolt files must match the pinned publisher revision's byte count and SHA256.
#>
[CmdletBinding()]
param(
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [string]$Executable = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    [switch]$Packaged,
    [string]$OutputRoot,
    [string]$QwenModel,
    [string]$GemmaModel,
    [string]$BoltModel,
    [ValidateSet('Qwen','Gemma','Bolt')][string[]]$Models = @('Qwen','Gemma','Bolt'),
    [ValidateSet('Commands','Conversation','Both')][string]$Phase = 'Both'
)
$ErrorActionPreference = 'Stop'

function Get-LocalFile([string]$Path, [string]$Label) {
    if ([string]::IsNullOrWhiteSpace($Path) -or !(Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label requires an existing local file; no download or fallback will be attempted: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.FullName -match '["\r\n\x00]') { throw "Unsupported character in $Label path" }
    return $item
}

function Get-Latencies($Values) {
    $sorted = @($Values | Where-Object { $null -ne $_ } | ForEach-Object { [double]$_ } | Sort-Object)
    if (!$sorted.Count) { return @{ count=0; median_ms=$null; p95_ms=$null; max_ms=$null } }
    $middle = [int][Math]::Floor($sorted.Count / 2)
    $median = if ($sorted.Count % 2) { $sorted[$middle] } else { ($sorted[$middle-1]+$sorted[$middle])/2 }
    return @{ count=$sorted.Count; median_ms=[Math]::Round($median,2);
        p95_ms=[Math]::Round($sorted[[int][Math]::Ceiling(.95*$sorted.Count)-1],2);
        max_ms=[Math]::Round($sorted[-1],2); percentile_method='nearest rank' }
}

function Read-Text([string]$Path) {
    if (Test-Path -LiteralPath $Path -PathType Leaf) { return [IO.File]::ReadAllText($Path) }
    return ''
}

function Get-ConversationSpeechTimings([string]$Log, $Cases) {
    $rows = @()
    for ($index=0; $index -lt @($Cases).Count; $index++) {
        $rows += [pscustomobject]@{ index=$index; speaker=$Cases[$index].speaker; question=$Cases[$index].question;
            time_to_speech_ms=$null; request_utc=$null; speaking_utc=$null }
    }
    $pending = $null
    foreach ($line in $Log -split "`n") {
        $eventTime = $null
        if ($line -match '^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]') {
            $parsed = [DateTime]::MinValue
            $styles = [Globalization.DateTimeStyles]::AssumeUniversal -bor [Globalization.DateTimeStyles]::AdjustToUniversal
            if ([DateTime]::TryParseExact($Matches[1], 'yyyy.MM.dd-HH.mm.ss:fff', [Globalization.CultureInfo]::InvariantCulture, $styles, [ref]$parsed)) { $eventTime=$parsed }
        }
        if ($line -match 'CONVERSATION_REQUEST speaker=(Rex|Duncan|Ruby) ') {
            $pending = @{ speaker=$Matches[1]; requested=$eventTime; reply=$null; queued=0; starts=0; speaking=$null }
        } elseif ($pending -and $line -match 'CONVERSATION_REPLY speaker=(Rex|Duncan|Ruby) .* text=(.*)$') {
            if ($Matches[1] -eq $pending.speaker) { $pending.reply=$Matches[2].TrimEnd() }
        } elseif ($pending -and $pending.reply -and $line -match 'Voice: ACE queued (Rex|Duncan|Ruby) ') {
            if ($Matches[1] -eq $pending.speaker) { $pending.queued++ }
        } elseif ($pending -and $pending.reply -and $line -match 'Voice: ACE speaking (Rex|Duncan|Ruby)\s*$') {
            if ($Matches[1] -eq $pending.speaker -and $pending.queued -gt 0) {
                $pending.starts++; $pending.speaking=$eventTime
            }
        } elseif ($line -match 'CONVERSATION_REVIEW_CASE index=(\d+) speaker=(Rex|Duncan|Ruby) .* answer=(.*)$') {
            $caseIndex=[int]$Matches[1]; $speaker=$Matches[2]; $answer=$Matches[3].TrimEnd()
            # Only a completed reviewed case can admit a timing. Extra playback markers
            # make the interval ambiguous; initial warmup/authored reports never count.
            if ($pending -and $caseIndex -lt $rows.Count -and $speaker -eq $pending.speaker -and
                $speaker -eq $Cases[$caseIndex].speaker -and $answer -ceq $pending.reply -and
                $answer -ceq $Cases[$caseIndex].answer -and $pending.queued -eq 1 -and $pending.starts -eq 1 -and
                $null -ne $pending.requested -and $null -ne $pending.speaking -and $null -ne $eventTime -and
                $pending.speaking -ge $pending.requested -and $eventTime -ge $pending.speaking) {
                $rows[$caseIndex].time_to_speech_ms=[Math]::Round(($pending.speaking-$pending.requested).TotalMilliseconds,2)
                $rows[$caseIndex].request_utc=$pending.requested.ToString('o')
                $rows[$caseIndex].speaking_utc=$pending.speaking.ToString('o')
            }
            $pending=$null
        }
    }
    return $rows
}

function Update-OwnedTree($Owned) {
    $snapshot = @(Get-CimInstance Win32_Process -ErrorAction Stop)
    # Expand only from a currently live parent whose creation time matches our record.
    do {
        $added = $false
        foreach ($candidate in $snapshot) {
            $parentId = [int]$candidate.ParentProcessId
            $candidateId = [int]$candidate.ProcessId
            if (!$Owned.ContainsKey($parentId) -or $Owned.ContainsKey($candidateId)) { continue }
            $parent = $snapshot | Where-Object { [int]$_.ProcessId -eq $parentId } | Select-Object -First 1
            if ($parent -and $parent.CreationDate.ToUniversalTime().Ticks -eq $Owned[$parentId]) {
                $Owned[$candidateId] = $candidate.CreationDate.ToUniversalTime().Ticks
                $added = $true
            }
        }
    } while ($added)
    return @($snapshot | Where-Object {
        $Owned.ContainsKey([int]$_.ProcessId) -and $_.CreationDate.ToUniversalTime().Ticks -eq $Owned[[int]$_.ProcessId]
    })
}

function Stop-OwnedTree($Owned) {
    # Newer descendants first; a recycled PID can never authorize terminating its new owner.
    $live = @(Update-OwnedTree $Owned | Sort-Object CreationDate -Descending)
    foreach ($entry in $live) {
        $targetProcess = $null
        try {
            $targetProcess = [Diagnostics.Process]::GetProcessById([int]$entry.ProcessId)
            $null = $targetProcess.Handle # Pin the process object before checking identity; kill through this handle.
            $current = Get-CimInstance Win32_Process -Filter "ProcessId=$([int]$entry.ProcessId)" -ErrorAction SilentlyContinue
            if ($current -and $current.CreationDate.ToUniversalTime().Ticks -eq $Owned[[int]$entry.ProcessId] -and !$targetProcess.HasExited) {
                $targetProcess.Kill()
            }
        } catch { Write-Warning "Owned process cleanup could not confirm/stop PID $($entry.ProcessId): $($_.Exception.Message)" }
        finally { if ($targetProcess) { $targetProcess.Dispose() } }
    }
}

function Invoke-NativeReview([string]$SelectedPhase, $Model, [string]$Directory) {
    $record = [ordered]@{ phase=$SelectedPhase; runtime_variant='ACE/GameAgentSDK-CUDA-separate-context';
        model_path=$Model.path; model_sha256=$Model.sha256; pipeline_pass=$false; quality_pass=$null;
        semantic_review='Required: compare answers with live context; pipeline success is not quality approval';
        errors=@(); halt_remaining=$false }
    $owned = @{}
    $process = $null
    try {
        & (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
        New-Item -ItemType Directory -Path $Directory -ErrorAction Stop | Out-Null
        $reportPath = Join-Path $Directory 'report.json'
        $runtimePath = Join-Path $Directory 'runtime.log'
        $stdoutPath = Join-Path $Directory 'stdout.log'
        $stderrPath = Join-Path $Directory 'stderr.log'
        $arguments = @('-unattended','-RenderOffscreen','-windowed','-forceres','-nosplash',
            '-DCDialogueBackend=ACE','-useCIGforAI=0',"-DCACEModel=$($Model.path)","-abslog=$runtimePath")
        if ($SelectedPhase -eq 'Commands') {
            $arguments = @('/Engine/Maps/Entry?game=/Script/DirectorsCut.DCDialogueReviewGameMode') + $arguments +
                @('-ResX=512','-ResY=512','-nosound','-DCNoACE','-DCDialogueProbe',
                  "-DCDialogueReport=$reportPath",'-ExecCmds=t.MaxFPS 30')
            $deadlineSeconds = 180
        } else {
            $arguments = @('/Game/DC/Maps/L_Ride') + $arguments +
                @('-ResX=1600','-ResY=900','-DCNoMenu','-DCEnableACE','-DCConversationReview',
                  "-DCConversationReport=$reportPath",'-ExecCmds=t.MaxFPS 60')
            $deadlineSeconds = 270
        }
        if (!$Packaged) { $arguments = @((Join-Path $ProjectRoot 'DirectorsCut.uproject'),'-game') + $arguments }
        # Start-Process uses the Windows command line, not a shell. Reject quotes/control
        # characters and quote each argument; none of these arguments ends in a backslash.
        $quoted = ($arguments | ForEach-Object {
            if ($_ -match '["\r\n\x00]' -or $_.EndsWith('\')) { throw 'Unsupported launch argument' }
            '"' + $_ + '"'
        }) -join ' '
        $record.arguments = $arguments
        $record.report = $reportPath
        $record.runtime_log = $runtimePath
        $record.started_utc = [DateTime]::UtcNow.ToString('o')
        Write-Host "MODEL_REVIEW model=$($Model.id) phase=$SelectedPhase runtime=$($record.runtime_variant) sha256=$($Model.sha256)"
        $process = Start-Process -FilePath $Executable -ArgumentList $quoted -WorkingDirectory $ProjectRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
        $record.pid = $process.Id
        $rootProcess = Get-CimInstance Win32_Process -Filter "ProcessId=$($process.Id)" -ErrorAction Stop
        if ($rootProcess) { $owned[[int]$process.Id] = $rootProcess.CreationDate.ToUniversalTime().Ticks }
        $timer = [Diagnostics.Stopwatch]::StartNew()
        do {
            $live = @(Update-OwnedTree $owned)
            if ($timer.Elapsed.TotalSeconds -gt $deadlineSeconds) {
                Stop-OwnedTree $owned
                $record.halt_remaining = $true
                throw "Owned review process tree exceeded ${deadlineSeconds}s; remaining GPU tests are deferred"
            }
            if ($process.HasExited -and !$live.Count) { break }
            Start-Sleep -Milliseconds 250
        } while ($true)
        $process.WaitForExit()
        $record.exit_code = $process.ExitCode
        $record.wall_ms = [Math]::Round($timer.Elapsed.TotalMilliseconds,2)
        $runtime = Read-Text $runtimePath
        $evidence = $runtime + (Read-Text $stdoutPath) + (Read-Text $stderrPath)
        $record.gpu_offload_layers = @([regex]::Matches($evidence, 'offloaded ([0-9]+)/([0-9]+) layers to GPU') | ForEach-Object {
            [pscustomobject]@{ offloaded=[int]$_.Groups[1].Value; total=[int]$_.Groups[2].Value; evidence=$_.Value }
        })
        $record.gpu_offload_verified = @($record.gpu_offload_layers | Where-Object offloaded -gt 0).Count -gt 0
        $record.selected_ace_verified = $runtime -match 'DIALOGUE_BACKEND selected=ACE enabled=1'
        if (!$record.gpu_offload_verified) { $record.errors += 'No actual GPU layer-offload evidence' }
        if (!$record.selected_ace_verified) { $record.errors += 'Selected ACE backend not confirmed' }
        if ($process.ExitCode -ne 0) { $record.errors += "Native exit code $($process.ExitCode)" }
        if ($evidence -match 'GPU timeout|Fatal error|EXCEPTION_ACCESS_VIOLATION|DXGI_ERROR_DEVICE_(HUNG|REMOVED|RESET)') {
            $record.errors += 'Native or GPU fault; remaining tests deferred'
            $record.halt_remaining = $true
        }
        if ($evidence -match 'disabling ACE for this run|Dialogue: ACE model missing') { $record.errors += 'Runtime disabled ACE or rejected the requested model' }
        if (!(Test-Path -LiteralPath $reportPath -PathType Leaf)) { throw 'Native review did not write its report' }
        $report = Read-Text $reportPath | ConvertFrom-Json
        if ($report.backend -ne 'ACE') { $record.errors += 'Report backend was not ACE' }
        if (!$report.success) { $record.errors += 'Native report indicates failure' }
        $record.native_report_success = [bool]$report.success
        if ($SelectedPhase -eq 'Commands') {
            $orders = @($report.results | Where-Object kind -eq 'order')
            $acks = @($report.results | Where-Object kind -eq 'acknowledgement')
            $passedOrders = @($orders | Where-Object pass -eq $true).Count
            $passedAcks = @($acks | Where-Object pass -eq $true).Count
            $record.command_accuracy = @{ passed=$passedOrders; expected=24; completed=$orders.Count;
                percent=[Math]::Round(100*$passedOrders/24,2); includes_nonorders=2 }
            $record.acknowledgement_accuracy = @{ passed=$passedAcks; expected=3; completed=$acks.Count }
            $record.order_latency = Get-Latencies @($orders | ForEach-Object latency_ms)
            $record.acknowledgement_latency = Get-Latencies @($acks | ForEach-Object latency_ms)
            $record.readiness_ms = $report.readiness_ms
            $record.case_results = $report.results
            if ($report.planned_cases -ne 27 -or $report.completed_cases -ne 27 -or $orders.Count -ne 24 -or $acks.Count -ne 3 -or
                $runtime -notmatch 'DIALOGUE_REVIEW_SUCCESS backend=ACE total=27') { $record.errors += 'Expected complete native 27-case review was not confirmed' }
        } else {
            $record.answers_for_semantic_review = $report.results
            $record.completed_questions = @($report.results).Count
            $record.generation_latency = Get-Latencies @($report.results | ForEach-Object generation_ms)
            $record.latency_scope = 'Question submission to history observation, including any queue wait; not token throughput or first-token latency'
            $record.speech_timings = @(Get-ConversationSpeechTimings $runtime @($report.results))
            $record.time_to_speech_latency = Get-Latencies @($record.speech_timings | ForEach-Object time_to_speech_ms)
            $record.time_to_speech_scope = 'UTC CONVERSATION_REQUEST to matching ACE speaking, bracketed by generated reply and completed review case; includes generation, Piper and A2F buffering. Missing/ambiguous markers are null, not zero.'
            $samples = @{}
            $avatars = @{ BP_Finch_C='Rex'; BP_Marlow_C='Duncan'; BP_Odessa_C='Ruby' }
            $verifiedClips = 0
            foreach ($line in $runtime -split "`n") {
                if ($line -match 'Voice: ACE queued (Rex|Duncan|Ruby) ') { $samples[$Matches[1]] = $false }
                if ($line -match 'received ([1-9][0-9]*) animation samples, ([1-9][0-9]*) audio samples for clip on (BP_Finch_C|BP_Marlow_C|BP_Odessa_C)') { $samples[$avatars[$Matches[3]]] = $true }
                if ($line -match 'CONVERSATION_REVIEW_CASE index=\d+ speaker=(Rex|Duncan|Ruby) ') {
                    if ($samples[$Matches[1]]) { $verifiedClips++ } else { $record.errors += "No positive face/audio samples for conversation clip: $line" }
                }
            }
            $record.positive_a2f_clips = $verifiedClips
            if ($record.completed_questions -ne 8 -or $verifiedClips -ne 8 -or $runtime -notmatch 'CONVERSATION_REVIEW_SUCCESS cases=8') { $record.errors += 'Expected eight generated conversational speech/lip-sync results were not confirmed' }
            if (@($report.results | Where-Object { !$_.orders_unchanged -or !$_.a2f_started_and_finished }).Count) { $record.errors += 'Conversation changed orders or missed speech completion' }
        }
        $record.pipeline_pass = $record.errors.Count -eq 0
    } catch {
        $record.errors += $_.Exception.Message
        if ($owned.Count) {
            try { Stop-OwnedTree $owned }
            catch { $record.errors += "Owned cleanup failed: $($_.Exception.Message)"; $record.halt_remaining = $true }
        }
        # Retain the Start-Process handle as a last resort if CIM failed during startup.
        if ($process -and !$process.HasExited) { $process.Kill(); $record.halt_remaining = $true }
    } finally {
        if ($process) { $process.Dispose() }
    }
    return [pscustomobject]$record
}

$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
$Executable = (Get-LocalFile $Executable 'Executable').FullName
if (!$Packaged) { $null = Get-LocalFile (Join-Path $ProjectRoot 'DirectorsCut.uproject') 'Unreal project' }
$null = Get-LocalFile (Join-Path $PSScriptRoot 'require_idle_gpu.ps1') 'GPU idle guard'
if (!$QwenModel) { $QwenModel = Join-Path $ProjectRoot 'Sidecars/models/Qwen3.5-4B-Q5_K_M.gguf' }
if (!$OutputRoot) { $OutputRoot = Join-Path $ProjectRoot 'Saved/ACEDialogue/ModelComparisons' }
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
if ($OutputRoot -match '["\r\n\x00]') { throw 'Unsupported output path' }
$paths = @{ Qwen=$QwenModel; Gemma=$GemmaModel; Bolt=$BoltModel }
$labels = @{ Qwen='Current Qwen3.5-4B Q5_K_M'; Gemma='Unsloth gemma-4-E4B-it Q4_K_M'; Bolt='Official Bolt Q4_K_M' }
$pins = @{
    Gemma=@{ publisher='unsloth'; repository='https://huggingface.co/unsloth/gemma-4-E4B-it-GGUF';
        revision='bfc15c382204943c3a8fff0c750b94ae2364d7a3'; size_bytes=[long]4977171584;
        sha256='85a896a047553e842f25297ee5b031d64ff30147d9c4af17b1e4b394cd1fab87' };
    Bolt=@{ publisher='nvidia'; repository='https://huggingface.co/nvidia/NVIDIA-Nemotron-3.5-Bolt-E4B-4GB';
        revision='1db54d5b12533ff1bcaa8e42cf4c667988a1f1be'; size_bytes=[long]6180446208;
        sha256='c8af2b8aea10a690a1d9491db744a9446660173cb38fc489612b556615a69612' }
}
$artifacts = @()
foreach ($modelId in @($Models | Select-Object -Unique)) {
    $file = Get-LocalFile $paths[$modelId] "$modelId artifact"
    if ($file.Extension -ne '.gguf' -or $file.Length -le 0) { throw "$modelId requires a nonempty local GGUF" }
    if ($modelId -eq 'Qwen' -and $file.Name -ine 'Qwen3.5-4B-Q5_K_M.gguf') { throw 'Qwen baseline must use Qwen3.5-4B-Q5_K_M.gguf' }
    if ($modelId -eq 'Gemma' -and $file.Name -ine 'gemma-4-E4B-it-Q4_K_M.gguf') { throw 'Gemma arm requires the requested gemma-4-E4B-it-Q4_K_M.gguf artifact' }
    if ($modelId -eq 'Bolt' -and $file.Name -notmatch '(?i)bolt.*Q4_K_M\.gguf$') { throw 'Bolt arm requires a Bolt Q4_K_M GGUF artifact' }
    $pin = $pins[$modelId]
    if ($pin -and $file.Length -ne $pin.size_bytes) { throw "$modelId byte count does not match the pinned artifact" }
    $sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    if ($pin -and $sha256 -ine $pin.sha256) { throw "$modelId SHA256 does not match the pinned publisher revision; refusing substitution" }
    $artifacts += [pscustomobject]@{ id=$modelId; requested_artifact=$labels[$modelId]; path=$file.FullName;
        filename=$file.Name; size_bytes=$file.Length; sha256=$sha256; publisher_pin=$pin;
        pin_verified=[bool]$pin; provenance=$(if ($pin) { 'Exact bytes match pinned publisher revision' } else { 'Current local Qwen baseline; measured hash, no publisher pin' }) }
}

# Hash the project module, not just the common UnrealEditor launcher.
if (!$Packaged) {
    $moduleName = 'UnrealEditor-DirectorsCut.dll'
    $moduleManifest = Join-Path $ProjectRoot 'Binaries/Win64/UnrealEditor.modules'
    if (Test-Path -LiteralPath $moduleManifest -PathType Leaf) {
        $modules = Read-Text $moduleManifest | ConvertFrom-Json
        if ($modules.Modules.DirectorsCut) { $moduleName = [string]$modules.Modules.DirectorsCut }
    }
    $applicationFile = Get-LocalFile (Join-Path (Join-Path $ProjectRoot 'Binaries/Win64') $moduleName) 'Project game module DLL'
} elseif ($Executable -match '(?i)[\\/]Binaries[\\/]Win64[\\/][^\\/]+\.exe$') {
    $applicationFile = Get-LocalFile $Executable 'Packaged game executable'
} else {
    # A bootstrap executable is not the game module. Resolve the matching leaf or
    # require the caller to select it explicitly rather than guessing between builds.
    $packageRoot = Split-Path -Parent $Executable
    $leafName = Split-Path -Leaf $Executable
    $leafCandidates = @(
        (Join-Path $packageRoot "DirectorsCut/Binaries/Win64/$leafName"),
        (Join-Path $packageRoot "Binaries/Win64/$leafName")
    ) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf }
    if (@($leafCandidates).Count -ne 1) { throw 'Cannot identify packaged game leaf unambiguously; pass its Binaries/Win64 executable via -Executable' }
    $applicationFile = Get-LocalFile (@($leafCandidates)[0]) 'Packaged game executable'
}
$projectGitHead = $null
if (Get-Command git -ErrorAction SilentlyContinue) {
    try {
        $headOutput = @(& git -C $ProjectRoot rev-parse --verify HEAD 2>$null)
        if ($LASTEXITCODE -eq 0 -and $headOutput.Count) { $projectGitHead = [string]$headOutput[0] }
    } catch { $projectGitHead = $null }
}
$applicationBuild = @{ path=$applicationFile.FullName; size_bytes=$applicationFile.Length;
    sha256=(Get-FileHash -LiteralPath $applicationFile.FullName -Algorithm SHA256).Hash }
$runDirectory = Join-Path $OutputRoot ([DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss')+'_'+[Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
$summaryPath = Join-Path $runDirectory 'summary.json'
$summary = [ordered]@{ started_utc=[DateTime]::UtcNow.ToString('o'); executable=$Executable;
    executable_sha256=(Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash; project_root=$ProjectRoot;
    project_git_head=$projectGitHead; application_build=$applicationBuild;
    packaged=[bool]$Packaged; runtime_variant='ACE/GameAgentSDK-CUDA-separate-context'; artifacts=$artifacts;
    configuration=@{ context_tokens_expected_from_current_build=4096; thinking=$false; shared_cuda_graphics_context=$false;
        orders=@{ temperature=0; deadline_seconds=4; requested_token_cap=80 };
        acknowledgements=@{ temperature=.6; deadline_seconds=1.5; requested_token_cap=32 };
        conversation=@{ temperature=.65; deadline_seconds=6; requested_token_cap=96 };
        ace_limitations='No per-request token cap or JSON grammar; requested caps above are not enforced by ACE';
        sampling_defaults=@{ top_p=.8; top_k=20; min_p=0; repeat_penalty=1 } };
    quality_pass=$null; semantic_review_required=$true; phases=@(); complete=$false }
$phases = if ($Phase -eq 'Both') { @('Commands','Conversation') } else { @($Phase) }
$halt = $false
foreach ($model in $artifacts) {
    foreach ($selectedPhase in $phases) {
        if ($halt) {
            $summary.phases += @{ model=$model.id; phase=$selectedPhase; skipped=$true; reason='Earlier native fault/watchdog requires GPU review before continuing' }
        } else {
            $directory = Join-Path $runDirectory ($model.id+'-'+$selectedPhase)
            $result = Invoke-NativeReview $selectedPhase $model $directory
            $result | Add-Member -NotePropertyName model -NotePropertyValue $model.id
            $summary.phases += $result
            $halt = [bool]$result.halt_remaining
        }
        [IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 24))
    }
}
$summary.complete = $true
$summary.completed_utc = [DateTime]::UtcNow.ToString('o')
$failed = @($summary.phases | Where-Object { !$_.pipeline_pass }).Count
$summary.all_selected_pipelines_pass = $failed -eq 0
[IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 24))
Write-Output "Model comparison summary: $summaryPath"
if ($failed) { throw "$failed selected review phases failed or were skipped; inspect the summary. No quality approval is implied." }
