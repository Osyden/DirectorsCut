# Setup: files that are not in this repo

The repo is public, so licensed content and large binaries stay local. To run the project on a fresh machine, add these by hand.

| What | Where it goes | Source |
| --- | --- | --- |
| Brigantine ship asset (purchased) | `Content/Brigantine/` | Fab library, added through the Epic launcher |
| NVIDIA DLSS 4.5 plugin v8.7.2 for UE 5.8 | `Plugins/DLSS`, `Plugins/StreamlineCore`, `Plugins/StreamlineDLSSG`, `Plugins/StreamlineNGXCommon`, `Plugins/StreamlineReflex` | NVIDIA developer site, "DLSS Unreal Engine plugin archive" |
| llama.cpp server, Vulkan build b12317 (`llama-b12317-bin-win-vulkan-x64.zip`) | `Sidecars/bin/llama/` | github.com/ggml-org/llama.cpp releases |
| whisper.cpp, CPU build b5454 (`whisper-bin-x64.zip`) | `Sidecars/bin/whisper/` | github.com/ggml-org/whisper.cpp releases |
| Piper 2023.11.14-2, Windows | `Sidecars/bin/piper/` | github.com/rhasspy/piper releases |
| LLM weights `Qwen3.5-4B-Q5_K_M.gguf` | `Sidecars/models/` | local copy in `E:\AI\Oobabooga\user_data\models` |
| Speech model `ggml-base.en.bin` | `Sidecars/models/` | huggingface.co/ggerganov/whisper.cpp |
| Voices `en_GB-alan-medium`, `en_GB-northern_english_male-medium`, `en_GB-alba-medium` (`.onnx` and `.onnx.json`) | `Sidecars/models/voices/` | huggingface.co/rhasspy/piper-voices |

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
