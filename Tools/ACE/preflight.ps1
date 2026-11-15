[CmdletBinding()]
param([string]$EngineRoot = 'E:\Utilities\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$pluginsRoot = Join-Path $projectRoot 'Plugins'
$pluginFiles = @(Get-ChildItem -LiteralPath $pluginsRoot -Filter '*.uplugin' -Recurse |
    Where-Object { $_.Name -match 'ACE|Audio2Face' })
$plugins = @($pluginFiles | ForEach-Object {
    $descriptor = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
    [pscustomobject]@{
        Name = $_.BaseName
        Version = $descriptor.VersionName
        EngineVersion = $descriptor.EngineVersion
        Path = $_.FullName
        Modules = @($descriptor.Modules | ForEach-Object { $_.Name })
    }
})
$rendererConfig = Get-Content -LiteralPath (Join-Path $projectRoot 'Config/DefaultEngine.ini') -Raw
$requiredSettings = [ordered]@{
    'r.GPUSkin.Support16BitBoneIndex' = 'True'
    'r.GPUSkin.UnlimitedBoneInfluences' = 'True'
    'SkeletalMesh.UseExperimentalChunking' = '1'
}
$missingSettings = @($requiredSettings.Keys | Where-Object {
    $rendererConfig -notmatch ('(?im)^\s*' + [regex]::Escape($_) + '\s*=\s*' + $requiredSettings[$_] + '\s*$')
})
$report = [ordered]@{
    RecordedAtUtc = [DateTime]::UtcNow.ToString('o')
    ProjectRoot = $projectRoot
    EngineBuild = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
    EditorAvailable = Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe')
    GPU = @(& nvidia-smi --query-gpu=name,memory.total,memory.used,driver_version --format=csv,noheader)
    Plugins = $plugins
    ACEPluginPresent = [bool]($plugins | Where-Object { $_.Name -eq 'NV_ACE_Reference' })
    LocalModelPluginPresent = [bool]($plugins | Where-Object { $_.Name -like 'NvAudio2Face*' })
    FixturesPresent = Test-Path -LiteralPath (Join-Path $projectRoot 'Saved/ACE/Fixtures/manifest.json')
    MissingMetaHumanSkinningSettings = $missingSettings
    Note = 'Inventory only. Plugin presence does not prove UE 5.8 compilation, inference, facial rig, or packaging compatibility.'
}
$output = Join-Path $projectRoot 'Saved/ACE'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'preflight.json') -Encoding utf8
$report | ConvertTo-Json -Depth 8
