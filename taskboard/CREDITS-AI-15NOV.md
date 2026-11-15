# Credits update, 15 November

Owner: Butler/credits chat, branch `codex/credits-ai-15nov`.

User-requested edits in `Source/DirectorsCut/DCMenu.cpp`: Developers becomes AI; existing AI names remain, with Google Gemma E4B, WhisperCPP, Phonon and Suno added. Thanks now lists Epic Games, NVIDIA, LlamaCPP and Georgi Gerganov. Suno appears only under AI. Credits use the same scroll-box pattern as How to Play so the longer list stays accessible.

Validation: reviewed the exact source diff and ran `git diff --check`. No native build, GPU review, packaging or upload was performed; the selected playable remains unchanged. Include this source change in the next package and inspect the credits screen there.

The user subsequently requested all suggested acknowledgements: Thanks now also includes Gradium, Laurent Mazare, Kyutai, NVIDIA ACE / Audio2Face and DLSS. These follow the existing local payload notices. Current Phonon-selected staging excludes Piper voices; older Piper notices describe a different backend.
