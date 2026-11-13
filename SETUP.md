# Setup: files that are not in this repo

The repo is public, so licensed content and large binaries stay local. To run the project on a fresh machine, add these by hand.

| What | Where it goes | Source |
| --- | --- | --- |
| Brigantine ship asset (purchased) | `Content/Brigantine/` | Fab library, added through the Epic launcher |
| Ultra Dynamic Sky (purchased; sky, clouds, weather, storm front) | `Content/UltraDynamicSky/` | Fab library. Without it the game uses its own sky (`-DCNoUDS` forces that) |
| Modular Ancient Ruins by archafizov (purchased; Atlantis) | `Content/AncientRuins/` | Fab library. Without it Atlantis is built from placeholder shapes |
| Dynamic Real Water 3.0 by Natural Level (purchased; the sea outside the tunnel and maelstrom) | engine `Plugins/Marketplace/` (installed by the launcher) | Fab library, "Install to engine". Optional plugin; `-DCNoDRW` uses the game's own sea |
| Claw Kaiju Monster (boss) | `Content/Fab/Claw_Kaiju_Monster/` | Fab library, then generate the isolated skin with `Tools/make_leviathan_skin.py` (instructions below) |
| Kraken (free) | `Content/Kraken/` | Fab library |
| NVIDIA DLSS 4.5 plugin v8.7.2 for UE 5.8 | `Plugins/DLSS`, `Plugins/StreamlineCore`, `Plugins/StreamlineDLSSG`, `Plugins/StreamlineNGXCommon`, `Plugins/StreamlineReflex` | NVIDIA developer site, "DLSS Unreal Engine plugin archive" |
| llama.cpp server, Vulkan build b12317 (`llama-b12317-bin-win-vulkan-x64.zip`) | `Sidecars/bin/llama/` | github.com/ggml-org/llama.cpp releases |
| whisper.cpp, CPU build b5454 (`whisper-bin-x64.zip`) | `Sidecars/bin/whisper/` | github.com/ggml-org/whisper.cpp releases |
| Piper 2023.11.14-2, Windows | `Sidecars/bin/piper/` | github.com/rhasspy/piper releases |
| Recorded sounds (optional; the game falls back to synthesised sound without them) | `Content/Sonniss/` | Sonniss GDC 2026 bundle in `E:ProjectsDirectorsCutDownloadsSonniss-GDC2026`; prepare with `Tools/wavprep.ps1`, import with `Tools/py.sh Tools/import_audio.py`. See `taskboard/AUDIO-SHORTLIST.md` |
| LLM weights `Qwen3.5-4B-Q5_K_M.gguf` | `Sidecars/models/` | local copy in `E:\AI\Oobabooga\user_data\models` |
| Speech model `ggml-base.en.bin` | `Sidecars/models/` | huggingface.co/ggerganov/whisper.cpp |
| Voices `en_GB-alan-medium`, `en_GB-northern_english_male-medium`, `en_GB-alba-medium` (`.onnx` and `.onnx.json`) | `Sidecars/models/voices/` | huggingface.co/rhasspy/piper-voices, **tag `v1.0.0`** (newer revisions of some voices do not load in this Piper build) |

Engine: Unreal Engine 5.8.4 (launcher build) with Visual Studio 2022.

## Everyday commands (Git Bash, editor closed)

- `Tools/build.sh` builds the editor target.
- `Tools/shots.sh "3,130,300"` runs the game unattended, saves screenshots at those ride times to `Saved/Screenshots/Auto`, prints the game log lines.
- `Tools/package.sh` packages a Windows build to `E:\projects\directorscut\Builds`.
- Open the editor with the Unreal MCP server: `UnrealEditor.exe DirectorsCut.uproject -ModelContextProtocolStartServer`.

## In-game debug commands

- `dc.Order <words>` gives the crew an order as text.
- `dc.Ride.Seek <seconds>` jumps the ride; `dc.Ride.Speed <scale>` changes its speed.
- `dc.Voice.TestWav <path>` feeds a wav file to speech-to-text as if spoken.
- Hold `V` (or middle mouse) to talk. Keys `1`-`6` are quick orders.

## Maelstrom assets and isolated review

The original maelstrom water/spray materials are in `Content/DC/Maelstrom/`. Regenerate with `Tools/make_maelstrom_materials.py` through Unreal's Python commandlet in the intended project. DRW's detail-normal texture is referenced in place when installed; the bowl is our own geometry and shader, not the plugin's FFT ocean simulation.

The Kaiju skin is local licensed content under the ignored `Content/DC/Maelstrom/LeviathanSkin/` directory. Run `Tools/make_leviathan_skin.py` with the same commandlet. It finds the purchased texture files in the local Fab cache; set `DC_KAIJU_TEXTURES` to their extracted folder on another machine. This avoids UDIM auto-discovery treating each imported tile as the whole atlas. Never commit those generated textures or their material.

From PowerShell, `Tools/review_maelstrom.ps1` starts an offscreen 1600x900 seek review in its own checkout and prints the process ID. Add `-Continuous` to start at ride time 238 and play normally through the exit. Images go to `Saved/Screenshots/Auto/`. `-DCLegacyMaelstrom` on the game command line uses the previous wave/boss path for comparison. The generators require the editor for that checkout to be closed; other checkouts remain independent.
