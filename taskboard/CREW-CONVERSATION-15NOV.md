# Free crew conversation

Owner: AI NPCs chat, isolated `E:/Projects/DirectorsCut/Crew-Conversation`, branch `codex/crew-conversation`, starting main `1cee5d6`.

Scope: `DCVoice.cpp/.h`, new `DCConversation*.h/.cpp`, and this handoff. Integration root owns the minimal `DCCrew::ExecuteOrder` entry seam; voice_audio owns DCCrew, DCTroubles, DCDialoguePrompts and authored firefighting lines. No engine/GPU jobs until integration root schedules them.

Requested behavior: natural conversation about the scene, weather, ship, crew, appearance and personal background, through typed input and microphone, with the same local dialogue backend and Piper/Audio2Face output. Questions mentioning guns/fire/rigging must not accidentally issue those commands. Crew know each other and the captain, maintain bounded recent conversation, and refresh live context for every answer.

Mechanics: fires are smothered by swatting with a rag. Rigging repairs happen at marked repair lines on deck. No instructions to fetch buckets or climb aloft.

Status: implementation in progress; no runtime validation claimed.

Implementation ready for integration; no engine build/runtime validation has been run in this checkout.

- Separate conversation from commands before the keyword parser. Natural questions cannot fire guns or assign duties merely by mentioning them. The model returning action:none also goes to conversation.
- Character cards for Rex, Duncan and Ruby, verified cast appearance descriptions, captain appearance and live duties, hull, fires, rigging, scene, weather and revealed threats.
- Three remembered exchanges per character, same speaker for unaddressed follow-ups, fresh state per answer, memory/pending work invalidated on scene reset.
- Shared local backend, Piper voices and Audio2Face output. ACE context grows from 2048 to 4096; CUDA safety configuration is unchanged.
- 181 automation assertions defined (153 routing, 28 response-cleaning); no runtime pass claimed.

Integration seam (DCCrew owner applies at the start of UDCCrewSubsystem::ExecuteOrder, before ++OrdersGiven):

```cpp
if (ADCVoice* Voice = ADCVoice::Get(this); Voice && Voice->TryConversationInput(Text))
{
    return RecentLines.Num() ? RecentLines.Last() : FString();
}
```

Validation owned by integration root:
1. Build editor/game after applying seam.
2. Run DirectorsCut.Dialogue.ConversationRouting and DirectorsCut.Dialogue.ConversationReplyCleaning automation tests, plus existing dialogue regression suite.
3. Run actual L_Ride with -DCNoMenu -unattended -DCConversationReview -DCDialogueBackend=ACE -DCConversationReport=<absolute-json-path>. Repeat with LlamaCpp backend, serialized. Keep A2F enabled; external watchdog at least270 seconds. No simultaneous game/editor/GPU tests.
4. Expect CONVERSATION_REVIEW_SUCCESS cases=8. Review JSON answers against captured live_context; pipeline success alone does not prove semantic accuracy. Audit eight generated lines for positive Audio2Face animation sample output in the engine log.
5. The review covers character backstory, follow-up memory, another crew member's appearance, player appearance, live duties, fog, weather/scene and ship/fire mechanics. It asserts unchanged order count, real generated replies and A2F start/end, and clears history at scene change. Missing entry seam intentionally fails immediately.

The taskboard README claim is local coordination only and is deliberately excluded from the source commit to avoid overwriting concurrent integration work.
