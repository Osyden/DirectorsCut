# Free crew conversation

Owner: AI NPCs chat, isolated `E:/Projects/DirectorsCut/Crew-Conversation`, branch `codex/crew-conversation`, starting main `1cee5d6`.

Scope: `DCVoice.cpp/.h`, new `DCConversation*.h/.cpp`, and this handoff. Integration root owns the minimal `DCCrew::ExecuteOrder` entry seam; voice_audio owns DCCrew, DCTroubles, DCDialoguePrompts and authored firefighting lines. No engine/GPU jobs until integration root schedules them.

Requested behavior: natural conversation about the scene, weather, ship, crew, appearance and personal background, through typed input and microphone, with the same local dialogue backend and Piper/Audio2Face output. Questions mentioning guns/fire/rigging must not accidentally issue those commands. Crew know each other and the captain, maintain bounded recent conversation, and refresh live context for every answer.

Mechanics: fires are smothered by swatting with a rag. Rigging repairs happen at marked repair lines on deck. No instructions to fetch buckets or climb aloft.

Status: integrated and validated on the current Qwen/llama.cpp path; next Windows package is owned by integration root. See the final validation record below. No alternative-model result is claimed.

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

## Final validation, 20:34 on 15 November

- Source commits: `63ac1ec`, navigation/wind grounding `a0affde`, and identity/scene separation `33e2d8a`, all integrated by root. Root applied the DCCrew entry seam and renamed the test's local Orders array to CommandCases to resolve a Unity-build shadowing error.
- Combined editor build passes. Eight native automation suites pass, including both new conversation suites and existing command/response regressions (`Saved/FourthAutomation.log` in main).
- Final current-prompt Qwen/llama.cpp native review: **8/8 generated conversations**, unchanged order counts, scene-memory reset, actual completed speech and positive Audio2Face animation/audio samples for every line. Report: main `Saved/FourthPlaytest/ConversationLlama/answers.json`, completed at20:34:47.
- Manual review confirms the final run answers cast/player appearance, actual wheel/port/starboard duties, fog and recorded weather, ship condition and rag-swatting fire mechanics. Some personal/comparative wording remains loose; this is a reviewed sample, not a guarantee against all model errors.
- Earlier tests caught bad advice to approach banks, an invented wind condition, and an outfit/scene mix-up. The focused context changes address those observed errors. Final gameplay facts do not reproduce them. Prior reports are preserved under this worktree's `Saved/ConversationBaseline/`.
- Qwen generation latency in the final run: median **510.62ms**, maximum **689.72ms**. Request-to-speech onset: median **13.36s**, maximum **15.65s**, including Piper and the existing fully buffered Audio2Face path. Changing the language model alone will not remove this audio buffering delay.
- ACE/Qwen completed an earlier8/8 speech/face pipeline run, but its last identity/scene prompt correction has not yet received a fresh complete ACE-backend review. The shipped default remains Qwen/llama.cpp; Audio2Face remains active for faces.
- Human microphone/environmental recognition and the next packaged executable are not covered by this native input-path review. Integration root owns the package and final voyage checks. No GPU job was launched from this isolated checkout.

The requested separate Gemma/Bolt GGUF comparison is prepared in [MODEL-COMPARISON-15NOV.md](MODEL-COMPARISON-15NOV.md); the models have not been switched or benchmarked. Its downloader was rejected before execution, so the required local artifacts remain unavailable.
