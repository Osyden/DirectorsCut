"""Rebuild only M_MaelstromWater, preserving independently edited water effects.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>. The canonical
builder's water section owns its shared helpers. Execute exactly that section;
do not regenerate cascade, spray, feed, whitewater, or impact-mist materials.
"""
from pathlib import Path

source = Path(__file__).with_name('make_maelstrom_materials.py')
text = source.read_text(encoding='utf-8')
marker = '\n# The steep lip is a separate masked sheet.'
if text.count(marker) != 1:
    raise RuntimeError('Water-only boundary changed; review builder before import')
water_source = text.split(marker, 1)[0]
if 'save_loaded_asset(water' not in water_source:
    raise RuntimeError('Missing water save before material boundary')
exec(compile(water_source, str(source), 'exec'), {'__file__': str(source), '__name__': '__main__'})
