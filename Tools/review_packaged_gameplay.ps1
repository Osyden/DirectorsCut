# Exercise the actual packaged Development executable, with isolated user settings/screenshots.
param(
    [Parameter(Mandatory=$true)][string]$ExecutablePath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [ValidateSet('rag','encounters','navigation','bow','fog','route')][string]$Case = 'rag'
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $ExecutablePath).Path
$gameRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $exe))
if ([IO.Path]::GetFileName($exe) -ne 'DirectorsCut.exe' -or !(Test-Path -LiteralPath (Join-Path $gameRoot 'Content\Paks'))) {
    throw 'Use the packaged DirectorsCut/Binaries/Win64/DirectorsCut.exe, with its cooked Content/Paks directory.'
}
$output = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $output) { throw 'Use a new review directory so stale evidence cannot pass.' }
$running = Get-CimInstance Win32_Process -Filter "Name = 'DirectorsCut.exe'" | Where-Object { $_.ExecutablePath -eq $exe }
if ($running) { throw 'This packaged executable is already running.' }
$options = switch ($Case) {
    'rag' { '-DCEarlyVerify -DCSkipIntro -DCRunThrough -DCTenths -DCAutoShots=2.9,3.1,3.8,4.0,4.7,4.9,5.4,9,12,20' }
    'navigation' { '-DCNavigationVerify -DCRunThrough' }
    'encounters' { '-DCEncounterVerify -DCStartAt=90 -DCRunThrough -DCOrders=Hold_fire -DCAutoShots=101,108,119,121,123,126,129,135,146,175,208,225,235,236 -DCShotCam=-2200,0,1200,7,55 -DCGunLog' }
    'bow' { '-DCStartAt=90 -DCRunThrough -DCTenths -DCAutoShots=98.6,98.8,99,99.2,100.5,101 -DCShotCam=1700,-600,550,-10,132 -DCBowLog -DCBowParticleAudit' }
    'fog' { '-DCRunThrough -DCOrders=Hold_fire -DCAutoShots=100,110,125 -DCShotCam=-2200,0,1200,5,-55' }
    'route' { '-DCRunThrough -DCAutoShots=34,55,100,125,175,210,236,239,246 -DCLaneLog -DCCurrentLog' }
}
New-Item -ItemType Directory -Path $output | Out-Null
$log = Join-Path $output 'runtime.log'
$arguments = '/Game/DC/Maps/L_Ride -RenderOffscreen -Unattended -NoSound -DCNoVoice -NoSplash -Windowed -ResX=1280 -ResY=720 -UserDir="' + $output + '\user" -AbsLog="' + $log + '" ' + $options
$run = Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe) -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output "Packaged $Case review started as process $($run.Id); log: $log"
if (!$run.WaitForExit(420000)) {
    Stop-Process -Id $run.Id -ErrorAction SilentlyContinue
    throw 'Review exceeded seven minutes; stopped only its own process.'
}
$lines = Get-Content -LiteralPath $log
$lines | Select-String 'LogDC:.*(VERIFY|VERIFICATION|Rag contact:|Encounter |Sentinel |Hull struck|Navigation:|Navigation plan:|Bow |Auto shot|Capture state:)' | ForEach-Object { $_.Line } | Set-Content -LiteralPath (Join-Path $output 'results.txt')
$shots = Join-Path $output 'user\Saved\Screenshots\Auto'
if (Test-Path -LiteralPath $shots) { Get-ChildItem -LiteralPath $shots -Filter 'shot_*.png' | Copy-Item -Destination $output }
if ($run.ExitCode -ne 0 -or $lines -match 'VERIFICATION FAILED|VERIFY FAIL|Fatal error|Assertion failed|Blueprint Runtime Error') { throw "Packaged review failed; inspect $log" }
if ($Case -in @('rag','encounters','navigation') -and !($lines -match 'VERIFICATION PASSED')) { throw 'Packaged verification did not reach success.' }
if ($Case -ne 'navigation' -and !(Get-ChildItem -LiteralPath $output -Filter 'shot_*.png')) { throw 'Review produced no screenshots.' }
Write-Output "Packaged review completed: $output. Inspect screenshots and missing-asset warnings before accepting. Voice/audio were disabled for this gameplay check."
