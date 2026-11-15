param([Parameter(Mandatory=$true)][string]$ProjectRoot)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path
# These dynamically loaded assets are essential to the voyage, but licensed
# source content is intentionally absent from Git-only checkouts.
$required = @(
    'DC/Maps/L_Ride.umap',
    'Brigantine/BP/BP_Brigantine.uasset',
    'Brigantine/SFX/Cue/Bell_cue.uasset',
    'UltraDynamicSky/Blueprints/Ultra_Dynamic_Sky.uasset',
    'UltraDynamicSky/Blueprints/Ultra_Dynamic_Weather.uasset',
    'AncientRuins/Meshes/Columns/SM_Colonnade_01.uasset',
    'Fab/Claw_Kaiju_Monster/ClawKaiju_allAnimation.uasset',
    'MetaHumans/Common/Common/Mocap/mh_arkit_mapping_pose.uasset'
)
$required += @('Captain','Finch','Marlow','Odessa') | ForEach-Object { "PirateCast/Assembled/$_/BP_$_.uasset" }
$required += @('Rex','Duncan','Ruby') | ForEach-Object { "DC/UI/T_Crew_$_.uasset" }
$missing = @()
foreach ($asset in $required) {
    $file = Join-Path $root "Content/$asset"
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { $missing += $asset; continue }
    $stream = [IO.File]::OpenRead($file)
    try {
        $head = New-Object byte[] 128
        $count = $stream.Read($head, 0, $head.Length)
        if ($count -lt 32 -or [Text.Encoding]::ASCII.GetString($head,0,$count).StartsWith('version https://git-lfs.github.com/spec/')) {
            $missing += "$asset (empty or unresolved LFS pointer)"
        }
    } finally { $stream.Dispose() }
}
$config = Get-Content -LiteralPath (Join-Path $root 'Config/DefaultGame.ini') -Raw
foreach ($match in [regex]::Matches($config, '(?m)^\+DirectoriesToAlwaysCook=\(Path="/Game/([^"]+)"\)')) {
    $relative = $match.Groups[1].Value
    $directory = Join-Path $root "Content/$relative"
    if (!(Test-Path -LiteralPath $directory -PathType Container) -or
        !(Get-ChildItem -LiteralPath $directory -Recurse -File -ErrorAction Stop | Where-Object { $_.Extension -in @('.uasset','.umap') } | Select-Object -First 1)) {
        $missing += "$relative (configured cook directory has no assets)"
    }
}
if ($missing.Count) { throw ("Release content is incomplete; install the local licensed assets before packaging:`n" + ($missing -join "`n")) }
Write-Output "Release content preflight passed ($($required.Count) essential assets and configured game cook directories). Runtime scene review is still required."
