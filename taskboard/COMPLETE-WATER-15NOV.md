# Complete-scene water pass

Owner: Codex, `codex/complete-water`, from `34b471a`. Root owns Unreal build, material import, and rendered acceptance. No UAssets, sky, cliff geometry, or boss files were edited here.

The actual `WhirlpoolBasinWide/shot_0300.png` and `WhirlpoolShipWide/shot_0315.png` show continuous but overly even hydraulic banks, metallic distant highlights, and scarce visible whitewater. `HeightAt` now varies the outer transition width in finite spatial reaches, with a smooth bounded 15 m correction. The complete +/-125 m route corridor and shoreline elevations remain protected. Additional breakers are excluded by sampled full footprints from the route corridor and the shoreline receiving band. Route timing, steering, basin radius/depth, and `DCMaelstromRoute.h` are unchanged.

The basin's former 1.2 m center opening is closed by a shared-vertex triangle fan. Its duplicated angular seam now evaluates at exactly zero rather than floating-point 2PI. The indexed water grid has no separate ribbon meshes or missing interior quads.

The material increases source-driven occupied foam area from .30 to .44, strengthens renewal from the actual animated breaking lips from .18 to .40, and raises unresolved distant liquid roughness from .30 toward .40. It preserves physical wave scale, source masks, explicit phase advection, and existing footprint antialiasing.

Run `Tools/make_maelstrom_water_only.py` in root's Unreal commandlet. It executes only the canonical builder's water section; it does not regenerate independently modified cascade, spray, whitewater, or impact-mist assets. The wrapper fails if its explicit source boundary changes.

Standalone CPU checks passed: existing bank packet regression, shared FFT/displacement-normal consistency regression, full-grid geometry/flow regression, material Python syntax, and `git diff --check`. The new source produced 230 finite breakers, minimum route/shore clearance 35 m, and maximum angular mesh edge 12.54 m. Before the exact seam-column correction, round-off separation was only .0202 cm XY/.0039 cm Z. The unchanged advected flow reports up to20.39x angular strain at1000 m radius; this can still stretch shading and is not a topology crack. Native visual quality and shader compilation remain to be checked by root.

## CompleteArenaWide1 correction

The actual full-scene render exposed a dark continuous inner bank. This was the height profile, not a mesh crack: the former flat 35 m shoulder returned to the bowl over 95 m, amplifying its radial grade. The inner blend now decays exponentially over 42 m (scaled with the route), with no terminal lip. Root explicitly authorized changed off-center heights; the ship already follows `HeightAt` at its actual steering offset. The previous full-corridor height invariant is superseded by this correction. Actual source comparison against the preceding water commit measured exact zero change at 721 route center samples and 6,144 shoreline samples, maximum inner radial grade reduced from 1.8964 to 1.0456, and a maximum off-center reduction of 34.577 m. Falls must be generated from this final source, as coordinated with the falls worker.

Both fallback and live FFT liquid paths now use a 0.30 near / 0.42 distant roughness floor. The live path previously overrode the fallback with 0.18 near. Source raft area budget increases to .68; actual finite animated breaking lips also contribute torn fresh fronts directly, avoiding a second occupancy lottery that almost erased their narrow support. There is still no unconditional slope foam or radial foam contour. The procedural normals, displacement, physical wave scales, and route timing remain unchanged.

Full-grid CPU geometry/flow, shared displacement/normal consistency, Python syntax, inner-bank source comparison, and diff whitespace checks pass. Root owns native shader compilation and visual acceptance; no Unreal launch or asset changes were made in this worktree.

## Wide2 material correction

The next actual wide view still showed a dark reflective spiral. The water had no remaining topology hole, and legacy cliff components did not enable hidden shadows. Its shading used the ship's abrupt terminal turn as an angular velocity field across the entire basin. Replacing that with a smooth inward vortex reduces sampled 1 km angular UV strain from 20.389x to 1.092x; the core stays at 1.222x and the separate tunnel mapping is unchanged. Geometry and waterfall contacts are unchanged. Wave amplitudes and the shared displacement/normal derivation are retained.

Clear-water body color and volume scattering are increased relative to absorption so non-reflecting faces retain blue-green body. Water review mode now forces roughness 1, specular 0, opacity 1, up normal, black albedo, and zero water-volume coefficients, leaving its emission diagnostic readable. Modes 1–4 retain their meanings; 5 shows the stationary RGB source atlas, 6 vertex crest red, and 7 the undisplaced geometric normal. Native results remain pending. Geometry/flow and shared wave-normal CPU probes, Python syntax, and whitespace checks pass.
