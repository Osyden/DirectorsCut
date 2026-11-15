"""Fetch a pinned Poly Haven CC0 cliff material into ignored authoring storage.

No Unreal import, system installation or credentials. The API supplies URLs and
MD5 integrity checks; SHA256 values are recorded for the local build pipeline.
Run extract_cliff_scan.py in Blender with --disable-autoexec afterwards.
"""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request


def get(url):
    request = urllib.request.Request(url, headers={"User-Agent": "DirectorsCut-cliff-authoring/1.0"})
    with urllib.request.urlopen(request, timeout=90) as response:
        return response.read()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset', choices=['marble_cliff_01', 'rock_face_03'], default='marble_cliff_01')
    parser.add_argument('--resolution', choices=['1k', '2k', '4k'], default='2k')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    if 'saved' not in [part.lower() for part in destination.parts]:
        parser.error('Use an ignored Saved authoring directory')
    destination.mkdir(parents=True, exist_ok=True)
    info = json.loads(get('https://api.polyhaven.com/info/' + args.asset))
    files = json.loads(get('https://api.polyhaven.com/files/' + args.asset))
    entry = files['blend'][args.resolution]['blend']
    downloads = {args.asset + '_' + args.resolution + '.blend': entry, **entry['include']}
    records = {}
    for relative, remote in downloads.items():
        # The source blend expects textures/ below its directory.
        relative_path = Path(relative)
        target = (destination / relative_path).resolve()
        if not target.is_relative_to(destination):
            raise ValueError('Source manifest escaped destination')
        if target.exists():
            payload = target.read_bytes()
            if hashlib.md5(payload).hexdigest() != remote['md5']:
                raise ValueError('Refusing to overwrite different existing source: ' + str(target))
        else:
            payload = get(remote['url'])
            if hashlib.md5(payload).hexdigest() != remote['md5']:
                raise ValueError('Source download failed integrity check: ' + remote['url'])
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(payload)
        records[relative] = dict(path=str(target), url=remote['url'], bytes=len(payload),
                                 md5=remote['md5'], sha256=hashlib.sha256(payload).hexdigest())
    manifest = dict(schema='directorscut.cliff_scan.v1', asset=args.asset,
                    asset_url='https://polyhaven.com/a/' + args.asset,
                    license='CC0', license_url='https://polyhaven.com/license',
                    authors=info['authors'], resolution=args.resolution,
                    width_metres=info['dimensions'][0] / 1000.,
                    height_metres=info['dimensions'][1] / 1000.,
                    files=records, info=info,
                    dimensions_note='API millimetres, agrees with asset page metres; height amplitude read separately from source blend')
    path = destination / 'source_manifest.json'
    path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(path)


if __name__ == '__main__':
    main()
