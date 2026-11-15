# Repair destination HUD — 15 Nov

Owner: voice_audio, `codex/repair-hud-marker`. Root explicitly assigned the narrow `ADCHUD::DrawHUD` block in DCGame.cpp; DCPlayer.cpp contains no HUD renderer.

The depth-tested world marker was hidden behind the mast from the helm. A compact screen-space gold destination marker now projects the actual rigging repair station through foreground geometry. It uses the existing resting-deck station transformed with `DeckToWorld`, shows distance, and draws a directional arrow when the destination is outside the clear HUD region or behind the camera. It reserves the crew-card column and bottom speech/cannon controls. It disappears when rigging is repaired, and does not draw in the menu or ending. Existing world arrow, player Hold E repair interaction, and gameplay are unchanged.

Validation: 126 CPU layout cases across seven resolutions/aspect ratios and three text widths pass viewport, crew-column and cannon-control bounds. Source lifecycle/deck-transform checks and `git diff --check` pass. No engine/build/GPU job was run. Root still needs the ordinary helm screenshot at 20 seconds with loose rigging, an on-foot near-target view, and confirmation that completing repair removes the overlay.
