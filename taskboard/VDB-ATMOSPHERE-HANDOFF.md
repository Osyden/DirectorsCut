# VDB atmosphere direction — owner via Codex, 13 November

## Owner feedback and availability

The game is much better, but the owner is still not happy with the fog. The goal remains sculpted, cinematic cloud/fog structure and spectacular reveals matching the supplied references. The owner acquired **Clouds Tunnel Vdb Effects Pack Loop by one.studio2** and sees its initial visuals as a match for the storm-front tunnel. Please investigate adopting its assets/techniques for other fog/smoke cases too.

**Not installed by Codex.** The Windows desktop was locked; no pack was found in the project, checked downloads or checked Epic cache. The owner explicitly replied: **"I'll install it when home."** Do not wait on installation to continue independent work. Do not mistake this for the separately sold static Clouds Tunnel Vdb Pack.

Correct product: [Clouds Tunnel Vdb Effects Pack Loop](https://www.fab.com/listings/7f0d28f5-aff1-400a-8468-391b96652902). Listing states ten tunnel VDBs with density, colour, emissive and speed control, UE 5.5+. Its preview is linked on that page. Actual assets, dependencies, file size, loop quality and packaged performance have not been inspected.

## Requested experiment once installed

1. Inspect the included demonstration and actual Blueprint/material/Sparse Volume Texture setup. Identify the content root and keep pack assets out of this public repository before staging any files. Record dependencies and how to install locally.
2. Pick one tunnel asset and match the entrance/reference composition from the playable helm. Aim for a massive curling front with an open navigable centre, readable billows and strong depth, rather than a distant tower or an even pale sheet. Preserve the calm-to-storm progression and clean handoff to the slide/maelstrom.
3. Use the pack as a technical reference for the fog section: assess detailed foreground/side cloud banks, ragged curtains in front of ruins, and directional low mist. Test changed orientation, scale, density, material parameters and carefully limited layering; a tunnel-shaped cache does not automatically become arbitrary ground fog. Request a differently shaped pack only after identifying the missing shape/motion.
4. Compare one Heterogeneous Volume hero asset against an SVT-based volume material in the existing fog renderer. Keep ordinary background haze/sky where it works. Avoid making the entire city another uniform white-out. Desired shot: dark monumental silhouettes emerging from luminous, textured, rolling fog, warm lantern foreground and lightning revealing depth.
5. Evaluate actual normal-speed gameplay with the existing sky, rain, wet deck, water, lightning and DLSS. Compare frame time with the effect off/on, streaming warmup/mip stability, multiple overlapping volumes, interior traversal, material response and a packaged build. Record the number of visible volumes and the chosen quality settings. A path-traced store preview is not evidence of real-time appearance.
6. Send the owner a short before/after capture set and recommend a specific additional pack only if the free tunnel demonstrably leaves a gap. Resume the other authorised tasks while waiting for the owner to install it.

Epic's [Sparse Volume Texture documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/sparse-volume-textures-in-unreal-engine) confirms animated SVTs can loop in Heterogeneous Volume actors, and SVT volume materials can also feed volumetric fog. It notes that the latter's grid can lose fine detail, and animated streaming may fall back to coarse mips. Profile and tune the actual renderer used by this game; no global path-tracing switch is needed to prove real-time gameplay.

## Other packs the owner is considering

These are provisional use-case judgments from listings, not local asset or preview-motion validation. No further purchase is requested yet.

- [Clouds Storm 3D VDB Pack loop](https://www.fab.com/listings/70c4180a-d96d-4b7f-96fa-b3befe0ceccf): listing advertises ten looping storm volumes with density, colour and speed controls. Most directly relevant next candidate if the free tunnel cannot supply the sculpted storm ceiling/large rolling banks. The listing's path-tracer command is for path tracing, not a requirement or quality guarantee for gameplay.
- [Whitewater Waterfall Vdb Pack Loop](https://www.fab.com/listings/1f51a29a-67ed-4bc0-9cf6-1bf356f8d0ce): ten looping waterfall effects with density, colour, emissive and speed controls, UE 5.5+. Candidate for selected maelstrom fall feet, cliff overflow and whitewater spray. Treat as local detail layered over the existing water surface, not proof it can cover the whole 3.2 km bowl, conform to its slope, simulate ship-water contact or supply gameplay collision. Codex resumes maelstrom visuals after reset; coordinate ownership before editing that stage.
- **Snow Mountain Storm VDB Pack by one.studio2:** the exact listing could not be verified through web search. Do not invent its contents or recommend a purchase based on the title. Need the precise product URL or local library entry before comparison. If its main feature is blowing snow rather than rolling vapour, it may not solve the maritime fog problem.

Desired feedback: which volume shapes/motion are missing after testing the free tunnel, whether that gap can be solved by the existing pack/material setup, and measured cost of the selected hero effect. More packs alone do not establish cinematic quality.


## Claude, 13 Nov 14:31: groundwork done before the pack is installed, and one finding

Read and understood: the target is sculpted volume, depth and lightning-lit detail, not thicker fog. The pack is not installed, so nothing below uses it.

**What exists in the game today (for comparison when the pack arrives).** The fog banks of the drowned city (`ADCFogBank`, `M_FogVolume`) are volume-domain materials drawn by the engine's volumetric fog grid: ellipsoids with drifting cloud noise, eaten at the edges, glowing with the sky and flaring with lightning. Their limits are the grid's: soft detail (the grid is a few hundred cells across the screen), and nothing beyond 700 m. That softness is most of what the owner is seeing when they say the fog is still below target. A sparse volume texture sampled in the same material would add authored shape but not sharpness: it goes through the same grid.

**So the pack's value depends on the other renderer**, the engine's heterogeneous volumes (ray-marched at the volume's own resolution, with its own shadows, any distance). I tried that renderer today without the pack, to have the plumbing ready: `-DCHeteroTest` puts one `UHeterogeneousVolumeComponent` 240 x 180 x 120 m beside the course with a volume material on it (`-DCHeteroPlain` uses a material that is nothing but constant density, `M_HeteroTest`; `-DCHeteroAt=`, `-DCHeteroDensity=`, `-DCHeteroRes=`).

**Finding: it does not draw.** The component registers, reports itself visible with the right bounds and material, and the material has its heterogeneous-volume usage flag, but nothing appears: with the sky pack or the game's old sky, with DLSS or without, at 90 m or 280 m, with density 0.05 or 5. The engine's settings that gate it read as on (`r.HeterogeneousVolumes` 1, `.Allowed` 1). I did not find the cause in the time I gave it. Two guesses, neither tested: the renderer may want a sparse volume texture on the material before it will draw anything (ours had none; the engine ships only an empty one to test with), or something in this project's renderer settings turns the pass off.

**Why this matters for the purchase:** if heterogeneous volumes do not draw in this project, a VDB pack can only be used through the fog grid, where it will look like better-shaped versions of what we have, not like its store preview. The first thing to do when the pack is installed is open its own demonstration map in this project's editor and see whether its volumes draw at all, before any work on placing them. If they do, the cause above is the missing texture and the plumbing here is ready; if they do not, that is a project setting to find before anything else. I would not buy the further packs until that is known.

**Ready for the pack:** the treadmill can carry any actor past the ship (`Register`), so a tunnel or bank from the pack can be laid like the ruins; lightning flare and sky glow are available to any material as `Director->FogGlow` and `FogFlare`; the front's timing (`StormFrontX`, growth 6 to 44 s) is where a tunnel mouth would be hung.

**Later, 14:47.** Three more engine settings tried on the heterogeneous volume (preshading on, lighting cache off, debug mode): still nothing drawn. Left there until the pack's own demo map can be opened.

Meanwhile the existing banks were given what the fog grid cannot give them, modelling: density now climbs quickly from nothing to full so a billow has an edge, and each point's glow depends on whether it is open to the sky or in a fold under more cloud, and on its height in the bank. From the deck the banks now read as rolling forms with lit tops and dark hollows (review set 40) where they were an even glow. This is still the fog grid; it is the "before" picture for the pack.

**15:06: a tunnel built from the existing banks, tried and switched off.** Rings of five heavy banks (either hand, both shoulders, overhead) every 42 m through the front's interior, middle left open (`-DCTunnel`). With the front's own fog as thick as it is, nothing of them shows; with that fog thinned to a half or a quarter (`-DCEnv="FogDensity=0.018"`), they show as pale patches to the sides and the city appears too early. It never reads as a throat of cloud. **This is the specific gap for the pack:** a continuous curling tunnel wall with an open centre, seen from inside, is a shape the fog grid and separate ellipsoids do not give. The low rolling banks round the ruins, by contrast, the current system does acceptably.
