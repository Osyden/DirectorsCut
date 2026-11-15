# Fog edge composition investigation — 15 November

Owner: Codex voice/audio subagent, isolated branch `codex/fog-edge-composition-15nov` at runtime `1f4fac8`. Root owns native rendering/builds; the selected 18:33 packaged candidate is reserved for the human playtest. No runtime visual changes applied yet.

The reported problem is pale outlines on temple edges and rigging in the city, with an illumination mismatch between lightning flashes. Inspected the actual `Saved/SceneReview/FinalFogPillars/shot_0110.png` and the 17:39 package's `Saved/Screenshots/Auto/shot_0150.png`. Both show pale stone edge highlights and fine grey rigging against dark broad surfaces. The images alone do not establish whether these are surface highlights or reconstructed halos.

## Evidence and exclusions

- `DCFogFront::TickTunnel` fades `M_StormComposite` out at 74–80 seconds and sets its postprocess blend weight to zero. It cannot produce the city outlines at 110/150 seconds. Leave its storm/water-depth behavior intact.
- `Saved/FinalFogPillars.log` records the live city fog's original FSSS flag as zero; the default city screen-scattering amount is also zero. The optional FSSS blur is not active in this baseline. Native sun-shaft bloom is disabled before 240 seconds. A historical `ShowFlag.LightShafts 0` experiment did not remove the broad central glare; that is distinct from proving the narrow-edge cause.
- Current city rain/lightning are zero after the 76–94 second transition. Nevertheless record `-DCLightningLog` in comparisons and exclude a flash frame rather than assuming two captures have identical illumination.
- `Tools/audit_fog_edge_materials.py` was run by root in a read-only NullRHI commandlet. `Saved/AuditFogMaterials.log` completed successfully on 15 November at 18:43, with 17 unique materials and no Python error. The five actual temple mesh types use opaque DefaultLit materials. Their emissive intensities are zero and emissive colors are black. Rope/mast materials also have zero emissive power and disabled aura switches. There is no demonstrated emissive-rim contribution.
- Rope materials have dry specular amount 0.15 and texture-controlled roughness. The director's existing wetness pass boosts static-ship specular and reduces roughness, whereas the skeletal mast retains its pack materials. Do not apply a shared material fix without proving the relevant surface is responsible.
- Default local exposure uses highlight/shadow contrast 0.8/0.8. UE5.8 defaults detail strength to 1 and blurred-luminance blend to 0.6. The scene uses manual exposure. Local exposure remains a candidate for the edge contrast, not a confirmed cause.
- No forced tonemapper sharpening was found in project source/config; engine sharpening defaults to zero. The installed DLSS plugin no longer applies its deprecated sharpness control. DLSS remains enabled as requested.

## Focused native comparison pending

At the next coordinated GPU slot, capture the same helm camera at 110 and 150 seconds with `-DCLightningLog`, one change per run:

1. Current baseline.
2. `-ExecCmds="ShowFlag.LocalExposure 0"` to isolate local exposure.
3. `-ExecCmds="ShowFlag.Specular 0"` to distinguish surface highlights from fog/compositing edges. This is a diagnostic only, not a proposed final look.

Both show flags are runtime accessible in the installed engine. If neither resolves the outlines, compare native-resolution AA separately, then volumetric cloud reconstruction. Compare adjacent calm frames and retain the existing dry city, grey banks, water occlusion, warm deck lighting and visibility gates. A permanent fix and acceptance require rendered evidence; the source/material audit alone does not establish visual success.
