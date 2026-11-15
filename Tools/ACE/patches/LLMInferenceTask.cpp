// SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
// Director's Cut: serialized native work, explicit errors and independent task lifetime.

#include "LLMInferenceTask.h"
#include "LLMChat.h"
#include "LLMModule.h"
#include "LLMInferenceOptions.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "HAL/PlatformProcess.h"
#include "HAL/ThreadSafeCounter.h"
#include "Modules/ModuleManager.h"

namespace
{
    std::atomic<bool> GACELLMInferenceBusy{false};
    std::atomic<bool> GACELLMShuttingDown{false};
    FThreadSafeCounter GACELLMNativeWorkers;
}

// Called on pre-exit and again at module shutdown before the SDK is destroyed.
// The worker counter drops once completion is enqueued, without waiting on it.
void ShutdownACELLMInferenceTasks()
{
    check(IsInGameThread());
    GACELLMShuttingDown.store(true);
    if (GACELLMNativeWorkers.GetValue() > 0)
    {
        UE_LOG(LogACELLM, Log, TEXT("[Task] Waiting for native ACE inference before teardown"));
        while (GACELLMNativeWorkers.GetValue() > 0) { FPlatformProcess::Sleep(0.01f); }
    }
    if (FTaskGraphInterface::IsRunning())
    {
        FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
    }
}

ULLMInferenceTask* ULLMInferenceTask::RunLLMChat(UObject* WorldContextObject, ULLMChat* ChatObj,
    ULLMInferenceOptions* Options, FString SystemPromptOverride)
{
    check(IsInGameThread());
    LLMModule& Module = FModuleManager::GetModuleChecked<LLMModule>(LLMModule::GetModuleName());
    if (!Module.GetContext() || !Module.GetModel() || !ChatObj || !Options ||
        !ChatObj->GetNativeObject() || !Options->GetNativeObject() || GACELLMShuttingDown.load())
    {
        return nullptr;
    }
    ULLMInferenceTask* Task = NewObject<ULLMInferenceTask>();
    Task->RegisterWithGameInstance(WorldContextObject);
    Task->m_worldContextObject = WorldContextObject;
    Task->m_chatObj = ChatObj;
    Task->m_infOptions = Options;
    Task->m_systemPromptOverride = SystemPromptOverride;
    return Task;
}

void ULLMInferenceTask::Activate()
{
    check(IsInGameThread());
    if (m_isActivated) { return; }
    m_isActivated = true;
    Super::Activate();
    // GI registration alone ends when PIE/GI ends. Root transient task/data
    // independently; its world context is weak so this cannot retain a world.
    AddToRoot();
    bool Expected = false;
    if (GACELLMShuttingDown.load() || !GACELLMInferenceBusy.compare_exchange_strong(Expected, true))
    {
        OnBothThreadsComplete(static_cast<int32>(ACEResultInvalidState), 0);
        return;
    }
    m_ownsInferenceGate = true;
    m_chatObj->SetLatestResponse(nullptr);
    GACELLMNativeWorkers.Increment();
    TFuture<int32> Producer = Async(EAsyncExecution::Thread, [this]
    {
        const int32 Result = RunChatInference();
        m_producerFinished.store(true);
        return Result;
    });
    TFuture<int32> Consumer = Async(EAsyncExecution::Thread, [this] { return GetChatInferenceOutputs(); });
    Async(EAsyncExecution::ThreadPool, [this, Producer = MoveTemp(Producer), Consumer = MoveTemp(Consumer)]() mutable
    {
        const int32 ProducerResult = Producer.Get();
        const int32 ConsumerResult = Consumer.Get();
        AsyncTask(ENamedThreads::GameThread, [this, ProducerResult, ConsumerResult]
        {
            OnBothThreadsComplete(ProducerResult, ConsumerResult);
        });
        GACELLMNativeWorkers.Decrement();
    });
}

void ULLMInferenceTask::OnBothThreadsComplete(int32 ProducerResult, int32 ConsumerResult)
{
    check(IsInGameThread());
    m_completed = true;
    // Release admission only after BOTH producer and event consumer have stopped.
    // Release before callbacks, which may synchronously submit another request.
    if (m_ownsInferenceGate)
    {
        m_ownsInferenceGate = false;
        GACELLMInferenceBusy.store(false);
    }
    const bool bSuccess = ProducerResult == 0 && ConsumerResult == 0 && m_nativeOutputMessage;
    if (bSuccess && !GACELLMShuttingDown.load() && m_worldContextObject.IsValid())
    {
        ACEChatRole Role{};
        const char* Content = nullptr;
        ace_chatMessageGetRole(m_nativeOutputMessage, &Role);
        ace_chatMessageGetContent(m_nativeOutputMessage, &Content);
        m_outputMessage = ULLMChatMessage::CreateChatMessage(this, static_cast<ELLMChatRole>(Role),
            Content ? UTF8_TO_TCHAR(Content) : TEXT(""));
        m_outputMessage->SetNativeObject(m_nativeOutputMessage);
        m_nativeOutputMessage = nullptr;
        m_outputMessage->BuildToolCallList(m_outputMessage);
        m_chatObj->SetLatestResponse(m_outputMessage);
        if (m_chatObj->IsChatHistoryEnabled()) { m_chatObj->AddMessage(m_outputMessage); }
        OnGeneratedResponse.Broadcast();
    }
    else if (!GACELLMShuttingDown.load() && m_worldContextObject.IsValid())
    {
        const FString Error = FString::Printf(TEXT("ACE inference failed or global model is busy (producer=%d consumer=%d)."),
            ProducerResult, ConsumerResult);
        UE_LOG(LogACELLM, Warning, TEXT("%s"), *Error);
        OnInferenceFailed.Broadcast(Error);
    }
    if (m_nativeOutputMessage)
    {
        ace_destroyChatMessage(m_nativeOutputMessage);
        m_nativeOutputMessage = nullptr;
    }
    m_chatObj = nullptr;
    m_infOptions = nullptr;
    m_outputMessage = nullptr;
    SetReadyToDestroy();
    RemoveFromRoot();
}

int32 ULLMInferenceTask::RunChatInference()
{
    LLMModule& Module = FModuleManager::GetModuleChecked<LLMModule>(LLMModule::GetModuleName());
    return static_cast<int32>(ace_Model_Chat(Module.GetModel(), m_chatObj->GetNativeObject(),
        m_infOptions->GetNativeObject(), &m_nativeOutputMessage));
}

int32 ULLMInferenceTask::GetChatInferenceOutputs()
{
    LLMModule& Module = FModuleManager::GetModuleChecked<LLMModule>(LLMModule::GetModuleName());
    TWeakObjectPtr<ULLMInferenceTask> WeakThis(this);
    std::string Accumulated;
    for (;;)
    {
        ACETokenEvent Event{};
        const ACEResult Result = ace_Model_GetNextEvent(Module.GetModel(), &Event, 100);
        if (Result == ACEResultWaitTimeout)
        {
            if (m_producerFinished.load()) { return 0; }
            continue;
        }
        if (Result != ACEResultOk) { return static_cast<int32>(Result); }
        switch (Event.eventType)
        {
        case ACEEventType_Begin:
            Accumulated.clear();
            break;
        case ACEEventType_Data:
        {
            Accumulated += Event.tokenData;
            const FString Token = UTF8_TO_TCHAR(Event.tokenData);
            const FString Partial = UTF8_TO_TCHAR(Accumulated.c_str());
            AsyncTask(ENamedThreads::GameThread, [WeakThis, Token, Partial]
            {
                if (WeakThis.IsValid() && !WeakThis->m_completed && !GACELLMShuttingDown.load())
                {
                    WeakThis->OnTokenOutput.Broadcast(Token, Partial);
                }
            });
            break;
        }
        case ACEEventType_End:
            return 0;
        default:
            break;
        }
    }
}
