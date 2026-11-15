"""Stage verified local31M weights and three pinned public voice embeddings.

This is a development payload, not a portable Python runtime or release package.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

MODEL_REVISION = '92741c5b07d085ff840e26fea8ce43110b87852f'
MODEL_FILES = {
    'config.json': ('b5d0f88353638370d94035b7d73d7d6577c0554d78f8d7ecfd40490aae39ae85', 4634),
    'tokenizer.json': ('691a00068fa7b989f2f3e5ae5daa2d1a5bb319f2d5360deafcfb19a8bb6e7936', 277967),
    'model.q8.gguf': ('a7d9aad2cd6dc66473f24a08bc6d2939030ef4d751ef82349361fbef3272dbae', 64512352),
    'default-voice.safetensors': ('dc537478eaadc1a5257fcda2e3501ab81e14c111a5d95a5838f350db2afd0f1a', 256136),
}

def verify(data, sha256, length):
    if len(data) != length or hashlib.sha256(data).hexdigest() != sha256:
        raise ValueError('Artifact checksum/length mismatch')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--download-voices', action='store_true')
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    args.output.mkdir(parents=True, exist_ok=True)
    model_out = args.output / 'model'
    model_out.mkdir(exist_ok=True)
    for name, (digest, size) in MODEL_FILES.items():
        data = (args.model_source / name).read_bytes()
        verify(data, digest, size)
        (model_out / name).write_bytes(data)
    manifest = json.loads((source / 'voices.json').read_text(encoding='utf-8'))
    for info in manifest['crew'].values():
        destination = (args.output / info['filename']).resolve()
        if not destination.is_relative_to(args.output.resolve()):
            raise ValueError('Invalid voice destination')
        if destination.exists():
            data = destination.read_bytes()
        elif args.download_voices:
            expected = f"https://huggingface.co/gradium/phonon-voices/resolve/{manifest['revision']}/"
            if not info['url'].startswith(expected):
                raise ValueError('Unexpected voice URL')
            with urllib.request.urlopen(info['url'], timeout=30) as response:
                data = response.read(info['bytes'] + 1)
        else:
            raise ValueError('Voice missing; explicit --download-voices required')
        verify(data, info['sha256'], info['bytes'])
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    shutil.copyfile(source / 'voices.json', args.output / 'voices.json')
    shutil.copyfile(source / 'serve.py', args.output / 'serve.py')
    shutil.copyfile(source / 'THIRD-PARTY-NOTICES.md', args.output / 'THIRD-PARTY-NOTICES.md')
    shutil.copytree(source / 'licenses', args.output / 'licenses', dirs_exist_ok=True)
    report = dict(model='gradium/phonon-31m-en', revision=MODEL_REVISION,
        model_files={name: dict(sha256=digest, bytes=size) for name, (digest, size) in MODEL_FILES.items()},
        voices_revision=manifest['revision'], voices_verified=list(manifest['crew']),
        runtime='External Python environment with ptts==0.7.1 and numpy; not bundled',
        release_status='Development only; include runtime/dependency/model/voice licences before redistribution')
    (args.output / 'staging.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))

if __name__ == '__main__':
    main()
