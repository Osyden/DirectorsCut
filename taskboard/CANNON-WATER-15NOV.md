# Cannon water impacts and helm wheel direction

Owner: cannons_helm; `codex/cannon-water`, based on main `9c3ecd3`. Root owns builds/GPU and adds the coordinated `ADCAudio::CannonSplash(const FVector&)` API. No engine/editor job was launched in this worktree.

## Changes

Ordinary-ocean cannonballs now intersect the rendered water: a dedicated DRW sample for each live shot, corrected from its delayed capture position using the sampled slope and bounded vertical velocity, followed by the exact existing steering/trough WPO offset. The query pool is capped at 64 channels. Channels remain attached to shots through array swaps, reused-channel samples from a prior shot are rejected, and stale/missing GPU data falls back to mean water plus the trough. The reflection path preserves the optional plugin dependency. This uses a local tangent approximation to the GPU waves between frames; it is not a synchronous render readback.

The first air-to-water crossing is refined inside every ballistic step; real geometry/hostiles still win if in front of that water point. The existing analytic maelstrom/tunnel surface intersection is unchanged. Sky shots live long enough to descend to water, capped at 15 seconds. Cannon ownership, independent reloads, aim behavior, speed, gravity and damage remain unchanged.

A water contact uses the already cooked `/Game/DC/FX/P_PillarWaterContact` at the surface, with a small 35cm lift. It never duplicates Cascade assets at runtime. Emission stops after 0.12 seconds, the component is destroyed by 1.6 seconds, and no more than 32 live bursts are retained. Actor teardown also destroys them. Missing content retains the simple geometric spray fallback. `CannonSplash` is called exactly once at terminal water contact, separate from stone explosions/thunder. Root implements the restrained voice-ducked recorded sound. The splash asset is already required by release preflight; no new asset generation is needed.

The wheel's visual angle sign is reversed about its existing forward axis and hub pivot. D remains positive starboard steering, A remains negative port steering; the upper rim now moves to the corresponding side from the helm/aft camera. Steering inertia and ship motion are unchanged.

## Checks and native review

`Tools/probe_cannon_water.py`: 24 CPU checks pass against production sweep/math. Covers flat water, rising slopes, both endpoints above an intervening crest, earliest of multiple crests, trough WPO, query age/reuse, upward and downward ballistic return, and wheel rim direction. `git diff --check` passes. This is not Unreal compile/render validation.

Root integrates the audio helper then this commit and compiles. Run existing `-DCCannonVerify` unchanged, and a normal-render review with `-DCStartAt=0 -DCStation=starboard -DCWaterImpactReview -DCGunLog` for at least 24 seconds. This fires near and far starboard water shots at 3/7 seconds, an elevated shot at 11 seconds, and port water at 15 seconds through the real gun path. `WATER_REVIEW` reports accepted triggers; each water miss logs one `WATER_IMPACT`, ideally `sampled=1`, with the cooked effect name and bounded live burst count. Inspect water contact from the actual cannon camera, sound level, no stone hit feedback on water, natural disappearance and no persistent columns. Repeat in the caldera and in the packaged build. CPU/null rendering necessarily uses the height fallback.

Wheel review: `-DCStartAt=0 -DCStation=helm -DCHelmSequence -DCHelmLog -DCLaneLog`. Positive steering from seconds 3–7 should produce negative wheel angle and starboard lane motion; seconds 7–11 should reverse both. Check first-person and aft ship views and zero hub drift. Root owns these runs and must preserve the user's selected 18:33 package.
