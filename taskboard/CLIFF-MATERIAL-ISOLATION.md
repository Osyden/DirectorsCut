# Isolate the 22m cliff fracture shading

Owner: Codex material worker, isolated `Crater-Workers/Flow-Seams`, branch `codex/crater-bank-review4`. This follow-on changes only `Tools/make_maelstrom_rock.py`; the preceding basin-normal candidate is separate.

Read `CLIFF-GEOLOGY-PASS4.md` and viewed root captures `crater4-wide/shot_0300.png`, `shot_0420.png` and `crater4-cliff-core/shot_0300.png`. The close wall still contains large planar blocks, while the corrected geometry removed much of the old projecting shelves. Material fracture shading remains a plausible contributor that needs isolation before any default change.

The opt-in generator environment variable `DC_ROCK_REVIEW_NO_FRACTURES=1` replaces only the procedural22m fracture result with zero. Its XYZ drives the fracture normal; W drives its paired22% colour darkening and14% AO reduction, so those three contributions are disabled together. Fine1.6m and intermediate12m installed texture normals,4m mineral colour, weathering, wetness, geometric vertex-recess shading, roughness, all geometry and all water materials remain unchanged. The default fracture HLSL is unchanged. Invalid values fail before material graph modification.

## Root A/B recipe

1. Regenerate only `Tools/make_maelstrom_rock.py` with the environment variable unset or `0`; capture the same close and wide cameras/times.
2. Set `DC_ROCK_REVIEW_NO_FRACTURES=1` in the Unreal process running that generator and regenerate that same script only; capture identical cameras/times. For a newly launched process, PowerShell `$env:DC_ROCK_REVIEW_NO_FRACTURES='1'` must precede its launch. Setting the parent environment cannot change an already running editor's environment: there set `os.environ['DC_ROCK_REVIEW_NO_FRACTURES']='1'` in its Python session before executing the script.
3. Restore the variable to `0` (or remove it) and **rerun the cliff generator** after the comparison. Clearing the environment alone does not restore the already saved diagnostic uasset. Do not package or commit that diagnostic asset as the default.

Check `DC_ROCK_REVIEW_NO_FRACTURES=...; saved material mode=...` in the generation log. If the large facets disappear in B, revise the procedural fracture layer in a later bounded pass. If they persist, investigate the actual geometry and remaining normal contributions; this test does not disable those.

Validation: Python AST and whitespace checks; the default fracture expression is compared against the pre-change AST. No Unreal, material generation, GPU or visual acceptance ran in the worker.
