param([switch]$Boss)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$marker = Join-Path $root 'Saved/Release/Current.json'
if (!(Test-Path -LiteralPath $marker)) { throw 'No tested local package selected. See taskboard/FINAL-INTEGRATION-15NOV.md.' }
$release = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
if (!(Test-Path -LiteralPath $release.Executable -PathType Leaf)) { throw "Packaged game is missing: $($release.Executable)" }
$arguments = @('-Windowed','-ForceRes','-ResX=1600','-ResY=900','-NoSplash','-useCIGforAI=0','-ExecCmds="r.Shadow.Virtual.AllowScreenOverflowMessages 0"')
if ($Boss) { $arguments += @('-DCStartAt=306','-DCStation=helm') }
Start-Process -FilePath $release.Executable -WorkingDirectory (Split-Path -Parent $release.Executable) -ArgumentList $arguments -WindowStyle Normal
