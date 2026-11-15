# Fourth playtest integration

Selected at20:51: `E:/Projects/DirectorsCut/Builds/2026-11-15_2048_fourth-playtest/Windows/Atlantean Odyssey.exe`, runtime8ee64ab09750acb39d088c2691201d83daf8a1e6. The normal `Play Atlantean Odyssey.cmd` launcher now selects it.19:24 remains the fallback in `Saved/Release/Previous-1924.json`. No itch upload. Root's engine/game/model processes are closed; the GPU is reserved for the next human playtest. Continue CPU-only investigation while the human tests.

Final acceptance: the20:48 executable/package succeeds; all five cooked pak/ucas/utoc files are SHA256-identical to the fully cooked and voyage/boss-tested20:39 candidate. Focused20:48 envelope review confirms.3120 descent gain and.2080 combat gain, and one bow landing at25.43. Its normal-speed music/voice review confirms222.01s track start,.0520 speech duck and all three Piper/Audio2Face voices. Exact provenance and checks are saved in the build's `build-info.json` and `cooked-content-comparison.json`.

## Requested changes now in source

- Boss: explicit projected joint/mouth targets, hits and time remaining, edge direction, obstacle warning/safe side, hit/interrupt feedback; stronger held slam anticipation; original Kraken limb skin/material posed along the committed attack path. Separate limbs retain submerged roots; existing permanent main-body cuts remain.
- Caldera: faster finite steering/reversal, no invisible centering across most of the lane, heading follows travel up to40deg, minimum75% authority with torn rigging, concentric foam at the actual inner limit on the existing FFT surface. Bow contact triggers landing about3.75s earlier.
- Crew: three timed arm swats use retained fire suppression; speech describes smothering. The experimental cloth prop intersected Ruby's face and was removed; swat animation/contact timing remain. Damaged rigging has a compact HUD destination/distance marker that remains visible through the mast, plus the existing deck arrow and nearby repair interaction.
- Other AI NPCs chat: free conversation, character cards, current scene/crew/ship/weather facts, bounded history and appearance knowledge, with conversation separated from orders. Existing voice/Audio2Face pipeline retained. Entry seam integrated by root.
- Music: second track starts at222s (two seconds later), louder through tunnel/descent (+3.5dB), old quiet mix preserved during full speech duck; boost fades back for combat.
- Ruby: ordinary+deep shadows from paired midmast lanterns reduce excessive hair backlighting. Hair color/material unchanged. VSM overflow text is disabled by default in config/launcher.
- Credits: repeated game title removed; director/developer blocks share a row, spacing fits every attribution on one page without scrolling. Native1600×900 capture includes the final Cinzel credit and Suno.
- Tunnel: unnecessary shaft-enable toggle deferred to caldera handoff. Matched captures did not demonstrate resolution of reported strange lighting, so this remains open alongside white sail/fog edges. No global exposure/fog change.

## Verification so far

- Final editor build succeeds (`Saved/FourthBuild9.log`), including repair HUD, cloth removal, final conversation guardrails, chat hint and compact Credits. Local-name compile collisions were fixed without behavior changes.
- CPU:47,839 caldera checks including384 committed obstacle dodges and cross-frame-rate landing;6,907 boss marker/limb mapping;11,117 cue checks;18 crew contact checks. These counts include iteration/assertion cases, not independent gameplay runs.
- Generated limb-mask, boss-band and boundary-water materials. Temporary incomplete water graph warnings occurred during generation; final native boss render has working water and no material compile failure. Release preflight passes49 essential assets and installed steering shader hashes.
- `Saved/FourthPlaytest/BossPassive`: actual authored17-bone limb renders with cups/skin; explicit marker readable at342/346/351, gone after strike353; no exposed torso/spikes in inspected frames. The distal limb is stretched and exits frame during anticipation; final motion quality is not established by these stills. World band remains obscured; readability improvement is the explicit marker.
- `BossInterrupt`: actual hit count drops2→1, interrupt event1 at elapsed76.39, ahead of strike82. Separate later whole-boss success/death and other-side review still pending.
- `Crew`: all three evaluated animation drivers load. Ruby walks to fire, visibly raises/lowers the arm and applies remaining2/1/0 once each. Final `CrewHelm` at20:37 validates the no-prop source, visible HUD repair destination14m away despite mast occlusion, and marker disappearance after Duncan repairs. `CrewFixed` also passed55 actual navigation route pairs. Cloth visibility was rejected in the intervening `CrewRag` review; zero anchor error did not establish an acceptable visual result.
- `Saved/HairLightingReview`: paired shadowed lamps retain the hair improvement of all four at lower measured sample GPU cost. Single frames are not a benchmark; combined voyage performance remains to check.

The combined Windows candidate started packaging at20:39 from runtime83235ab. Source/config frozen; accepted19:24 package remains selected. Required before selection: successful cook, packaged full voyage/ending, complete boss success/interrupt/death, and packaged music/three-voice checks. Original fog outline/tunnel lighting report remains under scoped investigation.

## Packaged verification and final audio correction

The20:39 candidate successfully compiled, fully cooked4399 packages, staged branded executables/sidecars and passed:

- Packaged menu, animated sea/ship, both information panels and Start. Credits are complete on one page, including Suno and the final font attribution.
- Continuous voyage through470s at3×, held-right route: no stone strikes, one physical bow landing at elapsed26.70 on the turned trajectory, ending reached. Native neutral-speed landing remains25.49.
- Full boss success: damage8, taken0, two limbs severed, six interrupts, deadline160, ending470. Mouth target/countdown at360, other-side feedback at401 and defeat/silhouette at430 were inspected.
- Normal-speed packaged three-crew Piper/Audio2Face smoke succeeds. Music starts222.01s and speech-ducked Caldera gain reaches.0520.

The full voyage also exposed a real music-envelope bug: audio BeginPlay could read the director's untrimmed route and keep the descent boost until440s. Commit8ee64ab reads phase timing during world ticks, after the director removes the optional hell leg. This is the only Source/Config/Content change from83235ab. The20:48 replacement rebuilds executable code and reuses that successfully tested full cook; no assets/config/class layouts changed. It receives focused packaged envelope and normal-speed voice/music checks before selection. The earlier pipeline-only pass did not imply the envelope was correct.

## Additional native evidence

- `Landing`: one bow contact at elapsed25.49 / ride295.49, bow clearance10.3cm and deck pitch−13.8deg. Inspected adjacent frames show pitch/rebound and settling, with no repeated impact or landing damage. Splash is still soft mist rather than a crisp water sheet.
- Eight native automation suites pass across commands, conversation routing/cleaning, crew deck routes, caldera steering and tunnel clearance (`Saved/FourthAutomation`).
- Initial `ConversationLlama` completes eight actual Qwen/Piper/Audio2Face responses with unchanged order counts and scene history reset. Six answers were grounded; one advised approaching banks and another invented still air. The AI NPCs chat supplied a narrow navigation/weather guardrail, integrated as15e7ad7; corrected semantic rerun remains required. Pipeline completion is not semantic acceptance.
- `SailSSS`: all nine actual sail MIDs retain SSS0 and Camera0 at165/167/169, yet the bright lattice remains. No production SSS change is justified. The original fog/temple outline issue stays open; the next diagnostic should isolate physical mast/rope geometry before changing more sail parameters.
- Final `ConversationLlama` at20:34:47 uses the corrected scene/identity context:8/8 generated responses, unchanged orders, positive Audio2Face samples and complete playback. Manual review accepts actual duties, outfits, fog/no-rain and fire/rigging mechanics. Minor loose personal phrasing remains. Median model generation511ms; buffered speech starts around13.4s, so generation speed is not spoken-response latency. The optional ACE backend passed its earlier eight-case pipeline; its last semantic guardrail iteration has not been rerun. See `CREW-CONVERSATION-15NOV.md`.
- `Music` native run starts Caldera at222.00 and retains.0520 speech-ducked gain. Its two-repeat smoke fixture crossed the240s scene transition and had a queued line canceled after two speakers; this is not a three-voice pass. Packaged music review starts205s with one cycle, while the continuous voyage separately checks the silent descent boost.
- Final `Boundary` and `BoundaryHigh`: wider broken foam crests and reduced nearby whitecaps are generated in the actual water material. Elevated325/330 captures show two separated curved crests on the water between ship and Kraken. Ship settles around821.8m; nearest hull envelope797.4m versus outer crest793.6m. Placement passes. Low cannon/helm views and sun glare still weaken readability; no all-angle visual claim.
