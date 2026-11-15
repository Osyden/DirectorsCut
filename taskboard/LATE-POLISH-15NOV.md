# Late combined polish

Root owns the serialized engine/build lane. The user explicitly released the previous playtest in the AI NPCs chat and directly requested the credits/lantern corrections. Current accepted playable remains 22:36 until a combined candidate passes. Trailer and scheduled polish remain paused.

Scope: one-page small muted credits; full lantern shadow coverage; regular creatures two hits with 33% first-hit kills; truthful/current encounter callouts; statue and Kraken target awareness from helm/deck/cannons; initial Hold V prompt until a successful microphone command or 60 seconds; fire-order and other-command routing corrections.

Owners: root integrates/tests/builds, tunnel_boss owns early creature damage and encounter-warning producers, cannons_helm owns statue/boss marker display and boss sighting callouts, voice_audio owns obstacle warning producers and the HUD voice tutorial, AI NPCs chat owns command grammar/execution, report freshness and successful voice-command signal. Preserve independently validated source hunks during integration.

Native credits pass at 1600x900/1280x720; lantern matched comparison passes with higher measured GPU cost (see corresponding handoffs). Command automation currently passes all 10 tests; actual-world/model execution and final combined report-lifecycle tests remain in progress. The caldera phase call now says only "We're in the caldera!"; the boss owner is moving its sighting call to actual visibility instead of the earlier phase boundary.

No final acceptance or upload is implied by source integration. Build a fresh candidate, retain the immutable fallback, inspect ordinary HUD views without review aim flags, verify successful microphone-command dismissal separately from typed/failed/chat input, and report exact remaining limits.
