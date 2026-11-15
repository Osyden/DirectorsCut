param([Parameter(Mandatory=$true)][string]$SourceRoot, [Parameter(Mandatory=$true)][string]$GameRoot, [switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
if ($game -eq $source -or $game.StartsWith($source + [IO.Path]::DirectorySeparatorChar)) { throw 'Stage into a separate packaged game directory.' }
$target = Join-Path $game 'Sidecars'
if (Test-Path -LiteralPath $target) { throw 'Stage sidecars only into a fresh candidate; never overwrite an existing candidate.' }
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$config = Get-Content -LiteralPath (Join-Path $project 'Config/DefaultGame.ini') -Raw
$voiceSection = [regex]::Match($config, '(?ms)^\[/Script/DirectorsCut\.DCVoice\]\s*\r?\n(?<settings>.*?)(?=^\[|\z)').Groups['settings'].Value
$selectedModel = 'Qwen3.5-4B-Q5_K_M.gguf'
$modelSetting = [regex]::Match($voiceSection, '(?m)^LLMModel=([^\r\n]+)')
if ($modelSetting.Success) { $selectedModel = $modelSetting.Groups[1].Value.Trim() }
if ($selectedModel -notmatch '^[A-Za-z0-9_.-]+\.gguf$') { throw 'LLMModel must be a local GGUF filename.' }
foreach ($model in @($selectedModel, 'ggml-base.en.bin')) {
    if (!(Test-Path -LiteralPath (Join-Path $source "models/$model") -PathType Leaf)) { throw "Selected model is missing: $model" }
}
if ($selectedModel -eq 'gemma-4-E4B-it-Q4_K_M.gguf') {
    $gemma = Get-Item -LiteralPath (Join-Path $source "models/$selectedModel")
    if ($gemma.Length -ne 4977171584 -or (Get-FileHash -LiteralPath $gemma.FullName -Algorithm SHA256).Hash -ne '85A896A047553E842F25297EE5B031D64FF30147D9C4AF17B1E4B394CD1FAB87') {
        throw 'Gemma E4B does not match the pinned Q4_K_M artifact.'
    }
}
if ($voiceSection -match '(?m)^TTSBackend=Phonon\s*$') {
    $phonon = [IO.Path]::GetFullPath((Join-Path $source 'bin/phonon'))
    function Get-PhononFile([string]$Root, [string]$Relative) {
        # Refuse traversal, alternate streams and junction/symlink escapes, including
        # manifest paths. Never hash or copy a manifest-selected external file.
        if (!$Relative -or [IO.Path]::IsPathRooted($Relative) -or $Relative -match '[:\\]' -or
            @($Relative.Split('/') | Where-Object { $_ -in @('', '.', '..') }).Count) {
            throw "Invalid Phonon relative path: $Relative"
        }
        $path = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
        if (!$path.StartsWith($Root.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Phonon path escapes payload: $Relative"
        }
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Selected Phonon payload is missing: $Relative" }
        $item = Get-Item -LiteralPath $path
        while ($item -and $item.FullName.Length -ge $phonon.Length) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Phonon reparse points are not supported: $Relative" }
            if ($item -is [IO.FileInfo]) { $item = $item.Directory } else { $item = $item.Parent }
        }
        return [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    }
    function Test-PhononArtifact([string]$Root, [string]$Relative, $Record) {
        $path = Get-PhononFile $Root $Relative
        if ([string]$Record.sha256 -notmatch '^[a-fA-F0-9]{64}$' -or
            [string]$Record.bytes -notmatch '^(0|[1-9][0-9]*)$') { throw "Invalid Phonon artifact metadata: $Relative" }
        if ((Get-Item -LiteralPath $path).Length -ne [long]$Record.bytes -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $Record.sha256) {
            throw "Phonon artifact checksum/length mismatch: $Relative"
        }
    }
    foreach ($part in @('serve.py','voices.json','staging.json','THIRD-PARTY-NOTICES.md',
        'licenses/LICENSE-APACHE','licenses/LICENSE-MIT','python/payload-manifest.json',
        'python/import-verification.json','python/python313._pth','python/THIRD-PARTY-NOTICES.txt',
        'python/licenses/PYTHON-LICENSE.txt','python/licenses/PYTHON-COPYRIGHT.txt')) {
        $null = Get-PhononFile $phonon $part
    }
    $stage = Get-Content -LiteralPath (Join-Path $phonon 'staging.json') -Raw | ConvertFrom-Json
    if ($stage.schema_version -ne 2 -or $stage.runtime.kind -ne 'portable-cpu-python' -or
        $stage.model -ne 'gradium/phonon-31m-en' -or $stage.revision -ne '92741c5b07d085ff840e26fea8ce43110b87852f') {
        throw 'Phonon staging metadata is stale or unsupported; refresh using stage.py --metadata-only.'
    }
    foreach ($name in @('config.json','tokenizer.json','model.q8.gguf','default-voice.safetensors')) {
        Test-PhononArtifact $phonon "model/$name" $stage.model_files.$name
    }
    $voicePath = Join-Path $phonon 'voices.json'
    if ((Get-FileHash -LiteralPath $voicePath).Hash -ne (Get-FileHash -LiteralPath (Join-Path $project 'Tools/Phonon/voices.json')).Hash) {
        throw 'Phonon voice manifest differs from the pinned project manifest.'
    }
    $voices = Get-Content -LiteralPath $voicePath -Raw | ConvertFrom-Json
    foreach ($speaker in @('Rex','Duncan','Ruby')) {
        $voice = $voices.crew.$speaker
        if ($voice.filename -notmatch '^voices/[A-Za-z0-9_-]+\.safetensors$') { throw "Invalid Phonon voice path: $speaker" }
        Test-PhononArtifact $phonon $voice.filename $voice
    }
    $python = Join-Path $phonon 'python'
    $runtime = Get-Content -LiteralPath (Join-Path $python 'payload-manifest.json') -Raw | ConvertFrom-Json
    if ($runtime.schema_version -ne 1 -or $runtime.ptts -ne '0.7.1' -or $runtime.numpy -ne '2.5.4' -or
        $runtime.python_version -notmatch '^3\.13\.' -or $runtime.cpu_only_adapter -ne $true -or !$runtime.files -or
        @($runtime.files).Count -ne $runtime.copied_files) {
        throw 'Unsupported Phonon portable runtime manifest.'
    }
    $paths = @{}
    foreach ($file in $runtime.files) {
        if ($paths.ContainsKey([string]$file.path)) { throw "Duplicate Phonon runtime file: $($file.path)" }
        Test-PhononArtifact $python $file.path $file
        $paths[[string]$file.path] = $true
    }
    foreach ($required in @('python.exe','python3.dll','python313.dll','Lib/encodings/__init__.py',
        'Lib/site-packages/ptts/__init__.py','Lib/site-packages/ptts/_ptts.pyd',
        'Lib/site-packages/ptts-0.7.1.dist-info/licenses/LICENSE-APACHE',
        'Lib/site-packages/ptts-0.7.1.dist-info/licenses/LICENSE-MIT',
        'Lib/site-packages/ptts-0.7.1.dist-info/sboms/ptts-pyo3.cyclonedx.json',
        'Lib/site-packages/numpy/__init__.py',
        'Lib/site-packages/numpy/_core/_multiarray_umath.cp313-win_amd64.pyd',
        'Lib/site-packages/numpy-2.5.4.dist-info/licenses/LICENSE.txt',
        'vcruntime140.dll','vcruntime140_1.dll','msvcp140.dll','msvcp140_1.dll',
        'msvcp140_2.dll','msvcp140_atomic_wait.dll','msvcp140_codecvt_ids.dll')) {
        if (!$paths.ContainsKey($required)) { throw "Phonon runtime manifest omits required file: $required" }
    }
    if (!@($paths.Keys | Where-Object { $_ -match '^Lib/site-packages/numpy\.libs/libscipy_openblas.*\.dll$' }).Count) {
        throw 'Phonon runtime manifest omits bundled NumPy libraries.'
    }
    $pth = @(Get-Content -LiteralPath (Join-Path $python 'python313._pth'))
    if (($pth -join "`n") -cne "Lib`nDLLs`nLib/site-packages") { throw 'Phonon Python import isolation differs from the approved portable layout.' }
    $probe = Get-Content -LiteralPath (Join-Path $python 'import-verification.json') -Raw | ConvertFrom-Json
    if ($probe.passed -ne $true -or $probe.isolated -ne 1 -or $probe.no_site -ne 1 -or
        $probe.ptts -ne '0.7.1' -or $probe.numpy -ne '2.5.4') {
        throw 'Phonon portable import verification is missing or failed; run stage_python.py --verify-only.'
    }
}
if ($ValidateOnly) { Write-Output 'Sidecar prerequisite validation passed; no files copied.'; return }
New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'bin') -Destination $target -Recurse
New-Item -ItemType Directory -Path (Join-Path $target 'models/voices') -Force | Out-Null
foreach ($model in @($selectedModel,'ggml-base.en.bin')) {
    Copy-Item -LiteralPath (Join-Path $source "models/$model") -Destination (Join-Path $target 'models')
}
# Do not redistribute the unused Alan model: its linked upstream licence is unclear.
foreach ($voice in @('en_GB-vctk-medium','en_GB-northern_english_male-medium','en_GB-alba-medium')) {
    foreach ($suffix in @('.onnx','.onnx.json')) {
        Copy-Item -LiteralPath (Join-Path $source "models/voices/$voice$suffix") -Destination (Join-Path $target 'models/voices')
    }
}
$notices = Join-Path $game 'ThirdPartyNotices'
New-Item -ItemType Directory -Force -Path $notices | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'Docs/ThirdPartyVoices') -Destination $notices -Recurse
if (Test-Path -LiteralPath (Join-Path $project 'Docs/ThirdPartyACELLM')) {
    Copy-Item -LiteralPath (Join-Path $project 'Docs/ThirdPartyACELLM') -Destination $notices -Recurse
}
if ($selectedModel -eq 'gemma-4-E4B-it-Q4_K_M.gguf') {
    Copy-Item -LiteralPath (Join-Path $project 'Docs/ThirdPartyGemma') -Destination $notices -Recurse
}
foreach ($plugin in @('NV_ACE_Reference','NvAudio2FaceMarkRegressive')) {
    $destination = Join-Path $notices $plugin
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project "Plugins/$plugin") -File | Where-Object { $_.Name -match 'licen[cs]e' } | Copy-Item -Destination $destination
}
& (Join-Path $PSScriptRoot 'prune_unused_gpt_model.ps1') -GameRoot $game
Write-Output "Staged sidecars and notices at $game"
