# Uses a separately built runtime project; never saves or changes content assets.
param(
    [Parameter(Mandatory=$true)][string]$ProjectPath,
    [ValidateSet('rag','cannons','encounters','statue','city','fogband','crew','default','exit','navigation','bow','bowclose','bowstorm','bowstormclose','bowdeck','bowrough','bowroughclose','bowparticles','bowdetail','bowroughdeck','bowlookdown','bowtransition','manual')][string]$Case = 'encounters',
    [string]$EngineRoot = 'E:\Utilities\UE_5.8',
    [string]$OutputPath = ''
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectPath).Path
$projectFolder = Split-Path -Parent $project
if (!$OutputPath) { $OutputPath = Join-Path $projectFolder "Saved\EarlyGameplayReview\$Case" }
$output = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$running = Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" | Where-Object { $_.CommandLine -and $_.CommandLine.Contains($project) }
if ($running) { throw "The selected runtime project is already open. Finish that capture before starting another." }
$options = switch ($Case) {
    'rag' { '-DCEarlyVerify -DCSkipIntro -DCRunThrough -DCTenths -DCAutoShots=2.9,3.1,3.8,4.0,4.7,4.9,5.4,9,12,20' }
    'cannons' { '-DCCannonVerify' }
    'encounters' { '-DCEncounterVerify -DCStartAt=90 -DCRunThrough -DCOrders=Hold_fire -DCAutoShots=101,108,119,121,123,126,129,135,146,175,208,225,235,236 -DCShotCam=-2200,0,1200,7,55 -DCGunLog' }
    'city' { '-DCRunThrough -DCOrders=Hold_fire -DCAutoShots=34,55,100,110,125,150,175,205,235 -DCShotCam=-2200,0,1200,5,-55 -DCLaneLog -DCCurrentLog' }
    'fogband' { '-DCRunThrough -DCOrders=Hold_fire -DCAutoShots=100,110,125 -DCShotCam=-2200,0,1200,5,-55' }
    'statue' { '-DCStartAt=112 -DCRunThrough -DCStation=starboard -DCTrackThreat -DCOrders=Hold_fire -DCTenths -DCAutoShots=118,122,126,128,130.2,130.8,131.2,132,140' }
    'crew' { '-DCBot -DCRunThrough -DCAutoShots=100,125,175,235 -DCLaneLog -DCCurrentLog -DCGunLog' }
    'default' { '-DCRunThrough -DCAutoShots=100,110,125,175,235 -DCLaneLog -DCCurrentLog -DCGunLog' }
    'exit' { '-DCRunThrough -DCAutoShots=210,225,230,234,236,239,242,246 -DCLaneLog -DCCurrentLog' }
    'navigation' { '-DCNavigationVerify -DCRunThrough' }
    'bow' { '-DCStartAt=20 -DCRunThrough -DCShipView -DCOrbit=145,5 -DCAutoShots=23,25,28,32' }
    'bowclose' { '-DCStartAt=20 -DCRunThrough -DCAutoShots=23,25,28,32 -DCShotCam=3200,-1000,650,-18,132' }
    'bowstorm' { '-DCStartAt=45 -DCRunThrough -DCShipView -DCOrbit=145,5 -DCAutoShots=50,53,56,59 -DCBowLog' }
    'bowstormclose' { '-DCStartAt=45 -DCRunThrough -DCAutoShots=50,53,56,59 -DCShotCam=2500,-1400,850,-18,132 -DCBowLog' }
    'bowdeck' { '-DCStartAt=45 -DCRunThrough -DCAutoShots=50,53,56,59 -DCShotCam=1000,-350,1000,-8,0 -DCBowLog' }
    'bowrough' { '-DCStartAt=90 -DCRunThrough -DCShipView -DCOrbit=145,5 -DCAutoShots=95,98,101,104 -DCBowLog' }
    'bowroughclose' { '-DCStartAt=90 -DCRunThrough -DCAutoShots=95,98,101,104 -DCShotCam=2500,-1400,850,-18,132 -DCBowLog' }
    'bowparticles' { '-DCStartAt=90 -DCRunThrough -DCAutoShots=95,98,101,104 -DCShotCam=2500,-1400,850,-18,132 -DCBowLog -DCBowParticleAudit' }
    'bowdetail' { '-DCStartAt=90 -DCRunThrough -DCTenths -DCAutoShots=98.6,98.8,99,99.2,100.5,101 -DCShotCam=1700,-600,550,-10,132 -DCBowLog -DCBowParticleAudit' }
    'bowroughdeck' { '-DCStartAt=90 -DCRunThrough -DCAutoShots=95,98,101,104 -DCShotCam=1000,-350,1000,-8,0 -DCBowLog' }
    'bowlookdown' { '-DCStartAt=90 -DCRunThrough -DCAutoShots=95,98,101,104 -DCShotCam=650,-450,900,-42,-35 -DCBowLog' }
    'bowtransition' { '-DCStartAt=235 -DCRunThrough -DCShipView -DCOrbit=145,5 -DCAutoShots=239,241,243,246 -DCBowLog' }
    'manual' { '-DCStartAt=90 -DCRunThrough -DCStation=helm -DCHelmSequence -DCAutoShots=94,98,102,106 -DCLaneLog -DCCurrentLog' }
}
$log = Join-Path $output 'runtime.log'
$started = Get-Date
$arguments = '"' + $project + '" /Game/DC/Maps/L_Ride -game -RenderOffscreen -Unattended -NoSound -DCNoVoice -NoSplash -Windowed -ResX=1280 -ResY=720 ' + $options + ' -AbsLog="' + $log + '"'
$run = Start-Process -FilePath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output "Review $Case started as process $($run.Id); log: $log"
if (!$run.WaitForExit(420000)) {
    Stop-Process -Id $run.Id -ErrorAction SilentlyContinue
    throw "This review exceeded seven minutes; stopped only its own process. Inspect $log."
}
$shots = Join-Path $projectFolder 'Saved\Screenshots\Auto'
Get-ChildItem -LiteralPath $shots -Filter 'shot_*.png' | Where-Object { $_.LastWriteTime -ge $started } | Copy-Item -Destination $output
$lines = Get-Content -LiteralPath $log
$lines | Select-String 'LogDC:.*(VERIFY|VERIFICATION|Rag contact:|Encounter |Sentinel |Hull struck|Navigation:|Navigation plan:|Bow |Auto shot|Capture state:)' | ForEach-Object { $_.Line } | Set-Content -LiteralPath (Join-Path $output 'results.txt')
if ($run.ExitCode -ne 0 -or $lines -match 'VERIFICATION FAILED|VERIFY FAIL') { throw "Review failed; inspect $log." }
if ($Case -in @('rag','cannons','encounters','navigation') -and !($lines -match 'VERIFICATION PASSED')) { throw 'The verification did not reach its success marker.' }
Write-Output "Review completed: $output. Inspect the screenshots before accepting visual quality."
