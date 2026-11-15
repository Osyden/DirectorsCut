#include "DCDlgSSEStream.h"
#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace DCDlgSSEStreamTestsPrivate
{
    FString ContentEvent(const FString& Text)
    {
        const auto Delta=MakeShared<FJsonObject>(); Delta->SetStringField(TEXT("content"),Text);
        const auto Choice=MakeShared<FJsonObject>(); Choice->SetNumberField(TEXT("index"),0); Choice->SetObjectField(TEXT("delta"),Delta);
        const auto Object=MakeShared<FJsonObject>(); Object->SetArrayField(TEXT("choices"),{MakeShared<FJsonValueObject>(Choice)});
        FString Json; FJsonSerializer::Serialize(Object.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        // SSE data is one line, independently of the JSON writer's pretty-print mode.
        Json.ReplaceInline(TEXT("\r"),TEXT("")); Json.ReplaceInline(TEXT("\n"),TEXT(""));
        return TEXT("data: ")+Json+TEXT("\n\n");
    }

    bool Feed(FDCDlgSSEStream& Stream,const FString& Text)
    {
        const FTCHARToUTF8 Bytes(*Text);
        return Stream.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCDlgSSEStreamCoverage,"DirectorsCut.Dialogue.SSEStreaming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDCDlgSSEStreamCoverage::RunTest(const FString&)
{
    using namespace DCDlgSSEStreamTestsPrivate;
    const FString First=TEXT("Café, Captain. ");
    const FString Second=TEXT("The sails are sound.");
    const FString Role=TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{\"role\":\"assistant\",\"content\":null}}]}\n\n");
    const FString Done=TEXT("data: [DONE]\n\n");
    const FString Response=TEXT(": keepalive\nid: 7\n\n")+Role+ContentEvent(First)+ContentEvent(Second)+
        TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n")+
        TEXT("data: {\"choices\":[],\"usage\":{\"completion_tokens\":10}}\n\n")+Done;
    FDCDlgSSEStream Bytewise;
    const FTCHARToUTF8 Wire(*Response);
    bool bAllAccepted=true;
    for (int32 Index=0;Index<Wire.Length();++Index)
    { bAllAccepted &= Bytewise.Append(reinterpret_cast<const uint8*>(Wire.Get()+Index),1); }
    TestTrue(TEXT("Every byte accepted, including split UTF-8 and SSE delimiters"),bAllAccepted);
    TestTrue(TEXT("Bytewise stream completes with DONE"),Bytewise.Finish());
    TestEqual(TEXT("Exact cumulative Unicode text, no role or metadata"),Bytewise.GetText(),First+Second);

    const FString Event=ContentEvent(First);
    const FTCHARToUTF8 EventWire(*Event);
    FDCDlgSSEStream Incomplete;
    TestTrue(TEXT("Accept incomplete frame"),Incomplete.Append(reinterpret_cast<const uint8*>(EventWire.Get()),EventWire.Length()-1));
    TestTrue(TEXT("No partial JSON spoken before frame completion"),Incomplete.GetText().IsEmpty());
    TestTrue(TEXT("Complete final delimiter"),Incomplete.Append(reinterpret_cast<const uint8*>(EventWire.Get()+EventWire.Length()-1),1));
    TestEqual(TEXT("Frame content available immediately"),Incomplete.GetText(),First);
    TestFalse(TEXT("EOF without DONE fails"),Incomplete.Finish());

    FDCDlgSSEStream CRLF;
    TestTrue(TEXT("Accept CRLF and multiline SSE data"),Feed(CRLF,
        TEXT("event: message\r\ndata: {\"choices\": [\r\ndata: {\"index\":0,\"delta\":{\"content\":\"Aye.\"}}]}\r\n\r\ndata: [DONE]\r\n\r\n")));
    TestTrue(TEXT("CRLF stream completes"),CRLF.Finish());
    TestEqual(TEXT("Multiline JSON content"),CRLF.GetText(),FString(TEXT("Aye.")));

    FDCDlgSSEStream Reasoning;
    TestTrue(TEXT("Reasoning field accepted but excluded"),Feed(Reasoning,
        TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{\"reasoning_content\":\"secret reasoning\"}}]}\n\n")));
    TestTrue(TEXT("Reasoning never enters spoken content"),Reasoning.GetText().IsEmpty());
    TestTrue(TEXT("Answer after reasoning"),Feed(Reasoning,ContentEvent(TEXT("Aye."))+Done));
    TestTrue(TEXT("Reasoning stream completes"),Reasoning.Finish());
    TestEqual(TEXT("Only answer accumulated"),Reasoning.GetText(),FString(TEXT("Aye.")));

    const TCHAR* BadEvents[]={
        TEXT("event: error\ndata: failed\n\n"),TEXT("event: error\n\n"),TEXT("data: {\"error\":{\"message\":\"failed\"}}\n\n"),
        TEXT("data: broken\n\n"),TEXT("data: {}\n\n"),TEXT("data: {\"choices\":[]}\n\n"),
        TEXT("data: {\"choices\":[null]}\n\n"),TEXT("data: {\"choices\":[{\"index\":1,\"delta\":{\"content\":\"wrong choice\"}}]}\n\n"),
        TEXT("data: {\"choices\":[{\"index\":0}]}\n\n"),TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{\"content\":12}}]}\n\n"),
        TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[]}}]}\n\n"),
        TEXT("data: {\"choices\":[{\"index\":0,\"delta\":{\"function_call\":{}}}]}\n\n")
    };
    for (int32 Index=0;Index<UE_ARRAY_COUNT(BadEvents);++Index)
    {
        FDCDlgSSEStream Invalid;
        TestFalse(FString::Printf(TEXT("Reject malformed/error SSE event %d"),Index),Feed(Invalid,BadEvents[Index]));
        TestFalse(TEXT("Failure cannot recover by appending DONE"),Feed(Invalid,Done));
        TestFalse(TEXT("Failed stream cannot complete"),Invalid.Finish());
    }
    FDCDlgSSEStream NoAnswer;
    TestTrue(TEXT("Role and DONE are parseable"),Feed(NoAnswer,Role+Done));
    TestFalse(TEXT("No spoken content is not a successful response"),NoAnswer.Finish());
    FDCDlgSSEStream AfterDone;
    TestTrue(TEXT("Complete first response"),Feed(AfterDone,ContentEvent(TEXT("Aye."))+Done));
    TestFalse(TEXT("No data accepted after DONE"),Feed(AfterDone,ContentEvent(TEXT("stale"))));
    FDCDlgSSEStream Truncated;
    TestTrue(TEXT("Partial event retained"),Feed(Truncated,TEXT("data: {\"choices\":")));
    TestFalse(TEXT("Truncated event cannot complete"),Truncated.Finish());
    FDCDlgSSEStream BadUTF8;
    const uint8 InvalidUTF8[]={'d','a','t','a',':',' ',0xC0,0xAF,'\n','\n'};
    TestFalse(TEXT("Reject overlong UTF-8 instead of replacing it"),BadUTF8.Append(InvalidUTF8,UE_ARRAY_COUNT(InvalidUTF8)));
    FDCDlgSSEStream HugeText;
    TestFalse(TEXT("Bound cumulative generated text"),Feed(HugeText,ContentEvent(FString::ChrN(FDCDlgSSEStream::MaxTextCharacters+1,TEXT('x')))));
    FDCDlgSSEStream HugeBody;
    TArray<uint8> Oversized; Oversized.SetNumZeroed(FDCDlgSSEStream::MaxResponseBytes+1);
    TestFalse(TEXT("Bound raw response before allocation/copy"),HugeBody.Append(Oversized.GetData(),Oversized.Num()));
    return true;
}
#endif
