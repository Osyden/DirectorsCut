# Crew rag hand anchor

## Final disposition: remove the procedural prop

The native `CrewRag` A/B still showed cloth across Ruby's lower face in frame236, despite `anchorError=0` and `driverGap=0` on the pirate Body. Timing/body-offset hypotheses are therefore unsupported. The 30cm cloth hangs vertically with its wide end 10cm forward; at this close camera its upper span can be hidden by the head while the wide end appears below the chin. That geometry/occlusion explanation matches the image, but is not independently proven by skeleton overlay.

Root authorized the bounded fallback: remove the procedural rag and all experimental anchoring callbacks/fields. Keep the existing animated right-arm swats and the unchanged three suppression contacts. Branch `codex/crew-swat-no-prop` starts at main634430e. The final change has no animation, timing, gameplay, material or asset modification. Rerun the existing CrewFixed visual case to confirm the face artifact is gone and fire still extinguishes. Existing18 crew-work CPU checks pass.

If a cloth is revisited later, first overlay projected `hand_r`/`hand_l` and both wrist positions on the actual clothed mesh, and show the full rag wireframe through occluders. This distinguishes skin/clothing mapping from a long cloth hidden behind the head; do not make another timing adjustment based only on anchor-error logs.

## Superseded candidate

Owner: cannons_helm, branch `codex/crew-rag-anchor`. Root owns native builds and GPU.

`Saved/FourthPlaytest/CrewFixed/shot_00236.png` visibly places Ruby's cloth across her lower face while her swatting hand is raised above her head. The crew nevertheless completes three suppression contacts and the fire clears at 24s; Duncan's repair also completes. Those functional passes do not establish cloth attachment or deck contact.

The candidate resolves the rendered pirate Body mesh, which follows the hidden mannequin through leader-pose mapping, and positions the rag from its mapped `hand_r` bone in the driver's bone-finalization callback. This uses the evaluated pose and rendered body's world transform. The cloth retains its existing hanging orientation, dimensions, material, lifetime and suppression timing. Delegate cleanup occurs on EndPlay. No animation or audio changes.

The installed UE5.8 source confirms `FinalizeAnimationUpdate` flips pose buffers through `FinalizeBoneTransform`, broadcasts the public callback, then updates socket children. `GetBoneTransform` on a follower uses the leader's mapped bone with the follower's component transform. Ordinary socket attachment should already update; therefore the exact cause of the previous visual displacement is not claimed proven until the native A/B below.

Validation: 18 existing production crew-contact CPU checks pass; `git diff --check` passes. No Unreal build or renderer run by this agent.

Root review: rerun the same CrewFixed scenario with `-DCCrewRagReview`. Capture a full-body angle at windup and contact, including 23.6s from the former close-up. `CREW_RAG` should name the pirate `Body`, report `anchorError=0`, and show the rag's top at the visible hand; `driverGap` identifies any hidden-driver/rendered-body offset. Verify no cloth remains after the final contact/assignment cancellation, still exactly three `CREW_SWAT` contacts, and fire clears. A zero anchor error proves internal anchoring only; visually verify hand/cloth and cloth/fire contact before accepting.
