# Full-height cliff trials, measured water context and stone sampling

Branch: `codex/maelstrom-saturday`, following `cac2cbf`. **Both new macro cliff trials are rejected for appearance.** Normal gameplay terrain, water, sky/light, boss and early game remain unchanged. This checkpoint retains a useful construction diagnosis, honest shape comparisons and an optional texture-based material proof.

## Shape diagnosis and actual results

The previous massif's raw mountain field spans only about 85 m. Its smooth boundary correction supplies most of the roughly 230 m cliff drop, explaining the long soft curtains. The new offline structural generator builds the measured drop directly and uses the fit only for local shore/seam/ruin constraints. The fraction of free samples needing over 50 m correction falls from 16.26% to 0.57% in the second trial.

That is a construction improvement, **not a visual success**. Trial A produces giant planar faces and straight trenches. A coordinate-only correction removes raster shoreline steps propagated into the face; it does not cure the large shapes. Trial B restores some heterogeneous hinterland and adds eleven finite unequal buttresses with bounded recesses, but still resembles a retaining wall with triangular supports. Root and both independent reviewers inspected actual wall, oblique and 1.5 km views and rejected both. Neither was imported into Unreal or promoted to the arena. See [generator, metrics and reproduction](STRUCTURAL-CLIFF-SECTOR-15NOV.md) and [independent visual decisions](STRUCTURAL-CLIFF-MACRO-REVIEW.md).

Existing audits establish unchanged XY, physical frame, coast, seams, outlets and protected support targets. The original 104,078 triangles remain one valid open manifold. Fully covered landmarks 33/34/48 remain supported; 36 remains partial. Numerical validity does not override the visible failure.

Retained actual rejected images:

- [Corrected A, wall](review/cliff-structure-15nov/rejected-a-wall.png)
- [B, wall](review/cliff-structure-15nov/rejected-b-wall.png)
- [B at 1.5 km](review/cliff-structure-15nov/rejected-b-far.png)

## Water context corrects interpretation of the lower edge

The isolated clay fringe is largely a truncated submerged toe: the first valid cliff samples lie 5.76–8.11 m inside measured receiving-water coverage. Do not relax the protected coast merely to hide that edge in a floating clay render.

`Tools/make_cliff_water_context.py` reads the existing `grid_queries.bin`: actual arena XY, `HeightAt(P,false)` and measured outer-water coverage. It creates a separate mean-water mesh in the exact source frame, without extrapolating outside water coverage. `Tools/review_cliff_outcrop.py --context-water` can now display this static diagnostic surface alongside unchanged cliff geometry. It is not an animated water/material or live contact test.

The actual A water-context review is [retained here](review/cliff-structure-15nov/rejected-a-water-context.png). It confirms that rendering water changes interpretation of the bottom crop, but leaves the engineered faces plainly visible. This image uses the initial raster-coordinate A, deliberately retained as failed evidence; it is not the corrected A or B.

Reproduction from the root worktree:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/make_cliff_water_context.py --heightfield 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/Saved/Massif-Cliff/source08_measured_boundary_massif_v3_open256.npz' --queries Saved/CliffErosion/CurrentSource08/grid_queries.bin --output Saved/StructuralCliff/mean_water_replay.npz
```

Use that output with the existing paired review tool and `--context-water`. Query SHA256 is `74da6a26564e48947ca30fed0d59b348049a2f553f828f94b9e2c5179ceb28c7`; the resulting context has 11,967 vertices and 23,302 triangles. Source/frame/XY checks pass. Mean water has no wave/WPO displacement and is sampled from the retained source snapshot, not a current animated frame.

## Installed stone texture proof

The installed Paragon Rampage pack includes tileable rock and slate albedo/normal maps. Root exported and visually inspected them locally under ignored `Saved/CliffTextureReviewParagon`. These are useful surface detail candidates; the lichen-rich rock map is not a generic bare cliff. No vendor texture was changed or committed. The packed rock mask is sRGB and its channel semantics are unverified, so it is not used.

`Tools/make_cliff_preview_material.py` now supports `DC_CLIFF_STONE=slate`. The default remains original procedural stone. Slate references the installed `T_SlateTextureTile_000_D/N` assets, using local-space triplanar projection at an authored 4 m tile size and native BC5 normal decoding. Roughness remains an explicit authored response; this is not a calibrated scan workflow. Local projection follows the moving crater instead of swimming in world space.

The first native test had a conspicuous regular pattern. The corrected version blends three offset samples on a triangular lattice, with shared offsets for albedo and normal and continuous UV derivatives for mip selection. Normal reorientation follows each projection's tangent signs and preserves the geometric normal for a flat source map. Slate uses neutral AO rather than unrelated procedural crack occlusion. Reports distinguish the active texture sampling from the inactive procedural normal bands.

- [Initial native material repetition](review/cliff-structure-15nov/slate-repeated.png)
- [Corrected native offset sampling](review/cliff-structure-15nov/slate-varied.png)

These matched 140 m engine views use the existing 72 x 40 m `JointBoundaryUnion` outcrop, **not either rejected new macro terrain**. Repetition is reduced, but the material still looks dark/mottled in this view and the underlying geometry remains unfinished. This does not establish final cliff quality, accurate wetness or distant material performance. The outcrop's RGB masks are diagnostic zeros, so historical flow/deposition effects are absent.

Regenerate the optional preview material with `DC_CLIFF_STONE=slate` and the existing Unreal Python commandlet. Run the offscreen review with `-DCCliffPreview -DCCliffPreviewFrameMesh -DCCliffPreviewDistance=140 -DCCliffPreviewRock`. Without the environment option, regeneration restores the original procedural material. Generated assets remain ignored.

## Verification and next implementation

Both material commandlets pass. Actual native `cliff13-slate-varied` shader rendering passes without a cliff compile failure or fallback, and active backward seek/exit restore terrain and camera at -4.99 and 181.03 seconds. Logs and captures remain in `Saved`. Existing Toolset startup errors and the non-Nanite VSM warning remain separate; no new build or performance claim is made. No C++ changed this pass.

Next retain the full-height physical envelope and continuous shore coordinate, but replace the rejected heightfield plane/wedge construction with connected three-dimensional bedrock masses and finite joint exposure across a representative face. The earlier bounded fractured outcrop is useful construction evidence; copying it repeatedly onto the current smooth parent is still rejected. Review a coherent region in full-sector/near/far context before increasing coverage. Preserve actual supports, coast, route and the common final-surface contract for waterfall contacts. Do not restart A/B plane tuning, general noise-only variants or additional audit frameworks. Waterfalls, water banks and boss remain behind the cliff geometry gate.
