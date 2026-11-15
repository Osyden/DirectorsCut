# Fresh physical Fastscape sector: two bounded offline candidates

Owner: Codex cliff worker, `codex/fastscape-crater-sector`, based on `763da86`. Owned only `Tools/generate_fastscape_crater_sector.py` and this note. No Unreal, runtime, live landmark, water, route, sky or material changes. This is a fresh physically dimensioned model, not the 20 km reference mesh scaled down and not another erosion pass on the rejected soft wall.

## Domain and source contract

The immutable `Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz` supplies the original arena XY/frame, valid domain, measured coast clearance and coast/seam classification. The authoritative boundary source is `Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz`. Input hashes and exact paths are recorded in each manifest.

The grid is 253 rows × 251 columns at 2 m, a 500 × 504 m rectangular sampling domain. Its actual triangulated land spans 500 × 492 m in the local chart. World coordinates, orientation, original valid domain, measured shoreline and outer seam positions remain unchanged. The 5,664 fixed physical nodes belong only to coast and seam bands. Invalid padding is excluded from flow and rainfall. The 251 actual coast outlets are explicitly the only FlowGraph base levels; immobile side/outer seams do not silently become additional river outlets.

Old interior heights are not used. A direct poison test replaced all free source/measured interior heights by kilometre-scale values and reproduced the complete fresh raw/initial fields bitwise. Independent audit confirms this, the exact 992 outer mesh edges, coast/seam targets and the absence of receivers into invalid padding.

The fresh initial terrain rises from a smoothed measured coastal datum to the measured outer-rim median datum through a 56 m scarp width. Two broad variations move the front about 20 m; small 0–0.8 m PCG64 perturbations initialize drainage. An 18 m screened residual fits only measured coast/seam targets, not the old interior. Median absolute fit is 0.27 m; p95 is 12.07 m and maximum 88.77 m near constrained boundaries. That maximum must not be concealed as natural geology.

## Numerical method and the ADI failure

The external dependency remains pinned `fastscapelib==0.3.0` in `Saved/FastscapeReference/python_packages`; no Blender/system installation is changed. See [attribution and licence](../Tools/ThirdParty/Fastscapelib-NOTICE.md). This API adaptation is separate from the previously unchanged official notebook reproduction.

Both candidates use `SingleFlowRouter → MSTSinkResolver → MultiFlowRouter(1.1)` and implicit stream-power erosion: `K=1e-4`, area exponent 0.4, slope exponent 1, tolerance `1e-5`, 80 × 100 model years. `K=0` at fixed and invalid nodes. These are authored model parameters, not calibrated geological ages or measured rock properties.

Fastscapelib's grid-only ADI diffusion was rejected before export: the actual domain probe moved interior fixed nodes by 1.91 m. An independent 9 × 9 test also found fixed-node motion and coupling from invalid padding. The FlowGraph mask does not constrain ADI. No ADI result is presented as a valid candidate.

- **PhysicalA:** stream-power only. Its actual visual review fails: smooth parallel full-height drapes, a nearly flat lid, and an artificial deep bay near a constrained side. Plateau drainage exists but does not produce a coherent cliff mass hierarchy. It is not accepted for import.
- **PhysicalB:** exactly the same initial field and stream-power settings, plus a small explicit valid-edge linear diffusion implementation, `D=0.05 m²/year`. Each valid neighbour pair receives equal/opposite height flux; no edge exists into invalid padding. Fixed nodes act as an explicitly accounted Dirichlet material reservoir. Stable substeps keep the outgoing conductance sum below one; this run uses seven substeps per 100 years and a maximum sum of 0.7142857. This is a controlled lateral-transport comparison, not a new macro shape or noise setting.

The diffusion ledger records active-domain volume change, signed boundary exchange and residual. Positive boundary exchange means material enters active terrain from fixed nodes. It does not model or claim transported stream sediment, and neither candidate provides scientifically derived stone/deposition PBR masks. Raising every excluded padding node by 1,000 m changes valid model results by exactly zero in the stored three-step differential probe.

## Actual candidates and review

Both immutable bundles are in `Flow-Seams/Saved/FastscapeCrater/PhysicalA` and `PhysicalB`. Each has fresh initial `baseline.npz`, final `candidate.npz`, physical `terrain.npz` and `manifest.json`. They export original triangles in the unchanged arena frame. PhysicalB additionally copies its exact generator source, external GPL licence and attribution. PhysicalA's exact pre-comparison generator and licence are preserved separately in `PhysicalAReproduction`.

Both have a valid height range of −169.56 to 38.06 m and preserve fixed values bitwise. PhysicalA maximum net lowering is 120.68 m, median lowering 0.68 m. PhysicalB maximum lowering is 64.78 m, maximum rise 24.89 m and median lowering 0.98 m. A rise in B is hillslope redistribution/boundary exchange, not transported river sediment deposition.

Matched clay review: `Fitted-Waterfall/Saved/ImplicitCliffReview/FastscapeAdaptation/PhysicalA` and `PhysicalB`, using the same wall/oblique/close cameras and measured mean-water context. Both are visually rejected by the worker and parent. PhysicalB removes the parallel grooves but turns the face into a nearly featureless smooth, dam-like bank. It changes lateral transport, not the primary mass hierarchy; the nearly flat lid and constrained deep side bay remain. No distant review, Unreal import or additional parameter sweep is justified by these failed primary views.

Independent PhysicalB checks confirm identical A/B initial/raw fields and stream-power settings, exact fixed heights/992 boundary edges and valid open topology. Closed-domain diffusion volume error is 4.3e-14 m³, no new extrema occur, and invalid padding shifted by 1e6 m changes valid output by zero. Actual diffusion boundary input is 96,513.6566476 m³; its active-volume ledger closes within 5.4e-10 m³, and the combined terrain/SPL/boundary ledger within 6.2e-10 m³. Maximum outgoing weight is 0.7142857143 over 560 substeps. Reports are `Cliff-Contact/Saved/WeatheredAudit/fastscape_crater_physical_a.json` and `fastscape_crater_physical_b.json`. These establish model/constraint correctness, not attractive geology.

## Temple support is explicitly unresolved

Old temple masks are deliberately removed from the simulation, avoiding frozen-island pedestals. Existing live actors were not moved. The current terrain proposals do not support their old placements: A buries Actor33/34 by approximately 12–14 m and removes ground beneath parts of Actor36/48; B also changes all four supports substantially. Each manifest reports sampled footprint height ranges and sets `full_footing_support_verified=false`. Actor36 remains a partly covered footprint; sampling is not a complete foundation test. Neither candidate may be integrated while presenting these placements as preserved.

## Reproduction

From `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams`, with the isolated dependency already installed as documented in the upstream handoff:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_fastscape_crater_sector.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz --dependencies Saved/FastscapeReference/python_packages --diffusivity 0 --output Saved/FastscapeCrater/ReplayA
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_fastscape_crater_sector.py --input Saved/Massif-Cliff/source08_measured_boundary_massif_v3_seed.npz --boundary-source E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz --dependencies Saved/FastscapeReference/python_packages --diffusivity .05 --output Saved/FastscapeCrater/ReplayB
```

Use a fresh Saved output directory. No dependency installation/download happens inside the generator. Exact first-A generator hash: `0c0d58425309107b51ba9376ec2d8fbc9595a7e83555d5a7e2054feaed8d5452`. The corrected B source is copied in its bundle and identified by manifest SHA.

The narrow initial rise and nearly flat contributing plateau are strong remaining authoring constraints: a 500 m sector with a roughly 200 m vertical range naturally favours a tall wall. If lateral transport merely rounds those drapes, the next substantive experiment must change the physical catchment, plateau relief and face setback before erosion. More texture, small noise or a third strength sweep is not justified by these results.

## Concrete next primary-shape experiment, not implemented

Generate the parent drainage/ridge terrain before defining its exposed cliff. The official [Fastscapelib Mountain example](https://fastscapelib.readthedocs.io/en/latest/examples/mountain_py.html) grows connected relief through uplift opposed by channel erosion and hillslope diffusion; unlike the escarpment example, its initial condition contains no imposed tall step or plateau lid. That is the relevant process architecture to test next, not a reason to scale its 75 × 50 km demonstration mesh.

For one physical authoring proof, use the real 500 m coast and a wider hinterland catchment domain rather than placing a 56 m vertical rise immediately behind it. Establish two unequal connected drainage divides, with several branching valleys reaching the coast at unequal positions. An upstream uplift/erosion run can generate the ridge network; art-directed large-scale divide/outlet placement may constrain its composition. Judge that complete parent landform in clay before fitting outer sector seams. The choice of hinterland extent, additional boundary data and final landmark elevations must be explicit; the current 500 m crop cannot prove adjacent terrain support.

Only after the parent ridges and valleys work should selected inward-facing steep surfaces be extracted and shaped into coherent three-dimensional rock masses. Stream-power erosion provides drainage/topographic structure; it does not generate hard-rock joint planes, undercuts, detached blocks, fracture spacing, PBR stone detail or a ready Nanite cliff. Those are later surface/volume authoring steps with their own visual gate. This pass implements neither the new parent model nor that rock surface step.
