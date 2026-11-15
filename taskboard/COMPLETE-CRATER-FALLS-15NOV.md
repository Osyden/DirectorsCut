# Complete crater waterfall contacts

Branch: `codex/complete-falls`. Root owns Unreal import, C++ build and visual acceptance.

The immutable final bundle is `Crater-Workers/Complete-Falls/Saved/CompleteFalls/Final`.
It uses `Cliff-Contact/Saved/CompleteCliffRing/FirstJoined/candidate.npz`, SHA256
`7af36bbdd621ef5dcf1e220cf8419703295609d51978a4ce01891c3360cd239c`.
Receiver sampling uses the final `codex/complete-water` source at `d9425b2`, including
the actual breaker population and production `HeightAt`. No pilot surface or VDB
is rebound to changed geometry.

Six unequal feeds occupy 18, 52, 215, 248, 323 and 342 degrees, with widths 15–58m.
Their 489–511m descents use **actual original cliff triangles**, with a 0.28m
vertical coating offset. This avoids the substantial ledge bridging found by
the independent audit of the initial resampled ribbon attempt. The initial
`First` bundle is rejected; use only `Final`.

`audit_complete_cliff_waterfalls.py` proves all 32,971 supported runoff triangles
equal their corresponding source triangle vertices plus the recorded offset,
with zero error. Thus their interiors retain the same barycentric surface.
Receiving foam is sampled against the fresh production water mesh; independent
triangle-interior clearance is 0.173–0.190m. Thirty receiving contacts emit
broken current-aligned foam and small scene-lit spray arcs. Spray is a mesh
proxy with translucent moving gaps, not volumetric fluid simulation.

Import using the existing `Tools/import_cliff_preview_fall.py` with
`DC_CLIFF_FALL_DIR` pointing to `Final`, after the matching cliff FBX has been
imported and audited. Explicit `volume_mode=fitted_mesh_spray` skips the native
SVT requirement without changing old VDB imports. The importer writes the usual
`Saved/CliffPreviewWaterfall.json`.

Runtime uses root-owned `bCompleteCliffArena` and `CliffArenaDataDirectory` members.
Complete mode enables falls without authoring flags. The installed
`CliffLandformPreviewImport.json` must contain `source_candidate_sha256` matching
the above hash. Preview mode still checks its existing export sidecar, and
still requires `-DCCliffPreviewFall`. Root must copy the waterfall manifest into
the complete arena runtime directory and include its generated mesh/materials.

Remaining validation: Unreal import/material compilation, build and actual
whole-arena/ship-view capture. Static CPU fitting does not claim material-WPO
contact or final visual acceptance.
