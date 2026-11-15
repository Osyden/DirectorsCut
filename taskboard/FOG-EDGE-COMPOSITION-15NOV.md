# Fog edge composition investigation — 15 November

Owner: Codex voice/audio subagent; latest read-only follow-up on isolated branch `codex/caldera-motion-15nov`. Root owns native rendering/builds. Main `25076ed` is frozen for the 19:24 package. No runtime visual change from this investigation has been accepted.

The reported problem is pale outlines on temple edges and rigging in the city, with an illumination mismatch between lightning flashes. Inspected the actual `Saved/SceneReview/FinalFogPillars/shot_0110.png` and the 17:39 package's `Saved/Screenshots/Auto/shot_0150.png`. Both show pale stone edge highlights and fine grey rigging against dark broad surfaces. The images alone do not establish whether these are surface highlights or reconstructed halos.

## Evidence and exclusions

- `DCFogFront::TickTunnel` fades `M_StormComposite` out at 74–80 seconds and sets its postprocess blend weight to zero. It cannot produce the city outlines at 110/150 seconds. Leave its storm/water-depth behavior intact.
- `Saved/FinalFogPillars.log` records the live city fog's original FSSS flag as zero; the default city screen-scattering amount is also zero. The optional FSSS blur is not active in this baseline. Native sun-shaft bloom is disabled before 240 seconds. A historical `ShowFlag.LightShafts 0` experiment did not remove the broad central glare; that is distinct from proving the narrow-edge cause.
- Current city rain/lightning are zero after the 76–94 second transition. Nevertheless record `-DCLightningLog` in comparisons and exclude a flash frame rather than assuming two captures have identical illumination.
- `Tools/audit_fog_edge_materials.py` was run by root in a read-only NullRHI commandlet. `Saved/AuditFogMaterials.log` completed successfully on 15 November at 18:43, with 17 unique materials and no Python error. The five actual temple mesh types use opaque DefaultLit materials. Their emissive intensities are zero and emissive colors are black. Rope/mast materials also have zero emissive power and disabled aura switches. There is no demonstrated emissive-rim contribution.
- Rope materials have dry specular amount 0.15 and texture-controlled roughness. The director's existing wetness pass boosts static-ship specular and reduces roughness, whereas the skeletal mast retains its pack materials. Do not apply a shared material fix without proving the relevant surface is responsible.
- Default local exposure uses highlight/shadow contrast 0.8/0.8. UE5.8 defaults detail strength to 1 and blurred-luminance blend to 0.6. The scene uses manual exposure. Local exposure remains a candidate for the edge contrast, not a confirmed cause.
- No forced tonemapper sharpening was found in project source/config; engine sharpening defaults to zero. The installed DLSS plugin no longer applies its deprecated sharpness control. DLSS remains enabled as requested.

## Initial native comparison protocol (completed below)

The initial protocol was to capture the same helm camera at 110 and 150 seconds with `-DCLightningLog`, one change per run:

1. Current baseline.
2. `-ExecCmds="ShowFlag.LocalExposure 0"` to isolate local exposure.
3. `-ExecCmds="ShowFlag.Specular 0"` to distinguish surface highlights from fog/compositing edges. This is a diagnostic only, not a proposed final look.

Both show flags are runtime accessible in the installed engine. If neither resolves the outlines, compare native-resolution AA separately, then volumetric cloud reconstruction. Compare adjacent calm frames and retain the existing dry city, grey banks, water occlusion, warm deck lighting and visibility gates. A permanent fix and acceptance require rendered evidence; the source/material audit alone does not establish visual success.

## Native A/B review, 19:00–19:03

Root captured five matched helm views in `Saved/FogEdgeAB/{Baseline,NoLocalExposure,NoSpecular,FullCloud,NativeAA}/shot_0110.png`, seeking to 100 then running to 110. Inspected every actual image and runtime log. All have zero rain/lightning at the shot and saved FSSS zero; no flash markers were observed. Commands were logged as applied: local exposure off, specular off, cloud render mode 3, or DLSS disabled with native 100% screen percentage.

None establishes a fix for the reported temple outlines. Removing local exposure mainly enlarges lantern bloom; removing specular strips wet sea/deck reflections while pale temple details remain. Full-resolution cloud mode looks essentially unchanged. Native AA changes thin-rope reconstruction but leaves the temple features. Keep the accepted global quality and lighting defaults.

The next focused comparisons are `ShowFlag.Fog 0` and, independently, `ShowFlag.DirectLighting 0` at the same view. These can separate fog/depth composition from diffuse raking-light or material-edge detail. Some bright vertical temple strips in this view also coincide with gaps between columns, so a precise matching human frame/time matters before treating every pale strip as a halo. The seek-start layout/history differs from a complete voyage. A normal-speed run through the actual reported view, including a logged lightning bracket if present, remains necessary. No permanent visual change is justified by these exclusions alone.

## Matched continuous-route review at 167 seconds

The stronger reproduction is `Saved/ThirdPlaytest/Fog/shot_0167.png`, with its `runtime.log`. This run starts the voyage normally and uses `-DCRunThrough -DCTimeScale=3 -DCSteer=1 -DCShipView -DCOrbit=180,8 -DCOrders=Hold_fire -DCLightningLog`, 1600×900, offscreen, sound/voice disabled. The diagnostic runs retain that route/camera and capture 165/167/169 seconds. These are distinct from the earlier seek-to-100 comparisons. The baseline logs rain/lightning zero at 167; the earlier flash was near 81 seconds, not a flash-frame comparison at 167.

Inspected every actual 167-second comparison under `Saved/FogEdgeAB`:

| Directory / isolated change | Observed result |
| --- | --- |
| `FullRouteNoFog`, `ShowFlag.Fog 0` | Strong white lattice becomes faint tan/dark lattice; the whole atmosphere changes substantially. This implicates fog-dependent appearance but does not establish its mechanism. |
| `FullRouteNoDirectLighting`, `ShowFlag.DirectLighting 0` | Strongest lattice remains; directly lit deck becomes dark. No justification for changing the sun key/fill. |
| `FullRouteNoShafts`, `ShowFlag.LightShafts 0` | Strongest lattice remains. No justification for disabling shafts. |
| `FullRouteNoVolumetric`, `r.VolumetricFog 0` | Strongest lattice remains; atmosphere becomes brighter and flatter. Rejected for shipping. |
| `FullRouteNoSpecular`, `ShowFlag.Specular 0` | Water/masonry sheen disappears, but strongest lattice remains. A roughness/specular change is not supported as its remedy. |

The accidental combined case directory `FullRouteNoShafts FullRouteNoVolumetric` did not apply the intended independent changes and is discarded. None of these diagnostic settings is a shipping fix; preserve the accepted fog, lighting and quality defaults.

### Revised geometry identification

Close crops of baseline, NoFog and NoSpecular use rectangle `(575,55,470,285)` from the original 1600×900 images. They are local, untracked inspection outputs in the isolated worktree's `Saved/FogEdgeCrops`. The strongest curved white lattice is bounded by the ship's left upper sail panel and yardarms. It is not clearly a temple bevel despite the architecture behind it. NoFog retains faint tan lattice in the same region, while NoSpecular retains its white appearance. This narrows the geometry to inspect; it does **not** prove a sail shader, depth or opacity defect, or explain every reported temple/rigging outline.

Source establishes that `ADCGameMode` spawns `/Game/Brigantine/BP/BP_Brigantine`, disables the actor's own voyage tick but leaves component sail/flag ticks active. The pack has `SK_Mast-brigantine` and multiple `SM_Rope-*` meshes. Installed sail instances include `MI_Sail_2-wp` and `MI_Sail_2-wp1`, but their actual component assignments must be recorded rather than inferred from names. The director's wetness overrides apply to static mesh components; skeletal mast materials retain their authored assignments. The prior 17-material temple/mast/rope audit did not establish the exact questioned sail surface.

### Next bounded pass

1. After the frozen build/tests, root may run `Tools/audit_ship_sail_fog.py` in a NullRHI Python commandlet. It constructs the actual ship Blueprint in the temporary editor world, logs all mesh components, their material slots and mesh defaults, traverses material parents, and exports material/function expression text to `Saved/ShipSailFogAudit`. Records use `DC_SAIL_FOG`. It reads blend mode, masked opacity threshold, fog/depth/translucency settings, WPO/PDO inputs and assigned texture/parameter data. Unsupported reflection properties are explicitly reported as unavailable, never interpreted as false. It destroys the temporary actor and never saves assets or maps. This audit does not run voyage BeginPlay/ticks and therefore cannot by itself establish runtime animation/dynamic parameter state.
2. Match the upper sail panel to a component and material slot before any more broad visual A/B. Inspect its opacity/normal/base-color texture pattern and graph connections, especially fog treatment of masked versus translucent surfaces and any WPO/depth mismatch. Graph export is diagnostic, not proof of a defect.
3. If asset evidence identifies a plausible mechanism, root can coordinate one scoped sail-only diagnostic at the same full-route view, bracketed by calm adjacent frames. Preserve the rest of the atmosphere and compare the questioned panel separately from temple silhouettes. Do not globally disable fog, specular, lighting, volumetrics or AA.

Only script syntax/source checks have been run for the new sail audit; its Unreal execution and a proven visual fix remain pending. Packaging should not be held for additional speculative global experiments.

