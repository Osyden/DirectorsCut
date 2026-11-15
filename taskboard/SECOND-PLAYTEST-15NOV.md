# Second human playtest pass, 15 November

Runtime source is `362464f`. The 17:25 candidate is packaging at `E:/Projects/DirectorsCut/Builds/2026-11-15_1725_second-playtest`. The 15:51 playable remains selected until packaged review passes. No itch upload is authorized yet.

## Changes

- Generated Atlantean Odyssey title logo, teal/gold menu, collapsible captain's handbook and Begin Odyssey action. The world continues animating ocean, sails and sampled ship motion while ride time remains zero. A restrained camera drift frames the ship beside the title.
- Inertial lateral steering, carried momentum, braking before reversal and inward recovery near channel boundaries. CPU wave queries and all six installed ocean LOD materials use the same steering displacement. See [water/steering handoff](STEERING-WATER-15NOV.md).
- Alternating pillar clusters leave a central passage; fixed sea-level anchors and submerged foundations prevent floating columns. Sampled foam and intermittent spray follow their feet. See [pillar handoff](PILLAR-CONTACT-15NOV.md).
- Ship-local crew navigation is derived from hull triangles with body clearance, station routes and grounded movement. Native testing exposed overly broad mast physics and a deck-wide rope assembly incorrectly classified as a compact gun obstacle; both have specific corrections.
- The storm keeps its original sculpted volume. A continuous height/depth atmosphere joins the sea and cloud foot, preserving actual wave depth and foreground ship occlusion. It fades out with the front at 74–80s. The city retains zero rain/lightning from 94s. Generator: `Tools/make_storm_depth_fog.py`; `-DCNoStormDepthFog` is a diagnostic comparison only.
- The approved caldera inlet-toe bundle from `Maelstrom-Saturday/Saved/CalderaInletToe/Handoff` is installed locally. All eight files match its manifest; matching source/FBX hashes are retained in the runtime markers. Original files were backed up to `Saved/SecondPlaytestCalderaBackup`.

## Evidence so far

- `Saved/MenuNavigationFinal.log`: title holds ride time and lane at zero even with forced helm input. At 5/9s, sampled heave changes from -6.834 to -5.697cm and roll slope from .0328 to -.0165; Begin Odyssey starts at time zero. The handbook fits the rendered layout.
- `Tools/probe_steering.py`: all 18 production-helper checks pass; maximum edge excursion 4450.98cm within the 5000cm boundary, frame-rate spread 0.7115cm. Late, physically unavoidable obstacles remain a failing-route fixture; reachable tests use actual bow-contact warning. Production avoidance now immediately replaces a blocked held course with a swept-clear alternative.
- Pillar CPU layout: 566,100 positions, at least 1200cm hull clearance and 17100cm longitudinal separation.
- `Saved/GenerateWaterStorm.log`: all four installed wave functions patched, six dependent ocean materials compiled and saved. Backups and exact installed hashes are in `Saved/OceanSteeringPatch/manifest.json`. Packaging now requires these hashes and the new presentation assets.
- `Saved/MenuNavigationFinal.log`: all 19 navigation, 13 cannon and 55 crew station-pair checks pass. `Saved/SecondPlaytestAutomation.log`: all 11 suites pass, including crew routes, pillar clearance, dialogue and caldera geometry.
- `Saved/CrewSteeringClose.log` and corresponding scene-review images: braking precedes reversal, released steering coasts to rest, heading relaxes toward neutral. Duncan/Ruby reach their stations with no blocked routes. Ruby's boots visibly contact the deck. The other two crew feet are obscured by this camera; sparse captures do not certify continuous gait smoothness. Exact water-coordinate agreement is supported by the shader/query audit and CPU checks, rather than a phase-tracked visual measurement.
- `Saved/FinalFogPillars.log` and corresponding images: continuous storm crossing, dry city at 94s; pillar actor Z remains -120cm while sampled waves vary and local spray pulses activate. No exposed pillar bottoms are apparent from the helm, but a closer side view remains for the packaged review.

Full-raymarch storm replacements and early thin-mist iterations were rejected. They are retained only under `Saved` as diagnostic evidence; the final generated asset uses the continuous depth-fog method with the original sculpted front. Packaged full-voyage/ACE/restart validation, closer pillar/crew views and candidate selection remain pending.

Logo source and generation record: `Art/UI/AtlanteanOdyssey-logo.png` and adjacent JSON, generated with the built-in image generation tool and imported as `/Game/DC/UI/T_AtlanteanOdysseyLogo`.
