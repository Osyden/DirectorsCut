# Integrated sky, scale and foam pass

Branch: `codex/maelstrom-saturday`, isolated project `E:/Projects/DirectorsCut/Maelstrom-Saturday`. Sky branch through `e32cd85` is merged as `ee156ec`; conflicts retained the newer water geometry and shoreline. No main merge or packaged build is implied.

## Direction and asset decision

The reference needs kilometre-scale composition supported by physically small detail. Uniform giant falls, rounded rocks and isolated white crescents defeat that scale even when the crater dimensions are correct. Keep the 3.2km rim and 900m depth, but separate the systems and their detail budgets:

- **Geology:** connected cliffs with coherent buttresses, ledges and talus. Small embedded outcrop clusters supply secondary scale; temples and statues remain distant size references.
- **Circulation:** the existing descending spiral is the large water shape. Breaking wave fronts and crossing chop supply shorter scales, with local displacement protected around the hull.
- **Foam transport:** retain foam behind crests and waterfall impacts, broaden it downstream, and break it into rafts/filaments. Source coverage must survive texture sampling and distant screen footprints.
- **Falls:** sparse, unequal, grouped channels, falling cores, detached spray and impact whitewater. Airborne water and receiving-water foam use the same generated contacts.
- **Atmosphere:** the integrated volumetric sky owns cloud/light settings; local receiving-water mist now uses actual waterfall feet rather than the old upper-rim profile.

No new asset purchase is required for this implementation. Existing rock assets and optional installed DRW detail are sufficient to test the corrected geometry and flow. If bespoke falling-water spray still fails after its motion/shading pass, a physically simulated whitewater flipbook or small baked volume could replace that specific layer. A generic ocean or cloud pack would not fix layout, metre scale or foam transport. Do not buy another pack without identifying the missing layer and checking its content.

## Implemented

- Full final sky branch integrated, including volumetric cloud material and stage-scoped lighting/restoration.
- Nine irregular falls, 11–44m full width, replace twenty evenly distributed broad falls. Three finer cores use actual falling-distance UVs.
- Coherent cliff relief and deeply embedded 8–45m outcrop clusters replace isolated 50–160m boulders. Original rock materials retained on scaled instances; substrate wetness/local coordinates cannot be copied blindly to static meshes.
- Procedural cliff self-shadowing enabled for combined review; rock material adds an intermediate 18m normal frequency alongside 5m detail.
- Foam source rasterization filters sub-texel crests instead of missing metre-wide lips in roughly 5m texels. Separate channels store fresh foam, waterfall trails and downstream crest wakes. The shader preserves mean bubble coverage at distance and combines rafts/filaments without multiplying most coverage away.
- Waterfall mist uses the actual generated contact list; height reduced from 90–220m to 15–55m, with smaller low mist banks. It no longer estimates falls from an unrelated overflow mask.
- Volumetric distance now has one restoration owner. The main stage no longer modifies distance before the atmosphere captures entry state; this fixes a direct-seek integration hazard.
- The inner channel shoulder now blends back into the descending bowl over 35–130m rather than 100–240m from the route centre. The centreline, outer bank and hull protection remain unchanged. A deeper camera-look experiment was rejected because it mostly enlarged foreground water; original framing is retained and the occluding geometry is addressed instead.

## Verification

Editor and Game Development targets pass. Both original material generators pass. The first combined review rejected repeated vertical cliff flutes; the final terrain uses interrupted benches and lower-amplitude fractures instead.

Continuous native 1920x1080 environment-only run `integrated-foam-b` captures 282/300/340/380/420/451s, exits at 450.01, and restores open sea. Logs confirm fog distance 70000, extinction 2.5, phase .20; skylight scattering 1.00/cloud AO off; 29 UDS properties and shaft blur .10 restored. Minimum calculated full-steer shoreline clearance remains 35m. This continuous run precedes the final inner-shoulder narrowing; sky, terrain, foam shader and mist are final versions.

GPU samples in that run were 12.8, 9.9, 8.5, 11.4 and 14.8ms from reveal through 420s. These are sparse environment-only samples on this workstation, not a boss-fight, target-machine or worst-case performance guarantee. A Game build ran concurrently during early frames. Existing startup VSM overflow and experimental Toolset Python warnings remain.

`integrated-final-seek` passed backward seek/re-entry, including direct entries at 282/300 and exits at150/451. Both exits restore fog distance70000 and the sky's29 properties. Its stronger downward camera was rejected visually. Final inner-bank review and captures follow below.

Final `integrated-inner-bank` native review passes requested 282/300/340/420/150/310/451 (settled actual times288/305/345/425/155/315/456). Both exits restore fog distance70000, extinction2.5, phase.20, 29 UDS properties and shaft blur.10. No maelstrom shader failure, fatal error or assertion was reported by the review helper. Editor and Game were rebuilt after the final bank change. Final narrower-bank captures expose more of the inner descent, but do not yet deliver the reference's depth composition; review that in motion next. These are offscreen renders/log checks, not a manual playthrough or a packaged release.

Actual unretouched frames:

- [Final combined scene at305s](review/maelstrom-integrated-14nov/final_0300.png).
- [Final mid-descent at345s](review/maelstrom-integrated-14nov/final_0340.png).
- [Final deep descent at425s](review/maelstrom-integrated-14nov/final_0420.png).
- [Final backward-seek re-entry at315s](review/maelstrom-integrated-14nov/final_0310.png), [restored open sea at456s](review/maelstrom-integrated-14nov/final_0451.png).
- Earlier continuous proof (before narrowing inner bank): [340s](review/maelstrom-integrated-14nov/continuous_0340.png), [420s](review/maelstrom-integrated-14nov/continuous_0420.png), [451s](review/maelstrom-integrated-14nov/continuous_0451.png).

Reproduce with `Tools/review_maelstrom_sky.ps1 -EnvironmentOnly -Clean -Native -Width 1920 -Height 1080 -Wait -ReviewName integrated-inner-bank -Shots '282,300,340,420,150,310,451'`. The checked-in generated sky/water/rock materials are included; normal installed-pack setup requirements still apply.

## Remaining visual work

This is an integration and structural progress checkpoint, not Crysis/reference-quality acceptance. The combined lighting exposes problems the isolated sky and water views hid:

1. Near water now has visible foam coverage, but the broad banks still look too smooth. Breaker packets need stronger geometric crest turnover, with source-aligned spray and local collisions/eddies; more uniform white coverage is not the remedy.
2. Terrain still needs convincing fractured surfaces and varied silhouettes rather than soft procedural shoulders. Self-shadowing helps volume but the backlit faces remain dark. Improve geometry/material structure before compensating with extra lights.
3. Narrow falls recover distance scale, but still look like thin strands. Give them multiple broken cores and finer aerated outer spray; promote selected impact plumes without enlarging every waterfall. A specific baked whitewater simulation could help this layer if available.
4. Dark shoreline bands and water/rock contact need closer motion review. Large animated displacements must stay embedded at the toe without a visible cutout seam.
5. Validate the reveal in motion from the actual helm, and keep both depth and sky visible. The small distant landmarks and tiny fall details must communicate size before the boss enters.
