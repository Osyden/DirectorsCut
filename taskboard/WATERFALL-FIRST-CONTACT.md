# First covered receiving-water contact

Owner: Codex cliff worker, `codex/crater-first-water-contact`, isolated `Crater-Workers/Cliff-Contact`, based on `d04c743`. Scope: surface-water core/contact API/tests; no cliff, route, sky or shader changes. Root owns engine builds/export/capture; the waterfall worker owns the exporter and generated volume.

The source/API checkpoint trims each stream at its first covered receiving-water contact and removes the final upward blend. `SampleReceivingWater` rejects points outside `120cm <= radius <= WaterEdgeRadius(angle)` before evaluating mean `HeightAt(..., false)`. `SampleWaterfallCorePoint` samples rendered cliff triangles and the shared clearance offset without a terminal blend. `FWaterFeed.ContactV` and `ContactPoint` describe the first central entry; `Foot` is the mean-water surface at that same XY and is shared by impact, foam trails and atmosphere queries.

Important limit: a cliff path can enter coverage from the side already below mean water. The raw endpoint is never lifted to the surface foot. `DC_WATERFALL_RECEIVING_CONTACT` reports the actual depth of that gap so it cannot be mistaken for a validated physical splash. Root should measure source 8 after the API build before deciding whether a small shoreline-contact-strip correction is required. No broad cliff-art changes are included here.
