#include "DCVoice.h"
#include "DCACEDialogue.h"
#include "DCDlgSSEStream.h"
#include "DCLanguageModelResponse.h"
#include "DCTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Async/Async.h"

namespace DCDialogueRoutingPrivate
{
    // Raw bytes are the only state touched on the HTTP thread. Parsing, callbacks,
    // weak UObject access and request-list mutation stay on the game thread.
    struct FHTTPDialogueStream
    {
        FCriticalSection ByteLock;
        TArray<uint8> Bytes;
        int32 ReceivedBytes=0;
        bool bOverflow=false;
        TAtomic<bool> bCancelled{false}, bDrainQueued{false};
        FDCDlgSSEStream Parser;
        int32 StatusCode=0;
        bool bFinished=false, bFailed=false;
        double Deadline=0;
        FString LastProgress;
        TFunction<bool()> IsCurrent;
        TFunction<void()> Cleanup;
        TFunction<void(const FString&)> Progress;
        TFunction<void(bool,const FString&)> Completion;

        bool Receive(const void* Data,int64 Length)
        {
            FScopeLock Guard(&ByteLock);
            if (bCancelled.Load()) { return false; }
            if (Length<0 || Length>FDCDlgSSEStream::MaxResponseBytes-ReceivedBytes)
            { bOverflow=true; return false; }
            if (Length) { Bytes.Append(static_cast<const uint8*>(Data),int32(Length)); ReceivedBytes+=int32(Length); }
            return true;
        }

        void Cancel()
        {
            check(IsInGameThread());
            bFinished=true; bCancelled.Store(true);
            Progress=nullptr; Completion=nullptr;
            { FScopeLock Guard(&ByteLock); Bytes.Reset(); }
            if (Cleanup) { Cleanup(); }
        }

        void Drain()
        {
            check(IsInGameThread());
            if (bFinished) { return; }
            if (!IsCurrent()) { Cancel(); return; }
            if (FPlatformTime::Seconds()>Deadline) { bFailed=true; Progress=nullptr; }
            if (StatusCode!=200) { return; }
            TArray<uint8> Received;
            {
                FScopeLock Guard(&ByteLock);
                Received=MoveTemp(Bytes); Bytes.Reset();
                bFailed|=bOverflow;
            }
            if (bFailed || !Parser.Append(Received.GetData(),Received.Num()))
            { bFailed=true; Progress=nullptr; return; }
            if (Progress && !Parser.GetText().IsEmpty() && Parser.GetText()!=LastProgress)
            {
                LastProgress=Parser.GetText();
                auto Notify=Progress;
                Notify(LastProgress);
                if (!IsCurrent()) { Cancel(); }
            }
        }

        void Finish(bool bTransportOK,int32 ResponseCode)
        {
            check(IsInGameThread());
            if (bFinished) { return; }
            if (!IsCurrent()) { Cancel(); return; }
            StatusCode=ResponseCode;
            if (!bTransportOK || StatusCode!=200) { bFailed=true; Progress=nullptr; }
            // Drain any bytes whose queued task has not run, so last progress always
            // precedes completion. Late drain tasks see bFinished and do nothing.
            Drain();
            if (bFinished) { return; }
            const bool bOK=bTransportOK && StatusCode==200 && !bFailed && Parser.Finish();
            bFinished=true; bCancelled.Store(true); Progress=nullptr;
            auto Notify=MoveTemp(Completion);
            if (Cleanup) { Cleanup(); }
            if (Notify) { Notify(bOK,bOK ? Parser.GetText() : FString()); }
        }
    };
}

void ADCVoice::CancelStreamingDialogueRequests()
{
    check(IsInGameThread());
    // Detach first: cancellation may synchronously trigger completion callbacks.
    auto Requests=MoveTemp(StreamingDialogueRequests);
    StreamingDialogueRequests.Reset();
    for (const auto& Request:Requests) { if (Request) { Request->CancelRequest(); } }
}

void ADCVoice::InitializeDialogueBackend()
{
    FParse::Value(FCommandLine::Get(), TEXT("DCLLMPort="), LLMPort);
    FParse::Value(FCommandLine::Get(), TEXT("DCDialogueBackend="), DialogueBackend);
    bUseACEDialogue = DialogueBackend.Equals(TEXT("ACE"), ESearchCase::IgnoreCase);
    if (!bUseACEDialogue && !DialogueBackend.Equals(TEXT("LlamaCpp"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogDC, Warning, TEXT("Dialogue: unknown backend '%s'; using LlamaCpp"), *DialogueBackend);
        DialogueBackend = TEXT("LlamaCpp");
    }
    UE_LOG(LogDC, Display, TEXT("DIALOGUE_BACKEND selected=%s enabled=%d"), bUseACEDialogue ? TEXT("ACE") : TEXT("LlamaCpp"), bUseLLM);
    if (!bUseLLM || !bUseACEDialogue) { return; }
    int32 SharedContext = 0;
    FParse::Value(FCommandLine::Get(), TEXT("useCIGforAI="), SharedContext);
    if (SharedContext != 0)
    {
        UE_LOG(LogDC, Error, TEXT("Dialogue: refusing ACE shared CUDA/graphics context override"));
        bUseLLM = false;
        return;
    }
    FString ModelPath = SidecarDir() / TEXT("models") / LLMModel;
    FParse::Value(FCommandLine::Get(), TEXT("DCACEModel="), ModelPath);
    ACEDialogue = NewObject<UDCACEDialogue>(this);
    ACEDialogue->RegisterComponent();
    ACEDialogue->Initialize(ModelPath);
}

void ADCVoice::RequestDialogue(const FString& System, const FString& User, int32 MaxTokens, float Temperature,
    bool bJson, float Timeout, TFunction<void(bool, const FString&)> Completion, TFunction<void(const FString&)> Progress)
{
    check(IsInGameThread());
    // Structured orders must be fully validated before they can execute.
    if (bJson) { Progress=nullptr; }
    if (bUseACEDialogue)
    {
        if (!ACEDialogue) { Completion(false, FString()); return; }
        // The ACE Chat API has no max_tokens/JSON-grammar option. Bound waiting at the
        // adapter and validate its response schema before any game action is executed.
        ACEDialogue->Submit(System, User, Temperature, Timeout, MoveTemp(Completion), MoveTemp(Progress));
        return;
    }
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Messages;
    auto AddMessage = [&Messages](const TCHAR* MessageRole, const FString& Text)
    {
        TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
        Message->SetStringField(TEXT("role"), MessageRole);
        Message->SetStringField(TEXT("content"), Text);
        Messages.Add(MakeShared<FJsonValueObject>(Message));
    };
    AddMessage(TEXT("system"), System);
    AddMessage(TEXT("user"), User);
    Root->SetArrayField(TEXT("messages"), Messages);
    Root->SetNumberField(TEXT("max_tokens"), MaxTokens);
    Root->SetNumberField(TEXT("temperature"), Temperature);
    if (Progress) { Root->SetBoolField(TEXT("stream"),true); }
    TSharedRef<FJsonObject> Kwargs = MakeShared<FJsonObject>();
    Kwargs->SetBoolField(TEXT("enable_thinking"), false);
    Root->SetObjectField(TEXT("chat_template_kwargs"), Kwargs);
    if (bJson)
    {
        TSharedRef<FJsonObject> Format = MakeShared<FJsonObject>();
        Format->SetStringField(TEXT("type"), TEXT("json_object"));
        Root->SetObjectField(TEXT("response_format"), Format);
    }
    FString Body;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Body));
    auto Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("http://127.0.0.1:%d/v1/chat/completions"), LLMPort));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Body);
    Request->SetTimeout(Timeout);
    if (Progress)
    {
        using namespace DCDialogueRoutingPrivate;
        auto State=MakeShared<FHTTPDialogueStream,ESPMode::ThreadSafe>();
        State->Deadline=FPlatformTime::Seconds()+FMath::Max(.1f,Timeout);
        State->Progress=MoveTemp(Progress); State->Completion=MoveTemp(Completion);
        TWeakObjectPtr<ADCVoice> Weak(this);
        TWeakPtr<IHttpRequest,ESPMode::ThreadSafe> WeakRequest(Request);
        const int32 Epoch=SpeechEpoch;
        State->IsCurrent=[Weak,WeakRequest,Epoch]
        {
            ADCVoice* Voice=Weak.Get();
            return Voice && !Voice->bEnding && Voice->SpeechEpoch==Epoch && Voice->StreamingDialogueRequests.Contains(WeakRequest.Pin());
        };
        State->Cleanup=[Weak,WeakRequest]
        {
            if (ADCVoice* Voice=Weak.Get()) { Voice->StreamingDialogueRequests.Remove(WeakRequest.Pin()); }
        };
        StreamingDialogueRequests.Add(Request);
        Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
        Request->SetHeader(TEXT("Accept"),TEXT("text/event-stream"));
        Request->OnStatusCodeReceived().BindLambda([State](FHttpRequestPtr,int32 Code)
        { State->StatusCode=Code; State->Drain(); });
        const bool bStreaming=Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda(
            [State](void* Data,int64& Length)
            {
                if (!State->Receive(Data,Length)) { Length=0; }
                if (!State->bDrainQueued.Exchange(true))
                {
                    AsyncTask(ENamedThreads::GameThread,[State]
                    { State->bDrainQueued.Store(false); State->Drain(); });
                }
            }));
        Request->OnProcessRequestComplete().BindLambda([State](FHttpRequestPtr,FHttpResponsePtr Response,bool bOK)
        { State->Finish(bOK,Response ? Response->GetResponseCode() : 0); });
        if (!bStreaming || !Request->ProcessRequest()) { State->Finish(false,0); }
        return;
    }
    Request->OnProcessRequestComplete().BindLambda([Completion = MoveTemp(Completion)](FHttpRequestPtr, FHttpResponsePtr Response, bool bOK)
    {
        FString Text;
        const bool bValid = bOK && Response.IsValid() && Response->GetResponseCode() == 200 &&
            DCLanguageModelResponse::ExtractChatContent(Response->GetContentAsString(), Text);
        Completion(bValid, Text);
    });
    Request->ProcessRequest();
}
