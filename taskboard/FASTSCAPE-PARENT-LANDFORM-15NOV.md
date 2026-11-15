# Fresh connected parent landform, source08

Owner: Codex cliff worker, branch `codex/fastscape-parent-landform`, based on `e39928c`. Owned only `Tools/generate_fastscape_parent_landform.py` and this note. Offline source-only work: no Unreal, runtime geometry, live landmarks, materials, sky, water or route changes.

## Construction and scope

The parent sampling domain is **500 m tangential × 1,100 m inland**, at 2 m resolution (551 × 251). It is wider inland only; no lateral coastal outlets are invented. Source08 remains exactly rows `[0:253]`, columns `[0:251]`, in the original arena frame. Original XY values and valid-domain mask are copied exactly into that crop. The extra 74,798 inland nodes are explicitly authored proposal data and cannot certify adjacent scene support.

The old wall interior, narrow step, flat upper plateau, arbitrary outer tile seam locks and temple pad locks are absent. The initial height is a low coastal datum with a gentle 0.015 inland grade and deterministic 0–0.8 m perturbations. Two unequal broad divide polylines join in the hinterland. They guide **uplift rate**, rather than stamping final ridge heights: primary width 105 m, secondary 76 m at 72% strength, background uplift 0.002 and maximum 0.012 m per model year. The polylines and exact parameters are in the manifest.

The parent runs 100 × 100 model years with external Fastscapelib 0.3.0: Single/MST/Multi1.1 routing, implicit stream-power erosion `K=1e-4`, area exponent 0.4 and slope exponent 1. Valid-edge finite-volume hillslope diffusion uses `D=0.005 m²/year`. These are authored process settings, not calibrated tectonic time or rock properties. No height scaling, exaggeration, clipping or pixel ceiling is used.

Only the 251 actual coastal outlets are routing base levels. Measured coastal locks and a one-cell collar preserve every source triangle touching the coast. Invalid padding is excluded; side and inland boundaries are mobile with no out-of-domain flux. That lateral boundary condition can influence watershed topology and is a limitation of this bounded strip. Old sector seams and temple foundations are deliberately **not** claimed preserved.

## ParentA measured outcome and rejection

Artifacts: `Flow-Seams/Saved/FastscapeParent/ParentA/`. The bundle contains `parent_terrain.npz`, `crop_terrain.npz`, matched original-triangle `parent/{baseline,candidate}.npz` and `crop/{baseline,candidate}.npz`, exact generator/helper source copies, GPL licence, attribution and manifest. Native model execution took 11.67 seconds.

The initial parent range is −169.56 to −54.79 m. After 10,000 model years the actual parent and crop range is −169.56 to **+38.07 m**, without a post-scale or cap. Parent median height is −42.72 m; p95 is +23.42 m. The crop's old upper edge now ranges from −133.17 to +28.96 m, so its old neighbouring seam cannot be joined without a new integration design.

The complete parent shows connected unequal primary/secondary ridges, a real change from the flat-lidded escarpments. **Its appearance is nevertheless rejected.** Actual oblique/plan and crop wall/oblique views show broad smooth forcing mounds, near-parallel shallow rills and a repeated coastal curtain, not mature branching rocky relief. This is not a finished mountain or cliff. No far/native promotion or detailed material work is warranted from these views.

Review images are in `Fitted-Waterfall/Saved/ImplicitCliffReview/FastscapeParent/ParentAWhole` and `ParentACrop`. Source hashes: parent mesh `cd763f05e964f6cd476f84bf11afc33ba94b337ed98e70d09138dc8e64240492`; crop mesh `c4586b8da370377bf79d8cf1bcf2bb627381870530a38ba6545519178b266b59`.

Total uplift adds 31.90 million m³; stream-power removal is 2.03 million m³, approximately 6.4%. On the 44,400 nodes with uplift above 0.008 m/year, mean integrated incision is 0.0003809 versus uplift 0.0101624 m/year, only 3.75%. This supports the observation that the first result remains dominated by its forcing corridors. It is not, alone, a visual justification for an unconstrained strength/time sweep.

## One authorized ParentB model revision

ParentB preserves the entire initial field, domain, coastal locks and divide definition exactly, and preserves the integrated uplift budget **bitwise**. It distributes that uplift across 400 × 200 = 80,000 model years at one-eighth the rate, and changes routing to `SingleFlowRouter + MSTSinkResolver`, matching the official Mountain model's concentrated channel architecture. Multi-direction routing is absent. This is deliberately a coherent **two-change model**, not an isolated test proving which change caused an image difference. `K`, hillslope diffusivity and all physical dimensions remain unchanged. No third trial is authorized or performed.

Actual generation took 17.75 seconds. The peak falls naturally from +38.07 to +27.17 m; parent median falls from −42.72 to −52.52 m. Stream-power removal grows to 9.08 million m³ against the same 31.90 million m³ uplift. The model is responding more strongly to drainage, without a hidden vertical adjustment. Its original crop upper edge nevertheless ranges from −133.77 to +18.11 m, and old temple foundations remain unsupported.

The actual whole-parent oblique/plan and crop wall/oblique all fail the natural-rocky-landform gate. Broad smooth connected forcing mounds remain dominant; fewer deeper cuts read as parallel ribs/terraces beside the central valley. Reduced fluting and relief do not create a convincing hierarchy of branching valleys, rock masses and low irregular rim plateaus. Worker, parent and independent reviewer reject both models. This is retained as an offline model failure, not a new arena asset. ParentB bundles are in `Saved/FastscapeParent/ParentB`; matched whole/crop images are under the reviewer's `Saved/ImplicitCliffReview/FastscapeParent/ParentBWhole` and `ParentBCrop`.

## Independent checks and explicit unsupported work

Independent audit passes the stated local coastal contract: 2,000 coast-touching triangles and their 1,311 vertices are exact to the measured source. Parent 253,078 triangles and crop 104,078 triangles form clean single open manifolds. All 251 coastal base levels are verified; no valid receiver enters invalid padding. Poisoning all free old source heights leaves the entire fresh raw field, initial field, uplift and targets unchanged. The combined uplift/incision/fixed-reservoir ledger residual is 2.95e-9 m³. Report: `Cliff-Contact/Saved/WeatheredAudit/fastscape_parent_a.json`.

This does **not** preserve source outer seams: 678 of 992 outer edges differ, with measured boundary displacement −152.56 to +13.44 m. Existing temple placements are unsupported: Actor33/34 lose roughly 64–96 m of ground; Actor36 loses 18–32 m; Actor48 is partly near the new surface but not certified. The manifest reports sampled ranges and sets all full-footing support flags false. No live actor was moved. The local source coastline audit is not a whole-arena ship-route check.

Independent ParentB audit confirms exact initial terrain, nodewise identical integrated uplift, exact coast-touching triangles, clean meshes and a combined ledger residual of 3.12e-9 m³. Recomputed SFD drainage matches the stored array bitwise. All 127,334 valid nodes reach the 251 actual outlets, but **one outlet receives 99.667% of total area** (507,640 / 509,336 m²), compared with A's 60.0%. Mere graph connectivity is not diverse or visually convincing catchment structure. The measured coastal datum gradient, narrow strip with closed sides and broad forcing corridors are important domain constraints; this result does not justify another blind erosion-strength adjustment. B leaves Actor33/34 as much as 117.72/101.53 m above the proposed terrain. Report: `Cliff-Contact/Saved/WeatheredAudit/fastscape_parent_b.json`.

## Reproduction and provenance

Input seed SHA256: `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f`. Measured boundary SHA256: `8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200`. ParentA generator SHA256: `1a97c06a58e93dbb8f485ec13bb53c2be3289e5e801f9682cf84e08fdce717a5`. Its immutable source copy is included in the output bundle.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_fastscape_parent_landform.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz --dependencies Saved/FastscapeReference/python_packages --output Saved/FastscapeParent/ReplayA
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_fastscape_parent_landform.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz --dependencies Saved/FastscapeReference/python_packages --steps 400 --dt-years 200 --uplift-rate-scale .125 --routing single --output Saved/FastscapeParent/ReplayB
```

Run from `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`, using the isolated dependency from the previous [reproduction](FASTSCAPE-UPSTREAM-REFERENCE-15NOV.md). See [GPL notice](../Tools/ThirdParty/Fastscapelib-NOTICE.md). No dependency is vendored or linked into Unreal. The generator performs no installation or network access.

ParentB generator SHA256: `1dfec3144dbb594dbc9b376f9bb96c8dd919affd3860a7352d17679f805fcdbb`. Its complete source and the exact masked-diffusion helper are saved beside its original mesh arrays and full GPL licence.

This method addresses parent drainage/topographic structure only. Hard-rock fracture planes, stratified ledges, undercuts, exposed blocks, physical stone shading and temple foundations remain separate unsolved tasks. Adding those details cannot confer visual approval on the current rounded massing.
