# Caldera lake - 15 November

**Current revision:** the inlet banks are part of the connected cliff mesh, with original fracture relief restored on the dry shoulders in `IntegratedBanksFractured`. The smooth first integrated banks were rejected in native review. The detached procedural embankment, stretched one-row lower wall and large mist cards from checkpoint **`a368e8f` are superseded**. Their screenshots and successful build/exit checks remain historical evidence, not acceptance of this revision. Root owns final native review and STATUS.

## Current assembly

**13:22 visual iteration:** the lower river now narrows from 440 m to 320 m inside its existing supported bed. This removes the large suspended blue side-curtain in the diagnostic frame without raising terrain into the full steering envelope. Half-width in metres is `160 + 25*SmoothStep(1040,1105,R) + 35*SmoothStep(1113,1130,R)`. An actual-triangle probe at 810 lower-edge samples reduced maximum water/bed separation from 32.64 m to 4.33 m, with no missing support. The remaining normal bed depth is not proof of a perfect seam from every angle.

The flat terminal overlap sinks 10 m instead of 1.5 m. A cataract-only hull-flatten gate is zero below `.56R` and full above `.58R`, so an inward-steered ship cannot lift that hidden extension back onto the lake. Displacement and both normal paths share the gate. The lake, terrain, centreline and steering route are unchanged.

Ramp foam now has slow, broad supply variation over 48-370 m scales while its fine detail still advects downhill at 30 m/s. The impact bank uses nine unequal 42-48 m volumes with independent height/density, deterministic small drift, softer radial extinction and a steeper height falloff. Dense-core radial support is conservatively 942-1067 m: clear of the nominal 928 m orbit, **not** the full 818-1038 m steering envelope. Outer steering can enter the fog; thin river/shore spray also crosses the nominal orbit locally. No additional fog volumes or larger budget were introduced.

The user requested a large caldera lake with ordinary waves, one foaming downhill inlet, fitted rocky banks and shallow local mist. Lake mean height is **-400 m**; nominal crater radius is **1,600 m**. The 440 m wide inlet uses `angle=pi-.40*(1-smoothstep(.58,.985,R/Radius))`, reaching lake height at radius 928 m from height 0 near radius 1,576 m.

The ship descends over encounter seconds **-8 to 32**, then stays on radius 928 m. Angular speed eases in during seconds 24-32 and settles at `.009 rad/s` (**8.35 m/s**); a 33.4 m tangential lead avoids stopping at the join. The route and lake height are unchanged by this bank revision. There is no further spiral descent or final throat; boss balance remains unfinished.

`generate_caldera_integrated_banks.py` remaps the original inlet-sector grid into a broad asymmetric apron, blending into existing side and upper cliff vertices. The previous abrupt toe drop is distributed over 220 lower rows. Upper rows 248 onward and triangle connectivity remain unchanged; altered lower UVs follow the new surface. `BuildInletBed()` runs after validated `BuildCliffPreview()`: the installed `integrated_inlet_banks` marker only skips the old standalone wedge when `CliffPreviewMesh && bCompleteCliffArena` confirms the integrated mesh is active. It uses **56 actual surface anchors** for modestly scaled, partly buried Nordic bank accents.

`fracture_caldera_integrated_banks.py` restores pre-cut geological radial and vertical residuals on those supported shoulders: radial addition capped at 18 m, vertical addition at -12/+20 m, with a shallow shared-vertex bevel. It changes 150,959 vertices while keeping the wet ±220 m corridor and all upper rows 248 onward bitwise unchanged. A local 10 m taper resolves nine inverted triangles; only 830 vertices receive more than 1% attenuation. This preserves the connected supporting apron rather than restoring the old suspended river or adding detached rock layers.

The water mesh and bed use matching polar cross-sections. The shader reconstructs that same curved river from arena XY, advects foam downhill along its surface and retains ordinary lake-wave coordinates at the zero-slope join. Only the overlap below radius `.58*Radius` sinks beneath the lake. Rebuild just the water material with `make_maelstrom_water_only.py`.

Lower cliffs and banks now use physically scaled owned rock through **`cliff_physical_material.py`**, applied after a clean upper-atlas graph rebuild by `import_cliff_geology.py`. Helper v2 cools the warm rock palette and combines measured scan-depth normals with restrained 8-30 m joint/facet normals. The upper unique geology remains above the 0-45 m arena-height blend. This avoids stretching a narrow retained UV strip down the tall cliff extension; shading does not alter source geometry or vendor pixels.

There are **128 shoreline volumes with 40 m radius**, centred at Z=-399 m over actual final-surface intersections with Z=-400 m, **110 staggered river spray volumes with 37-43 m radius**, and **nine connected impact-fog volumes with 42-48 m radius** across the foot. The denser foot fog follows the user's request to obscure the waterfall bottom. Spray local Z aligns with the water surface normal to avoid horizontal fog bands; the former giant cards are removed. This is 247 scene volumes, or 251 including four ending-fog volumes, within the stage-scoped budget of 256. Budget changes use `SetWithCurrentPriority` and restore without priority escalation. Ending fog covers ride seconds **440-457**, with seek-safe fade reset and suppression of the old white exposure flash. Perimeter falls remain disabled.

## Frozen source and export

Source: `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/IntegratedBanksFractured/candidate.npz`
SHA-256: **`6eac5bba828f84ef2196f3a6598c23829c503722eb056f7ad59269bd6ab444e7`**.

FBX: `E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact/Saved/CompleteCliffRing/IntegratedBanksFracturedNative/complete_caldera_banks_fractured.fbx`
SHA-256: **`264605a859674b1288cc1f8feb7c6a534bb2b46ae9b75997688ddc87d9448640`**.

Matching JSON/NPZ and `original_mesh_reference.json.gz` are beside the FBX. The source retains **863,513 vertices / 1,720,320 triangles**, arena XYZ metres, identity basis and export origin `(-1072,1154,-25)m`. Its manifest supplies `integrated_inlet_banks`, `bank_boulder_anchors_m` and 128 `shore_mist_centers_m` points. Actual ruin support must be regenerated for this source.

The base generator reads historical `CalderaSubmergedToe` (`e4d85d00...`) and its recorded pre-toe `CalderaEntry` parent (`8ce34f7f...`). The final correction reads `IntegratedBanks` (`ac616b7c...`) plus the actual residuals from `SurfaceRockBounded` (`a0ef5da4...`), which also remains the explicit ancestor of retained upper context paint. These are dependencies, not the final runtime mesh. All owned scans and generated assets remain local and Git-ignored.

## Replay and install

Choose fresh output directories for a replay; the paths below identify the frozen deliverable. Keep the parent source bundles available. Run Unreal jobs serially with the editor closed.

```powershell
$dcRoot = 'E:/Projects/DirectorsCut/Maelstrom-Saturday'
$cliffRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Cliff-Contact'
$waterRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Complete-Water'
$flowRoot = 'E:/Projects/DirectorsCut/Crater-Workers/Flow-Seams'
$py = 'E:/Utilities/Blender/5.3/python/bin/python.exe'
$blender = 'E:/Utilities/Blender/blender.exe'
$ue = 'E:/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$mesh = "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanksFractured/candidate.npz"
$fbx = "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanksFracturedNative/complete_caldera_banks_fractured.fbx"
$sidecar = "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanksFracturedNative/complete_caldera_banks_fractured.json"
$reference = "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanksFracturedNative/original_mesh_reference.json.gz"

& $py "$dcRoot/Tools/generate_caldera_integrated_banks.py" --source "$waterRoot/Saved/CompleteCliffRing/CalderaSubmergedToe/candidate.npz" --output "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanks"
& $py "$dcRoot/Tools/fracture_caldera_integrated_banks.py" --source "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanks/candidate.npz" --rock-source "$cliffRoot/Saved/CompleteCliffRing/SurfaceRockBounded/candidate.npz" --output "$cliffRoot/Saved/CompleteCliffRing/IntegratedBanksFractured"
& $blender --background --factory-startup --threads 8 --python-exit-code 1 --python "$dcRoot/Tools/export_cliff_surface_fbx.py" -- --input $mesh --output $fbx --normal-angle-deg 60
& $py "$dcRoot/Tools/audit_cliff_preview_import.py" --prepare $sidecar --output $reference
$env:DC_CLIFF_PREVIEW_FBX = $fbx
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_landform_preview.py" -unattended -nullrhi -nosplash -DDC=InstalledNoZenLocalFallback
$env:DC_CLIFF_AUDIT_SOURCE = $reference
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/audit_cliff_preview_import.py" -EnablePlugins=GeometryScripting -unattended -nullrhi -nosplash -DDC=InstalledNoZenLocalFallback
& $py "$dcRoot/Tools/prepare_cliff_preview_surface.py" --sidecar $sidecar --anchors "$dcRoot/Saved/CliffSupportAnchors.json" --saved "$dcRoot/Saved"

$env:DC_CLIFF_GEOLOGY_ATLAS = "$flowRoot/Saved/CliffGeology/FacetedUniqueV3"
$env:DC_CLIFF_GEOLOGY_CONTEXT = "$dcRoot/Saved/CompleteCliffGeologyRock"
$env:DC_CLIFF_GEOLOGY_DETAIL = "$cliffRoot/Saved/CliffDetailNormals/NativeMeasuredBC5"
$env:DC_CLIFF_PHYSICAL_TILE = "$flowRoot/Saved/OwnedCliffProjection/vfmjadufa/Tile"
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/import_cliff_geology.py" -unattended -nullrhi -nosplash -DDC=InstalledNoZenLocalFallback
& $ue "$dcRoot/DirectorsCut.uproject" -run=pythonscript "-script=$dcRoot/Tools/make_maelstrom_water_only.py" -unattended -nullrhi -nosplash -DDC=InstalledNoZenLocalFallback
& $py "$dcRoot/Tools/install_complete_maelstrom.py" --project $dcRoot --caldera
```

`import_cliff_geology.py` rebuilds the clean graph and calls the physical helper itself; do not append that helper again to the same graph. Do not run the full `make_maelstrom_materials.py` replay, which would overwrite independently edited spray/cascade/mist materials.

The installer verifies the source audit, support binary and material ancestry, then writes `CompleteArena.json` last. It propagates the integrated-bank flag, actual boulder anchors and shoreline points, and enables the normal caldera game with no old falls JSON dependency. `Play Caldera.cmd` starts before the reveal; `Tools/play_complete_maelstrom.ps1 -FullRun` starts at the beginning. This is a local Unreal game, not a newly cooked standalone release. Copy/regenerate ignored local assets on the integration machine and preserve newer crew HUD/names/voice work when merging.

## Validation and review state

CPU geometry checks on the fractured source: original connectivity retained, zero angular seam error, zero collapsed native-float triangles or opposite source triangle normals; minimum doubled triangle area 0.5934 m². All **2,365 sampled active descent positions** have terrain beneath them; the navigable **±165 m core** has no sampled rock roof, and centreline water/bed clearance remains **3.676-4.006 m**. Outer shoulders intentionally intersect the ribbon edges. The 128 shoreline points are interpolated on actual source mesh edges at lake level, not copied from the submerged toe. This is sampled evidence, not a full collision or art certification.

`& $py "$dcRoot/Tools/probe_cataract_chart.py"` checks the actual CPU and shader polar river chart over 39,961 samples. Last errors were 0.000122 m radial and 0.000793 m across; the lake/foot wave phase matched. It verifies mapping, not GPU appearance.

The fractured mesh imported, passed the actual source geometry/mask audit and installed successfully. Final Editor/Game builds pass (`BuildCalderaFootClearEditor.log`, `BuildCalderaFootClearGame.log`). Water-only and physical-rock material imports pass; native rendering reports no material compilation failure. The ordinary UDS editor-utility and experimental toolset startup errors remain unrelated existing log noise.

The first native pass exposed smooth banks, sandy material and horizontal white spray bands. Those were revised with restored source relief, measured-depth normal mapping, porous foam and surface-aligned mist. The owner then requested much denser waterfall-bottom fog. Native `CalderaImpactFogFinal` verifies that coverage, but `CalderaShipFinal` caught the ship sitting inside it too long. The impact bank now sits at radius **995 +/-7m**, radius48m, Z=-383m: its inner edge stays outside the nominal radius928m battle lane. `CalderaFootClearShip` confirms a clear battle view at ride321 and340. That frame contains the old boss implementation; the new boss chat is not merged into this checkout.

`CalderaShipFinal` verifies opaque ending fog at ride448, restored open sea at458 and a clear backward seek to275. The subsequent change only moves the nine impact volumes uphill. Its sampled ship frame at321 ran63fps; the external inlet frame ran90fps at1600x900 native. These are individual samples, not long-run performance certification.

Remaining visual work: broader bank faces still look sculpted, the small brown scan accents remain conspicuous, foam is still fairly uniform across the wide ramp, and the dense impact fog has a smooth edge. Do not call this the final reference-quality arena. Improve these locally without reopening the flat-lake/route design. Boss gameplay is assigned to the separate **Caldera boss fight** chat (`Boss-Caldera`); preserve that work and newer main crew code on integration.

The existing `directorscut-crater-visual-iterations` heartbeat was updated and reactivated every30minutes for the current flat-lake design. It must not resume the obsolete spiral,900m-deep basin or previous waterfall bindings. The current checkpoint is a local Unreal game, not a newly cooked release.

## Current native captures

- [Inlet diagnostic overview with dense impact fog](review/caldera-local-mist-15nov/inlet-fog.png): final `CalderaFootClearOverview`, actual ride321. The lower waterfall is obscured; the broad ramp and uniform mist edge still need art refinement.
- [Normal-game ship view outside the fog](review/caldera-local-mist-15nov/ship-clear.png): final `CalderaFootClearShip`, actual ride321; the old creature is visible because this is not the separate boss branch.
- [Restored open sea](review/caldera-local-mist-15nov/restored-sea.png): normal-game ride458. Backward seek to275 also cleared the ending fade.

These are unedited native1600x900 screenshots. Both final review processes exited successfully; no new water/cliff material compile failures or fatal errors. No claim of full gameplay balance, long-run performance or final art acceptance.

## Rejected lower-bank contact trial

`LowerBankContact` source SHA-256 `fdc192102f40c5a4b6f9f0576d0e8989ee49c372cd5462ef9f5ca246235fe054` is rejected. Raising narrow side strips replaced the exposed blue water triangle with a projecting rock fin. It also intrudes into the full steering orbit: radius 928+110=1038 m crosses the 185-235 m across-river strip around encounter second 50. The original lower-bank clearance clamp protects that envelope; passing central-river and water-edge contact checks did not establish full-route safety.

Root restored `IntegratedBanksFractured`, re-audited the actual imported mesh, regenerated supports and reinstalled its runtime marker. Its source identities remain those documented above. The accepted fix tapers the water ribbon within the existing terrain instead of raising rock into the steering corridor. `fill_caldera_lower_bank_contact.py` remains an untracked experimental helper; do not replay or install that trial.

## Latest verification and critique

`BuildCalderaFlowEditor.log` and `BuildCalderaFlowGame.log` pass. The water-only import passes; the 39,961-point CPU chart probe retains zero lake/foot wave-phase difference. `CalderaFlowTaper` renders the actual narrowed inlet at native1600x900 without water/cliff material compilation failures. Its frame replaces the large projecting blue curtain with a much smaller edge. This is a targeted improvement, not final visual acceptance.

`CalderaFlowFinalShip` verifies clear player views at315/335, dense ending fog at448, open sea at458 and fade cleanup after seeking backward to275. Steering input+1 reached offsets30-33 m during these short captures; it is **not** a native full110 m steering sweep. No new fatal/material compilation errors. The inlet-foot diagnostic camera is external and must not be mistaken for the player camera. No long-run performance or newly packaged-build certification.

The broad river remains too regular, the banks still look sculpted, and the yellow-brown scan accents do not belong to the surrounding stone palette. Impact fog is less uniform but still has visible lobes. Next priorities are a scoped material override for those accents and further contact/foam breakup without changing the accepted lake, sky or boss route. Do not restart a cliff reconstruction or install the rejected lower-bank trial.

- [Latest actual inlet overview](review/caldera-flow-15nov/inlet.png)
- [Final player-view steering check](review/caldera-flow-15nov/ship.png)
- [Restored open sea after ending fog](review/caldera-flow-15nov/sea.png)

The boss visible in this checkout is the old implementation; the separate boss branch is not integrated here. This remains a local Unreal game checkpoint, not a newly cooked release. All engine jobs are serial and offscreen; this pass leaves no standing engine ownership lock.
