param([string]$EngineRoot = 'E:/Utilities/UE_5.8')
$ErrorActionPreference = 'Stop'
# The installed pack includes two old render-target materials whose calls omit
# the newer leading wave-roughness argument. Keep the current ocean path intact;
# give only those legacy overloads the neutral wave value used by the helper.
$matches = @(Get-ChildItem -LiteralPath "$EngineRoot/Engine/Plugins/Marketplace" -Directory |
    ForEach-Object { Join-Path $_.FullName 'Shaders/Private/DRW_Roughness.usf' } |
    Where-Object { Test-Path -LiteralPath $_ })
if ($matches.Count -ne 1) { throw 'Expected exactly one installed Dynamic Real Water roughness helper.' }
$path = $matches[0]
$text = [IO.File]::ReadAllText($path)
$marker = '// DirectorsCut legacy render-target compatibility'
if ($text.Contains($marker)) { Write-Output 'DRW legacy calls already compatible.'; return }
$busy = @(Get-Process UnrealEditor,UnrealEditor-Cmd,DirectorsCut -ErrorAction SilentlyContinue)
if ($busy.Count) { throw 'Close Unreal rendering/cooking processes before patching the shared plugin shader.' }
if (!$text.Contains('float ComputeRoughness(float WaveRoughness,') -or !$text.Contains('float ComputeSpecular(float WaveRoughness,')) {
    throw 'Unexpected DRW API; inspect the installed pack before patching.'
}
$end = $text.LastIndexOf('};')
if ($end -lt 0) { throw 'DRW helper has no struct terminator.' }
$shim = @'
    // DirectorsCut legacy render-target compatibility
    float ComputeRoughness(float3 WorldPosition, float3 Normal, float RoughnessStrength)
    {
        return ComputeRoughness(0.5, WorldPosition, Normal, RoughnessStrength);
    }
    float ComputeSpecular(float3 WorldPosition, float3 Normal, float3 HalfwayVector, float LightDiffuse, float HalfwayDiffuse, float ViewDiffuse, float RoughnessStrength, float SpecularStrength, float Anisotropy)
    {
        return ComputeSpecular(0.5, WorldPosition, Normal, HalfwayVector, LightDiffuse, HalfwayDiffuse, ViewDiffuse, RoughnessStrength, SpecularStrength, Anisotropy);
    }

'@
$backup = "$path.dc_legacy_backup"
if (!(Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $path -Destination $backup }
[IO.File]::WriteAllText($path, $text.Insert($end, $shim), [Text.UTF8Encoding]::new($false))
Write-Output "Patched installed DRW legacy calls; original retained at $backup"
