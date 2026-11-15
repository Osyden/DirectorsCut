"""Stage an isolated Windows Phonon CPU Python from existing local installations.

No downloads, installer execution, model runs, or modifications to source installs.
The destination must be new/empty. Use --verify-only to recheck a staged payload.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


PROBE = r'''
import ctypes, json, pathlib, sys
import ptts, numpy
root = pathlib.Path(sys.executable).resolve().parent
assert sys.flags.isolated and sys.flags.ignore_environment and sys.flags.no_site
assert ptts.__version__ == '0.7.1' and numpy.__version__ == '2.5.4'
assert sys.path and all(pathlib.Path(p).resolve().is_relative_to(root) for p in sys.path)
assert pathlib.Path(ptts.__file__).resolve().is_relative_to(root)
assert pathlib.Path(numpy.__file__).resolve().is_relative_to(root)
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.GetCurrentProcess.restype = ctypes.c_void_p
kernel.K32EnumProcessModules.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p), ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong)]
kernel.GetModuleFileNameW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_ulong]
handles = (ctypes.c_void_p * 2048)()
needed = ctypes.c_ulong()
assert kernel.K32EnumProcessModules(kernel.GetCurrentProcess(), handles, ctypes.sizeof(handles), ctypes.byref(needed))
assert needed.value <= ctypes.sizeof(handles)
modules = []
for handle in handles[:needed.value // ctypes.sizeof(ctypes.c_void_p)]:
    name = ctypes.create_unicode_buffer(32768)
    assert kernel.GetModuleFileNameW(handle, name, len(name))
    modules.append(str(pathlib.Path(name.value).resolve()))
windows = pathlib.Path(__import__('os').environ['SystemRoot']).resolve()
foreign = [p for p in modules if not pathlib.Path(p).is_relative_to(root) and not pathlib.Path(p).is_relative_to(windows)]
assert not foreign, foreign
print(json.dumps(dict(passed=True, version=sys.version, executable=sys.executable,
    prefix=sys.prefix, base_prefix=sys.base_prefix, sys_path=sys.path, ptts=ptts.__version__,
    numpy=numpy.__version__, ptts_path=ptts.__file__, numpy_path=numpy.__file__,
    isolated=sys.flags.isolated, no_site=sys.flags.no_site, loaded_modules=modules)))
'''


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def verify(destination):
    environment = dict(os.environ)
    environment.update(PATH=str(Path(os.environ['SystemRoot'])/'System32'),
                       PYTHONHOME='Z:/intentionally-unavailable-python-home',
                       PYTHONPATH='Z:/intentionally-unavailable-python-path',
                       CUDA_VISIBLE_DEVICES='', HF_HUB_OFFLINE='1',
                       TRANSFORMERS_OFFLINE='1')
    completed = subprocess.run([str(destination/'python.exe'), '-B', '-c', PROBE],
                               cwd=destination, env=environment, capture_output=True,
                               text=True, timeout=20, shell=False,
                               creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if completed.returncode:
        raise RuntimeError('Isolated import probe failed: '+completed.stderr[-4000:])
    result = json.loads(completed.stdout)
    (destination/'import-verification.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', type=Path, default=Path('E:/Utilities/Blender/5.3/python'))
    parser.add_argument('--site-packages', type=Path, required=True)
    parser.add_argument('--blender-root', type=Path, default=Path('E:/Utilities/Blender'))
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    destination = args.destination.resolve()
    if args.verify_only:
        verify(destination)
        print(json.dumps(dict(passed=True, destination=str(destination))))
        return
    base = args.base.resolve(strict=True)
    packages = args.site_packages.resolve(strict=True)
    blender = args.blender_root.resolve(strict=True)
    if any(destination.is_relative_to(p) or p.is_relative_to(destination) for p in (base, packages, blender)):
        raise ValueError('Destination must be separate from all source installations')
    if destination.exists() and any(destination.iterdir()):
        raise ValueError('Destination must be new or empty; staging never deletes or replaces a payload')
    if sys.version_info[:2] != (3, 13):
        raise ValueError('Run staging with the source CPython 3.13 interpreter')
    destination.mkdir(parents=True, exist_ok=True)
    inventory = []

    def copy(source, relative, component):
        source = source.resolve(strict=True)
        target = destination/relative
        if not target.resolve().is_relative_to(destination):
            raise ValueError('Target escapes destination')
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        inventory.append(dict(path=relative.as_posix(), bytes=target.stat().st_size,
                              sha256=sha(target), component=component, source=str(source)))

    def tree(source, relative, component, excluded=()):
        for path in sorted(source.rglob('*')):
            subpath = path.relative_to(source)
            if path.is_file() and not any(p in excluded or p == '__pycache__' for p in subpath.parts) and path.suffix != '.pyc':
                copy(path, relative/subpath, component)

    for name in ('python.exe', 'python3.dll', 'python313.dll'):
        copy(base/'bin'/name, Path(name), 'CPython')
    tree(base/'lib', Path('Lib'), 'CPython stdlib',
         ('site-packages', 'ensurepip', 'test', 'tests', 'idlelib', 'tkinter', 'turtledemo', 'venv'))
    tree(base/'DLLs', Path('DLLs'), 'CPython extensions and bundled libraries')
    for name in ('vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll',
                 'msvcp140_1.dll', 'msvcp140_2.dll', 'msvcp140_atomic_wait.dll', 'msvcp140_codecvt_ids.dll'):
        copy(blender/'blender.crt'/name, Path(name), 'Microsoft VC runtime')
    for name in ('ptts', 'ptts-0.7.1.dist-info', 'numpy', 'numpy.libs', 'numpy-2.5.4.dist-info'):
        tree(packages/name, Path('Lib/site-packages')/name, name)
    # Preserve supplied third-party references as evidence, not as a claim that
    # Blender's own GPL applies to this separately staged Python interpreter.
    tree(blender/'license', Path('licenses/blender-supplied-reference'), 'Bundled licence references')
    copy(blender/'license/spdx/Python-2.0.txt', Path('licenses/PYTHON-LICENSE.txt'), 'CPython licence')
    (destination/'licenses/PYTHON-COPYRIGHT.txt').write_text(sys.copyright+'\n', encoding='utf-8')
    (destination/'python313._pth').write_text('Lib\nDLLs\nLib/site-packages\n', encoding='ascii')
    (destination/'THIRD-PARTY-NOTICES.txt').write_text(
        'Portable Phonon CPU Python payload\n\n'
        'CPython 3.13: existing Blender-distributed interpreter, stdlib and extensions.\n'
        'No Blender executable, add-ons, or unrelated site-packages are included.\n'
        'Changes: relocated files; omitted caches, pip bootstrap, virtualenv and GUI/test modules;\n'
        'added isolated python313._pth. Interpreter/extension binaries are unmodified.\n'
        'CPython copyright and PSF licence: licenses/PYTHON-*.txt. Supplied broader dependency\n'
        'notices: licenses/blender-supplied-reference/. These also document components not shipped here.\n\n'
        'ptts 0.7.1: MIT OR Apache-2.0; original licence files and Rust dependency SBOM\n'
        'are retained in Lib/site-packages/ptts-0.7.1.dist-info/.\n'
        'NumPy 2.5.4: retain complete wheel notices in numpy-2.5.4.dist-info/licenses/,\n'
        'including bundled BLAS and other dependencies; NumPy is not wholly one licence.\n'
        'Microsoft VC runtime DLLs retain original binaries/version information. This payload\n'
        'does not include a separate Microsoft redistribution grant; release packaging must\n'
        'verify applicable VC redistributable terms. Windows system libraries remain OS prerequisites.\n'
        'Speech model and voice embeddings are separate payloads with separate notices.\n', encoding='utf-8')
    report = dict(schema_version=1, python_version=sys.version, ptts='0.7.1', numpy='2.5.4',
                  isolation='python313._pth; no import site; no environment or user site paths',
                  cpu_only_adapter=True, downloads=False, source_base=str(base),
                  source_packages=str(packages), files=inventory,
                  copied_bytes=sum(f['bytes'] for f in inventory), copied_files=len(inventory))
    (destination/'payload-manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    result = verify(destination)
    print(json.dumps(dict(passed=result['passed'], copied_bytes=report['copied_bytes'],
                          copied_files=len(inventory), destination=str(destination))))


if __name__ == '__main__':
    main()
