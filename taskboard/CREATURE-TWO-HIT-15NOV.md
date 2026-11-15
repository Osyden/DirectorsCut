# Regular creature cannon balance

Owner: tunnel_boss, `codex/creature-two-hit`, based on main `7468327`.

Regular fog creatures now die in two valid cannon contacts, with an exact 33/100 chance to die on the first. The random percentile is drawn only on their first accepted hit. Statues retain four health; boss damage is untouched. Existing death, interruption, targeting and cue paths are unchanged. Creature callbacks preserve the usual 100-point kill award.

The treadmill prevents damage callbacks from amplifying incoming damage. Creature targets therefore use a normalized one-point budget: a surviving first hit costs half, and the next hit consumes the remainder. Current cannon contacts always supply one damage. No shared treadmill code changes.

Validation: `Tools/probe_creature_damage.py` compiles the production policy with MSVC and passes 7,069 checks: all first-hit percentiles, guaranteed second hits, normal damage clamping, one kill score, and unchanged statue health. `DCEncounterVerify` now checks one/two valid contacts (with the existing allowance for one missed shot); `DCStatueVerify` remains four hits. Native execution belongs to root and was not run in this worktree.

Separate follow-up: audit creature/statue callout side and visibility timing against reported mismatch; not included in this damage change.
