# Outer-bank wave packets, pass5

Owner: Codex bank worker on `codex/crater-bank-packets-pass5`, based on `0d49f2b`. Claimed `DCMaelstromBreakers.cpp` and new self-contained `DCMaelstromBankWaves.h`; root owns engine/build/GPU work.

Inspected `taskboard/review/crater-pass4-14nov/current-bank.png` and `current-wide.png`. This pass adds signed finite physical wave packets to the large outer recovery bank. It preserves the mean hydraulic cross-section, route and protected receiving-water strip. The existing corrected water shader, sky and cliff work remain unchanged.

Implemented through `BreakerHeight`; no shared `DCMaelstrom.cpp/.h` patch is required. The new helper evaluates exactly a3x3 Cartesian cell neighbourhood. Each cell contains one randomly positioned finite packet, oriented along the local circulation with up to12 degrees of variation. Its compact envelope reaches zero height and slope at the boundary; no polar-coordinate phase or circular crest ring exists.

The crest/trough profile is the derivative of an enveloped sine divided by its wavenumber, so each unmasked packet has zero integrated displacement. A small second harmonic sharpens the crest. Signed packets are summed and smoothly limited to +/-2m, then masked to the outer recovery bank. This is static standing-wave geometry with existing animated shader detail, not a fluid simulation or newly animated CPU mesh.

Physical sizes and safeguards:

- Nominal wavelengths24–38m are raised to at least26.786m for the actual mesh resolution. Active mesh neighbours reach4.397m in planar coordinates, providing at least6.09 samples per smallest wavelength. The corresponding curved3D edges reach6.906m; phase is defined in the underlying physical XY coordinates, so its spacing stretches on a steep surface rather than aliasing into shorter waves.12m geometric waves are deliberately excluded at this mesh density.
- Packets span approximately51–88m along flow and36–56m across it, with only a few short crests each. Random position, phase, wavelength, amplitude and modest direction changes break up repeated rows. They use the existing basin topology; no extra draw call or mesh triangles are added.
- The effect is zero throughout the125m outer steering envelope (110m steering plus15m hull allowance), the final35m before the shoreline, the inlet and the terminal throat. The existing bank elevation and route remain the mean shape. An odd smooth limit prevents overlapping packets from becoming mountains.
- Existing foam sources and material normals/roughness are untouched. This is a geometry comparison; the candidate cannot create fresh contour-line foam. Additional bank whitecaps/spray should follow only after actual geometry review.

## CPU verification

`Tools/probe_bank_wave_packets.py` extracts the production helper and actual basin sampling/shore equations into a standalone CPU harness. No formulas are copied into the repository test as substitutes for production. Generated C++/executables/results remain ignored under `Saved/GeometryProbe`.

On the actual448x1536 grid:24,158 vertices receive nonzero relief, heights range-1.268 to+1.451m, area-weighted mean shift is-2.49mm and RMS relief is0.335m. The largest added slope is0.473. The protected corridor changes by at most3e-9cm (float roundoff); the shoreline strip is exactly unchanged.1,394 active spatial-cell boundary checks show no discontinuity: maximum height change over2cm is0.343cm and maximum slope jump0.00122. Independent packet integration and endpoint checks pass.

The existing full-basin probe also passes: maximum mesh angular edge12.543m, protected route/shore clearance35m, inlet/pole/tunnel regressions unchanged. The actual-rock/core contact probe passes for all nine waterfall feeds. Python syntax and whitespace checks pass. These checks do not establish Unreal compile success, runtime cost or visual acceptance.

Shared extraction helpers updated: `Tools/probe_maelstrom_geometry.py` and `Tools/probe_waterfall_contacts.py` conditionally include the new packet namespace. Older `--baseline` commits remain supported. The contact change only adds the namespace after route extraction; preserve the other worker's separate `SampleAttachedWaterfallPoint` selection when merging.

Root must build the source targets and render matching current-bank/current-wide views, then inspect motion and neighbouring times. No material regeneration is required for this candidate. Verify that the formerly smooth bank now has readable short geometric crests without a noisy mountain silhouette, the route remains unchanged and the added detail is stable in motion. No Unreal build, engine run or GPU work was performed by the worker.
