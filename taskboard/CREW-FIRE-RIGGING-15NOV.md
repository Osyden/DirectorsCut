# Crew fire actions and rigging repair guidance — 15 November

Owner: voice_audio subagent, isolated `codex/caldera-motion-15nov`; root owns builds and visual review. Source only; no editor, import, render or game process launched.

## Change

- Authored crew fire warning/acknowledgement now describes smothering or swatting flames. The order parser accepts `swat` and `smother`; the old `bucket` input synonym remains accepted for player speech, but is not an emitted line. Order/acknowledgement prompts explain the rag mechanic and deck-level rigging repair. Root coordinates the two boss bucket strings with the cannon owner and free-conversation/report prompts with the AI NPC chat, which owns `DCVoice`.
- Crew fire watch applies the same three retained `SwatNear` suppression contacts as the player. One stroke lasts 1.05 seconds with contact at 0.42; a delayed frame cannot apply multiple contacts, and recovery completes after extinguishing. Reassignment, walking, loss of reach, or removal of the target before contact cancels the work. Navigation and station reservations are unchanged.
- `UDCCrewWorkAnim` extends the existing single-node idle/walk pose driver. Its animation proxy bends two torso bones and solves a bounded right-arm downstroke during pose evaluation, keeping pelvis/root/feet fixed. The assembled pirate body already copies this driver's pose through `DCPirateCast`; no manual post-evaluation bone writes, face/ACE changes, asset mutation or new skeleton is involved. A small double-sided rag follows the evaluated `hand_r` socket and disappears outside the action. Existing idle/walk assets remain in use.
- A damaged-rigging marker is anchored to the same actual rigging station used by player and crew repair, transformed with the moving deck. A gold downward cone points to the destination; camera-facing text shows distance, repair progress, `Hold E at the marked lines` and the crew-order alternative. It hides immediately on repair/reset and on the ending tick. The tear call directs the player to the marked lines on deck.

## Animation inventory and limits

CPU filesystem inventory found no authored humanoid swat, extinguish, smother, repair or work clip. Available humanoid attack candidates were `Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01`, `_02`, `_03`, and `MM_ChargedAttack`; none was watched or verified as a downward firefighting action, so none is claimed or used as one. The procedural layer uses the existing verified mannequin/pirate pose relationship. Its final arm shape, cloth contact and appearance on Ruby/Rex/Duncan require native visual review; source inspection cannot establish those outcomes.

## Checks and handoff

`Tools/probe_crew_work.py` compiles the production timing policy in a standalone MSVC executable without Unreal or GPU use: **18 checks passed, 0 failed**. Covers 30/60/120 Hz, coarse frames, a hitch beyond the whole stroke, exact contact boundaries, cancelled strokes, and three independent suppression contacts. `git diff --check` passes. This does not substitute for Unreal compilation or skinned-pose review.

Root's focused native hook: `-DCCrewWorkReview` creates a real deck fire and loose rigging at ride time 12, then gives Ruby the normal extinguish order. Keep normal route/crew simulation. Review a close deck camera for her travel → planted stance → three downstrokes → recovery and return to idle/walk. The assembled body, held rag and ACE face must all remain intact. Compare logs:

- `CREW_WORK_DRIVER name=... anim=...DCCrewWorkAnim...` for all crew.
- `CREW_SWAT name=Ruby remaining=2`, then `1`, then `0`; one report on completion, no `CREW_ROUTE_BLOCKED` regression.
- `RIGGING_REPAIR_MARKER visible=1 station=...`; marker must point to the existing usable station, move with pitch/heave, and show actionable text. Holding E there or ordering a crew repair must end with `visible=0`.

Also reassign a hand during windup and let the player finish a crew-targeted fire before contact; neither should cause an extra suppression/completion reward. Re-run the existing crew route verification and confirm 55 reachable station pairs. Root should verify existing player fire interactions, rigging repair and menu/restart/ending cleanup in the combined build. No native/build success is claimed by this handoff.

The worktree merged current main in `b20caeb`; only the subsequent feature commit needs cherry-picking. Prior sail audit `eb47b86` remains a separate tools/doc-only change, with its scoped render test pending.
