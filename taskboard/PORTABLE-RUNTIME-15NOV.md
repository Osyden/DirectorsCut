# Installer-free runtime, 15 November

Owner: Butler/credits chat, branch `codex/portable-runtime-15nov`.

User requested installer-free bundling and a system-requirements text beside the launcher, with the wording "Minimum 12 GB VRAM expected."

`Tools/package_isolated.ps1` now runs `Tools/stage_portable_runtime.ps1` after sidecar staging. It discovers the official Visual Studio x64 release CRT and OpenMP redistributables, checks Microsoft signatures, and copies the DLLs beside every executable directory. Existing differing files cause a failure rather than overwriting pinned sidecar payloads. All output hashes are verified. A manifest and Microsoft notice are included, along with root `SYSTEM_REQUIREMENTS.txt`. Windows system libraries and graphics drivers are not bundled.

Prepared copy: `E:/Projects/DirectorsCut/Builds/2026-11-15_2236_crew-followup-portable/Windows/Atlantean Odyssey.exe`. The original selected 22:36 build and `Saved/Release/Current.json` were not changed. This is the old 22:36 runtime plus app-local dependencies, not a new gameplay build. Future packages automatically include the same runtime step and requirements text.

Validation:

- 55 signed app-local CRT/OpenMP DLL instances across bootstrap, game, llama, Whisper and Phonon Python directories; SHA256 checks and ValidateOnly pass. Source version 14.44.35211.0.
- Inspected imports of 244 executable/library files; 216 reference the VC runtime. The first audit found missing VCOMP140.DLL for ACE/Whisper, then passed with zero missing VC/OpenMP imports after bundling the official OpenMP runtime.
- Packaged llama-server and whisper-server --help exit 0 with PATH limited to Windows directories.
- Phonon Python imports NumPy/ptts with invalid PYTHONHOME/PYTHONPATH and restricted PATH. Loaded VC runtime paths resolve inside the packaged Python directory. Initial probe assumed msvcp140 was eagerly loaded; corrected probe loads it explicitly and verifies its actual location.
- No engine/GPU process launched; no clean second-PC or new gameplay acceptance claimed.

Evidence: sibling `portable-runtime-validation/` contains the import inventory/summary, sidecar help output and Python probe/report. `portable-build-info.json` records this derived package; copied original build metadata remains historical provenance. No itch upload.
