# Credits update, 15 November

Owner: Butler/credits chat, branch `codex/credits-ai-15nov`.

User-requested edits in `Source/DirectorsCut/DCMenu.cpp`: Developers becomes AI; existing AI names remain, with Google Gemma E4B, WhisperCPP, Phonon and Suno added. Thanks now lists Epic Games, NVIDIA, LlamaCPP and Georgi Gerganov. Suno appears only under AI. Credits use the same scroll-box pattern as How to Play so the longer list stays accessible.

Validation: reviewed the exact source diff and ran `git diff --check`. No native build, GPU review, packaging or upload was performed; the selected playable remains unchanged. Include this source change in the next package and inspect the credits screen there.

Additional acknowledgement candidates found in existing local payload notices: Gradium, Laurent Mazare and Kyutai for Phonon; explicit NVIDIA ACE/Audio2Face and DLSS product names. These were suggested to the user, not added without their selection. Current Phonon-selected staging excludes Piper voices; older Piper notices describe a different backend.
