# Physical implicit bedrock review, 15 November

Branch `codex/implicit-cliff-physical-review`, based on `b43ba15`.
Scope: reproducible CPU clay renderer and actual visual review. No upstream
generator edits, Unreal runs/imports, source-mesh export or gameplay changes.

## Current judgement

The corrected metre-scale, minimally rounded, zero-image-warp blocks provide
clear unequal planes and real projecting undersides. This is a useful geometric
improvement over both the soft displaced terrain and the pitted upstream
examples. It is **not yet convincing natural cliff geometry**.

The free polyhedral sample reads as oversized triangular slabs separated by
wide cracks. The connected shallow version retains that pattern on a visible
rectangular backing: pristine flat faces and almost uniformly dark gaps make it
look like triangular plaques or a constructed wall panel. The regular border
must disappear through actual embedding/blending into bedrock, rather than
being hidden only in the camera framing. The large exposed gaps also need a
better hierarchy. Repeating this panel across the crater would amplify the
problem.

Low oblique views establish actual underfaces. The identical-geometry flat
diagnostic retains the same appearance and exposes marching-cube stair steps on
some edges. Imported normals soften those extraction artifacts but do not
create or solve the large shape. The fixed deck-distance perspective also shows
the triangular pattern; the camera does not zoom to fill the screen with the
30 m proof. Final arena/context acceptance remains with the main integration
chat, after its cropped-slope placement review.

## Actual inputs and outputs

Both input meshes are explicitly authored in metres, Z-up, with exposed front
toward +Y. The renderer uses scale 1 and a rigid 180-degree rotation around Z to
put that face toward its camera at local -Y. The authoring translation from the
first sample is reused for the connected version. This is not inferred from
the upstream grid-coordinate OBJ samples.

Inputs under `Flow-Seams/Saved/ImplicitFaceProof/`:

- `Polyhedral/Geometry/sample.obj`: actual bounds about29.96×29.94×29.95m,
  341,532 vertices and683,106 triangles; accompanied by generator metadata.
- `ConnectedShallow/Geometry/sample.obj`: about30.30×8.98×30.30m,
  191,144 vertices and382,284 triangles. This supersedes the earlier full-depth
  connected candidate, which was not reviewed here.

Images and full hash/camera/count records are in this worker's ignored
`Saved/ImplicitCliffReview/{Polyhedral,ConnectedShallow}/`. Each contains close,
low-oblique, flat-oblique diagnostic and deck perspectives, plus `review.json`.
All generated third-party geometry/rendered bulk stays ignored.

The figure is a simple measured 1.8 m scale marker, not a game character or an
additional asset. The deck review assumes an8m eye height,35mm focal length and
36mm horizontal sensor. Its initial position is250m from the first sample's
front; preserving that exact camera puts it262m from the shallower connected
face. The latter distance is measured and labelled in its image/report. These
are review assumptions, not a measured current gameplay camera.

Rendering uses Cycles CPU, eight threads,24 samples, the same rough neutral clay
and raking sun. No texture, renderer displacement, fog, atmospheric perspective,
geometry smoothing, rescaling-to-fit, or decimation is applied. Only the camera
framing is fitted for the close views; the perspective deck lens and distance
stay physical.

## Reproduce

`Tools/review_implicit_cliff.py` is new original review tooling. It requires an
explicit metres-per-unit factor and up axis. The optional saved camera manifest
also fixes translation and scale-marker placement. The low-oblique flat
diagnostic reconstructs an exact mesh copy without custom normals and verifies
positions and loop indices are unchanged before rendering. This is a renderer
guard, not an additional topology or geology certification.

Run with installed Blender in background mode, eight threads:

```text
--background --factory-startup --threads 8
--python Tools/review_implicit_cliff.py --
--obj ../Flow-Seams/Saved/ImplicitFaceProof/Polyhedral/Geometry/sample.obj
--metadata ../Flow-Seams/Saved/ImplicitFaceProof/Polyhedral/Geometry/metadata.json
--name "Physical polyhedral blocks" --metres-per-unit 1 --up-axis Z
--output Saved/ImplicitCliffReview/Polyhedral --samples 24
```

For the shallow sample, substitute its OBJ/metadata paths and output directory,
and add `--camera-manifest Saved/ImplicitCliffReview/Polyhedral/review.json`.
The generator's structural validity, mapping fixes and licensing are handled in
its separate source/audit notes. This review does not certify self-intersection,
cliff support, collision, Nanite quality, or performance in Unreal.
