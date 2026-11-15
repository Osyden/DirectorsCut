# Crew follow-up build

Root owns the serialized build and packaged acceptance. The user asked the coordinated trailer chat to HOLD for a playable build; trailer returned the engine lane at 22:36. No further trailer capture is scheduled before the human playtest. Continued-polish automation remains paused.

Candidate: E:/Projects/DirectorsCut/Builds/2026-11-15_2236_crew-followup. Runtime source cc7a8b7 includes the verified AI/player wheel animation, continuous swat joint blending and the credits chat's latest acknowledgements. The Atlantean Adventure menu music was already accepted in 22:11 and remains included: it loops in the menu, starts a 0.30-second fade immediately on Start, and stops explicitly when that interval ends.

Native crew evidence is in Saved/CrewFollowup/Native. All four wheel cases pass across 147 samples with zero logged hub drift: actual AI port/starboard, player reversal/release and full restart. Ruby's 16 sampled swat poses show no detached arm/sleeve and preserve three suppression contacts. Still-frame sampling does not establish every between-frame or close-angle deformation; native typed-input-open and takeover during an AI turn were not separately exercised. See the linked wheel and swat handoffs for exact limits.

Accepted and selected at 22:39. This is a C++-only follow-up: Content, Config and project file are unchanged from 22:11. All five reused pak/ucas/utoc SHA256 hashes match the accepted 22:11 output. Fresh packaged menu/start, actual AI starboard wheel and Ruby fire-swat checks pass and exit 0. Menu music stops 0.310 seconds after Start. The wheel reaches -65 degrees with zero hub drift; Ruby's three contacts reduce remaining swats 2/1/0 and extinguish the fire by ride 22.9. Menu/credits and swat frames were inspected. The expanded credits now scroll to their final audio/type lines. Voice code and selected model/file/size inventory are unchanged; previous cold-command, speech and lifecycle evidence is inherited rather than rerun.

The usual launcher now selects this candidate via Saved/Release/Current.json. Evidence is copied into the candidate's ReviewEvidence folder; build-info.json records source and acceptance. The immutable 22:11 fallback and its previous launcher marker remain. GPU is reserved for the human playtest. Trailer and further polish remain paused until the user releases them.

Distribution remains local only. No itch upload.
