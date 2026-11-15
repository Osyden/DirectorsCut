#include "DCVoice.h"
#include "DCACEDialogue.h"
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
    bool bJson, float Timeout, TFunction<void(bool, const FString&)> Completion)
{
    if (bUseACEDialogue)
    {
        if (!ACEDialogue) { Completion(false, FString()); return; }
        // The ACE Chat API has no max_tokens/JSON-grammar option. Bound waiting at the
        // adapter and validate its response schema before any game action is executed.
        ACEDialogue->Submit(System, User, Temperature, Timeout, MoveTemp(Completion));
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
    Request->OnProcessRequestComplete().BindLambda([Completion = MoveTemp(Completion)](FHttpRequestPtr, FHttpResponsePtr Response, bool bOK)
    {
        FString Text;
        const bool bValid = bOK && Response.IsValid() && Response->GetResponseCode() == 200 &&
            DCLanguageModelResponse::ExtractChatContent(Response->GetContentAsString(), Text);
        Completion(bValid, Text);
    });
    Request->ProcessRequest();
}
