# Capture the real HUD and assignment states without speech/GPU inference.
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out = Join-Path $root 'Saved/CrewUI'
New-Item -ItemType Directory -Force -Path $out | Out-Null
foreach ($case in @(
    @{ Name='cannons'; Width=1280; Height=720; Orders='Duncan,_hold_fire' },
    @{ Name='duties-small'; Width=888; Height=500; Orders='Duncan,_repair_the_rigging;Ruby,_put_out_the_fire' }
)) {
    & (Join-Path $PSScriptRoot 'require_idle_gpu.ps1')
    $log = Join-Path $out ($case.Name + '.log')
    $started = [DateTime]::UtcNow
    $launch = @((Join-Path $root 'DirectorsCut.uproject'),'/Game/DC/Maps/L_Ride',
        '-game','-unattended','-RenderOffscreen','-windowed','-forceres','-nosound','-nosplash',
        '-DCNoVoice','-DCNoACE','-DCSkipIntro','-DCRunThrough','-DCAutoShots=2,14',
        "-DCOrders=$($case.Orders)","-ResX=$($case.Width)","-ResY=$($case.Height)",
        "-ExecCmds=r.SetRes $($case.Width)x$($case.Height)w,t.MaxFPS 30","-abslog=$log")
    $quoted = ($launch | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $proc = Start-Process 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $quoted -WindowStyle Hidden -PassThru
    Write-Output "HUD $($case.Name) PID=$($proc.Id)"
    if (!$proc.WaitForExit(90000)) { Stop-Process -InputObject $proc -Force; throw "HUD capture timed out: $log" }
    $runtime = Get-Content -LiteralPath $log -Raw
    if ($proc.ExitCode -ne 0 -or $runtime -match 'GPU timeout|Fatal error|portrait missing|running without a ship') {
        throw "HUD capture failed: $log"
    }
    foreach ($time in @('0002','0014')) {
        $shot = Join-Path $root "Saved/Screenshots/Auto/shot_$time.png"
        if (!(Test-Path $shot) -or (Get-Item $shot).LastWriteTimeUtc -lt $started) { throw "Missing fresh capture: $shot" }
        Copy-Item -LiteralPath $shot -Destination (Join-Path $out "$($case.Name)-$time.png")
    }
    Write-Output "Inspect $out/$($case.Name)-0014.png"
}
