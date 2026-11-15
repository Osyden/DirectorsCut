# Cliff erosion material candidate

Owner: Codex material worker, `codex/crater-bank-review4`, isolated `Crater-Workers/Flow-Seams`. Source-only replacement in `Tools/make_maelstrom_rock.py`; root owns regeneration and visual acceptance.

The actual isolated close capture removes the masonry grid but reveals a smooth wall. Compared `cliff4-material-isolated-close/shot_0300.png` with `waterfall4-combined/shot_0300.png`: the old22m normal layer manufactures the rectangular faces, independently of the corrected geology. Its3.5m fake relief and cell-wide colour/AO darkening are removed.

Replacement physical scales:

| Component | Horizontal/surface scale | Height range or depth |
| --- | --- | --- |
| Directional erosion |0.8m across by4m along vertical projections |12cm full height range |
| Irregular mineral relief |3.2m, rotated coordinates |10cm full height range |
| Fine chips |32cm |1.6cm full height range |
| Sparse finite cracks |1.8–5m long,16–44cm wide; curved, randomly placed and leaning |2.5–7cm deep |

These layers use analytic gradients. Physical pixel footprints filter each layer separately; tiny cracks disappear before turning into unresolved sparkle. The lattice only places independent finite cracks and gives its cells no faces, height, colour or AO. Integer hash mixing avoids the previous trigonometric cell hashes. Crack colour darkening is at most6% inside resolved cuts; AO now follows only the existing actual geometric recess channel. Fine1.6m and intermediate12m installed normal textures,4m colour sampling, weathering/wetness, roughness, geometry, water and sky remain unchanged.

`DC_ROCK_REVIEW_NO_FRACTURES=1` still isolates the entire procedural replacement, leaving installed textures and geometry. Unset or`0` generates this candidate. The older `CLIFF-MATERIAL-ISOLATION.md` describes the historical22m A/B; its process/restore instructions still apply, but the flag now disables the replacement layer.

Validation: Python AST and `git diff --check` pass. An800-case CPU finite-difference check of the authored tapered curved-groove height/gradient agrees to2.69e-10, including variable length/width/depth/lean. No Unreal, material generation or GPU ran here. Regenerate only the cliff generator and compare the same close and wide captures; acceptance remains pending. This replaces false material detail and cannot fix the remaining smooth large cliff geometry by itself.
