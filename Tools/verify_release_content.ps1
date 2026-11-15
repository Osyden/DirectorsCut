param(
    [Parameter(Mandatory=$true)][string]$ProjectRoot,
    [string]$EngineRoot = 'E:\Utilities\UE_5.8'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path
$branding = Get-Content -LiteralPath (Join-Path $root 'Config/DefaultGame.ini') -Raw
$gameTarget = Get-Content -LiteralPath (Join-Path $root 'Source/DirectorsCut.Target.cs') -Raw
if ($branding -notmatch '(?m)^ProjectName=Atlantean Odyssey\s*$' -or
    $branding -notmatch '(?m)^ProjectDisplayedTitle=.*Atlantean Odyssey' -or
    !$gameTarget.Contains('OutputFile = "Binaries/Win64/Atlantean Odyssey.exe";')) {
    throw 'Atlantean Odyssey window/product branding or Win64 executable target path is missing.'
}
function Assert-SteeringWaterPatch([string]$ProjectDirectory, [string]$EngineDirectory) {
    $manifestPath = Join-Path $ProjectDirectory 'Saved/OceanSteeringPatch/manifest.json'
    $repair = 'Run Tools/patch_drw_steering.py --backup, then its native generation with -AllowCommandletRendering before cooking. Preserve the resulting Saved/OceanSteeringPatch manifest in the selected project.'
    if (!(Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Missing steering water patch manifest. $repair" }
    $patch = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($patch.schema -ne 'dc.ocean-steering.v1') { throw "Unsupported steering water patch manifest. $repair" }
    $coverage = @{'Core/MF_GetUV'=1; 'MF_GetWPO'=2; 'MF_GetBaseNormal'=1; 'MF_GetHQFoam'=2}
    foreach ($function in $coverage.Keys) {
        $rows = @($patch.audit | Where-Object { $_.function -eq $function })
        if ($rows.Count -ne 1 -or $rows[0].nodes -ne $coverage[$function] -or
            $rows[0].status -notin @('patched','already patched')) {
            throw "Incomplete steering water patch coverage: $function. $repair"
        }
    }
    # Check the actual installation used for cooking, not the backup paths in
    # the manifest. A restored plugin must not pass just because evidence exists.
    $pluginContent = Join-Path $EngineDirectory 'Engine/Plugins/Marketplace/DynamicR1d9938e98fdbV5/Content'
    $paths = @($coverage.Keys | ForEach-Object { "Materials/MaterialFunctions/$_.uasset" })
    $paths += @('High','Med','Low','Far','UltraLow','Billboard') | ForEach-Object { "Materials/OceanLOD/M_DRW_$_.uasset" }
    foreach ($relative in $paths) {
        $entry = $patch.files.PSObject.Properties[$relative]
        $installed = Join-Path $pluginContent $relative
        if (!$entry -or $entry.Value.patched_sha256 -notmatch '^[a-fA-F0-9]{64}$' -or
            !(Test-Path -LiteralPath $installed -PathType Leaf)) {
            throw "Missing installed steering water asset or completed hash: $relative. $repair"
        }
        if ((Get-FileHash -LiteralPath $installed -Algorithm SHA256).Hash -ne $entry.Value.patched_sha256) {
            throw "Steering water asset differs from the verified patch: $relative. Restore/reconcile vendor changes and regenerate before cooking."
        }
    }
    Write-Output 'Steering water preflight passed (four patched functions, six ocean LOD materials, installed SHA-256 verified).'
}
Assert-SteeringWaterPatch -ProjectDirectory $root -EngineDirectory $EngineRoot
$project = Get-Content -LiteralPath (Join-Path $root 'DirectorsCut.uproject') -Raw | ConvertFrom-Json
if ($project.Plugins | Where-Object { $_.Name -eq 'LLMPlugin' -and $_.Enabled }) {
    foreach ($relative in @('Plugins/LLMPlugin/LLMPlugin.uplugin','Plugins/LLMPlugin/ThirdParty/bin/Win64/ACE.SDK.dll')) {
        if (!(Test-Path -LiteralPath (Join-Path $root $relative))) { throw "Missing ACE LLM dependency: $relative; run Tools/ACE/setup_llm_plugin.ps1" }
    }
    $engineSource = Get-Content -LiteralPath (Join-Path $root 'Plugins/LLMPlugin/Source/LLMPlugin/Private/LLMEngine.cpp') -Raw
    if (!$engineSource.Contains('const EACELLMBackend PreferredBackend = EACELLMBackend::CUDA;')) {
        throw 'ACE LLM separate-CUDA patch is missing. Apply Tools/ACE/setup_llm_plugin.ps1 before packaging.'
    }
}
# These dynamically loaded assets are essential to the voyage, but licensed
# source content is intentionally absent from Git-only checkouts.
$required = @(
    'DC/Maps/L_Ride.umap',
    'DC/Music/TheFog.uasset',
    'DC/Music/TheCaldera.uasset',
    'DC/UI/T_AtlanteanOdysseyLogo.uasset',
    'DC/UI/T_AtlanteanOdysseyLogoV2.uasset',
    'DC/UI/T_AtlanteanMenuPanel.uasset',
    'DC/UI/Fonts/Cinzel.ttf',
    'DC/UI/Fonts/OFL.txt',
    'DC/Materials/M_StormComposite.uasset',
    'DC/Materials/M_CombatCue.uasset',
    'DC/Materials/M_BossWeakPoint.uasset',
    'DC/Materials/M_BossLimbHidden.uasset',
    'DC/FX/P_PillarWaterContact.uasset',
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
$required += @('Kraken/Meshes/KRAKEN.uasset','DC/Boss/M_BossSkin.uasset','DC/Boss/M_BossTell.uasset')
$required += @('idle','smashAttackForward','smashAttack','getHit','death') | ForEach-Object { "Kraken/Animations/KRAKEN_$_.uasset" }
$required += @('T_CliffDetail_a79918b3ceff','T_CliffDetail_e869505bbbf6','T_Geology_context_albedo_5ff6f0d8c2b5','T_Geology_context_normal_5ce5b638d7f5','T_Geology_context_roughness_a505faf00c2f','T_Geology_pilot_albedo_b73dd29458a9','T_Geology_pilot_normal_c4de7e55716d','T_Geology_pilot_roughness_1d40b5361b81') | ForEach-Object { "LocalScans/cliff_geology/$_.uasset" }
$required += @('T_NordicFace_BaseColor_06937f5f3f','T_NordicFace_NormalGL_4561c80152','T_NordicFace_Roughness_6e79049729') | ForEach-Object { "LocalScans/nordic_face_tile/$_.uasset" }
$runtime = Join-Path $root 'Content/DC/Maelstrom/GeneratedCliffPreview/Runtime'
$arena = Get-Content -LiteralPath (Join-Path $runtime 'CompleteArena.json') -Raw | ConvertFrom-Json
$import = Get-Content -LiteralPath (Join-Path $runtime 'CliffLandformPreviewImport.json') -Raw | ConvertFrom-Json
if (!$arena.normal_game_enabled -or !$arena.caldera_lake -or $arena.cliff_fbx_sha256 -ne $import.source_sha256 -or $arena.source_candidate_sha256 -ne $import.source_candidate_sha256) {
    throw 'Caldera runtime marker does not match the installed active terrain import.'
}
foreach ($assetPath in @($import.asset,$import.material,$arena.material)) {
    $required += ($assetPath.Split('.')[0] -replace '^/Game/','') + '.uasset'
}
foreach ($runtimeFile in @('CliffPreviewSurface.bin','CliffPreviewSupports.json','CliffPreviewSourceAudit.json','CalderaShoreMist.json')) {
    if (!(Test-Path -LiteralPath (Join-Path $runtime $runtimeFile))) { throw "Missing caldera runtime file: $runtimeFile" }
}
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
