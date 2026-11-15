param([switch]$Wait)
$ErrorActionPreference='Stop'
# Native slot must be agreed before invoking this script. No assets are saved by the audition.
$busy=Get-CimInstance Win32_Process | Where-Object { $_.Name -match 'UnrealEditor|DirectorsCut' }
if($busy) { throw 'An engine job is active. Wait for the shared native slot; do not interrupt it.' }
& (Join-Path $PSScriptRoot 'review_maelstrom_sky.ps1') -Native -Wait:$Wait -ReviewName KrakenAudition -Shots '309,317,325,333,341,349,357' -Orders '' -Station starboard -Width 1600 -Height 900 -ExtraArgs '-DCKrakenAudition'
