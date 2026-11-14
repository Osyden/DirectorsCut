# Waterfall water-contact pass 3

Owner: Codex waterfall worker on `codex/crater-volume-water-contact`, in `Crater-Workers/Fitted-Waterfall`. Owns waterfall runtime/export, VDB generator/importer and this handoff. The cliff contact worker owns shared core/contact APIs and `FWaterFeed`; root owns integration, builds, Unreal runs and screenshots. Remains opt-in with stable source IDs.

Exporter now uses `SampleWaterfallCorePoint(Angle,V,Across)` over `.84 .. Feed.ContactV`, stopping at the first valid receiving-water contact. It removes the old final-six-percent upward blend. It records the actual core contact separately from the mean-water impact Foot. Receiving-water data includes a 49x49 height grid at 2 m spacing, explicit coverage and signed shoreline clearance. Heights outside the basin no longer imply water exists there.

Each of the nine lateral tracks also resolves its own first contact with `DCCliffGenerator::FirstWaterContact`; they do not all stop at the central `ContactV`. Tracks share the lower source parameter and independently reach their own raw endpoint without lifting it. The export records their individual parameters/endpoints and rejects a profile if any track never finds covered water.

The fitted generator now requires exported water coverage. It uses signed shoreline clearance for subcell classification when available and treats positions outside the height grid as unknown/not water. It finds each strand's first actual water entry, synchronizes impact emission to that time and XY, and stops impact droplets at their first water re-entry. A core endpoint below water is never lifted to the surface: surface spray is a separate response. A short ballistic continuation permits strands slightly above the mean sea at the sampled endpoint to land naturally.

The old last-fraction upward projection is removed. Airborne particles are clipped only where water actually exists. After raster reconstruction, density below covered mean water is also removed, so smoothing does not reintroduce submerged mist. The eight `1e-8` bounding anchors remain the only intentional exception; they preserve stable SVT bounds and are below visible extinction/8-bit import quantization.

In-memory verification passed before receiving the real export: original generic first-frame grids remain bit-identical; a synthetic coverage boundary correctly rejects phantom water and out-of-grid locations; all synthetic strands find water; fitted loops are exact; no positive-weight airborne particles remain submerged; reconstructed underwater density is zero apart from bounding anchors; endpoint continuation does not curl upward. No new VDB sequence was generated for these tests.

Actual regenerated candidate is pending the real updated export. Do not regenerate against the previous pass's profile, which included the underwater dip/upward curl. The new manifest records strand contact counts, unmatched source-mass fraction, coverage requirement and post-blur clipping.

Entry depth below the mean sea is measured separately (median/p95/max). A side entry can occur below the surface. The impact plume is a separate approximate surface response at that XY/time; it does not simulate bubble transit from a submerged entry and should not be called physically validated without reviewing the measured depth and scene geometry.
