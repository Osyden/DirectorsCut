# Boss warning lifetime

Owner: cannons_helm, `codex/boss-warning-lifetime` fromed15955; DCBossCombat only plus CPU probe. No engine/build/GPU jobs; root integrates and verifies native speech.

Attack telegraphs now use the guarded boolean Crew Report API. Each callback retains a weak boss, exact event/start/kind and weak target identity. It expires on interruption, cancellation, contact, target removal/replacement, phase change, encounter end or battery-side change. Rejected admissions retry at most every0.25 ride seconds within the live window; an accepted line is enqueued once. Completion reports remain on the existing short-TTL path.

Mouth and slam spoken battery directions use the same deck-local transform as the HUD. Obstacle steering advice retains its committed safe lane regardless of camera or later heading. The fire-incoming guard follows the actual released projectile and three-second flight interval, independently of the removed mouth target/thrower. No attack/damage or timing changes.

Validation: `Tools/probe_boss_warning.py` extracts and compiles the actual production callback using small CPU lifetime/transform mocks;19checks pass. Covers dead owner/target, replaced target/event/start, interrupted/cancelled/applied state, strike boundary, phase transition, ship-side change, committed obstacle lane, and projectile validity through exact impact time. Native acceptance: interrupt during queued speech, turn away during mouth charge, then passive fire release/impact; verify no warning continues after its subject expires and no wrong-side battery call. Root owns native jobs.
