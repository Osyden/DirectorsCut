# ACE workstation hang — 15 November 2026

After receiving this report, the owner explicitly requested continued GPU ACE integration. Separate-context GPU inference is now integrated and enabled in main following repeated editor and standalone-package checks. The implicated shared-context path remains blocked (`-useCIGforAI=0`), with paced inference, serialized replies and bounded tests. See [the integration handoff](ACE-INTEGRATION.md).

## What happened

The ACE integration test launched the full voyage at 07:49:55 GMT. At 07:50:17 it submitted Finch's first line to the local Mark Audio2Face provider. The final startup messages show NVIDIA loading its `hwi.cuda` feature. Unlike the successful face-only tests, the voyage never logged `Created CIG context` or started voice playback.

Windows recorded NVIDIA display-driver `nvlddmkm` event 153 at 07:50:17 and 07:50:19, both reporting an error on GPU ID 200. At 07:50:32 Unreal reported GPU timeouts on both its Compute and 3D queues. The workstation subsequently required an unclean restart; Windows Kernel-Power event 41 at 07:54:58 records BugcheckCode 0. This was a system/GPU hang, not an ordinary handled game exception. Event 41 establishes the unclean restart, not the original cause.

The strongest current explanation is a GPU driver/interoperability failure while ACE 2.5 created its shared CUDA/Direct3D12 context under the full UE 5.8 rendering workload. This is a probable trigger, not a proven kernel-level diagnosis. No kernel dump has been analysed. The plugin targets earlier Unreal versions and has local compatibility patches for 5.8; successful compilation did not establish runtime stability.

Hardware recorded by Unreal: RTX PRO 6000 Blackwell Workstation Edition; NVIDIA driver 610.88 dated 22 July 2026; UE 5.8.4. Do not change driver versions or Windows TDR/watchdog settings as a speculative fix.

## Mitigations implemented in the isolated checkout

- Live ACE was initially switched off. After the owner's explicit continuation request and successful isolated/package tests, main enables separate-context GPU inference by default; `-DCNoACE` keeps an audio-only fallback.
- The implicated shared CUDA/graphics path is disabled with the vendor's `-useCIGforAI=0` switch. The gameplay and comparison paths refuse an explicit nonzero override.
- Burst inference is disabled; GPU inference is paced and replies are serialized/buffered.
- The comparison helper refuses local inference unless explicitly opted in; the diagnostic game mode also requires the opt-in flag. This prevents an ordinary comparison command from silently starting GPU inference.
- GPU testing stopped after the owner's crash report, then resumed only after the owner explicitly requested continued GPU ACE work. Main received the scoped integration after repeated isolated tests; editor compilation, command tests and the standalone candidate then passed. Other chats were directly coordinated after the owner's explicit messaging authorization.

After the restart, but before the owner reported the hard crash, one full-voyage retest with CIG disabled completed all three crew lines successfully and exited normally. That supports the suspected trigger; one successful run is not sufficient to certify stability or promise that the PC cannot crash again.

## Evidence

- `ACE-Comparison/Saved/ACE/Voice-smoke.log`: failed voyage, NVIDIA startup and GPU timeout breadcrumbs.
- `ACE-Comparison/Saved/ACE/Voice-smoke-separate-context.log`: successful opt-in separate-context test, `VOICE_SMOKE_SUCCESS mode=ACE completed=3`.
- `ACE-Comparison/Saved/ACE/Voice-smoke-fallback.log`: audio-only fallback passed all three speakers, `VOICE_SMOKE_SUCCESS mode=audio completed=3`. This test started before the owner's crash report.
- `ACE-Comparison/Saved/ACE/Commands-before.log` and `Commands-after.log`: 38 deterministic command cases, failed before parser fixes and passed after.
- `ACE-Comparison/Saved/ACE/SpokenOrders/transcripts.json`: 13 synthesized captain orders transcribed by local Whisper. These are clean-speech tests, not live microphone validation.

## Subsequent validation and remaining limits

CPU-only compilation and both command suites passed (38 text cases and 13 synthesized speech round trips), including main's editor build and both suites. Separate-context GPU playback passed 18 crew lines in the full 1080p voyage, then another 12 with DLSS-SR=1 and buffered replies. The combined Whisper/LLM/ACE flow passed in the editor and standalone package: a recorded "Odessa, report" returned Odessa's status, and "Finch, keep our heading unchanged" was interpreted as steady course.

Standalone candidate `Builds/2026-11-15_0825_ace-gpu-safe/Windows` passed nine crew lines (`Saved/ACE/Smoke/20261115_082859_ACE/runtime.log`), cooked Finch face deformation (`Saved/ACE/Runs/20261115_083156_878_finch_confirmation/`, jaw delta 0.090380 radians), and the normal default-ACE voice loop (`Saved/ACE/Full-voice-loop.log`). All exited normally with DLSS active and no GPU timeout. Windows System had zero new `nvlddmkm` events from 07:55 through the 08:34 verification. These runs support the workaround but do not establish a universal stability guarantee. Live microphone/mix review and testing on another representative machine remain open. Keep the shared-context path blocked.

## Separate packaging failure and workaround

The first full cook failed on CPU with an array bounds assertion. A focused cook reproduced it using only the existing `M_DeckFire` material. The same material's SHA-256 matches main. Resolving the engine's exported function addresses identified `UMaterial::TryGetUsedTexturesInCookCache`, called while stripping default texture references. The installed engine source indexes `EditorReferencedDefaultTextures[Parameter.TextureIndex]` at this point. This is separate from the earlier GPU hang.

The focused cook and full package passed with `-ini:Engine:[SystemSettings]:r.Material.StripUnusedDefaultTextures=0`. The package helper applies this override only to the cooker, alongside `-NullRHI`, retaining textures instead of modifying the material, driver or runtime rendering configuration. The new ACE facial-mapping dependency cook independently passed. Existing Dynamic Real Water example-material shader warnings and nonfatal NGX update warnings remain; this was not a warning-free cook/startup.

The comparison and smoke helpers now refuse to overlap another Unreal editor/game GPU process. This guard does not detect every possible GPU-consuming application and cannot guarantee driver stability.
