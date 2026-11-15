# Regular creature cannon balance

Owner: tunnel_boss, `codex/creature-two-hit`, based on main `7468327`.

Regular fog creatures now die in two valid cannon contacts, with an exact 33/100 chance to die on the first. The random percentile is drawn only on their first accepted hit. Statues retain four health; boss damage is untouched. Existing death, interruption, targeting and cue paths are unchanged. Creature callbacks preserve the usual 100-point kill award.

The treadmill prevents damage callbacks from amplifying incoming damage. Creature targets therefore use a normalized one-point budget: a surviving first hit costs half, and the next hit consumes the remainder. Current cannon contacts always supply one damage. No shared treadmill code changes.

Validation: `Tools/probe_creature_damage.py` compiles the production policy with MSVC and passes 7,069 checks: all first-hit percentiles, guaranteed second hits, normal damage clamping, one kill score, and unchanged statue health. `DCEncounterVerify` now checks one/two valid contacts (with the existing allowance for one missed shot); `DCStatueVerify` remains four hits. Native execution belongs to root and was not run in this worktree.

Separate follow-up: audit creature/statue callout side and visibility timing against reported mismatch; not included in this damage change.

## Encounter warning follow-up

Early report producers now use the coordinated `bool Report(Text, Urgent, IsStillRelevant)` API; this follow-up depends on the AI chat's DCCrew/DCVoice API commit. No voice implementation or statue HUD edits are included here.

Predicates capture a weak encounter owner and stable weak actor identity, never an array element/reference. Initial warning validity rechecks the actual live target, visibility, current firing window and `HasClearShot`, including the gun's current ship-local side and arc. Statue "before it fires" reports expire when its beam begins. Beam reports require the exact statue's active beam. Fireball reports follow the individual launched projectile, so a dead thrower does not cancel a genuinely incoming ball; impact/removal or scene exit invalidates it. Completion reports require the same visible defeated body and expire six seconds after defeat, even if crew cooldown has not yet accepted them.

Announcements mark accepted only when Report returns true; cooldown rejection retries while relevant. Phase exits/clear remove tracked identities. The voice layer owns its six-second deadline and cancellation of queued, synthesizing or playing stale reports.

Validation: `Tools/probe_early_warnings.py` compiles production state policy and passes 117 lifecycle checks; creature damage probe remains 7,069 passing. Native integration still belongs to root. Review should cover crew cooldown overlap, killing a charging statue during speech, firing a projectile then killing its source, and entering the tunnel with a queued warning.
