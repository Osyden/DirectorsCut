param([Parameter(Mandatory=$true)][string]$SourceRoot, [Parameter(Mandatory=$true)][string]$GameRoot)
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
    foreach ($part in @('python/python.exe','python/python313.dll','serve.py','voices.json','model/config.json','model/model.q8.gguf','staging.json')) {
        if (!(Test-Path -LiteralPath (Join-Path $source "bin/phonon/$part") -PathType Leaf)) { throw "Selected Phonon runtime is missing: $part" }
    }
}
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
