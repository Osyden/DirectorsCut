# Atlantean Odyssey executable branding

Owner: cannons_helm, `codex/game-branding`. Source-only handoff; root owns native build/cook/launch.

The Win64 game target now uses Unreal's `TargetRules.OutputFile` to emit `Binaries/Win64/Atlantean Odyssey.exe`. The `DirectorsCut` target/module/project identity, editor target, cooked asset paths and save directory stay intact. `DefaultGame.ini` sets the product name and displayed window title to **Atlantean Odyssey**. This also changes the game executable's Windows ProductName/FileDescription from the template branding.

`package_isolated.ps1` lets UAT stage its receipt-derived executable and bootstrap first, verifies the bootstrap embeds the new leaf path and the leaf has the expected Windows product name, then renames the generated top-level bootstrap. Accepted output paths are:

- `Windows/Atlantean Odyssey.exe` (user-facing launcher).
- `Windows/DirectorsCut/Binaries/Win64/Atlantean Odyssey.exe` (real game process).

The bootstrap retains the stock engine bootstrap VERSIONINFO; the real game process carries the project product metadata. Renaming the leaf after cook is intentionally avoided, because the bootstrap records its exact relative path. The package/review/GPU-idle/sidecar scripts recognize the new name. Review and pruning guards still accept old candidate executables; existing packages remain usable. `play_current_build.ps1` reads its path from `Saved/Release/Current.json` and needs no code change.

## Root build and verification

Cherry-pick this source commit, then run the usual `Tools/package_isolated.ps1` with a fresh output directory. It builds the unchanged `DirectorsCut` target and automatically checks the branded executable. Do not reuse or rename an earlier candidate. The editor build command remains `Build.bat DirectorsCutEditor Win64 Development -project=...`.

Verify both the top-level launcher and direct leaf start successfully, the title begins with Atlantean Odyssey, the actual process is `Atlantean Odyssey.exe`, and the product/file description matches. Confirm the main menu can start/restart the voyage and sidecars still resolve. Only then update the current-release marker's `Executable` field to the new accepted path. Development builds retain Unreal's normal platform/RHI suffix in the window title.

## Verification completed without native jobs

- Read installed UE 5.8 `TargetRules.OutputFile`, `UEBuildTarget.MakeBinaryPaths` and Windows UAT bootstrap generation. Output paths flow through target receipts; the bootstrap resource uses the staged executable path.
- Parsed all five edited PowerShell scripts successfully.
- Compiled the target C# against a minimal UBT API stub and checked exact Win64 output, preserved module, and no override on other platforms. This is not an actual Unreal build.
- Validated the bootstrap path inspection against the existing 17:49 package's resource layout.
- New source/config pass branding preflight; sparse-worktree execution stops at its intentionally absent vendor-water manifest.
- `git diff --check` passes. Native compilation, cook, executable metadata and runtime title remain root verification steps.
