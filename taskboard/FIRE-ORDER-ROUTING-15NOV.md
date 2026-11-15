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

At handoff: source diff checks pass; compilation, automation and native execution are pending root's serialized engine slot. The user has explicitly released the human playtest reservation and requested an immediate fix and other-command checks. Do not claim native success until the logs confirm it.
