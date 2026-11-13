#!/usr/bin/env bash
# Package a Windows build into its own folder under E:\projects\directorscut\Builds, named by date,
# time and commit (e.g. Builds/2026-11-13_1420_dc7fef4). Earlier builds are never overwritten or
# removed: they are kept for the dev log. Close the editor first.
# usage: Tools/package.sh [label]     label is appended to the folder name, e.g. "fog-pass"
ROOT=/e/projects/directorscut
PROJ=$ROOT/DirectorsCut
NAME="$(date +%Y-%m-%d_%H%M)_$(git -C $PROJ rev-parse --short HEAD)${1:+_$1}"
[ -n "$(git -C $PROJ status --porcelain)" ] && NAME="${NAME}_dirty"
OUT=$ROOT/Builds/$NAME
WINOUT="E:/projects/directorscut/Builds/$NAME"
cmd //c "E:\Utilities\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=E:\projects\directorscut\DirectorsCut\DirectorsCut.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=$WINOUT -nop4 -utf8output -unattended -noP4" > $PROJ/Saved/package.log 2>&1
echo "exit $?"; grep -E "BUILD SUCCESSFUL|BUILD FAILED|AutomationTool exiting|Error:" $PROJ/Saved/package.log | sort | uniq -c | head -20
# The local AI sidecars are not Unreal content; copy them next to the packaged project.
if grep -q "BUILD SUCCESSFUL" $PROJ/Saved/package.log; then
  mkdir -p $OUT/Windows/DirectorsCut/Sidecars
  cp -ru $PROJ/Sidecars/bin $PROJ/Sidecars/models $OUT/Windows/DirectorsCut/Sidecars/
  echo "$NAME  $(git -C $PROJ log -1 --format=%s)" >> $ROOT/Builds/INDEX.txt
  du -sh $OUT
  echo "Play: $WINOUT/Windows/DirectorsCut.exe"
fi
