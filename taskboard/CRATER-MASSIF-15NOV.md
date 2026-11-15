# Connected cliff massif — 23:48 continuation, completed 15 November

Branch `codex/maelstrom-saturday`, following `f06d420`. **Normal gameplay cliffs, waterfalls, water, route, sky/light and boss remain unchanged.** This pass replaces the rejected offline shape and improves the opt-in engine authoring view. It is not final arena integration.

## Actual progress

The old blanket cap/apron protection is removed. `Tools/generate_massif_cliff_seed.py` generates connected ridged/heterogeneous terrain across the cliff and hinterland, then fits specific constraints with a screened two-dimensional solve. The final candidate fixes 15.29% of physical samples, compared with 65.51% in the freshly generated clean input; 27,354 formerly fixed cells are released. These figures differ from the older historical input used in pass7.

Root exported all 50 current ruin footprints through opt-in `-DCExportCliffAnchors`, using the exact oriented bounds and nine current-triangle ground samples from placement. It also exported fresh current cliff geometry through the existing source-driven CPU tool. The clean seed was found to alter nominal boundary heights by up to77.3m, so it is **not** the coast/seam authority. The final generator takes a separate measured boundary source and preserves those heights exactly.

Three complete footprints (Actor_33,34,48) are supported by the final mesh within1.6e-6m. Actor_36 has only30.7% of its footprint in this tile and remains explicitly unapproved as a complete footing. Exact clipped-triangle audit checks coverage rather than trusting a protection mask. The live source geometry hash is `87BB77D76A90BAEE8DD8515CC4DB7E92F4C49F38`; measured position-function SHA256 is `b0349251bec80a6ffa3677afcb742dc73eddb456d63dc820a0a73c32f2b3a524`, from source head `3bcdb2f`.

The bounded 256-step hydraulic/thermal pass preserves all coast, seam, outlet and pad constraints. Maximum net cut is2.269m and fill2.204m; solver solid error is zero and water error5.53e-10m³. Its visual effect is subtle. The genuine improvement is the connected, unequal silhouette; this is not evidence of mature natural gullies or finished talus. See [generator/reproduction](MASSIF-CLIFF-LANDFORM.md), [independent audit](MASSIF-INDEPENDENT-AUDIT.md), and [matched clay/split-normal review](CLIFF-CLAY-NORMALS-PASS8.md).

## Actual review and remaining failures

Matched physical-scale CPU views demonstrate that the continuous upper cap is gone and multiple gullies extend behind the crest. Angle-based normals remove sample-triangle speckling while retaining deliberate sharp edges, without changing positions, triangles or masks. Both flat and angle modes expose the same remaining problem: broad soft faces and inflated fins still look like terrain rather than fractured bedrock. The clipped toe and side strips are unresolved tile boundaries, not acceptable final joins.

Root accepted the macro shape **only for another isolated engine preview**. The final native1600x900 review is `Saved/SceneReview/cliff8-massif-supported-final`, with a requested300s frame and exit451s frame. Retained CPU and engine images are in `taskboard/review/crater-pass8-15nov`. It uses the actual stone material after fixing its Nanite usage; no cloud/light values were changed. The material's small detail cannot repair the soft large forms. The scene remains far from the reference.

The preview optionally shows only verified existing ruins via `-DCCliffPreviewSupports` and a hash-bound manifest made by `Tools/make_cliff_preview_supports.py`. Two runtime identities match: Actor_34's top is38.455m and Actor_48's top80.606m. The statue reads against the sky; the low arch is mostly hidden by surrounding terrain. Actor_33 remains hidden because its live bounds/placement do not match the audited instance. `DCRuins::SpawnColonnade` uses global random choices; the exported instance is not guaranteed across launches. Do not weaken this check or claim all three source-tested footprints have been verified in this runtime. `DCRuins` was read, not modified. Deterministic landmark reproduction/support will need coordination before integration.

## Two import/render failures caught and corrected

1. Reimporting different geometry into the existing preview silently selected an Interchange/reimport route and produced effectively white vertex masks. Changing `replace_existing_settings` did not solve the route. The importer now uses a **fresh named asset** and disables the FBX Interchange feature synchronously for that commandlet process, verifying the CVar before import. `ExecCmds` was too late. The successful import reports `FbxStaticMeshImportData`; all original52,536 vertices,104,078 triangles and312,234 corner masks pass the source audit, with mask error≤1/255. No exporter gamma correction was applied. Audit, material assignment and preview now read the actual asset path from the matching import report. Rebuilding an already existing hash asset requires a fresh `DC_CLIFF_PREVIEW_NAME=SM_CliffSector_<new_suffix>`.
2. **Correction to pass7 and the first pass8 native images:** their materials lacked Nanite usage, so Unreal substituted its default material. Those images cannot establish the claimed stone appearance. Both isolated materials now explicitly enable Nanite usage, and the review helper rejects this warning as a failed capture. The final pass8 log contains neither this fallback nor a cliff material compile failure. CPU clay comparisons were unaffected.

Current ignored imported asset:

`/Game/DC/Maelstrom/GeneratedCliffPreview/SM_CliffSector_88e587e7794e`

FBX: `Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/MassifPreview/source08_massif_v3_open256_angle35.fbx`, SHA256 `88e587e7794e153bb662e4ff9c609116674fb9b847a2c9d47da192605fa81e44`. Physical dimensions500×492×230.627m. The source audit uses the original mesh; the4,048 fallback triangles are not an exact contact surface. Native Nanite LOD quality/performance has not been measured independently.

## Verification and provenance

- Seven generator tests and seven independent audit tests pass; deterministic generation matches23 numeric arrays. Actual output audits establish fixed coast/seams,251 outlets, unchanged physical domain, exact source triangle support and the partial-footprint exception.
- Full FBX roundtrip preserves positions/masks; split-normal angular error≤0.02704°. Native fresh import/source audit passes after the failed reimport was isolated.
- Final Editor/Game builds pass in `Saved/CliffMassifEditorFinal.log` and `CliffMassifGameFinal.log`.
- Actual final review restores old terrain/camera on an active backward seek at elapsed−4.99, reactivates, and restores the ending at elapsed181.01. Requested screenshot timestamps include settling; the265 still is after reactivation, while the log establishes the negative-time behavior. This is not a long-run performance or motion test.
- Current measured/clean inputs are under `Saved/CliffErosion/CurrentSource08`; do not overwrite these files for a later pass. Anchor snapshot is also retained as `Saved/CliffSupportAnchors-pass8-14nov.json`, SHA256 `2e89c7506e68390eea305f5cb937261dde2376bd473cb9740c4068d7534772b5`. The generated candidate metadata references the identical original `CliffSupportAnchors.json`; if a later export replaces it, use the recorded snapshot explicitly and preserve the old hashes rather than silently substituting new anchors.
- Final candidate is `Crater-Workers/Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz`, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. See the worker handoffs for seed hash, complete commands and audits. Bulk remains ignored; no vendor pixels or asset sources were added to Git.

## Next useful work

Use this connected massif as the macro authoring base. **Do not add another noise octave or increase erosion strength to disguise its soft faces.** Build a small representative steep outcrop with actual joint planes, recessed breaks and limited overhangs, preserving the measured shore and verified support regions. The current2m grid cannot resolve sub-metre joint cuts: refine topology locally or use a genuine3D surface/remesh. Show physical-scale clay views before multiplying it across the sector. Keep large massing unequal and small detail physically small on distant walls.

Review the skyline with reproducible actual landmarks: footing support does not guarantee visible temple silhouettes, as the hidden arch demonstrates. Lower selected ridges behind/along sightlines without creating a continuous flat cap. Coordinate any shared ruin-generator changes; do not change that owner's code here.

Final integration still needs exact visible tile seams and one authoritative arbitrary surface for terrain, waterfall contacts, ruin support and export. Heightfield queries cannot represent the upcoming overhangs. Waterfall cores, spray and receiving foam must be regenerated from that accepted surface; old falls/VDB remain opt-in. No main merge, package, boss or early-game changes were made.
