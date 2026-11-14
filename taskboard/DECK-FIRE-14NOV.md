# Deck fire realism — 14 November

Publication: source, new materials and curated review images are collected on `codex/fog-fire-polish`; see [publication scope](FOG-FIRE-PUBLICATION-14NOV.md). Earlier uncommitted-status notes below describe the historical working state.

Owner requested more realistic deck fire after approving the latest grey city fog. This pass uses `codex/city-fog-depth` and the private pending-weather project for iteration; the accepted fog and storm work is preserved.

## Change

The previous effect used five stretched additive cones with high emissive values and a broad, intense point light. It read as solid white spikes in the fog-phase player view.

`DCDeckFireEffect` replaces the deck effect with seven camera-facing, independently moving flame sheets, twelve rising smoke puffs and twenty-two small drifting embers. Fire samples the project's existing Paragon fire noise/wisp textures; smoke uses the existing Brigantine smoke texture. Soft opacity, depth fade, per-pixel fogging and translucent sorting soften their intersection with the scene. Flame colour retains orange edges and hotter cores, with partial exposure compensation to retain detail in the dark city. Smoke uses scene lighting. A smaller shadowed point light flickers on the deck and contributes much less to volumetric fog.

`DCTroubles.cpp` still owns hazard lifetime, damage, active-fire count, crew reach and scoring. Its visual creation and update now delegate to the new actor. Extinguishing removes the hazard immediately; flame/light disappear over .35 seconds, smoke dissipates over 3.3 seconds and the actor expires at 3.5 seconds. Natural burnout and the ending use the same visual cleanup. The shared `M_Flame` used elsewhere and vendor assets are untouched.

New assets: `/Game/DC/Materials/M_DeckFire`, `M_DeckSmoke`, `M_DeckEmber`. Generator: `Tools/make_deck_fire_materials.py`. Their hard texture references are cooked through the existing `/Game/DC` always-cook directory. The texture assets must already be installed, as on this machine; no vendor source content is copied into Git. There are no new engine-module or plugin dependencies.

## Iteration and verification

First render rejected: the DepthFade opacity input had not connected and the cards rendered as rectangles. Corrected the pin lookup and made material-expression connections fail explicitly on error. Second rendered pass showed ragged orange flames without the rectangular smoke/card boundaries. Adjacent views at 115, 120 and 124 seconds were inspected, together with the extinguished view at 126 seconds.

The controlled 3x run used `-DCNoGiants -DCFireReviewAt=110 -DCFireReviewOut=125`. It logged one active fire, a successful extinguish with the existing +50 score, then zero active fires and zero effect actors four seconds later. These development-only hooks live in DCTroubles and are inactive in ordinary play. They are excluded from Shipping builds. Source and the final materials compiled successfully. Main source integration was guarded against concurrent DCTroubles edits and its editor module rebuilt successfully.

Final normal-speed main run completed from 0 to 200 seconds with exit code 0 and eleven captures. Inspected naturally started giant-impact fires at 125 and 150 seconds, including adjacent 150.0/150.6/151.2-second views. Flames retain orange detail and a changing broken silhouette under city lighting. A separate normal-speed close-camera run completed with exit code 0 and three captures at 120–122 seconds; inspected the flame shape, smoky plume, individual embers and deck lighting. The original five-cone appearance and the rejected candidate's rectangular cards are absent from the inspected final views.

Both final logs have no fatal error, assertion, Blueprint Runtime Error, material compile failure or missing deck-fire-material matches. Existing engine toolset/Python startup errors remain. Other checkouts were rendering concurrently, so these are not isolated performance measurements. No source/configuration files other than DCTroubles.cpp differ from the starting snapshot, apart from the two new effect files. Main and the owned worktree's final source/generator hashes match. No standalone package has been rebuilt by this pass; changes remain uncommitted.

Before/after: [before 150 seconds](review/deck-fire/before-150.png), [after 150 seconds](review/deck-fire/after-150.png), [close-up](review/deck-fire/close-120.png). These are unedited engine captures, with normal run-to-run differences in ship motion, hazard positions and lightning. The close-up uses an alternate review camera; the 150-second comparison uses the normal player view.

Evidence is in `E:/projects/directorscut/deck-fire-review/`, including rejected captures, build/material logs, source/hash snapshot and `deck-fire.patch`. All capture processes owned by this pass exited normally; other checkouts were left alone.
