"""Extract a source-authored displacement field using Blender's linear decoder.

Invoke Blender --background --factory-startup --disable-autoexec --python this
file -- --manifest <fetch_cliff_scan.py manifest>. Only material datablocks are
loaded from the source blend; no source scene or embedded script is executed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np
import bpy


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    manifest = json.loads(args.manifest.read_text(encoding='utf-8'))
    if manifest['schema'] != 'directorscut.cliff_scan.v1':
        raise ValueError('Unexpected source manifest')
    for record in manifest['files'].values():
        if hashlib.sha256(Path(record['path']).read_bytes()).hexdigest() != record['sha256']:
            raise ValueError('Source changed: ' + record['path'])
    blend = next(r['path'] for name, r in manifest['files'].items() if name.endswith('.blend'))
    with bpy.data.libraries.load(blend, link=False) as (source, loaded):
        loaded.materials = source.materials
    nodes = [node for material in loaded.materials if material and material.node_tree
             for node in material.node_tree.nodes if node.type == 'DISPLACEMENT']
    if len(nodes) != 1:
        raise ValueError('Expected one source displacement node')
    node = nodes[0]
    if node.inputs['Scale'].is_linked or node.inputs['Midlevel'].is_linked:
        raise ValueError('Source displacement parameters need graph evaluation')
    height_path = next(r['path'] for name, r in manifest['files'].items() if '_disp_' in name)
    image = bpy.data.images.load(height_path, check_existing=False)
    image.colorspace_settings.name = 'Non-Color'
    rgba = np.empty(len(image.pixels), np.float32)
    image.pixels.foreach_get(rgba)
    height = rgba.reshape(image.size[1], image.size[0], 4)[:, :, 0]
    if not np.isfinite(height).all():
        raise ValueError('Non-finite source displacement')
    destination = args.manifest.parent / 'height_linear.npy'
    np.save(destination, height)
    report = dict(schema='directorscut.cliff_scan_height.v1', source_manifest=str(args.manifest.resolve()),
                  asset=manifest['asset'], width_metres=manifest['width_metres'],
                  height_metres=manifest['height_metres'],
                  authored_displacement_scale=float(node.inputs['Scale'].default_value),
                  authored_displacement_midlevel=float(node.inputs['Midlevel'].default_value),
                  amplitude_note='Source material authoring value, not independent survey calibration',
                  field_path=str(destination.resolve()), field_sha256=hashlib.sha256(destination.read_bytes()).hexdigest(),
                  rows='bottom to top, Blender image.pixels convention', colorspace='linear Non-Color',
                  shape=list(height.shape), minimum=float(height.min()), maximum=float(height.max()))
    (args.manifest.parent / 'height_manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
