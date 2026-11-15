# Install the official ACE LLM v0.5 source plugin and reproduce the local safety fixes.
# NVIDIA download: https://developer.download.nvidia.com/assets/ace/ue-plugins/ACE_LLM_5.7_v0.5.zip
param(
    [string]$SourceRoot = 'E:/Projects/DirectorsCut/Downloads/ACE_LLM_v05/ACE_LLM_5.7_v0.5',
    [string]$ProjectRoot = ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))),
    [switch]$ApplyPatchesOnly
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$project = (Resolve-Path -LiteralPath $ProjectRoot).Path
$destination = Join-Path $project 'Plugins/LLMPlugin'
if (!(Test-Path -LiteralPath (Join-Path $source 'LLMPlugin.uplugin'))) { throw 'Expected the extracted official LLMPlugin source.' }
if (!$ApplyPatchesOnly) {
    if (Test-Path -LiteralPath $destination) { throw "Destination already exists; inspect it instead of overwriting local changes: $destination" }
    New-Item -ItemType Directory -Path $destination | Out-Null
    # Keep the vendor SDK DLLs under ThirdParty, but rebuild Unreal modules for this engine.
    & robocopy $source $destination /E /XD (Join-Path $source 'Binaries') (Join-Path $source 'Intermediate') /R:1 /W:1 /NFL /NDL /NJH /NJS /NP
    if ($LASTEXITCODE -ge 8) { throw "Plugin copy failed: $LASTEXITCODE" }
}
elseif (!(Test-Path -LiteralPath (Join-Path $destination 'LLMPlugin.uplugin'))) { throw 'No existing source plugin to patch.' }

function Patch-Exact([string]$Relative, [string]$Before, [string]$After) {
    $path = Join-Path $destination $Relative
    $text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
    $Before = $Before.Replace("`r`n", "`n")
    $After = $After.Replace("`r`n", "`n")
    if ($text.Contains($After)) { return }
    if (!$text.Contains($Before)) { throw "Unexpected upstream source in $Relative; inspect before changing it." }
    [IO.File]::WriteAllText($path, $text.Replace($Before, $After))
}

$descriptorPath = Join-Path $destination 'LLMPlugin.uplugin'
$descriptor = Get-Content -LiteralPath $descriptorPath -Raw | ConvertFrom-Json
$descriptor.PSObject.Properties.Remove('EngineVersion')
$descriptor.Installed = $false
$descriptor | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $descriptorPath -Encoding utf8NoBOM

Patch-Exact 'Source/LLMPlugin/Public/LLMTypes.h' @'
	CPU UMETA(DisplayName = "CPU")
'@ @'
	CPU UMETA(DisplayName = "Legacy no-graphics handoff"),
	// CUDA backend selected by the SDK, without CUDA/D3D graphics interop.
	CUDA UMETA(DisplayName = "CUDA (Separate Context)")
'@
Patch-Exact 'Source/LLMPlugin/Public/LLMSettings.h' 'EACELLMBackend PreferredBackend = EACELLMBackend::RTX;' 'EACELLMBackend PreferredBackend = EACELLMBackend::CUDA;'
Patch-Exact 'Source/LLMPlugin/Private/LLMEngine.cpp' 'case EACELLMBackend::RTX: return TEXT("RTX (CIG)");' @'
case EACELLMBackend::CUDA: return TEXT("CUDA (no graphics interop)");
        case EACELLMBackend::RTX: return TEXT("RTX (CIG)");
'@
Patch-Exact 'Source/LLMPlugin/Private/LLMEngine.cpp' 'const EACELLMBackend PreferredBackend = LLMSettings ? LLMSettings->PreferredBackend : EACELLMBackend::RTX;' @'
// Director's Cut: shared CUDA/D3D initialization previously hung this workstation.
    // Force None graphics handoff: the SDK still selects CUDA and offloads all
    // model layers, but never receives Unreal's D3D device or command queue.
    // Upstream incorrectly labels this handoff CPU; runtime GPU-offload logs
    // remain the source of truth. Never silently attempt CIG as a fallback.
    const EACELLMBackend PreferredBackend = EACELLMBackend::CUDA;
    if (LLMSettings && LLMSettings->PreferredBackend != PreferredBackend)
    {
        UE_LOG(LogACELLM, Warning, TEXT("[Engine] Graphics interop disabled by Director's Cut crash safeguard; using separate CUDA context."));
    }
'@
Patch-Exact 'Source/LLMPlugin/Private/LLMEngine.cpp' @'
    switch (PreferredBackend)
    {
'@ @'
    switch (PreferredBackend)
    {
    case EACELLMBackend::CUDA:
        BackendChain.Add(EACELLMBackend::CUDA);
        break;
'@
Patch-Exact 'Source/LLMPlugin/Private/LLMEngine.cpp' 'EACELLMBackend ActiveBackend = EACELLMBackend::CPU;' 'EACELLMBackend ActiveBackend = EACELLMBackend::CUDA;'
Patch-Exact 'Source/LLMPlugin/Private/LLMEngine.cpp' 'TEXT("[Engine] Stage 4: backend CPU (ACEDeviceType_None)")' 'TEXT("[Engine] Stage 4: separate CUDA context, ACEDeviceType_None, no D3D device/queue; verify GPU layer offload in SDK log")'

. (Join-Path $PSScriptRoot 'patches/apply_lifetime_fixes.ps1')

# Chat keeps native objects alive; keep their UE wrappers alive as well.
Patch-Exact 'Source/LLMPlugin/Public/LLMChat.h' 'TArray<ULLMChatMessage*> m_msgArray{};' 'UPROPERTY() TArray<ULLMChatMessage*> m_msgArray;'
Patch-Exact 'Source/LLMPlugin/Public/LLMChat.h' 'TArray<ULLMTool*>        m_toolArray{};' 'UPROPERTY() TArray<ULLMTool*> m_toolArray;'
Patch-Exact 'Source/LLMPlugin/Public/LLMChat.h' 'ULLMChatMessage*         m_latestResponse{};' 'UPROPERTY() ULLMChatMessage* m_latestResponse = nullptr;'

# The game already stages the identical comparison model in Sidecars. Do not
# ship a second unused quantization just because the vendor sample bundles it.
Patch-Exact 'Source/LLMPlugin/LLMPlugin.Build.cs' '"Qwen3.5-4B-Q4_K_S.gguf"' '// Director''s Cut stages its selected Q5_K_M model through stage_sidecars.ps1.'

Write-Output "Installed patched ACE LLM source at $destination. Build and verify GPU layer-offload logs before accepting it."
Write-Output 'The plugin uses NVIDIA Game Agent SDK with its bundled CUDA/llama runtime; no cloud account or API key is required.'
