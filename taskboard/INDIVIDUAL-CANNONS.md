# Individual cannon pass — 14 November

Branch: `codex/individual-cannons`, isolated worktree `cannon-work` in the desktop workspace.

Owns DCGunnery.*, cannon station fields/routing in DCCrew.*, player gun interaction in DCPlayer.cpp, and cannon station setup/HUD in DCGame.cpp. No live editor ownership. Maelstrom, helm/current and character art are outside this pass. Goal: one player gun per side, exclusive independent crew guns, one ball per shot, authentic pack muzzle effect and distinct water/solid impacts.

Implementation:

- Player uses `SK_Cannon_2-L9` / `SK_Cannon_2-R9`, one ball per click, each with its own reload. Barrel/component transforms are read at launch rather than retaining resting world positions.
- Other accessible upper-deck guns have separate NPC-only stations. Marlow starts at port, Odessa at starboard. Named orders reserve a distinct gun; repair/fire duty preserves the reservation; changing sides releases the old reservation. Player interaction and labels expose only the two player guns.
- Crew select visible hostile targets within their gun's side, firing arc and range, prioritizing urgent threats and rejecting blocked sightlines. Player and crew no longer consume each other's reload.
- Retained Brigantine's authored muzzle Blueprint/sound, removed the extra oversized orange light and solid streaks. Water, stone and creature impacts use Rampage's corresponding particles, with the old effects only as missing-asset fallbacks. Water misses no longer cause area damage.
- Aim and flight ignore the identified DRW actor and native sea component consistently; the sampled water surface decides splash crossings. Hostile proxy intersections are shared between aim and flight, clipped against geometry/water, and real hostile collider hits apply damage directly.
- `DCTreadmill.h` has only three read-only hostile accessors plus `GetHostileActor`; no helm/current or treadmill implementation changes.

Verification: Final Development game and editor targets compiled. Offscreen port/starboard station captures confirm the correct named gun fires and its muzzle follows ship motion. `-DCCannonVerify -DCGunLog` provides an opt-in real-map integration scenario: two gunners on one side, repair-return reservation, reserved player gun, exactly one queued ball per trigger, independent reloads and NPC movement/firing that destroys a colliding hostile sphere. Final scenario exited successfully with `CANNON VERIFICATION PASSED`; both player water shots reported `hit 0 splash 1` and the NPC shot reported `hit 1 splash 0`. All three Rampage impact particle assets loaded successfully. Native proxy and authored collider paths both apply damage without water splash area damage.

Licensed packs remain ignored. No new pack assets are committed. Character art is still queued. Sound requires human audition; offscreen tests used `-NoSound`. These checks are not a full-voyage balance test.
