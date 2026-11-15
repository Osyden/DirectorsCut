# Atlantean Odyssey menu and Windows branding

Runtime `31abe47`, candidate `2026-11-15_1819_atlantean-odyssey`.

The game uses an original generated gold title, dark teal stone panel and Cinzel type, following the owner's supplied reference. The right side remains the live ship and ocean: waves, sails, sampled ship motion and camera drift continue while voyage time is held at zero.

Exactly four menu buttons: Start Game, How to Play, Credits, Quit. How to Play and Credits toggle mutually exclusive information panels on the right; clicking the selected panel's button again closes it. The owner's requested instructions and team credits are preserved. Art creator attribution is verified in [ArtCredits](../Docs/ArtCredits.md). Credits scroll to the additional audio/type attribution. The controls remain visible when either panel opens; this small Slate menu is volatile to prevent cached sibling paint disappearing after panel visibility changes.

Thanks To includes Epic Games, NVIDIA and the subsequently requested Suno. A brief branded preparation layer covers the ocean until Dynamic Real Water reports ready, then permits at least two seconds and 60 game frames to settle before a .8-second reveal. The world keeps rendering during preparation, and Start becomes available after the reveal. Native review measured readiness at 3.30s and reveal at 6.10s; these are measured examples, not a fixed loading duration.

Both the packaged bootstrap and the actual Win64 process are `Atlantean Odyssey.exe`. Windows product and window-title configuration use Atlantean Odyssey. Internal DirectorsCut module, map and save paths remain stable. Packaging checks the bootstrap's embedded path and the leaf's ProductName, and stages the Cinzel font and its SIL OFL notice. Use `E:/Projects/DirectorsCut/Play Atlantean Odyssey.cmd`; the older launcher remains a compatible alias.

## Art source

- `Art/UI/AtlanteanOdyssey-logo-v2.png`, imported as `T_AtlanteanOdysseyLogoV2`.
- `Art/UI/AtlanteanOdyssey-menu-panel.png`, imported as `T_AtlanteanMenuPanel`.
- Both created using built-in image generation; exact generation/edit prompts are in `Art/UI/AtlanteanOdyssey-menu-generation.json`. The first logo pass's pale backing was removed in a second generation edit.
- `Content/DC/UI/Fonts/Cinzel.ttf` and adjacent `OFL.txt`, from the Google Fonts Cinzel package.

## Verification

Editor compilation and import succeeded. `Saved/BrandedMenuPanels2.log` confirms the menu holds ride time at zero, shows Help then Credits, closes the information panel and starts a fresh voyage. Captures at 1600x900 show the requested content and four controls. A menu-local invalidation correction preserves the left controls during panel changes. Package completed successfully with both renamed executables and verified product metadata. The initial 18:09 package passed real bootstrap launch, font loading, gameplay entry, local LLM readiness and all three ACE voices (`BrandedPackageReview.log`, `BrandedVoiceSmoke.log`). Final packaged preparation/menu/voice verification is recorded in the selected build's `Windows/DirectorsCut/Saved/FinalBrandedReview.log`. The 18:18 cook was deliberately cancelled to incorporate the owner's Suno addition before packaging again.

Normal voyage timing is unchanged: fog58s, water tunnel240s, breach270s, receiving water about299s, boss death430s, ending450s, ride length470s. No itch upload.

Final 18:19 package passed the actual renamed bootstrap launch and exited0. `FinalBrandedReview.log`: ocean ready0.93s, revealed3.73s; menu holds ride0 at5/9s with changing heave/slope; Help12s, Credits18s, close24s, Start27s begins ride0. All three Rex/Duncan/Ruby ACE playbacks finish, `VOICE_SMOKE_SUCCESS mode=ACE cycles=1`. Packaged screenshots show Suno, the intended typography and the live ocean; left logo/control pixels remain present in every panel capture. This targeted menu/branding change did not rerun the unchanged full voyage; its preceding full-voyage and final pillar evidence remain linked from [the second playtest record](SECOND-PLAYTEST-15NOV.md).
