# Cliff algorithms and the rejected 3D outcrop — 15 November

The user's question is whether an existing mountain-generation algorithm can generate realistic terrain and textures, with the result adapted to our crater. **Yes.** Use a geological terrain workflow for the broad landforms, then detailed three-dimensional cliff surfaces. This checkpoint establishes neither a finished cliff nor a live arena replacement.

## Recommended workflow

1. Generate connected ridges and catchments with ridged multifractal/heterogeneous terrain. Fit the crater envelope, actual shore, route clearance and specific ruin supports before the final erosion pass. Avoid wrapping an already finished rectangular landscape around the rim: that can stretch physical features and invalidate its drainage.
2. Use hydraulic erosion for channels and sediment transport, and thermal erosion for weathered slopes and talus. Vary rock hardness and precipitation spatially. Preserve meaningful drainage outputs for materials and waterfall placement. Our current bounded erosion is subtle; it is not evidence of mature natural gullies.
3. Convert selected steep faces to locally refined meshes. A heightfield stores only one height per horizontal location, so projecting overhangs back into it destroys them. Use genuine 3D geometry for ledges and undercuts, and a shared original-triangle surface for visible rock, waterfall contacts and support queries. Bake detailed static sectors for Nanite after the shape is accepted.
4. Give the rock intermediate-scale surface relief as well as the broad mass and fine grain. Use measured-size displacement and tileable PBR stone where available; blend mineral variation, sediment, exposed rock and narrow runoff with the derived masks. Noise-only colour and centimetre detail on an otherwise smooth hundred-metre face will still look like clay. Erosion history is not automatically current wetness.
5. Review a representative sector from both nearby deck distance and across the crater. Far-wall rock features must stay physically small; temple/statue silhouettes and long uninterrupted drops supply scale. Accept the shape and material together before multiplying sectors, rebuilding the falls or resuming the boss.

Primary references checked this pass:

- [SideFX Project Pegasus: cliff generation](https://www.sidefx.com/tutorials/project-pegasus-cliff-generation/) extracts cliff sections from a heightfield, subdivides/displaces them, tiles the result and imports Nanite geometry into Unreal. It also documents the memory cost of excessive subdivision/import batches. This is a useful established production workflow; no Houdini installation or licence is assumed here.
- [SideFX HeightField Erode](https://www.sidefx.com/docs/houdini/nodes/sop/heightfield_erode.html) documents hydraulic/thermal controls, physical erosion feature size and height/sediment/debris/flow layers. Its implementation is a reference, not a claim that our small offline solver matches its output.
- [SideFX terrain texture layers](https://www.sidefx.com/docs/houdini/heightfields/texturelayers.html) describes deriving shading layers from terrain outputs.

The existing Blender ridged/heterogeneous generator and offline erosion provide the first two stages. Their current broad faces still fail visually. Do not describe the presence of these algorithms as a quality result.

## Actual work and rejected images

The first 72 x 40 m outcrop used finite joint cuts in a closed volume. Independent rays confirm genuine multiple surface intervals, but actual wall/oblique images show straight saw slots through the same smooth slab. Rejected.

One bounded revision replaced slots with unequal convex wedge detachments. This produces visible planar faces and undercut shadows, but repeated polygon pockets, pristine planes and similarly sized recessions read as a quarry/blockout. Also rejected for live integration. The protected crop border and artificial backing are diagnostic boundaries, not finished joins. See [generation](FRACTURED-CLIFF-OUTCROP.md), [matched clay critique](CLIFF-OUTCROP-REVIEW-PASS9.md) and [original-surface query evidence](CLIFF-TRIANGLE-SURFACE.md).

Do not repeat these large cutter footprints around the crater or hide them under material noise. The next shape experiment should follow the terrain-to-detailed-cliff workflow above, with coherent rock structure and intermediate-scale weathering across the actual exposed faces. Existing texture names under `AncientRuins/Textures/Bake/Cliffs` are mesh bakes; their mere presence does not establish that they are tileable displacement sources. No additional asset is required to continue the shape work, but a suitable tileable rock scan could improve surface realism more efficiently than more generic shader noise.

## Native rendering check

Root ran the existing isolated 500 m tile with `r.Nanite.Visualize Triangles` at native 1600 x 900, without changing persistent render settings. `Saved/SceneReview/cliff9-nanite-triangles/shot_0300.png` shows dense Nanite triangles covering the actual terrain. The soft appearance is not explained by rendering only the coarse fallback mesh. This does not measure exact active triangle count, motion quality or long-run performance. The existing non-Nanite VSM marking-overflow warning remains elsewhere in the scene.

No new preview FBX was imported because the outcrop failed its shape review. Normal gameplay geometry, water/falls, route, clouds/lighting, early-game and boss are unchanged. The new arbitrary-triangle BVH is a standalone utility, not wired into live contacts. Its MSVC CPU tests and independent comparisons pass; no new Unreal build is needed for an unused header and offline tools. Full integration still needs accepted rock, exact seams, reproducible supported landmarks and contacts/falls regenerated from the same original surface.
