# Cloud front and fog realism review

Codex passive review, 13 November 2026. No editor interaction, rendering runs, configuration changes or code edits were performed. Claude is actively changing the front and VDB integration; reconcile this plan against its next handoff before implementation. This plan complements the maelstrom plan but does not transfer ownership of the storm front.

## Evidence and visual diagnosis

Reviewed the owner's three images in `E:/Projects/DirectorsCut/Feedback`: `cloud issues.png` (20:09), `tunnelissues.png` (22:48), and `fog issues.png` (22:51). Also viewed the existing `Saved/Screenshots/contact.png` contact sheet written at 23:03. The latter is a changing scratch output, not a permanent review artifact. Source reads include uncommitted work newer than the feedback images, so they cannot prove what settings produced each image.

The scene needs consistent depth, illumination and cloud structure before more effects. The target is a coherent, storm-dark wall with a curling navigable throat, followed by damp, layered sea fog that reveals monumental architecture selectively. Photorealism is a visual acceptance target, not something guaranteed by importing a VDB.

| Evidence | Visible problem | Interpretation and confidence |
| --- | --- | --- |
| `fog issues.png` | Bright isolated cloud patches, dark scenery-shaped gaps, a nearly even grey backdrop | Strong evidence of an inconsistent lighting/depth result. Exact contribution of physical backlighting, compositing and missing foreground extinction is unproven. |
| `tunnelissues.png` | Brown cloud arch, bright rims around mast/rigging, sunlit open sea visible through it | The tunnel looks detached from its weather system. Edge halos may involve depth reconstruction, post-processing or real backlight; isolate before assigning blame. |
| `cloud issues.png` | Flat pale belt at the sea/cloud join, with visible cloud forms seeming to stop above it | Likely mismatch among cloud base, low fog skirt and distance fog; compare layers individually. |
| 23:03 contact sheet | Small independent ring grows toward the ship, then becomes a fairly uniform grey interior | The transition lacks a continuous storm mass and readable throat. It does not yet establish entry into an immense cloud front. |

Do not call all silhouettes a rendering bug. A statue in front of a bright cloud should occlude it. A statue behind dense foreground fog should lose contrast progressively. A clean statue-shaped hole through fog that lies in front of it is an error. This distinction is the first diagnostic test.

## Confirmed source findings

- `DCRideDirector::SetupUDS` unregisters the original director sun, skylight, atmosphere, fog and cloud component before loading UDS. Do not assume the old `M_StormCloud` shader is active alongside UDS simply because it exists in the repository.
- Legacy spherical cloud meshes in `DCFogFront` are off by default (`bBillows=false`). They are not an established explanation for the current popcorn look.
- The active design combines UDS sky/radial storm, UDS height/volumetric fog, large local fog volumes under the front, generated local fog banks and now a heterogeneous VDB tunnel. EasyFog and FogArea are installed; their presence alone does not prove they contribute to the shown frames.
- `FogGlow` is continuously derived from sky intensity. `M_FogVolume` writes a shaped version of that light into Emissive. Heterogeneous banks also receive a scaled Glow. These are confirmed sources of self-emission, not just a guess from white pixels.
- `TickUDS` raises fog colour intensity toward 3 and overcast day brightness toward 1 as mist builds. These controls can further lift the fog independently of directional modelling. Exposure also varies by phase.
- `M_HeteroCloud` is an eroded ellipsoid using the same noise at three frequencies, with a clipped lower region. `M_FogVolume` uses a related ellipsoid/noise construction. Repeated placements can visibly advertise those forms.
- During this review, the current uncommitted `DCTreadmill` changed major banks/reveal curtains to opt-in heterogeneous rendering via `-DCHeroBanks`. Older STATUS entries describe them as default. Capture logs must record the actual binary and flags; the two looks cannot be conflated.
- New `M_VdbCloud` separates density, albedo and emission, but still uses a second SVT channel labelled temperature to drive Glow. The tunnel sets a small nonzero baseline glow. Validate that channel's meaning rather than assuming it encodes cloud illumination.
- The new tunnel enables blocking SVT streaming requests. This can help diagnose complete frames but needs a shipping playback test and replacement with preloading/nonblocking streaming if it stalls.
- Cloud render target mode is explicitly set to **1** in the director. Advice to switch from mode 2 to mode 1 would not address the currently inspected code.
- Two places write the global heterogeneous-volume view distance: the new tunnel sets 6 km, while hero-bank setup sets 1.2 km. If both execute, order can change the result. Consolidate ownership and log effective values; this is a potential clipping issue, not a confirmed cause of the submitted images.

## First pass: isolate the rendering faults

Save one fixed-camera, fixed-time baseline with explicit output resolution, screen percentage, exposure, lighting and feature flags. Warm shaders and streamed volume frames. Keep a second small camera-pan clip to reveal sorting and temporal errors. Do not compare different lightning phases or exposure values as evidence of a rendering fix.

Use a minimal diagnostic scene or isolated development mode: water, mast/rigging, one opaque statue and one cloud. Place the statue in front of, inside and behind the volume. This establishes whether depth works before evaluating an entire city.

| Comparison | Question answered | Pass condition |
| --- | --- | --- |
| UDS clouds alone; VDB alone; each fog system alone; then pairs | Which pair introduces the cutout or seam? | The first failing combination is identified with matched images. |
| All cloud/fog emission zero, lightning off, fixed exposure | Are white patches lit or self-emitting? | Shadowed folds darken; bright rims correspond to the light direction. |
| Native resolution with frame generation off; then intended upscaling | Is the halo caused or amplified by temporal reconstruction? | Thin ropes remain stable without broad bright borders. |
| Intended post-process versus local exposure, bloom, sharpening disabled | Is tone mapping exaggerating edge contrast? | Foreground edges stay plausible with and without the final grade. |
| Full-resolution volume diagnostic versus current downsampling | Is this a low-resolution depth/composite boundary? | A quality change has a clear, repeatable effect on the same edge. |
| One volume versus overlapping volumes, with camera crossing their centres | Is object-level volume sorting unstable? | No cloud jumps in front of another or reveals a box-shaped intersection. |
| UDS ground skirt off/on and height fog off/on separately | What creates the flat horizon belt? | Fog joins the sea continuously without a horizontal compositing strip. |
| Main depth and actor visibility inspection for suspect ruins | Is it real occlusion or incorrect proxy/material participation? | Visible geometry and depth agree; no unseen proxy cuts a hole. |

The tunnel screenshot also contains a virtual-shadow-map queue-overflow warning involving non-Nanite meshes. Treat it as a separate shadow-budget defect. Inspect offending casters and their screen/light coverage; do not simply suppress the warning or claim it caused the fog outlines without evidence.

### Verified engine controls for diagnostic use

The following were read in the installed UE 5.8 renderer source, not inferred from old internet advice. Their effective runtime values still need recording. Use temporary comparisons, then keep only measured changes.

- `VolumetricRenderTarget.cpp`: mode 1 traces at half resolution; mode 3 is full-resolution cinematic tracing and is useful as a diagnostic reference. Mode 2 cannot intersect opaque meshes correctly. Current game source requests mode 1; confirm UDS/scalability does not later override it.
- `HeterogeneousVolumes.cpp`: `r.HeterogeneousVolumes.DownsampleFactor` controls viewport resolution. Compare 1 with the actual shipping value for edge quality, then measure cost.
- The same file exposes `HeightFog`, `VolumetricFog` and `ApplyFogInscattering` under `r.HeterogeneousVolumes`. Test their interaction separately; do not permanently disable atmospheric integration just because it makes a cloud sharper.
- `r.HeterogeneousVolumes.Composition` requires the project's **Composite with Translucency** support. Without support the code forces composition after translucency. Flipping the console value alone may do nothing. Also, translucency order is not automatically the explanation for an opaque statue artifact.
- `r.HeterogeneousVolumes.DepthSort` sorts by volume centroid. This does not establish correct per-voxel blending for every intersecting arrangement. Prefer fewer deeply overlapping hero volumes until the actual rendering path is proven.
- `VolumetricCloudRendering.cpp` includes late fog application and an optional fogged cloud buffer for translucent composition. Inspect these only if the failing pair implicates that path.

Keep the working image plus the smallest necessary setting change. A collection of unrelated console-variable tweaks will make the next problem harder to diagnose.

## Second pass: one consistent lighting model

Start the front, VDB and fog with **zero persistent emission**. Disable lightning during this calibration. Use the same sun direction, plausible skylight fill and fixed exposure across the test shot. Water droplets should mostly scatter incident light: dark undersides should result from optical depth and shadow, not simply colouring every material nearly black.

Calibrate one VDB in open lighting, then shade it under the intended storm. Preserve thick dark interiors, mid-tone folds and a limited lit edge. Match the UDS cloud brightness to this object, rather than giving every independent layer its own artificial brightness floor. Reintroduce a small artistic fill only if a measured renderer limitation requires it, and judge it across daylight, shade and lightning.

The tunnel's demo brown/teal colours should not survive into the storm palette. Use neutral to slightly cool scattering and a restrained warm edge from the receding daylight. A constant luminous blue cloud interior suggests fire or magic; reserve that for an intentional supernatural event.

Lightning should illuminate a region, expose several depths briefly, then fall back into shadow. It must not make every bank white at once. Check the brightest frame as well as the frames immediately before and after, with and without temporal history. A localised emission mask can be an artistic lightning fallback; it should have a source, spatial falloff and short lifetime.

## Third pass: coherent front, throat and city

Use a deliberate division of responsibilities:

| System | Primary role | Constraint |
| --- | --- | --- |
| UDS sky/cloud layer | Distant sky, large storm envelope, environment lighting | Keep the sky component authoritative; do not place an unmodified solid storm wall through the navigable throat. |
| Selected VDB tunnel | Close/middle-distance rolling throat and its immediate mouth | One coherent volume first; no detached doughnut floating ahead of a separate weather wall. |
| Volumetric/height fog | Continuous low-frequency extinction through air and around ruins | Provides depth, not every visible detailed cloud lump. |
| Selected local detailed banks | A few foreground shoulders, occluding curtains and billows | Irregular distribution, consistent light, no repeated pairs along the entire route. |
| EasyFog cards | Sparse close wisps and sea-level scud | Supporting detail; validate edge-on views, lighting and depth against ship and volumes. |

The front and throat need matching space as well as matching colour. Either shape an actual opening in the front density for the VDB, or stage a carefully overlapped handoff in a region where the front no longer fills the throat. Prefer a supported UDS mask/control if one is available; otherwise use a project-owned integration material or a spatially separated transition. Do not edit the paid master blindly, and do not assume overlapping opaque-looking volumes will subtract from one another.

The entrance should occupy a broad section of the storm face. Its outside merges into the larger mass; its interior has overhead spiralling structure and an open water corridor. Conceal the exterior bounds and rear opening from approach viewpoints. If the tunnel asset cannot provide enough surrounding mass, expand the surrounding storm shape rather than uniformly scaling the tube until every voxel becomes obvious.

### Cloud structure without popcorn

Build a hierarchy: a few kilometre-scale masses, broad shelf/roll shapes hundreds of metres across, then smaller wind-torn edges. Use large connected density regions with quiet areas between them. Reduce the repeated small rounded forms that currently receive equal visual emphasis.

For the city, favour low stratified mist, slanted scud and a few larger banks curling over the channel shoulders. Tall uniformly crisp cumulus towers beside every temple are the wrong weather language for the reference. Allow architecture to emerge through changing depth and openings; avoid placing a glowing cloud directly behind every statue.

Do not replace all procedural banks with copies of the same tunnel volume. Its characteristic ring and loop would repeat. Use a selected density slice/variant only if it reads naturally from the gameplay view; otherwise retain simpler, well-lit fog. Noise frequency alone cannot fix a bad macro silhouette.

### Formation and movement

Current source moves the distant front rapidly toward the ship and moves the radial storm away even faster after entry. Different rates and density ramps can make the layers separate visually. Drive the front face, throat mouth, skirt and lighting transition from one passage-progress curve, with explicit spatial alignment.

Let a broad dark shelf form and thicken, then roll and advect toward the route. The tube should become legible within that mass, not materialise as an isolated expanding ring. Keep VDB playback speed distinct from actor travel: matching the ship approach does not require fast-boiling cloud motion.

After the bow enters, receding daylight, overhead volume detail and nearby scud should remain readable for several seconds. Let the throat dissolve into layered fog gradually. Reveal the city through one or two openings before exposing the larger composition. Avoid an instant change from bright ocean to grey screen while the weather geometry is still behind the ship.

## Ruins and statues in believable fog

Place real foreground extinction between the viewer and concealed structures. More light behind a dark statue increases its silhouette and makes it more conspicuous; raising the brightness of distant clouds will not hide it.

Use three depth groups: a near, largely concealed base; a middle-distance fragment or shoulder revealed by a gap; a far crown that barely separates from the sky. Reveal the near structure by drifting/eroding the foreground bank rather than switching off all fog around it simultaneously. Preserve the ship's route and hazards throughout.

Do not add an opaque screen or use abrupt actor hiding to conceal a depth/compositing bug. Once the rendering is correct, normal visibility/culling rules can prevent premature distant reveals, but transitions must be staged outside the player's view or behind adequate occlusion.

## Delivery order and visual acceptance

1. **Diagnose one failing shot.** Produce an annotated before/after for the statue cutout and rigging halo. Record the responsible system combination and effective settings. Do not declare a cause from one attractive screenshot.
2. **Light one cloud correctly.** Zero-emission baseline, dark internal folds, credible lit edge, clean water contact. Review against both bright sky and dark sky.
3. **Make one continuous front and tunnel.** Record a normal-speed approach/entry/exit clip with no detached ring, exposed bounds or conflicting cloud motion.
4. **Compose one city reveal.** Wet foreground, quiet low fog, one monumental silhouette and depth-separated light. Use this as the standard before spreading effects through the entire level.
5. **Restore storm details.** Rain, lightning, wisps and spectral effects return one at a time; each must preserve the preceding result. Spectral faces may be deliberately unnatural, but ordinary ruins must not acquire unintended outlines.
6. **Optimise and package.** Profile GPU milliseconds and volume streaming with frame generation disabled, then judge temporal quality at the intended upscaling setting. Preload selected VDB sequences, avoid blocking playback, test camera turns and repeated loops, and verify the same shot in a packaged build.

Save full-size stills and motion evidence in a new taskboard review folder, with binary revision, asset/material version, run flags and camera time. Inspect at actual output size; reduced contact sheets hide halos and voxel edges. Do not claim packaged success based on editor rendering.

Acceptance is visual and continuous: no scene-shaped holes through foreground fog; no broad halos on thin rigging; no bright unlit popcorn; no uniform horizon strip; no detached tunnel ring; no visible rectangular volume bounds; no lighting jump on entering the tunnel; and no loss of all navigational cues. The scene should still look coherent with lightning and bloom disabled.

No new pack purchase is needed for this diagnosis. The installed assets are enough to prove a convincing front, throat and one fog reveal. A further pack is justified only when a specific missing shape or motion remains after depth and lighting are correct.

## Source references

Project source: `DCFogBank.cpp`, `DCFogFront.cpp/.h`, `DCRideDirector.cpp`, `DCTreadmill.cpp`, `make_fogvolume_material.py`, `make_heterocloud_material.py`, `make_vdbcloud_material.py`. Existing source has both active and fallback paths; runtime selection must be logged.

Engine source: `E:/Utilities/UE_5.8/Engine/Source/Runtime/Renderer/Private/VolumetricRenderTarget.cpp`, `VolumetricCloudRendering.cpp`, `LocalFogVolumeRendering.cpp`, and `HeterogeneousVolumes/HeterogeneousVolumes.cpp`.

Epic's [Volumetric Cloud documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine) explains scattering and cloud shadowing. Its [Heterogeneous Volume documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/heterogeneous-volumes-in-unreal-engine) describes SVT playback, resolution controls and why blocking streaming is unsuitable for normal real-time playback. The public pages currently select 5.9; the installed 5.8 source above is the basis for the specific diagnostic controls in this plan.
