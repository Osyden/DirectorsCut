#!/usr/bin/env bash
# Run an editor Python script headless. usage: Tools/py.sh Tools/some_script.py
"/e/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "E:\projects\directorscut\DirectorsCut\DirectorsCut.uproject" -run=pythonscript -script="E:/projects/directorscut/DirectorsCut/$1" -unattended -nosplash -nullrhi > /e/projects/directorscut/DirectorsCut/Saved/py.log 2>&1
grep -E "DC_RESULT|LogPython: Error|Traceback|Error:.*[Mm]aterial|error X" /e/projects/directorscut/DirectorsCut/Saved/py.log | sed 's/^\[[^]]*\]\[[ 0-9]*\]//' | cut -c1-300 | head -${2:-20}
