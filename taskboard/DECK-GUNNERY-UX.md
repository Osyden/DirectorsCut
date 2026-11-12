# Deck and gunnery handoff

Branch: codex/deck-gunnery-ux. Base: 45ca405. Merge or cherry-pick the source commit after the active weather changes.

- Deck movement has quicker stops, normalized diagonal input, Shift sprint, jump release and subtle walking bob. Station tags show Helm, Port Guns and Starboard Guns, with an E prompt in range on the right deck.
- At the helm, A/D turns the wheel and C toggles an offset view around the mast.
- Battery mode puts the camera behind the rail. Mouse traverses the sight, LMB fires one volley per click, and E returns to the deck. The HUD shows firing range, reload progress and hit/splash feedback.
- The ship pack supplies BP_EffectExplosionStartCannon and BP_EffectExplosionEnd with smoke and sound. Native code adds a muzzle light, shot streaks and a short water plume. The four exposed guns on a side fire first for manual volleys. Both batteries still share their reload with crew fire.
- Shots use the existing sea-level plane, visible blocking geometry and treadmill hostile checks. Retest against the new water surface once the weather/water work lands. The ship motion model remains with the ride work.

Verification: Final UE 5.8 Win64 Development game target compiled and linked. The Editor target built before the final sight alignment and water plume pass. Two offscreen runs using DCStation and DCAutoFire confirmed firing/reloads and both effects loaded. The port run found nine broadside guns per side. Test actual mouse traversal, sprint/jump on the stairs, helm clearance and effect visibility/audio in the merged build. The offscreen editor logged startup errors in the Ultra Dynamic Sky editor utility and Epic Python toolsets, then ran through the capture and exited.
