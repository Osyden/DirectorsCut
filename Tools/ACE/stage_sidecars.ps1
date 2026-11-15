param([Parameter(Mandatory=$true)][string]$SourceRoot, [Parameter(Mandatory=$true)][string]$GameRoot)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
if ($game -eq $source -or $game.StartsWith($source + [IO.Path]::DirectorySeparatorChar)) { throw 'Stage into a separate packaged game directory.' }
$target = Join-Path $game 'Sidecars'
if (Test-Path -LiteralPath $target) { throw 'Stage sidecars only into a fresh candidate; never overwrite an existing candidate.' }
New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'bin') -Destination $target -Recurse
New-Item -ItemType Directory -Path (Join-Path $target 'models/voices') -Force | Out-Null
foreach ($model in @('Qwen3.5-4B-Q5_K_M.gguf','ggml-base.en.bin')) {
    Copy-Item -LiteralPath (Join-Path $source "models/$model") -Destination (Join-Path $target 'models')
}
# Do not redistribute the unused Alan model: its linked upstream licence is unclear.
foreach ($voice in @('en_GB-vctk-medium','en_GB-northern_english_male-medium','en_GB-alba-medium')) {
    foreach ($suffix in @('.onnx','.onnx.json')) {
        Copy-Item -LiteralPath (Join-Path $source "models/voices/$voice$suffix") -Destination (Join-Path $target 'models/voices')
    }
}
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$notices = Join-Path $game 'ThirdPartyNotices'
New-Item -ItemType Directory -Force -Path $notices | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'Docs/ThirdPartyVoices') -Destination $notices -Recurse
if (Test-Path -LiteralPath (Join-Path $project 'Docs/ThirdPartyACELLM')) {
    Copy-Item -LiteralPath (Join-Path $project 'Docs/ThirdPartyACELLM') -Destination $notices -Recurse
}
foreach ($plugin in @('NV_ACE_Reference','NvAudio2FaceMarkRegressive')) {
    $destination = Join-Path $notices $plugin
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project "Plugins/$plugin") -File | Where-Object { $_.Name -match 'licen[cs]e' } | Copy-Item -Destination $destination
}
Write-Output "Staged sidecars and notices at $game"
