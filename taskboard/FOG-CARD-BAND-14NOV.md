# Horizontal city-fog band — 14 November

Owner: delegated fog worker in the early-gameplay chat, `codex/early-gameplay`.

The reported 110-second port view shows a thin bright sheet between the central aqueduct and the ship's rigging, above the creature. The city lays 44 EasyFog detail cards flat over the sea, 140–380 m across. The generic scenery transform lifts each whole sheet by the trough height at its centre. From the low captain camera, these sheets expose a thin edge or their underside above the channel. The real volumetric grey banks are separate.

## Isolation

A normal-speed continuous run using the reported camera (`-DCShotCam=-2200,0,1200,5,-55`), natural helm and `-DCOrders=Hold_fire`, with only `-DCNoFogCards` added, removed the thin band at 110 seconds. Grey atmospheric layers and the obscured ruins remained. The old bow-spray implementation was still enabled in that diagnostic DLL, so removing city cards was sufficient for that view. Random scenery, contacts, ship roll and the updated creature spacing mean these are matching ride times and camera settings, not pixel-identical scene poses.

Earlier 4x forced-helm diagnostics did not reproduce the original pose and are not used as proof of the fix. Raw logs and captures are in `E:/projects/directorscut/early-gameplay-review/fog-band/`.

## Correction

- `DCFogCardFader.cpp/.h`: auxiliary cards smoothly lose density at grazing angles (roughly 8–20 degrees above their plane). Water-level cards also hide their underside. Face-on mist in elevated views remains. The active camera is used, including cannon and external views.
- `DCTreadmill.cpp`: attaches that helper only to EasyFog detail cards. Volumetric bank density, colour, noise, city reveal and portal visibility are unchanged. `-DCLegacyFogCards` restores the old card behavior for comparison.
- `DCWeatherFX.cpp`: the old bow-spray fog sheets are now diagnostic-only (`-DCLegacyBowSpray`), because the parent gameplay pass now owns bow spray in `DCWake`. This prevents duplicate systems and another source of edge-on sheets.

## Validation

The parent rebuilt the combined editor module successfully. A second normal-speed continuous run with the final fader enabled, no diagnostic fog suppression and the original low camera completed through 125 seconds with exit code 0. Reviewed 100, 110 and 125 seconds: the reported thin bright band is absent; grey volumetric layers remain. The elevated smoke check also completed with exit code 0 and retains mist viewed from above, without the exposed sheet edge. That elevated run seeks to 110 and allows six seconds to settle (actual capture at 116); it is a separate angle check, not a matched baseline comparison.

No fatal error, assertion, Blueprint Runtime Error or material-compilation-failure matches were found in either final capture log. Existing unattended asset/plugin startup warnings remain. This check does not establish a performance improvement or address the separate forward volumetric-light shaft documented in the city-fog handoff.

Evidence: [reported view](review/fog-card-band/before-110.png), [no-card isolation](review/fog-card-band/diagnostic-no-cards-110.png), [final 110-second view](review/fog-card-band/after-0110.png), [final 100-second view](review/fog-card-band/after-0100.png), [final 125-second view](review/fog-card-band/after-0125.png), [elevated smoke check](review/fog-card-band/after-elevated-110.png). The parent saved and pushed the source changes and screenshots in gameplay checkpoint `eff93a7` on `codex/early-gameplay`. The private play build includes the fix. Main and shared assets were untouched. All test processes belonging to this fix have exited.
