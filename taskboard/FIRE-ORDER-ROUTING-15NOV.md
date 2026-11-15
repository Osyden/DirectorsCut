# Fire orders and command execution

Owner: AI NPCs chat, `codex/speech-streaming`. Root owns integration, the engine lane and delivery.

The accepted 22:36 playtest log records `Rex, get the flames`, `Rex, get the fire`, `Rex, kill those fires` and `Rex, kill this fire`. Conversation replies promised to handle the fires, but no new crew station was assigned. The conversation classifier ran before command interpretation and did not recognize these requests. The keyword fallback also treated an unqualified `fire` as cannon fire. This is confirmed routing failure, not evidence of blocked deck navigation.

Commit `2494056` adds shared explicit fire-request classification, including plural hazards and polite phrasing. These orders now reach the deterministic Extinguish task immediately, even when long enough to otherwise go through the model. Addressed name aliases work with greetings, and fire locations do not become cannon-side parameters. The model prompt also distinguishes removal of a fire from shooting.

The wider command audit found short `stop shooting` and `could you not fire` requests could become cannon Fire actions; their cease-fire interpretation is corrected. Every existing supported command test now checks that it passes the conversation gate as well as producing the expected intent. Additional regression checks protect enemy names such as `fire elemental` and keep negated extinguish orders/questions out of the keyword fallback.

Validation hooks:

- `DirectorsCut.Commands.SupportedOrders`: complete existing command grammar plus short cease-fire regressions and routing assertions.
- `DirectorsCut.Dialogue`: existing dialogue, conversation and streaming tests, plus `FireOrderRouting` covering 20 positive paraphrases, negative/question guards and enemy distinction.
- Existing native `DCDialogueReview` now has 31 model/acknowledgement cases, including four additional fire paraphrases.
- Launch the actual voyage with `-DCNoMenu -DCCommandExecutionReview -useCIGforAI=0`. The opt-in subsystem waits for selected LLM readiness, holds the voyage clock, and issues 19 orders through the real ExecuteOrder entry. It checks helm directions, cannon fire/hold, all three named crew assignments, rigging and report routing, and each reported fire phrase releasing a cannon. It then places Rex at his cannon, starts two authored deck fires and issues `Rex, kill those fires`. Success requires observed movement and two scored extinguishes. Look for `COMMAND_EXECUTION_SUCCESS passed=20 cannon_walk=1`, plus six `CREW_SWAT name=Rex` contacts. The review exits by itself, with a 150-second overall deadline. It mutates only that disposable test world.

At 22:55, root compiled the routing fixes and all ten Commands.SupportedOrders/Dialogue automation tests passed. The actual-world execution review passed all 20 checks: Rex released his cannon, moved to two fires, and produced six `CREW_SWAT` contacts (2/1/0 twice). `Saved/FinalCommands/Native/Execution/runtime.log` records `COMMAND_EXECUTION_SUCCESS passed=20 cannon_walk=1`, and the process exited normally. The accepted package is not changed until root completes combined packaging/acceptance.

## Coordinated voice follow-ups

`c433f76` adds report freshness through queued text, pending synthesis, ready audio, actual playback admission and active target validity. Reports have one six-second start deadline, never renewed by synthesis. A weak-target predicate can cancel an obsolete warning; commands carry no report guard. Cancellation uses existing Phonon/ACE cancellation and drain paths rather than resetting all dialogue. The ACE start callback only mutes an invalid report; the actor tick performs cancellation outside plugin callback locks. `Report` returns rate-limit admission and accepts an optional validity predicate so producers can retry rejected warnings. Producer edits belong to the encounter/boss chats.

Validation: `DirectorsCut.Dialogue.ReportSpeechFreshness` exercises deadline/target guards, queued/pending/ready pruning and actual playback-admission callback while preserving commands. `-DCPhononLifecycleReview=Reports` exercises native expiry before playback, invalid target before playback, invalid target during playback, and a preserved queued command after each cancellation. Required final marker: `REPORT_LIFECYCLE_SUCCESS ... commands_preserved=3`. These new tests were pending the combined compile at handoff.

Combined validation at 23:01: the final source compiled, all **11** command/dialogue automation tests passed, and the native report lifecycle passed all three cancellation cases with all three queued command replies preserved. `Saved/LatePolish/Native/Reports/runtime.log` records two cancellations before playback, one during actual playback, the success marker and normal exit. These were real Phonon/A2F sessions, not a bookkeeping-only simulation.

`4d1da34` exposes `ADCVoice::HasIssuedSuccessfulVoiceCommand()` for the separately owned first-minute HUD hint. Only microphone recording supplies microphone provenance. File transcription, typed commands, silence/noise, conversation and rejected occupied/missing station orders do not set the flag. Both quick commands and asynchronous LLM interpretation require a successful crew dispatch. Old-epoch STT responses are discarded. The native command review additionally exercises the actual post-STT `OnHeard` path with explicit source provenance; require `VOICE_COMMAND_SUCCESS_REVIEW passed=1`. This test does not simulate physical microphone capture or claim end-to-end STT hardware validation.

Root confirmed the updated native review logged `VOICE_COMMAND_SUCCESS_REVIEW passed=1 typed_ignored=1 chat_ignored=1 rejected_ignored=1 wav_ignored=1 mic_dispatch=1`. The actual HUD logged dismissal for `voice_command` at 5.14 seconds. The 31-case model harness is queued against the final package; source is frozen for final visual/package acceptance.
