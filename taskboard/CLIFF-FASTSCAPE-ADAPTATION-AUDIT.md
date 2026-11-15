# Fastscape crater adaptation: independent audit

15 November 2026. Read-only CPU audit after merging root `763da86`. Existing triangle helpers and bounded API checks were used; no new audit framework, Unreal job or runtime modification. PhysicalA and PhysicalB pass their declared domain/boundary contracts. **Neither preserves the existing temple foundations or constitutes final visual acceptance.**

## Source identity and fresh terrain

The adaptation input is `Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz`, SHA256 `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f`. Boundary heights come from `Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz`, SHA256 `8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200`.

This is distinct from the previously reviewed immutable eroded `v3_open256` source, SHA256 `1cd1fc6c94421e36bd0ed0266514fa6491372558ba69c0b03d66a3815abfdf74`. All three have identical triangle topology and identical complete external boundary positions. The new candidate preserves those boundary positions exactly; different source filenames do not imply a shifted shoreline.

An independent function replay added 12,345 m to every unconstrained old source height and subtracted 23,456 m from every unconstrained measured height. The complete fresh raw and initial fields remained bitwise identical. Only frame/domain, coastal values, outer seam targets and the declared upper-boundary datum influence the new terrain. The old soft wall interior is not reused.

Both outputs retain the physical 2 m grid, source arena coordinates and valid domain. The rectangular array spans 500 × 504 m; the valid mesh spans 500 × 492 m. They are not scaled versions of the upstream 20 × 10 km example. Heights remain in metres without vertical exaggeration.

## Correct boundary API

Installed Fastscapelib 0.3.0 confirms `FlowGraph.mask=True` excludes a node from routing. The actual proposal uses exactly 251 measured coastal outlets as explicit base levels. Other fixed seams remain immobile but do not become unintended drainage sinks. Independent inspection of the final routing found no active receiver link into invalid padding.

Native `DiffusionADIEroder` is unsuitable for this irregular masked domain without additional treatment. A bounded API check moved an interior `FIXED_VALUE` height from 100 to 90.59235 m and allowed invalid-padding changes to affect adjacent valid terrain. The graph mask is not the ADI grid boundary. The proposal correctly omits ADI. Native eroder outputs reuse buffers, so retained diagnostic results were copied before subsequent calls.

## PhysicalA

`Flow-Seams/Saved/FastscapeCrater/PhysicalA/candidate.npz` has SHA256 `540778f443df0f7520ea621f43af7ed9cbe9c7d9f4b0c851f2cf888a501441e3`. Frame, XY positions, valid-domain topology and all 992 external boundary edges match measured source geometry exactly. All 5,664 fixed physical coast/seam nodes retain their exact target heights. The 52,536-vertex, 104,078-triangle surface is one valid open manifold, without degenerate, duplicate, nonmanifold or inconsistent-winding faces.

PhysicalA is stream-power-only and was visually rejected. Its longest mesh edge reaches 116.03 m across the 2 m grid, consistent with extremely steep narrow cuts. Numerical validity does not make those cuts believable cliffs.

## Matched PhysicalB diffusion check

`Flow-Seams/Saved/FastscapeCrater/PhysicalB/candidate.npz` has SHA256 `99a401602894ef653a687db6c468da8f1b0ee412afa86788a0c760c50cfb23f5`. Its initial and raw fields, source hashes, stream-power settings, frame/domain and fixed masks match A exactly. The sole method change is explicit linear diffusion over valid four-neighbour edges, with signed exchange against fixed-height reservoirs. It is not native ADI or a stream-sediment transport model.

Independent checks confirm:

- All fixed heights and 992 outer boundary edges remain exact; the mesh is one clean open manifold.
- The maximum outgoing update weight is 0.714286, below one, over 560 substeps. Closed-domain tests produce no new extrema and conserve height-volume to approximately 4.3e-14 m³.
- Changing invalid padding by one million metres changes valid diffusion output by zero. Fixed and invalid heights remain exact.
- Actual diffusion adds 96,513.65665 m³ to active terrain through fixed-boundary exchange. The independently summed gain matches the recorded exchange within 5.4e-10 m³.
- Final active terrain-volume change is -1,013,004.41344 m³; stream-power removal is 1,109,518.07009 m³. Adding that removal and subtracting fixed-boundary exchange closes the combined ledger within 6.2e-10 m³. This records erosion removal and reservoir exchange; it does not claim sediment deposition conservation for the stream-power solver.

## Integration remains unresolved

Root and the independent visual reviewer reject B as well: diffusion removes the narrow cuts but leaves an almost featureless smooth wall. Neither A nor B should receive a native import. Physical correctness and a balanced ledger do not create the missing large-scale terrain structure.

The next landform should be generated over a broader parent domain before cliff extraction, with the real route and coast retained. Arbitrary inherited preview-tile seams should not dictate the parent landform. Matching sector boundaries is an integration step, not a substitute for coherent ridges, catchments and a deliberate cliff silhouette.

Old temple pads are deliberately not frozen. In B, sampled ground beneath Actors 33/34 is roughly 11.3–13.9 m above their previous support levels; Actors 36/48 are as much as 33.9/56.4 m below theirs. Current placements are therefore not approved. Their supports must be redesigned or relocated and then checked against the authoritative new surface, including footprint coverage. The former partial-footprint limitation is not erased by this proposal.

No overhang, physical geological-timescale calibration, PBR-mask accuracy or final art quality is claimed. Reports are retained in the audit worker's ignored `Saved/WeatheredAudit/fastscape_crater_physical_a.json` and `fastscape_crater_physical_b.json`.
