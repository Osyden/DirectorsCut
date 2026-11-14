# Procedural waterfall VDB prototype

Owner: Codex waterfall worker on `codex/crater-vdb-prototype`, isolated at `Crater-Workers/Waterfalls`. Root owns Unreal import/runtime integration and all shared maelstrom source. This worker owns only `Tools/generate_waterfall_vdb.py` and this handoff; no editor, cloud or purchased asset changes.

Implemented: `Tools/generate_waterfall_vdb.py` creates original density sequences using periodic particle emissions, gravity, terminal drag, lateral flutter, secondary ballistic impact droplets and a smaller drifting mist component. It does not use a cloud cache or vendor content. Generated files stay under ignored `Saved/GeneratedWaterfalls`.

## Actual preview available

Local output root: `E:/Projects/DirectorsCut/Crater-Workers/Waterfalls/Saved/GeneratedWaterfalls`.

- `Falling/Falling_0000.vdb` through `Falling_0047.vdb`.
- `Impact/Impact_0000.vdb` through `Impact_0047.vdb`.
- `manifest.json`: every file's SHA-256, byte count, active voxel count, peak/sum density, clipped particle mass, fixed bounds and numerical verification.
- `Falling_density_projection.png` and `Impact_density_projection.png`: four front-view maximum-density projections, **not Unreal renders or evidence of final visual quality**.

Defaults: 80 m height, 24 m source width, 10 m downstream displacement, 1 m voxels, 3,500 particle emissions/second, seed 54117. Each sequence has 48 frames at 12 fps, a four-second loop with no duplicate endpoint. The entire 96-file preview generated in **1.26 seconds CPU time**, totalling **10,191,855 bytes / 9.72 MiB**. This is generation/disk cost, not GPU performance.

| Sequence | XYZ resolution | Fixed voxel-centre bounds, cm | Maximum density | Disk bytes |
| --- | --- | --- | --- | --- |
| Falling | 39 x 23 x 84 | (-1900,-500,-100) to (1900,1700,8200) | 0.501521 | 7,345,260 |
| Impact | 53 x 27 x 14 | (-2600,500,-100) to (2600,3100,1200) | 0.206458 | 2,846,595 |

The eight extreme corners contain density `1e-8` to retain consistent active bounds without a visible box. The manifest also includes voxel-outer bounds, which extend half a voxel beyond the centre bounds above. Test actual SVT import bounds rather than assuming the importer preserves the anchors after quantization.

## Import and placement contract

Both sequences have one FloatGrid named `density`, tagged as a fog volume. **VDB world units are centimetres**: voxel transform is 100 cm for this preview. Computation and command-line dimensions use metres. X runs across the waterfall, Y runs downstream away from the cliff and Z points up. No axis swapping or additional 100x transform should be necessary; verify after import.

Shared local origin is at receiving-water height vertically below the source lip. Nominal lip is `(0,0,8000)` cm; nominal impact is `(0,1000,0)` cm. Preserve the authored transform and use `PivotAtCentroid=false`. For an actor anchored to an existing impact point, subtract the rotated `(0,1000,0)` cm offset. The separate impact sequence uses the same coordinate frame, so it needs the same transform as the falling sequence.

Import either numbered frame to import the entire sequence. Begin with density in a float channel so faint spray survives import; choose quantization after seeing it. Use zero emission and scene lighting, a configurable density-to-extinction multiplier, frame range 0-47, 12 fps and looping. Keep streaming nonblocking. Root owns import, material, actor lifecycle, cooking and packaged verification. Root reports successful native UE import of both 48-frame sequences with expected bounds. The importer derives names `Falling` and `Impact` from the source prefix, ignoring an attempted `SVT_` destination name. The integration remains opt-in (`DCProceduralFalls`) pending visual review.

**This 80 m module only covers a lower falling/impact reach of our 300 m-plus cliffs.** It does not complete the whole fall, justify enlarging a volume across the crater, or replace the liquid core, surface foam trails or near-camera droplets. Instantiate at its intended physical scale first.

## Reproduce and configure

From this worktree:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py
```

The script locates the bundled OpenVDB DLL directories from that Python executable; optional `BLENDER_ROOT` can override their base. It does not start Blender, install dependencies or touch Unreal. It rejects a nonempty sequence directory to prevent mixing old and new frame counts. Use a fresh output for variants:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --output Saved/GeneratedWaterfalls/HalfMeter --voxel-m 0.5 --emission-rate 7000 --fps 24 --duration 4 --height-m 80 --width-m 24 --depth-m 14 --downstream-m 10 --seed 54117
```

That second command is a suggested quality experiment, not an already generated or profiled variant. `--depth-m` controls source thickness and downstream/impact envelope bounds; `--downstream-m` controls actual lip-to-foot travel. Density mass is normalized by emission rate and voxel volume. Preview limits bound memory and sequence size.

## Verification and critique

- Every exported frame has finite nonnegative density and exactly the expected active bounds.
- `t=0` and `t=4` densities are bit-identical for both components. Independently instantiated same-seed generators, negative-time wrapping and four-period offsets also match; a different seed changes the density.
- First-frame VDB write/read roundtrip max absolute error is zero.
- Last-to-first mean absolute density step is 0.000571 for Falling, versus median 0.000595 inside the sequence; Impact is 0.000301 versus 0.000283. Boundary change over 2 ms is 0.0000168 / 0.00000876 respectively. Numerical continuity is plausible, but motion must still be reviewed at playback speed.
- Maximum clipped particle mass is 0.0000572% for Falling and zero for Impact.
- Diagnostic projections show downward strands and a low outward impact fan. Strong vertical source channels remain, the one-metre voxel size is coarse, and 12 fps may reveal stepping. This is a compact import/motion prototype, not reference-quality whitewater.
- The model does not solve coupled water/air dynamics, collisions with complex ledges, surface tension or secondary spray generation from a liquid simulation. If the falling strands look like hanging ropes in-game, vary emitter width/flow coherently and introduce sheet breakup; do not hide the issue by increasing mist density.
- **Lighting, translucency composition, cliff fit, motion appearance, GPU cost and packaged playback remain unverified by this worker.** Root received the preview paths immediately and reports native import success, as noted above.

## Finer candidate generated after initial import

Actual output: `E:/Projects/DirectorsCut/Crater-Workers/Waterfalls/Saved/GeneratedWaterfallsFine`, with the same `Falling/Falling_0000.vdb` and `Impact/Impact_0000.vdb` prefixes. Command:

```powershell
& 'E:/Utilities/Blender/5.3/python/bin/python.exe' Tools/generate_waterfall_vdb.py --output Saved/GeneratedWaterfallsFine --voxel-m 0.5 --emission-rate 14000 --fps 24 --duration 4
```

This generated **96 frames per sequence**, **73,048,468 bytes / 69.66 MiB** combined, in **12.70 seconds CPU time**. Falling resolution is 77 x 44 x 167, with centre bounds `(-1900,-500,-100)..(1900,1650,8200)` cm and peak density 0.510603. Impact is 105 x 53 x 27, bounds `(-2600,500,-100)..(2600,3100,1200)` cm, peak 0.180713. Physical lip/foot and all axes remain unchanged.

The generator already normalizes particle weight by actual emission rate and voxel volume and defines smoothing support in metres. No normalization fix was needed. Mean integrated density (`sum(density) * voxel_m^3`) differs from the coarse preview by **-0.707% Falling / -0.278% Impact**, within sampling/threshold differences. This supports keeping the same material extinction multiplier across candidates; it does not assert identical local optical thickness at every pixel. Increasing emissions 4x reduces sampling noise.

Both sequences retain exact period equality and first-frame VDB roundtrip, stable bounds and finite nonnegative densities. Maximum clipped particle mass is 0.00000145% for Falling / 0.00339% for Impact. Last-to-first frame changes remain comparable to interior frame changes. Density projections show smoother coherent strands without sparse dots, but the strong vertical source channels remain. No physics change or Unreal import/render was performed for this finer candidate; root will evaluate after its current capture.

## Integration review: placement failed visual acceptance

Root imported the finer sequences successfully and captured `Maelstrom-Saturday/Saved/SceneReview/waterfall-close-b/shot_03060.png`. I inspected that actual image: the new fall is a faint hanging column in front of and to the left of the bright surface-water core, with a small low impact plume. Its upper end has no convincing source. Keep `DCProceduralFalls` opt-in; this is not an accepted waterfall replacement.

The imported centimetre scale, yaw mapping (+Y toward the basin), shared impact offset and arena attachment are correct. Runtime teardown resets actors and the review camera, and guarded restoration avoids overwriting a subsequent trace-distance change. The capture explicitly passed `-DCFallFPS=24`, so the finer asset played at its authored rate. Root is fixing the runtime's old 12 fps default by deriving rate from frame count / four-second duration, with a `DCWaterfallLoopSeconds` override.

**The failure is attaching only the foot, without fitting the source and intervening cliff path.** Feed 8's logged foot is `(-110816.30,112197.80,-10956.99)` cm; its lip is `(-122372.89,123898.46,2025.84)` cm. The actual path retreats approximately 164.5 m horizontally while rising 129.8 m. Our module rises 80 m but retreats only 10 m from its foot. It therefore occupies the open air well in front of the sloping rock/core rather than following the lower receiving reach. Increasing density or uniformly enlarging the module would expose this disconnection more clearly. A cliff geometry revision also invalidates any fitted profile.

Next placement pass, after the cliff profile settles:

1. Export the chosen feed's actual lower water-core centreline from `CliffSurfacePoint` and the same front-of-rock offsets used by `BuildWhitewater`, together with receiving-water height, source width and `CurrentAt`/trail direction. Sample enough points to retain ledges; store the profile/configuration hash with the VDB manifest.
2. Find the lower-reach start by intersecting that profile with `Foot.Z + 8000 cm`, rather than adding `(0,-1000,8000)` to the foot. Measure its true horizontal run and slope. A straight freefall module is suitable only where this start and foot form a plausible gravity-driven arc clear of the rock.
3. For a steep exposed drop, regenerate the module using the measured height, source and landing displacement. For water that follows stepped/sloping rock, advect and rasterize particles along the sampled path with gravity projected onto the local flow direction, plus outward secondary spray. Do not nonuniformly stretch an 80 m vertical cache into a long hillside stream. Keep discrete ledge falls separate where their trajectories differ.
4. Place the impact at the measured receiving-water contact and align its downstream drift with the spiral current, not automatically the radial cliff normal. The present plume assumes a horizontal receiving plane; on a sloping water bank, particles must be checked against the actual local surface to avoid hovering or early disappearance.
5. Check the upper join, all ledge crossings and receiving-water contact from both the deck and a side view before tuning density. Then review the full loop, shadowing and a moving camera at native resolution. Retain a liquid core and advected surface foam; the VDB covers only aerated falling water and airborne spray.

Two secondary rendering findings are recorded for that later comparison; neither warrants hiding the placement failure:

- The installed UE5.8 `SparseVolumeTextureFactory.cpp` automatically imports a single grid named `density` into **Attributes A.R as UNORM8**. The current automated importer accepts that default; it does not preserve float16 density. Quantizing the first finer frame to 8-bit drops **36.4% of positive Falling tail voxels / 40.1% of Impact tail voxels** to zero, but these account for only **0.90% / 1.40% of total density mass**. This can trim thin wisps; it is not the main source of the disconnected appearance. Verify an explicit float16 import path later rather than assuming the source FloatGrid determines the Unreal format.
- Using the generated first frame and the current extinction multipliers (`.004` Falling / `.0015` Impact), straight +Y rays through occupied pixels have median optical depths approximately **0.054 / 0.020** (roughly 5% / 2% attenuation), with 95th percentiles **0.170 / 0.110**. These are CPU estimates from integrated density, not measured rendered opacity or radiance; lighting and view angle also matter. The initial settings are deliberately faint. Keep them unchanged until the source/path connection is repaired, then evaluate a modest extinction change and float-density preservation independently.

This audit made no runtime, import, generator, material or scene changes. Only this handoff was updated.
