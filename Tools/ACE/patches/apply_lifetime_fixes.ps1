# Invoked by setup_llm_plugin.ps1 with its checked source destination/Patch-Exact.
# Refuse unknown revisions; normalized hashes permit harmless CRLF/LF changes.
function Install-ReviewedSource([string]$Relative, [string]$OriginalHash, [string]$PreviousHash) {
    $path = Join-Path $destination $Relative
    $patchPath = Join-Path $PSScriptRoot ([IO.Path]::GetFileName($Relative))
    $current = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
    $reviewed = [IO.File]::ReadAllText($patchPath).Replace("`r`n", "`n")
    if ($current -ceq $reviewed) { return }
    $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($current))).ToLowerInvariant()
    if ($hash -ne $OriginalHash -and $hash -ne $PreviousHash) { throw "Unrecognized source revision in $Relative; refusing to overwrite it." }
    [IO.File]::WriteAllText($path, $reviewed)
}
Install-ReviewedSource 'Source/LLMPlugin/Private/LLMInferenceTask.cpp' '3b414ae975bfd49bc543d86cc0e52d0e2fd7a7f137ab4523a939839459f10e0c' 'f3101c2d7b7cec84e3552f2eb1c8f0d030c8f1689f8291373bfab389a3cb4841'
Install-ReviewedSource 'Source/LLMPlugin/Public/LLMInferenceTask.h' '53ab0b5e4e0415d66783a9ee5ee92a9344cf620557e0e385ba0399a482d3f4b4' '794bf536ccac5b37a078cc0f34ae2025412ef4431eba568adc614eefdeff24e1'
Install-ReviewedSource 'Source/LLMPlugin/Private/LLMModule.cpp' 'f1a24d994c8e459be8e5243eb8bad65bb44e38536235e41cd3d3968af76e6ee1' '4ad67f62bec531dc0d67d00f0c297d6eb5e8128b74ab791513becbc40b4c1d98'

# Rooted transient data must not retain a stopped PIE world's Outer chain.
Patch-Exact 'Source/LLMPlugin/Private/LLMChat.cpp' 'UObject* Outer = WorldContextObject;' 'UObject* Outer = GetTransientPackage(); // Inference can outlive its original world.'
Patch-Exact 'Source/LLMPlugin/Private/LLMChat.cpp' 'ULLMTool::CreateTool(WorldContextObject, Def.Name' 'ULLMTool::CreateTool(Obj, Def.Name'
Patch-Exact 'Source/LLMPlugin/Private/LLMInferenceOptions.cpp' 'UObject* Outer = WorldContextObject;' 'UObject* Outer = GetTransientPackage(); // Owned by component/task UPROPERTY references.'
Patch-Exact 'Source/LLMPlugin/Private/LLMChat.cpp' @'
void ULLMChat::DeleteAllMessages()
{
'@ @'
void ULLMChat::DeleteAllMessages()
{
    m_latestResponse = nullptr; // Failed inference must never replay a previous response.
'@
Patch-Exact 'Source/LLMPlugin/Public/ACELLMComponent.h' 'void HandleGeneratedResponse();' @'
void HandleGeneratedResponse();

    UFUNCTION()
    void HandleInferenceFailed(const FString& Error);
'@
Patch-Exact 'Source/LLMPlugin/Private/ACELLMComponent.cpp' 'Task->OnGeneratedResponse.AddDynamic(this, &UACELLMComponent::HandleGeneratedResponse);' @'
Task->OnGeneratedResponse.AddDynamic(this, &UACELLMComponent::HandleGeneratedResponse);
    Task->OnInferenceFailed.AddDynamic(this, &UACELLMComponent::HandleInferenceFailed);
'@
Patch-Exact 'Source/LLMPlugin/Private/ACELLMComponent.cpp' 'void UACELLMComponent::HandleGeneratedResponse()' @'
void UACELLMComponent::HandleInferenceFailed(const FString& Error)
{
    bBusy = false;
    ActiveOptions = nullptr;
    if (bComponentReady) { OnLLMError.Broadcast(Error); }
}

void UACELLMComponent::HandleGeneratedResponse()
'@
