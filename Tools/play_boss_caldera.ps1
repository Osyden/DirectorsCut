param([switch]$FullRun,[float]$Start=306,[switch]$Mute)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
if(!(Test-Path -LiteralPath (Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-DirectorsCut.dll')))
{ throw 'Build the isolated DirectorsCutEditor target before launching this boss test.' }
$binary=Get-Item -LiteralPath (Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-DirectorsCut.dll')
$newest=Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Source/DirectorsCut') -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if($newest.LastWriteTime -gt $binary.LastWriteTime)
{ throw 'Boss source has changed since the build. Rebuild before playing; the old binary uses the superseded attack plan.' }
& (Join-Path $PSScriptRoot 'play_complete_maelstrom.ps1') -FullRun:$FullRun -Start $Start -Mute:$Mute
