"""Prepare and assemble in a full Unreal editor via -ExecutePythonScript.

Do not use -run=pythonscript: TextureGraph does not initialize in that commandlet.
Preparation alone can run in the commandlet; assembly requires the full editor.
"""
from pathlib import Path

scripts = Path(__file__).resolve().parent
for stage in ('prepare_pirate_cast.py', 'assemble_pirate_cast.py', 'restore_pirate_strands.py', 'validate_pirate_cast.py'):
    source = scripts / stage
    exec(compile(source.read_text(encoding='utf-8'), str(source), 'exec'), {'__file__': str(source), '__name__': '__main__'})
