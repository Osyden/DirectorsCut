# Report speech cutting off at onset

Owner: voice_audio, `codex/voice-cutoff-fix`, based on main `7cafdfa`. Source-only work; the uploaded/selected 23:10 package is unchanged. Root owns build and native testing. Narrow DCEarlyEncounters warning hunks were coordinated with cannons_helm's separate damage/spawn changes.

## Evidence

Human playtest log: `E:/Projects/DirectorsCut/Builds/2026-11-15_2310_playtest-fixes/Windows/DirectorsCut/Saved/Logs/DirectorsCut.log`, recorded 23:18–23:24. There are 32 playback starts and 12 explicit report cancellations, ten after playback begins:

- Four grouped-raider warnings stop after 0.526, 0.373, 0.557 and 0.363 seconds. Each coincides with the group leader's death; other members of that same group remain. The warning was bound to a single leader despite describing the group.
- Four `One down! Keep that battery firing!` reports stop after 0.322, 1.429, 1.579 and 0.120 seconds. A defeated actor's six-second freshness/retirement window was continuously reapplied to the playing acknowledgement. Two further acknowledgements expire before playback, which should remain possible.
- A statue warning and a Kraken mouth warning cancel when those actual threats are defeated/interrupted. The patch preserves hard threat cancellation.

`ExpireReports` used the same transient clear-shot/visibility/event-age predicate before and during playback. A one-frame firing-window change could therefore also stop a sentence. The captured onset failures above specifically correlate with defeat/retirement; transient aim dropout is a source-identified additional regression case.

## Change

Reports may supply a separate continuation predicate through `DCCrew::Report` and the existing DCVoice forwarding methods. Admission retains the original six-second deadline and strict target/phase/visibility/clear-shot checks, including the ACE playback-start callback. Default continuation preserves the original urgent-threat predicate. Commands still carry no report guard.

Once admitted, early encounter calls ignore transient gun aim/occlusion. A group warning remains valid while a living member of its exact group remains on the named side and within the city visibility range. It ends when that group is dead, gone, changes sides, or leaves the phase. Statue pre-fire and beam warnings still end on the corresponding actual attack-state change or target death. An admitted defeat acknowledgement can finish after the corpse retires or its admission window expires; phase exit and world/reset epoch cancellation remain unchanged.

Cancellation logs now include `elapsed` and `reason=stale_before_start|threat_ended`. No LLM prompts, command interpretation, music, speech gain, prebuffer or worker transport changes.

## Independent audio startup audit

Phonon uses the configured one-second Audio2Face prebuffer; ordinary lines begin about 1.8 seconds after the synthesis request and complete. This log has zero `PHONON_SPEECH_FAILED`, ACE playback-timeout or ACE inference-failure markers. No underflow log was found; absence of a log alone does not prove glitch-free audio.

Some old-avatar `start playing audio` logs occur after cancellation. Installed ACE source `Plugins/NV_ACE_Reference/Source/ACERuntime/Private/ACEAudioCurveSourceComponent.cpp` shows `Stop()` sets the underlying audio component volume multiplier to zero, stops it, and detaches the consumer. Phonon drains native generation before scheduling that stop. A later playback-start log alone therefore does not prove an audible stale restart. No speculative buffer/transport change was made; listening during native cancellation remains necessary.

## Validation and remaining checks

`Tools/probe_early_warnings.py` compiles the production encounter policy and report guard with MSVC: **134 checks pass**. These cover strict admission, deadline boundaries, transient aim dropout after admission, group leader versus last-member death, actual side reversal, statue firing/cooldown, event completion, phase exit and unchanged default cancellation. `git diff --check` passes.

The Unreal `DirectorsCut.Dialogue.ReportSpeechFreshness` regression is extended for actual ACE admission/subtitle paths, continuation versus admission expiry, hard invalidation, queued-command preservation, and reset epoch clearing. It has **not** been run in this worktree; no engine, model, native or GPU jobs were started.

Root native follow-up: run that automation test plus a normal-time fog dialogue pass. Kill only the leader after a grouped warning starts and confirm the remainder is spoken; briefly lose the gun arc while the group remains and confirm no cancellation. Let an admitted defeat line cross its event TTL/corpse retirement and confirm it finishes. Check that all-members-dead, actual statue firing/death, a resolved boss attack, and Fog→Slide still cancel inappropriate urgent calls. Listen for stale audio during those valid cancellations. Retain before-start expiry and the separate command/voice tutorial tests.
