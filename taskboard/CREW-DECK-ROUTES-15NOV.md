# Crew deck routes — 15 November 2026

Status: source integrated into main; native connectivity and CPU geometry audit passed. Close visual review of feet and walking is still pending.

## Result

Crew now follow routes sampled from the installed brigantine instead of moving directly through its cabin, masts and guns. Spawn positions and movement are grounded against the upper deck. The route graph uses the actual side stairs between the quarterdeck and main deck. Failed route requests stop and retry; they never fall back to a straight line through obstacles.

Station reservations, assigned gunners, immediate player helm takeover/vacating, crew speech and ACE behavior remain in place. This pass does not change character or ship assets.

## Implementation

- `Source/DirectorsCut/DCCrewNavigation.cpp/.h`: a ship-local, 20 cm route graph built after ship physics is ready, with retries if initialization is too early. Floor and support traces use the hull triangles; support and midpoint probes stay near the expected deck height so overhead surfaces do not hide the planks below.
- The crew retain 24 cm body clearance and use a separate 10 cm foot support radius. Compact skeletal guns, the wheel, door and capstan reject overlapping positions. Precise component rays check body clearance and connecting segments. Diagonal links cannot cut blocked corners.
- The mast asset has broad physics boxes spanning usable deck. Two reference-pose shaft bones provide geometry-anchored trunk obstacles instead: 65 cm trunk radius plus 24 cm crew clearance. Both standing positions and route edges enforce those exclusions.
- Static rope assemblies are checked with geometry rays, not treated as a single cannon-sized solid merely because their name contains “cannon.”
- `Source/DirectorsCut/DCCrew.cpp/.h`: spawn projection, deck-relative route following, eased turning/acceleration, speed-scaled walking animation and continued foot grounding. The graph is cached; it is not rebuilt every frame.
- `Source/DirectorsCut/DCCrewNavigationTests.cpp`: automation coverage for a cabin detour, stairs, unreachable routes, stale route clearing, invalid indices, sunken initial placement and compact-fitting classification. This handoff does not claim a completed automation run for that test.

Source commits on `codex/crew-deck-routes-15nov`: `9dc48b7`, `7cb1b39`, `e2165b7`, `65688c6`, `9914856`, `32255c8`, `6967e65`. Root integrated these changes into main with cherry-picks; the final route refinement is main `8dc0f75`.

## Verification evidence

Evidence files are local to the main project, `E:/Projects/DirectorsCut/DirectorsCut`:

- `Saved/FogHybridLate.log`, 15 November at 17:10:24: `CREW_ROUTE_VERIFY passed=55 failed=0`. All 11 stations project onto usable deck, and all 55 unordered station pairs have a route.
- The same log: `CREW_DECK_GRAPH nodes=3416 ready=1 buildMs=242.6`. This is a one-time startup cost, not a per-frame cost. The exported graph has 10,688 undirected edges; all station projections belong to the same 3,183-node connected component.
- The same log at 17:10:27: grounded spawn feet are Duncan Z=505.08 cm, Ruby Z=499.56 cm and Rex Z=673.41 cm in ship-local coordinates. These are numerical grounding observations, not a visual feet-placement approval.
- `Saved/CrewNavigation.json`, exported by that run: independent Python CPU replay passes all 55 station pairs. Every exported node stays outside the inflated compact-fitting bounds. Every edge clears both mast exclusion circles; links are reciprocal. Maximum edge rise is 49.52 cm, within the 50 cm limit. No violations were found in these checks.
- The CPU replay confirms helm-to-gun paths use the side stairs around Y=±190–250 cm, descending through approximately Z=552, 543, 540, 520 and 498 cm. The longest station route is 19.8 m with 93 waypoints.
- `git diff --check` passed for the final source change. Native compilation and execution were performed by root; this agent did not launch a competing build or GPU session.

Earlier diagnostic runs documented and resolved real failures: `Saved/FogCompositeRefined.log` exposed the mast physics boxes blocking all usable deck; `Saved/SteeringCrewNative.log` and its topology dump exposed the deck-wide rope assembly being mistaken for a cannon; `Saved/FogSculptRoutes.log` showed the remaining stair/aisle fragmentation (28/55 pairs). These are superseded by the 55/55 result above.

## Remaining visual check

Root still needs a close native view of initial feet placement and walking along the stairs and narrow gun aisles. Confirm the animation does not visibly slide or clip, and that the player can take the helm with the crew member immediately clear of the camera. Numerical route connectivity and bounds checks do not prove the character animation looks natural. No close visual approval is claimed in this handoff.
