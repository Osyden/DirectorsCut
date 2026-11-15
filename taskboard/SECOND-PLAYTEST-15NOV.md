# Second human playtest pass, 15 November

Final runtime source is `b28e58a`, packaged at `E:/Projects/DirectorsCut/Builds/2026-11-15_1749_second-playtest`. Use `E:/Projects/DirectorsCut/Play DirectorsCut.cmd` for the selected local build. No itch upload is authorized yet.

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

Full-raymarch storm replacements and early thin-mist iterations were rejected. They are retained only under `Saved` as diagnostic evidence; the final generated asset uses the continuous depth-fog method with the original sculpted front.

## Packaged verification

- The 17:25 candidate was rejected after runtime duplication of a cooked Cascade module crashed on first pillar creation. The spray is now authored before cooking (`Tools/make_pillar_splash.py`, 45 local-space modules), loaded without runtime mutation and required by release preflight.
- The 17:39 candidate (`ca18f29`) passed a complete continuous voyage, menu, Start and level restart. `Windows/DirectorsCut/Saved/SecondFullVoyage.log`: six boss interrupts, two permanent limb breaks, ending at 450s, final capture at 465s with hull 72.4%, no fires or rigging damage, exit 0. Forty synthesized ACE playbacks completed after restart across Rex/Duncan/Ruby. Exactly three bell strikes, storm silence at 94.00/277.01, zero stale obstacle calls after tunnel entry and no blocked crew routes or application voice failures. One landing impact fired at ride 299.16. NVIDIA worker flush warnings remain nonfatal; this is not a claim of a warning-free log or human audio review.
- Close pillar review then exposed contact effects below the trough's rendered water surface. The final 17:49 package adds the exact shader height offset to pillar contacts and bow spray, keeps the ship's raw wave fit unchanged, and fits upright columns once around fixed submerged feet. Horizontal obstacle clearance is unchanged. This is the only gameplay difference from the completed full-voyage package.
- Final `PillarHeightReview.log` and `Screenshots/Auto/shot_01070.png`, `shot_01075.png`, `shot_01080.png` show exposed columns, submerged feet, visible foam/spray contact, hull 100% and exit 0. Three first-cluster tops are 1317.8cm; fixed feet 9.5cm, foundations 800cm. No runtime asset fallback or crash. `Saved/FinalPillarAutomation.log` passes the updated clearance/exposure suite; 122,026 CPU footprint/lane cases also pass.
- The final package's `FinalMenuSmoke.log` checks menu animation and Start again. The longer restart/ACE/ending run above was on 17:39, before the isolated contact-height correction; those systems were not changed afterwards.
- Additional native close crew captures (`CrewDuncanFeet`, `CrewRexInside`) show grounded bodies at deck/stairs. Still images and route tests do not certify continuous gait quality. Human steering feel, headphone mix, microphone input and second-machine testing remain local playtest checks.

Logo source and generation record: `Art/UI/AtlanteanOdyssey-logo.png` and adjacent JSON, generated with the built-in image generation tool and imported as `/Game/DC/UI/T_AtlanteanOdysseyLogo`.
