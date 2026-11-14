# Waterfall water-contact pass 3

Owner: Codex waterfall worker on `codex/crater-volume-water-contact`, in `Crater-Workers/Fitted-Waterfall`. Owns waterfall runtime/export, VDB generator/importer and this handoff. The cliff contact worker owns shared core/contact APIs and `FWaterFeed`; root owns integration, builds, Unreal runs and screenshots. Remains opt-in with stable source IDs.

Exporter now uses `SampleWaterfallCorePoint(Angle,V,Across)` over `.84 .. Feed.ContactV`, stopping at the first valid receiving-water contact. It removes the old final-six-percent upward blend. It records the actual core contact separately from the mean-water impact Foot. Receiving-water data includes a 49x49 height grid at 2 m spacing, explicit coverage and signed shoreline clearance. Heights outside the basin no longer imply water exists there.

Generator updates and actual regenerated candidate are pending the real updated export. Do not regenerate against the previous pass's profile, which included the underwater dip/upward curl.
