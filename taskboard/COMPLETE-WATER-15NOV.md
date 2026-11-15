# Complete-scene water pass

Owner: Codex, `codex/complete-water`, from `34b471a`. Root owns Unreal build, material import, and rendered acceptance. No UAssets, sky, cliff geometry, or boss files were edited here.

The actual `WhirlpoolBasinWide/shot_0300.png` and `WhirlpoolShipWide/shot_0315.png` show continuous but overly even hydraulic banks, metallic distant highlights, and scarce visible whitewater. `HeightAt` now varies the outer transition width in finite spatial reaches, with a smooth bounded 15 m correction. The complete +/-125 m route corridor and shoreline elevations remain protected. Additional breakers are excluded by sampled full footprints from the route corridor and the shoreline receiving band. Route timing, steering, basin radius/depth, and `DCMaelstromRoute.h` are unchanged.

The basin's former 1.2 m center opening is closed by a shared-vertex triangle fan. Its duplicated angular seam now evaluates at exactly zero rather than floating-point 2PI. The indexed water grid has no separate ribbon meshes or missing interior quads.

The material increases source-driven occupied foam area from .30 to .44, strengthens renewal from the actual animated breaking lips from .18 to .40, and raises unresolved distant liquid roughness from .30 toward .40. It preserves physical wave scale, source masks, explicit phase advection, and existing footprint antialiasing.

Run `Tools/make_maelstrom_water_only.py` in root's Unreal commandlet. It executes only the canonical builder's water section; it does not regenerate independently modified cascade, spray, whitewater, or impact-mist assets. The wrapper fails if its explicit source boundary changes.

Standalone CPU checks passed: existing bank packet regression, shared FFT/displacement-normal consistency regression, full-grid geometry/flow regression, material Python syntax, and `git diff --check`. The new source produced 230 finite breakers, minimum route/shore clearance 35 m, and maximum angular mesh edge 12.54 m. Before the exact seam-column correction, round-off separation was only .0202 cm XY/.0039 cm Z. The unchanged advected flow reports up to20.39x angular strain at1000 m radius; this can still stretch shading and is not a topology crack. Native visual quality and shader compilation remain to be checked by root.
