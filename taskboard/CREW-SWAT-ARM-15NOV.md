# Crew swat arm continuity

Owner: cannons_helm, isolated `codex/crew-swat-arm` from main9fc9962. Scope: crew procedural pose and its CPU policy checks. Frozen22:11 package remains unchanged.

The user observed an arm coming loose while Ruby swats a fire. CPU-decoded frames from `Trailer/live/jobs2-visible/capture.mp4`11–14seconds show the sleeve folding up behind the head and a rapid pose change during each recovery. This is distinct from the removed rag prop and the trailer camera turn.

The previous solver blended the hand target with its idle position, but then applied a full two-bone solution with a fixed global elbow pole. Even at zero envelope weight, that solution can place the elbow somewhere other than the idle elbow. Thus the arm did not converge to its underlying pose at the branch that ends the stroke. The shoulder-high, closely folded windup also exposed severe sleeve deformation. A separate MetaHuman helper-bone/skin issue cannot be ruled out without the native comparison.

The correction blends final local joint rotations by the stroke envelope, retains every original local translation/scale plus wrist and fingers, transports the existing elbow plane with the reach, and checks the actual two-bone parent chain before solving. The raised hand is36cm ahead and5cm above the shoulder rather than20cm ahead and28cm above. Torso bend, contact at0.42seconds, duration1.05seconds, three suppression contacts and cancellation behavior remain unchanged.

Validation: `Tools/probe_crew_work.py` compiles the production policy with MSVC;2,124checks passed. These check contact timing, cancellation, bounded blending and convergence at both ends. They do not establish rendered skin quality. No engine/build/render job was run by this agent.

Required native acceptance: fixed deck camera, `-DCCrewWorkReview -DCNoVoice -DCNoMenu -DCStartAt=8`, capture Ruby's full shoulder/elbow/hand through all three strokes at60fps (including first windup, contact, recovery and idle). Do not use the rotating trailer jobs camera as the acceptance reference. Confirm no sleeve/arm separation, wrist reversal or endpoint snap; log three `CREW_SWAT` contacts withremaining2/1/0 and no changed fire timing. Also inspect one Duncan/Rex stroke if practical because their visible bodies share the same pose driver scheme. Root owns build/GPU and integration.
