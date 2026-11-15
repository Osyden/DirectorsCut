# Fourth playtest integration

Root integrates on main; accepted19:24 package stays selected until a replacement passes. Human closed the game and released the GPU; root serializes all engine/build jobs. No itch upload.

## Requested changes now in source

- Boss: explicit projected joint/mouth targets, hits and time remaining, edge direction, obstacle warning/safe side, hit/interrupt feedback; stronger held slam anticipation; original Kraken limb skin/material posed along the committed attack path. Separate limbs retain submerged roots; existing permanent main-body cuts remain.
- Caldera: faster finite steering/reversal, no invisible centering across most of the lane, heading follows travel up to40deg, minimum75% authority with torn rigging, concentric foam at the actual inner limit on the existing FFT surface. Bow contact triggers landing about3.75s earlier.
- Crew: three timed rag swats use retained fire suppression; speech describes smothering. Damaged rigging has a marked on-deck repair location. First native review exposed excessive nearby label size; a compact projected-size locator and smaller rag are integrated for recheck.
- Other AI NPCs chat: free conversation, character cards, current scene/crew/ship/weather facts, bounded history and appearance knowledge, with conversation separated from orders. Existing voice/Audio2Face pipeline retained. Entry seam integrated by root.
- Music: second track starts at222s (two seconds later), louder through tunnel/descent (+3.5dB), old quiet mix preserved during full speech duck; boost fades back for combat.
- Ruby: ordinary+deep shadows from paired midmast lanterns reduce excessive hair backlighting. Hair color/material unchanged. VSM overflow text is disabled by default in config/launcher.
- Tunnel: unnecessary shaft-enable toggle deferred to caldera handoff. Matched captures did not demonstrate resolution of reported strange lighting, so this remains open alongside white sail/fog edges. No global exposure/fog change.

## Verification so far

- Editor builds successfully after two unity-name collisions were fixed (local variables only); current smaller locator still needs rebuild.
- CPU:47,839 caldera checks including384 committed obstacle dodges and cross-frame-rate landing;6,907 boss marker/limb mapping;11,117 cue checks;18 crew contact checks. These counts include iteration/assertion cases, not independent gameplay runs.
- Generated limb-mask, boss-band and boundary-water materials. Temporary incomplete water graph warnings occurred during generation; final native boss render has working water and no material compile failure. Release preflight passes49 essential assets and installed steering shader hashes.
- `Saved/FourthPlaytest/BossPassive`: actual authored17-bone limb renders with cups/skin; explicit marker readable at342/346/351, gone after strike353; no exposed torso/spikes in inspected frames. The distal limb is stretched and exits frame during anticipation; final motion quality is not established by these stills. World band remains obscured; readability improvement is the explicit marker.
- `BossInterrupt`: actual hit count drops2→1, interrupt event1 at elapsed76.39, ahead of strike82. Separate later whole-boss success/death and other-side review still pending.
- `Crew`: all three evaluated animation drivers load. Ruby walks to fire, visibly raises rag/arm, applies remaining2/1/0 once each and extinguishes. First locator was too large; fixed source awaits native recheck. Crew face/skin remained intact.
- `Saved/HairLightingReview`: paired shadowed lamps retain the hair improvement of all four at lower measured sample GPU cost. Single frames are not a benchmark; combined voyage performance remains to check.

Still required before selecting a package: compact repair marker/repaired cleanup; actual bow landing/inner boundary; conversation automation and real model/A2F semantic review; louder delayed music with voices; final combined Windows cook, gameplay and launch checks. Original fog outline/tunnel lighting report remains under scoped investigation.
