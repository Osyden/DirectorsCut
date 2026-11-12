#!/usr/bin/env bash
# Build the editor target. The editor must be closed (Live Coding locks the module).
cmd //c "E:\Utilities\UE_5.8\Engine\Build\BatchFiles\Build.bat DirectorsCutEditor Win64 Development -project=E:\projects\directorscut\DirectorsCut\DirectorsCut.uproject -waitmutex" 2>&1 | grep -E "error|Error|warning C|Result:|Total execution" | head -40
