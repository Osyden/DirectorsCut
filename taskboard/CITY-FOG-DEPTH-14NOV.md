# City fog depth and exit concealment — 14 November

Publication: source, new materials and curated review images are collected on `codex/fog-fire-polish`; see [publication scope](FOG-FIRE-PUBLICATION-14NOV.md). Earlier uncommitted-status notes below describe the historical working state.

Owner requested deeper layered city fog and a properly concealed exit, with repeated screenshot comparisons to fog-city-reveal.jpg and fog-to-maelstrom-portal.jpg. Work is isolated on codex/city-fog-depth, with runtime tests in the existing private pending-weather project. Its content junction is read only for this pass. DCPortal and maelstrom-owned files are not edited; the treadmill remains the single owner of city actor visibility.

## Baseline and first pass

The exit is a water shell about 700 m tall, eligible to appear with the city. The global fog falls off rapidly with height, and the local fog grid ends at 700 m. Consequently its crown can remain visible above the low banks from far away. The current city also increases fog colour intensity threefold, washing out depth.

First candidate: withhold the exit beyond 650 m, add two full-height approach curtains that thin continuously over distance, raise and deepen the banks around the ruins, and give landmark curtains gradual distance fades. The gate accounts for the diagonal distance to the 700 m crown inside a 1 km fog grid. Blend these changes in after the accepted storm and out on leaving the city. Reduce the city fog brightness/scattering boosts and use a broader height distribution.

The reference target is a readable warm deck against several differently obscured layers of cold ruins, with the destination hidden until the approach. Reject a uniform whiteout, featureless dark fog, obvious curtain boundaries, hard portal appearance, or renewed waterline gaps. Passing compilation is not visual acceptance.

Results will be recorded after matched captures and normal-speed transition checks.

## First comparison / second candidate

Baseline captured continuously at normal speed through 246 s. The first candidate used continuous 3x time for exploration, so its captures are not the final matched evidence. The exit is withheld until 650 m (about 169.6 s in that run), but the city looks darker rather than materially deeper; reject as final. The curtain hides the opening at 190 s while the deck remains visible, though nearby fires colour the fog strongly.

Second candidate restores some mist brightness, adds a broad upper layer through the existing UDS fog component, and gives four landmark curtains native heterogeneous volumes for shape and shadow. Trial restrained native Fog Screen Space Scattering. Capture and restore the original second-layer and scattering settings on city exit, backward seeks and end play. No changes to the sky/maelstrom owner's files or vendor materials.

## Second comparison / third candidate

The 100/150/165/170/175/220 s continuous 3x captures show stronger depth and no exposed destination when the visibility gate opens. However the upper haze is too extensive and washes out the sky; scattering also softens the image more than needed. Reduce the upper-layer density from .018 to .012, make it diminish faster with height (.025 falloff), and lower its height offset to 30 m. Reduce scattering amount .45 to .25 and spread .06 to .04. Set city fog colour scale to 1.6, between the overly dark first pass and pale second pass. Next evidence must use normal speed and include the original 100/150/190/220 s views.

## Runtime integration corrections

The first third-pass launch stopped during startup without a reported fatal error or a new crash dump. Retained its log; retried and reached normal-speed gameplay. Other checkouts were also running unattended captures; no external process was stopped.

The normal-speed 100 s capture retains foreground detail and conceals the distant destination. Runtime logging found that updating UDS's distance parameter left its live volumetric component at 700 m. Apply and restore the native component's distance directly. Also remove the old fog bank's global shadow-trace-distance override: the newly enabled banks must not alter the accepted storm's shadows. These corrections require a final rebuilt run.

## Applied and verified

Integrated DCFogBank.cpp/.h, DCRideDirector.cpp/.h and DCTreadmill.cpp into main after checking each against its starting hash. All other source and configuration files still match the baseline snapshot. No purchased assets, portal geometry, maelstrom code or material assets changed. Main editor module rebuilt successfully. Changes remain uncommitted.

Final main run completed normally at normal speed from 0 to 252 seconds, with 22 captures at 1280x720. Inspected the opening front, city entry, settled city, portal approach and slide handoff. Captures around 169.5–169.7 seconds bracket the 650 m visibility gate: the destination remains behind the fog at activation. By the close approach the opening emerges through the thinning curtain. The 100/150/190 s before-and-after pairs are under `review/city-fog-depth/`; these are unretouched gameplay captures at matching ride times. Random lightning, ship motion, steering collisions and fires vary between runs.

Runtime confirms the native fog grid reaches 100000 cm in the city and restores 70000 cm at exit. The upper layer restores to density 0 and screen-space scattering restores disabled, with its original amount 1. A separate restart/backward-seek/re-entry/exit run also completed with exit code 0 and confirmed these restorations three times. A side-facing diagnostic was visually inspected; seek diagnostics are not used for the before-and-after comparison because seeking does not advance scenery like continuous play.

Final main and restoration logs have no fatal error, assertion, Blueprint Runtime Error or material compilation failure matches. Existing UDS editor-utility and engine toolset/Python initialization errors remain in unattended startup. Other checkouts rendered concurrently during verification, so GPU snapshots are not an isolated performance benchmark. No standalone package was built.

Evidence: `E:/projects/directorscut/city-fog-review/` contains the baseline, rejected candidates, final normal-speed main run, restoration log, source snapshot and an incremental `city-fog.patch`. The runtime capture processes owned by this pass exited; other checkouts were left running.

Visual result: the exit no longer advertises its rings across the early city; ruins have more height-dependent haze and separate banks, with gradual landmark reveals and a readable warm deck. The fog still lacks the references' fine authored turbulence, and the existing broad city lightning can flatten contrast briefly. This is a tested improvement in depth and concealment, not a claim of AAA parity.

## Owner feedback: white banks and uniform haze

The owner rejected the tall white patches in the 150 s capture. The neighbouring grey fog is the desired direction. This pass works on the same owned files and isolated checkout, with no material assets or portal geometry edits. Evidence is in `E:/projects/directorscut/city-fog-cohesion/`.

A diagnostic continuous 3x run with heterogeneous-bank density set to zero confirmed that more than one fog treatment needed attention; it is not used as a matched before/after. The first candidate uses smaller, more eroded city billows, lower cloud albedo and extinction, no extra emissive light on the four heterogeneous curtains, half the upper haze density, and no screen-space scattering by default. A normal-speed 0–252 s run completed successfully. The 150 s tall patches blend better, but a bright low band remains, so this is not the final candidate.

The second candidate also lowers native city-bank albedo and emissive fill and reduces their added lightning flare. Staggered banks at varying heights across the shoulders of the city provide local obscuration in place of the removed uniform haze. They use a separate random stream and seeds, preserving existing curtain and card placement. The portal visibility gate and its density/fade settings are unchanged.

The second normal-speed run's 100/125/145/150/155 s views are more coherent in colour, but too transparent in places. The third candidate restores some extinction to the four heterogeneous curtains (.00085 versus the original .0012 and trial .00045), increases the new banks' density and lowers their centres, and restores a little background haze (.008 upper layer versus original .012 and trial .006). The same city lighting treatment now fades onto ship-following sea mist, removing its separate bright fill after the storm. Screen-space scattering remains off.

The second run also exposed a strong sun-facing white patch at 170 s. The third candidate lowered city sun volumetric scattering from 1.25 to .45. A temporary scattering-distribution override was removed after runtime logging confirmed the sky already uses the desired .2 value. The third normal-speed run completed successfully but made the early sea mist too subdued. Adjacent later captures also show the central veil brightening sharply, so sun intensity alone did not resolve it.

Fourth candidate: restore part of the sea-mist fill (.35 of original, instead of .08) and sun scattering (.75 instead of .45); strengthen the staggered grey banks and retain more of their noisy body; lower the exit curtains' albedo without reducing their extinction; lower city lightning flash intensity to 1.3 and its fog scattering to .35, blended back to the existing values outside the city. Lightning bolts remain. A normal-speed run through 252 s completed, but a strong central light shaft remains near 170 s.

Further 3x diagnostic runs independently disabled fog cards, all local banks, the exit, sky fog noise, volumetric clouds, and volumetric fog. The central bright shaft is not removed by disabling cards, local banks, the exit, sky noise or clouds; it is associated with the volumetric fog lighting. Material parameters and mesh bounds were inspected directly in Unreal rather than inferred from generator scripts. Diagnostic switches are not final settings, and their accelerated captures are not matched before/after evidence.

Final candidate drives the live UDS sun component's volumetric scattering directly to .25 in the city, alongside its Blueprint setting. It captures and restores the actual prior light value on city exit, seeks and end play. Only DCFogBank.cpp, DCRideDirector.cpp/.h and DCTreadmill.cpp change in this follow-up; no material assets or portal geometry edits.

## White-bank follow-up: applied, with remaining visual work

Applied the final four files to main after checking ownership and starting hashes. Main editor module rebuilt successfully. The final main run completed continuously at normal speed through 252 seconds, capturing 20 views at 1280x720. Inspected entry, the marked 150-second view, later city views, close exit approach and the slide handoff. Before/after images are in [review/city-fog-cohesion](review/city-fog-cohesion/), including [before 150](review/city-fog-cohesion/before-150.png) and [after 150](review/city-fog-cohesion/after-150.png). These are raw gameplay captures at matching ride times; ship motion, collisions, fires and lightning vary.

The circled side banks now have less separate white glow, smaller irregular billows, and staggered grey layers. Reduced uniform upper haze and disabled screen-space scattering retain more separation. The 650 m exit gate and its gradual curtains remain in place; the final run logged activation at 169.58 seconds behind the curtain. The accepted early storm geometry is unchanged.

The live sun scattering measured 1.080 when saved during continuous play, .250 in the city, and 1.080 on restoration. This does not establish a Blueprint caching defect. A separate restart/backward-seek/re-entry/exit run completed cleanly and logged three successful restorations, including the originally saved sun values (3.000 on direct entry, .600 after returning to the opening). Both runs restored fog distance to 70000 cm and second-layer density to zero. Neither log contains fatal-error, assertion, Blueprint Runtime Error or material-compilation-failure matches. The existing unattended startup warnings remain. Only the four owned source files differ from the source/configuration snapshot taken at the start of this follow-up; main and the owned worktree match. No standalone package was rebuilt.

Remaining issue: a strong central forward light shaft is still visible around 150–172 seconds. The sun-direction diagnostic also failed to remove it and was not applied. A final isolated 3x run with `ShowFlag.LightShafts 0` completed with exit code 0 and still showed the bright patch at 170 seconds, so disabling the separate screen-space shaft effect is not the fix. The current result improves the marked side banks but does not meet the references' layered, enveloping fog quality. Do not call the white-glare issue fully fixed or claim AAA parity. The next focused investigation should isolate the live volumetric light contributions and fog in-scattering in that view, then compare adjacent frames while preserving obscuration; further global darkening or thinning risks losing the better grey banks. The reference also needs more authored mid-distance variation, rather than another uniform haze increase.

Evidence and rejected diagnostics remain in `E:/projects/directorscut/city-fog-cohesion/`, including `main-final.log`, `main-restore.log`, build logs, the starting source/hash snapshot and `city-fog-cohesion.patch`. All capture processes belonging to this pass have exited. Other agents' checkouts were left alone. No isolated frame-rate claim is made because other renders overlapped some verification runs. Changes remain uncommitted.
