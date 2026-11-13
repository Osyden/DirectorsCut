"""Adds a steady surface current to the Dynamic Real Water plugin's wave shader, so the sea streams past a ship that stands still.

Run with plain Python, editor closed:  Tools/pyx.sh Tools/patch_drw_current.py          (apply)
                                       Tools/pyx.sh Tools/patch_drw_current.py --undo   (restore the original)

The plugin's waves are fixed to the world. In this game the ship stays at the origin and the scenery
scrolls, so without this the water does not move past the hull. The patch shifts the whole wave
field along X in the plugin's spectrum compute shader (a translation is a phase shift per wave).

It only acts when the game asks for it: swell height 0 and a swell direction of exactly
(+-1, 0.0123) normalised, which no ordinary setup uses. Other projects on this engine are unaffected.
The original file is kept beside it as DRW_TimeSpectrum.usf.dc_backup. The plugin is purchased
content installed in the engine; nothing of it is copied into this repo.
"""
import glob
import os
import shutil
import sys

SPEED = 12.0  # metres a second

roots = glob.glob(r"E:\Utilities\UE_5.8\Engine\Plugins\Marketplace\DynamicR*")
if os.environ.get("DC_DRW_PLUGIN"):
    roots = [os.environ["DC_DRW_PLUGIN"]]
if not roots:
    print("Dynamic Real Water not found; set DC_DRW_PLUGIN to its folder")
    sys.exit(1)
path = os.path.join(roots[0], "Shaders", "Private", "Spectrum", "DRW_TimeSpectrum.usf")
backup = path + ".dc_backup"
MARK = "// Director's Cut current"

if "--undo" in sys.argv:
    if os.path.exists(backup):
        shutil.copyfile(backup, path)
        print("restored", path)
    else:
        print("no backup; nothing to undo")
    sys.exit(0)

src = open(path, encoding="utf-8").read()
if MARK in src:
    if os.path.exists(backup):
        src = open(backup, encoding="utf-8").read()   # re-apply from the original, so edits to this script take effect
    else:
        print("already patched and no backup to rebuild from")
        sys.exit(0)
else:
    shutil.copyfile(path, backup)

ANCHOR = "    float2 ConjugateTimeDependentSpectrumValue = float2(-TimeDependentSpectrumValue.y, TimeDependentSpectrumValue.x);"
if src.count(ANCHOR) != 1:
    print("shader layout not as expected; not patched")
    sys.exit(1)

PATCH = """    %s (Tools/patch_drw_current.py): a steady current along X. Shifting the whole
    // surface is a phase turn per wave; the shift is a whole number of tiles per time loop, so nothing jumps
    // when the plugin's clock wraps. Asked for with swell height 0 and swell direction (+-1, 0.0123).
    if (SwellHeight <= 0.0 && abs(abs(SwellDirection.y) - 0.0123) < 0.0006)
    {
        float TileMeters = PatchLength[SliceIndex].x;
        float Cycles = round(sign(SwellDirection.x) * %.1f * SpectrumLoopSeconds / TileMeters);
        float Along = frac(Cycles * (Time / SpectrumLoopSeconds));
        float Mode = round(WaveVector.x * (TileMeters / TWO_PI));
        float Shift = TWO_PI * frac(Mode * Along);
        TimeDependentSpectrumValue = ComplexMultiplication(TimeDependentSpectrumValue, float2(cos(Shift), sin(Shift)));
    }

""" % (MARK, SPEED)
open(path, "w", encoding="utf-8", newline="").write(src.replace(ANCHOR, PATCH + ANCHOR))
print("patched", path)
