# Director's Cut submission tracker

Maintainer: Codex submission chat (`01a1dae7-064d-78d1-adcc-a1bdfd8d9306`). Updated 15 November 2026, Europe/London. This is the release checklist; [SUBMISSION.md](SUBMISSION.md) holds supporting disclosure drafts. Updates are evidence-based: implementation, integration, packaging, human verification and submission are separate states.

**Target: Sunday 15 November, 18:00 GMT. Hard deadline: Monday 16 November, 01:59 GMT. Not submitted: no submission receipt or live game URL verified.**

## Requirements verified against primary sources

- [Epic's jam page](https://itch.io/jam/2026-epic-ai-megajam): upload to the team's itch.io account and enter the jam; archive naming `TeamName_GameName.zip`. Larger desktop packages are explicitly supported through Butler or Blendo.
- [Official Rules, English](https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z), revision 12 November, sections 1, 2, 4, 5, 7, 8, 10 and 15: packaged playable game, one entry per team, 1–5 eligible adult entrants with active Epic accounts; identify the team owner and supply every entrant's full name/email. Video: recorded gameplay, 30–60 seconds, **at least 1920×1080**. Game text/audio, video and entry information must be English. Theme and custom gameplay required. The game should need no additional software. Declare sourced assets and pre-jam material; retain appropriate rights and required credits. Check teen suitability and branding restrictions. Glass Box needs a devlog supplied with the entry before closing.
- [Butler documentation](https://itch.io/docs/butler/pushing.html): folders or ZIPs accepted; current backend limit is 30 GB uncompressed. Account, destination and actual upload remain unverified. Butler was not found on PATH in this audit.

The PDF was downloaded from Epic's linked Box share and its relevant sections read. Local audit copy: `E:/projects/directorscut/Submission-Tracking/Saved/SubmissionAudit/Official-Rules.pdf`; SHA256 `226155a71041c747902652583321e56231810f0e044b2052341b714ce226599e`. This is requirement tracking, not a completed personal eligibility or asset licence review.

## Submission deliverables

| Item | Status / evidence | Next action / responsible person |
| --- | --- | --- |
| Team, team owner, human member count, itch account/game URL | Owner confirmed 14 Nov: **1 human**, itch.io account **Shakkara**. Team name and game-page URL not yet confirmed. | Use Solo Driver for team-size category. Confirm team display name and game destination; enter legal name/Epic email directly in the form, never this public repository. Personal eligibility remains to confirm. |
| Final Windows package | Complete combined 09:27 package available; final submission selection/human review pending | `Builds/2026-11-15_0927_final-polish/Windows`. The 08:25 isolated ACE candidate is rejected: missing ship/sky/scenery produced a black world. Main content preflight passes; fresh combined gameplay/ACE verification is recorded in ACE-INTEGRATION.md. |
| Upload route | Butler supported; no upload/authentication tested | Rehearse with owner's itch destination early; retain final build version, upload result and downloadable URL. Do not cut runtime AI solely for the old 1 GB assumption. |
| Gameplay video | No completed video/link evidenced | Capture actual final-candidate play at 1080p or higher, 30–60 seconds; publish accessible link. Recommended: voice order, visible crew action, manual cannon hit, city and finale. Include gameplay sound and check intelligibility. |
| itch page | Draft exists, outdated | Update [ITCH_PAGE.md](ITCH_PAGE.md) against the chosen build: actual route, controls, requirements, launch instructions, known limitations and theme explanation. |
| Cover/screenshots | 13 PNGs present in `E:/projects/directorscut/StoreShots/2026-11-14` | Select cover and screenshots after integration. Existing shots predate giants, sea smoke and today's graphics changes. |
| Sourced asset declaration and notices | Partial list; newer packs missing from old draft | Reconcile actual shipped content and exact licences/voice cards; include third-party notices in package where required. See audit below. |
| AI/tools disclosure | Draft exists; understates Codex contribution | Explain Claude Code and Codex development, local Whisper STT, grammar/LLM order handling, Qwen via llama.cpp, Piper voices. Separate development tools from runtime models. |
| Award answers | NVIDIA draft exists; OpenAI answer missing; Solo Driver applies to confirmed one-human roster | Target Best Overall UE, Render Royalty and Solo Driver. Describe actual DLSS use; answer OpenAI question with Codex development and Whisper speech recognition. Agents are development tools, not human team members. |
| Optional Glass Box devlog | Extensive raw evidence, no submitted link | Owner decision pending. Use [overnight notes](DEVLOG-NOTES-NIGHT-13-14NOV.md), commits and review captures for a concise human-directed development story. Do not expose private agent transcripts. |
| Final jam entry | No evidence of submission | Complete form, check links/download from a signed-out view, verify jam listing/entry URL, save confirmation and timestamp. Uploading the game alone does not establish jam entry. |

## Package and acceptance record

Preserved fallback: `E:/projects/directorscut/Builds/2026-11-14_0748_69449a3_handover/Windows/DirectorsCut.exe`, named source `69449a3`. This audit found **617 files, 8,716,367,310 bytes (8.72 GB / 8.12 GiB)** in its Windows folder. That is an uncompressed folder measurement, not an archive size or final download requirement.

Claude reports a full automated packaged voyage at 1600×900, double speed, reaching the ending without errors. That is historical automated evidence; this chat has not launched the game. Today's branches and main's uncommitted cannon/weather changes are not proven present in that package.

- [ ] Record candidate commit, outstanding local changes, required ignored assets, package path, size and SHA256/archive manifest.
- [ ] Verify package includes local servers, model weights, voices, runtime dependencies and necessary licences. No development asset downloads should be required for players.
- [ ] Cold launch on a second machine; run offline and without development tools installed. Check sidecar startup/shutdown and fallback subtitles/stock lines.
- [ ] Human normal-speed full voyage: voice orders, crew response/action, typed and quick orders, port and starboard guns, steering dodge, firefighting/repair, ending, restart and quit. Record tester/date/build.
- [ ] Listen to the mix: microphone pickup, speech clarity, sound levels, missing/clipped sounds.
- [ ] Check graphics defaults and performance on a representative GPU; do not extrapolate minimum specs from the 96 GB development GPU.
- [ ] Check final sky/fog integration across phase changes and restart; assess VDB motion/stalls in motion.
- [ ] Download the uploaded candidate into a fresh location, extract and launch that copy; verify final video/page match it.
- [ ] Preserve submitted jam build, upload version and checksum separately from later development.

Open gameplay risks from [Saturday review](SATURDAY-PRIORITIES-14NOV.md): crew helm/current disagreement causes head-on rock contacts; manual aim/flight must agree; normal-speed damage balance and spout streaming need human verification. Improvements may have landed since that review: do not mark fixed without current candidate evidence.

## Active work snapshot

Read-only app status and repository inspection, 14 November. These are existing chats, not new assignments.

| Existing chat | State observed | Release dependency |
| --- | --- | --- |
| Check access to DirectorsCut (`01a1cee1-6793-7ee1-81f3-8ca5ec15c4aa`) | Active; improving distant waves, waterfall streams and whitewater; `codex/maelstrom-saturday` | Accepted water/reveal changes need integration and package verification. |
| Improve Maelstrom sky visuals (`01a1da51-b616-7310-aa20-deece1b9dc52`) | Active; photographic storm sky, gameplay views and exit restoration; `codex/maelstrom-sky` | Include new sky attribution if shipped; verify restoration with weather changes. |
| Fix early-game cloud lighting (`01a1d9d2-a32a-7202-9c69-2b729f7b6104`) | Active; fog shafts/sun scattering investigation; `codex/city-fog-depth` and pending-weather copy | Shares director/config integration surface with sky; verify one coherent final route. |
| Fix cannon gameplay and VFX (`01a1d9d3-64ba-7261-abd7-2a91b671f022`) | Completed report: `a3465af`, player/crew guns and impact checks pass; character art queued, audio unreviewed | Main contains uncommitted cannon-related edits. Final package inclusion and hands-on both-side checks remain open. |
| Claude overnight run | Ended at 07:54; maelstrom claim released | 07:48 package is preserved fallback; human verification was still outstanding in handoff. |

Submission chat owns submission documentation and the isolated codex/ace-comparison experiment. Main gameplay remains with current owners. Refresh compact chat statuses and handoffs when reviewing readiness; record evidence when a row changes. A completed chat or successful compile alone does not close a release gate.

## Asset/disclosure reconciliation

Retain the old sourced inventory in [SUBMISSION.md](SUBMISSION.md), then reconcile:

- Add **Clouds Tunnel Vdb Effects Pack Loop / Clouds Tunnel Loop**, one.studio2: volume 01 used for front/tunnel/spouts in overnight build.
- Add **Easy Fog** and **Paragon: Rampage**, Epic Games: mist/cards and stone giants/animations/rock.
- If final sky branch ships, add **Quarry Cloudy**, Poly Haven; photo Dimitrios Savva, processing Jarod Guest. Branch records CC0 and exact source in `Maelstrom-Sky/Content/DC/Maelstrom/Sky/ASSET_LICENSES.md`; retain that provenance on integration.
- Purchased **MHC: Pirates | Metahuman Mega Bundle**, Quang Phan, and Epic MetaHuman are now integrated: captain plus Rex, Duncan and Ruby. Declare them. FogArea, Ancient Marchosias and proposed future packs were not used in the overnight handoff; check the eventual final build.
- Verify **each Piper voice** model card and bundled Piper/eSpeak dependencies, model weight licences and required runtime notices. A blanket engine-library licence label does not cover all bundled voices/dependencies.
- Correct “made during jam” wording: code/layouts/adaptations are project work; purchased ruins, ship, creatures and sourced textures/audio are not original art. Identify generated assets separately if used.
- Correct store copy: hell route and ghost ships are disabled by default; spawnlings ram rather than board; old memory, frame-rate and disk figures are not final-candidate guarantees.

## NVIDIA award / ACE assessment

15 November update: the actual pirate crew now have local GPU ACE lip-sync and grounded British/Scottish voices. Main editor build and both command suites pass. The standalone ACE candidate passed nine crew lines with DLSS-SR=1. See [integration](ACE-INTEGRATION.md) and [the hard-crash report and separate-CUDA-context mitigation](ACE-GPU-CRASH-15NOV.md). Keep shared CUDA/D3D contexts blocked. Human microphone/mix review and a representative second-machine test remain open. Selected voice cards and NVIDIA plugin/model notices are included in the candidate; final distribution/licence reconciliation remains required.

Historical 14 November assessment follows; its “production unchanged” statement describes that earlier experiment, before the integration above.

The [current jam terms](https://itch.io/jam/2026-epic-ai-megajam) accept DLSS or ACE (among other listed technologies) for the NVIDIA RTX Award. ACE is not required in addition to DLSS. This is a judged Render Royalty award, not an automatic prize for integration; qualification is Epic's decision.

Read-only evidence: main enables DLSS/Streamline; plugin descriptor is `8.7.2-NGX310.6.0` for UE 5.8. Preserved 07:48 package `Windows/DirectorsCut/Saved/Logs/Whole.log` records DLSS-SR=1 at line 1277, 67% request at 1327, and NGX CreateFeature validation at 1505. Reconfirm functioning DLSS in the final candidate and retain its log. Do not claim the submitted game uses ACE unless the final package actually includes it; the separate test is not release integration.

ACE feasibility: [NVIDIA's current catalog](https://developer.nvidia.com/ace-for-games) lists ASR/LLM/TTS Unreal downloads through 5.7 and Audio2Face through 5.6; the initial catalog assessment left 5.8 compatibility untested, but the isolated experiment below now establishes a working local test. Existing `DCVoice` provides Whisper, Qwen/llama.cpp and Piper equivalents. Its WAV output could feed Audio2Face, but current crew code uses Manny/Quinn and single-node body animation. Facial animation needs a suitable face rig and animation graph; see [character setup](https://archive.docs.nvidia.com/ace/ace-unreal-plugin/2.5/ace-unreal-plugin-animation.html). Local Audio2Face 2.5's documented providers require Ampere/Ada/Blackwell and approximately 2.9–4.4+ GiB additional model VRAM, depending on model ([support matrix](https://archive.docs.nvidia.com/ace/ace-unreal-plugin/2.5/ace-unreal-plugin-support-matrix.html)); do not generalize this to every ACE backend.

Recommendation: preserve the DLSS qualification route and prioritize release quality. If ACE is pursued for character quality, first prove one supported plugin/sample under UE 5.8 in isolation, then one facially rigged crew member using Piper WAVs, synchronized audio, fallback and packaged performance tests. Full NPC backend replacement would also need game-state/action bindings and regression testing. Update at 17:38 GMT: user authorized a parallel setup. ACE base 2.5 and local Mark regressive model 2.3 now build and run on UE 5.8.4 in `E:\projects\directorscut\ACE-Comparison`, branch `codex/ace-comparison`, with two source API fixes. Nine Piper fixtures and a MetaHuman reference-head A/B harness are ready. Repeated local tests verify audio completion, facial curves and actual jaw movement; warm playback-start event was 511 ms, first process use about 3 seconds (very first model run 8.29 seconds). See [ACE comparison](ACE-COMPARISON.md) for launch instructions and limits. Production crew/voice code, DLSS and release package are unchanged; packaged performance and final asset integration remain outstanding.

## Release sequence

1. Saturday: confirm team/account, upload route and award/devlog intent; assemble licence inventory and rehearse a complete candidate.
2. Sunday before the 18:00 target: freeze accepted changes, human-test, capture video/screenshots, finish accurate page/disclosures and upload with time for a fresh download test.
3. Submit the one jam entry and record its URL/time; keep the remaining buffer for access or upload failures. Preserve the submitted version.

Do not mark “submitted” without observing the entry. This file persists tracking between reviews; it is not a claim of unattended monitoring after this chat stops.
