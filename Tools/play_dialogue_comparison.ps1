param([ValidateSet('ACE','LlamaCpp')][string]$Backend = 'ACE')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$marker = Join-Path $root 'Saved/Release/ACEComparison.json'
if (!(Test-Path -LiteralPath $marker)) { throw 'No validated dialogue comparison package is selected yet.' }
$release = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
if (!(Test-Path -LiteralPath $release.Executable -PathType Leaf)) { throw "Packaged game is missing: $($release.Executable)" }
& (Join-Path $PSScriptRoot 'ACE/require_idle_gpu.ps1')
$arguments = @('-Windowed','-ForceRes','-ResX=1600','-ResY=900','-NoSplash',
    '-useCIGforAI=0','-DCEnableACE','-DCLLMPort=8797',"-DCDialogueBackend=$Backend")
Start-Process -FilePath $release.Executable -WorkingDirectory (Split-Path -Parent $release.Executable) -ArgumentList $arguments -WindowStyle Normal
