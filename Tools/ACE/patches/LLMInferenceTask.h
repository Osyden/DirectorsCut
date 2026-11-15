// SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TimerHandle.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ace.h"
#include <string>
#include <atomic>
#include "LLMChatMessage.h"
#include "LLMInferenceTask.generated.h"

class ULLMToolCallList;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeneratedResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInferenceFailed, const FString&, Error);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTokenOutput, FString, CurrentToken, FString, CurrentString);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTokensCompleted);

class ULLMChat;
class ULLMInferenceOptions;

UCLASS()
class ULLMInferenceTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	// Event generated for each token
	UPROPERTY(BlueprintAssignable)
	FTokenOutput OnTokenOutput;

	// Event generated when all tokens have been produced (end of inference)
	UPROPERTY(BlueprintAssignable)
	FGeneratedResponse OnGeneratedResponse;

	UPROPERTY(BlueprintAssignable)
	FInferenceFailed OnInferenceFailed;

	// This function runs inference and outputs the response message in "OnGeneratedResponse"
	UFUNCTION(BlueprintCallable, Category = "ACE LLM|Functions", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Run LLM Chat"))
	static ULLMInferenceTask* RunLLMChat(UObject* WorldContextObject,
		                                 ULLMChat* ChatObj,
		                                 ULLMInferenceOptions* Options,
		                                 FString SystemPromptOverride);

	virtual void Activate() override;

private:

	int32 RunChatInference();        // producer thread
	int32 GetChatInferenceOutputs(); // consumer thread
	void OnBothThreadsComplete(int32 Result1, int32 Result2);

	UPROPERTY()
	TWeakObjectPtr<UObject> m_worldContextObject;
	bool     m_isActivated = false;

	// LLM Inference Params
	UPROPERTY() ULLMChat* m_chatObj = nullptr;
	UPROPERTY() ULLMInferenceOptions* m_infOptions = nullptr;
	FString m_systemPromptOverride;
	UPROPERTY() ULLMChatMessage* m_outputMessage = nullptr;
	std::atomic<bool> m_producerFinished{false};
	ACEChatMessage* m_nativeOutputMessage = nullptr;
	bool m_ownsInferenceGate = false;
	bool m_completed = false;
};
