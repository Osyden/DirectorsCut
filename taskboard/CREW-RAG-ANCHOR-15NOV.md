# Crew rag hand anchor

Owner: cannons_helm, branch `codex/crew-rag-anchor`. Root owns native builds and GPU.

`Saved/FourthPlaytest/CrewFixed/shot_00236.png` visibly places Ruby's cloth across her lower face while her swatting hand is raised above her head. The crew nevertheless completes three suppression contacts and the fire clears at 24s; Duncan's repair also completes. Those functional passes do not establish cloth attachment or deck contact.

The candidate resolves the rendered pirate Body mesh, which follows the hidden mannequin through leader-pose mapping, and positions the rag from its mapped `hand_r` bone in the driver's bone-finalization callback. This uses the evaluated pose and rendered body's world transform. The cloth retains its existing hanging orientation, dimensions, material, lifetime and suppression timing. Delegate cleanup occurs on EndPlay. No animation or audio changes.

The installed UE5.8 source confirms `FinalizeAnimationUpdate` flips pose buffers through `FinalizeBoneTransform`, broadcasts the public callback, then updates socket children. `GetBoneTransform` on a follower uses the leader's mapped bone with the follower's component transform. Ordinary socket attachment should already update; therefore the exact cause of the previous visual displacement is not claimed proven until the native A/B below.

Validation: 18 existing production crew-contact CPU checks pass; `git diff --check` passes. No Unreal build or renderer run by this agent.

Root review: rerun the same CrewFixed scenario with `-DCCrewRagReview`. Capture a full-body angle at windup and contact, including 23.6s from the former close-up. `CREW_RAG` should name the pirate `Body`, report `anchorError=0`, and show the rag's top at the visible hand; `driverGap` identifies any hidden-driver/rendered-body offset. Verify no cloth remains after the final contact/assignment cancellation, still exactly three `CREW_SWAT` contacts, and fire clears. A zero anchor error proves internal anchoring only; visually verify hand/cloth and cloth/fire contact before accepting.
