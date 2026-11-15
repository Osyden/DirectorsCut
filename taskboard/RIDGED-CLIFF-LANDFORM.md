# Ridged cliff landform: offline comparison and reproducible handoff

The corrected four-octave ridged candidate is the strongest **offline shape proposal** from this pass. Matched clay views show diagonal projecting masses and recesses of unequal heights, rather than the previous repeated full-height slots. It still has a flat plateau, unfinished support transitions and some soft fins. Erosion makes comparatively subtle changes. This note makes **no runtime, Nanite, game integration or final visual-quality claim**. Root owns the isolated Unreal import and its bounds/Nanite checks.

## Source ownership and revisions

- `91a2f38`: new `Tools/generate_ridged_cliff_seed.py`, two-octave baseline using Blender's installed `mathutils.noise.ridged_multi_fractal` and `hetero_terrain`.
- `3ff7f7d`: `--octaves 2..4`; extra-octave detail comparison with its broad positive bias removed. Default two-octave numeric output remains bitwise identical to `91a2f38`.
- This handoff also adds compatibility with `base_profile_angles_rad`, the field name in tracked clean generator `bf0ab29`. It changes no parameters or generated terrain. Exact numeric replay from that generator was verified.
- `d31f31c`: optional explicit shoreline outflow in `Tools/generate_eroded_cliff_sector.py`, with 13 passing conservation/protection/domain tests. No erosion-kernel changes were made during the ridged-terrain pass.
- `bf0ab29`: tracked `Tools/generate_cliff_landform.py`, used to reconstruct the clean input constraints from the retained historical measured NPZ.
- `f72e98e` (integrated as `a71ca52`): original `Tools/export_cliff_erosion_seed.py` measuring the historical production geometry.

The source generator evaluates genuine physical XY fields: ridge octaves at 96/48 m by default, plus 24/12 m with four octaves; heterogeneous terrain at 68/34 m, plus 17/8.5 m. The domain warp is 18 m at a 180 m scale. It does not require each radial column to be monotone. Input shore, plateau and tile-edge support heights remain fixed; the old interior survives only within explicitly documented support/edge blend margins.

## Immutable inputs: historical geometry is a real dependency

The measured NPZ came from historical source **`3339c570c66a89b9ab87e2d2fce06f9f89411c7b`**, whose position-generating functions were checked against **`4bb5b0c`**. These were experimental historical cliffs, not necessarily the current runtime formula. Do not silently rerun the exporter on today's source and call it the same input.

Measured dependency:

`E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/Source08-Seed/cliff_erosion_seed.npz`

SHA256: `28c8d038247eb1a48b79549a670a5fe17371b507bce59e56ace01ce9eb82dc09`

Position-functions SHA256 recorded inside it: `e49caa66cb9ca3beddae64088e6dbbea0abe2586ed925d52b6d6dfadf7bf54d0`.

Exact archived clean input used for both reviewed ridged seeds:

`E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CliffLandform/source08_clean_branched_seed.npz`

SHA256: `ca878e69099177d451044516ca38585cb528dbe23f1ae0bedebfcde27d83c2f6`

This is a 500 x 504 m source-8 sector, 253 x 251 samples at 2 m spacing. It has 52,536 physical samples, 47,095 fixed samples, 16,408 active samples and 251 explicit fixed shoreline outlets. Coordinates are absolute **design-world/arena-local metres before the moving ship-centred pose**, with the original right-handed frame retained.

Generated NPZs live under ignored `Saved`; they are **not contained in Git**. Preserve the measured dependency or regenerate it from the explicit historical source snapshot. The end-to-end re-export from a fresh historical checkout was not repeated in this pass. Its original command was:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/export_cliff_erosion_seed.py `
  --project-root '<isolated checkout of 3339c570c66a89b9ab87e2d2fce06f9f89411c7b>' `
  --geometry-ref 4bb5b0c --source 8 --width-m 500 --cell-size-m 2
```

The exporter requires the local UE source/MSVC CPU probe toolchain. A current-formula mismatch should fail `--geometry-ref`, not be bypassed. Its highest-surface heightfield omits overhangs; the ridged generator uses the extracted shore/plateau constraints, not that rejected wall interior.

## Reproduction from tracked generators

Use a checkout containing `bf0ab29`, `91a2f38`, `3ff7f7d`, the angle-field compatibility change in this handoff, and `d31f31c`. Run from its repository root. The following clean-input rebuild was executed and the resulting ridged seed's **every numeric field** matched the reviewed corrected-four seed exactly. The clean generator renamed `clean_profile_*` to `base_profile_*` and added metadata, so NPZ bytes/hashes differ even though all fields consumed by the ridged generator and the final numeric output match.

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_cliff_landform.py `
  --input E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/CliffErosion/Source08-Seed/cliff_erosion_seed.npz `
  --output Saved/Ridged-Cliff/replay_clean_branched_seed.npz --seed 71421

& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python Tools/generate_ridged_cliff_seed.py -- `
  --input Saved/Ridged-Cliff/replay_clean_branched_seed.npz `
  --output Saved/Ridged-Cliff/replay_ridged2_seed.npz --seed 71421 --octaves 2

& 'E:/Utilities/Blender/blender.exe' --background --factory-startup --threads 8 `
  --python Tools/generate_ridged_cliff_seed.py -- `
  --input Saved/Ridged-Cliff/replay_clean_branched_seed.npz `
  --output Saved/Ridged-Cliff/replay_ridged4_detail_seed.npz --seed 71421 --octaves 4
```

The actual archived seeds were generated with the same commands but the immutable `source08_clean_branched_seed.npz` input above. Blender was 5.3.0, hash `02f0899d78e3`; no packages, GPU or renderer were needed for field generation. The corrected seed records generator-source SHA256 `a903f79e8199df440bd018be38ac9e4c77667fb5cb40811c98f59fd7c588390e` at `3ff7f7d`. The later angle-field alias affects only compatibility with regenerated clean metadata.

For either seed, actual bounded erosion used exactly:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' -B Tools/generate_eroded_cliff_sector.py `
  --input Saved/Ridged-Cliff/replay_ridged4_detail_seed.npz `
  --output Saved/Ridged-Cliff/replay_ridged4_detail_open256.npz `
  --steps 256 --seed 71421 --rainfall-m .003 --thermal-rate .05 `
  --sediment-capacity .4 --erosion-rate .08 --deposition-rate .2 `
  --max-erosion-cell-fraction .005
```

For the two-octave comparison substitute its seed/output names. The seed's `outflow_mask` enables open drainage; unmarked boundaries remain closed. Flow, erosion, deposition and outlet fields are actual solver outputs, not artistic masks. Every protected height remains bitwise fixed. No end-of-run sediment dump is performed.

## Preserved results and hashes

All paths in this table are beneath `E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams/`.

| Artifact | Path | SHA256 |
| --- | --- | --- |
| Two-octave seed | `Saved/Ridged-Cliff/source08_ridged_seed.npz` | `8704db130abcf36f3d704924d6c3ff3539f08ba6311a6854b30fd36ccfb995dc` |
| Corrected four-octave seed | `Saved/Ridged-Cliff/source08_ridged4_detail_seed.npz` | `13ec99bf32a5b3c980f3f73b0d3293c66eb01c15dbf60e4da0211b7a2b087e5e` |
| Two-octave actual erosion | `Saved/Erosion-Outflow/Ridged-Source08/clean_source08_open256.npz` | `d57a50f5ba3d995360debb8722b427a5b650fea47100a3192dd51bab7dd1d186` |
| Corrected four actual erosion | `Saved/Erosion-Outflow/Ridged4-Source08/ridged4_detail_open256.npz` | `d6f3f07852d53de5e2e121267d361ccccc0122a54f19051b373da5931048ae8e` |

`Saved/Ridged-Cliff/artifact_provenance.json` records the complete paths and hashes. Reproducibility means matching numeric arrays and settings; NPZ compression metadata, embedded paths and later descriptive metadata can change archive hashes on regeneration.

Raw uncentred four-octave files (`source08_ridged4_seed.npz`, `ridged4_open256.npz`) are retained only as rejected uplift diagnostics. Do not confuse them with the `*_detail_*` candidate.

## Controlled octave comparison

Both Blender fractals add positive mean energy when adding octaves. The raw four-octave test lifted the two-octave surface by a median 5.44 m and maximum 24.51 m. That confounded the requested comparison of finer hierarchy with a change in broad rock masses.

The corrected comparison keeps the original two-octave signal and subtracts a 12 m-sigma masked Gaussian mean from **only the added-octave residual**. It then adds that residual before the existing bounded height mapping. This is a controlled comparison technique, **not a blanket requirement that rock alterations be zero-mean**. A deliberately different macro landform may legitimately add or remove mass.

Corrected four minus two: P5/median/P95 = -3.13/-0.0015/+3.30 m, mean +0.030 m. Both retain the same 39.59 m maximum procedural relief envelope and zero active samples above the measured rim. Full terrain deviation from the linear ramp can be larger in the retained support blend, so that envelope is not claimed as a bound on every historical boundary contribution.

## What the tests and clay views establish

- Identical seeds generate identical arrays; a different seed changes the interior. The default two-octave geometry remains bitwise equal to the original baseline.
- All protected input heights, shoreline outlets and input arrays remain unchanged. Outputs are finite, bedrock never lies above terrain, and no active sample exceeds its measured rim bound.
- A constant field stays constant through the masked mean filter; invalid/protected padding does not bias the detail centering.
- The tracked `bf0ab29` clean generator plus the retained measured input reproduces every final corrected-four numeric array exactly after the angle-field compatibility change.
- Bounded erosion obeys mass conservation to less than `1e-9 m3` in the double-precision solver; float32 output error remains within its recorded rounding bound.
- Two-octave actual erosion: maximum cut 1.64 m, maximum fill 3.31 m. Corrected four: maximum cut 2.27 m, maximum fill 4.34 m. These are accelerated morphology settings, not real-world weather times.
- Ridged-two incision components are separate roughly 38-104 m masses rather than the previous 472 m-wide foot band. Flow/net-cut correlation is only +0.13: do not advertise successful physically realistic gullies from that weak metric.
- The D8 solver retains an approximately 8% directional capacity bias measured on equivalent rotated planar slopes. This pass did not change capacity physics.

Matched CPU clay reviews are under:

`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/RidgedSeedReview/`

`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/RidgedOpen256Review/`

`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/Ridged4DetailSeedReview/`

`E:/Projects/DirectorsCut/Crater-Workers/Fitted-Waterfall/Saved/CliffLandform/Ridged4DetailOpen256Review/`

Use `wall_before.png`, `wall_after.png` and `oblique_*` with the review worker's locked camera manifest. No displacement shader or vertical exaggeration is used. The corrected four has more useful ridge hierarchy; the actual erosion difference is subtle. It still needs a 3D rock/fracture pass, better support transitions, and authoritative individual temple footing checks. Current `protected_mask` preservation alone does not establish compatibility with today's runtime scene.

The review worker owns preview mesh/FBX export and real diagnostic vertex channels; root owns any isolated UE import. Stop parameter iteration here until that separate preview is assessed.
