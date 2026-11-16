# City black-billow composition candidate

Owner cannons_helm, isolated codex/city-billow-composition fromac37b0c. Own DCFogBank.cpp only; tunnel_boss retains DCTreadmill banks/obstacles. Parent serializes all native/build jobs. Human playtest untouched.

Four landmark curtains force bHero=true even without DCHeroBanks (DCTreadmill landmark positions76000/150000/240000/262000), plus exit interior mist. These use independent heterogeneous rendering, dark albedo(.28,.33,.4)/(.3,.36,.44), zero city emissive fill and strong self-shadowing. Other city banks use M_FogVolume. Storm depth composite is already zero after80s, excluding it from110-170s city-plume causes. Prior global lighting/specular/local-exposure/AA tests did not establish an outline fix.

Candidate routes only city bHero descriptions through existing M_FogVolume and regular city albedo (including existing0.68city attenuation). Exact bounds, placement, drift, density, reveal range/falloff and weather fade code remain unchanged. Storm/front/noncity heterogeneous rendering untouched. No material asset mutation/import required. It removes that independent dark-plume layer while keeping fog at those locations. Source-supported candidate, not proof of the reported outline cause.

-DCLegacyCityBillows restores exact original city renderer/albedo for matched A/B. CITY_BILLOW logs seed/renderer/density/radii. Default logs fog-grid; legacy heterogeneous. Noncity behavior unchanged.

Checks: Spawn-only source diff; original Tick body identical. Renderer combinations checked so only city+hero changes and legacy restores all. No engine/build/GPU/import run here.

Root A/B: same continuous voyage/camera. Existing reproduction: -DCNoMenu -DCRunThrough -DCTimeScale=3 -DCSteer=1 -DCShipView -DCOrbit=180,8 -DCOrders=Hold_fire -DCLightningLog -DCTenths -DCAutoShots=110,150,165,167,169. Capture default and with-DCLegacyCityBillows at1600x900. Also ordinary forward helm110/150 if relevant. Compare dark towers, rigging edges, landmark concealment/reveal, water contact and storm handoff in calm adjacent frames. White-outline claim remains pending rendered evidence.
