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
powershell.exe -NoProfile -File E:/projects/directorscut/DirectorsCut/Tools/package_isolated.ps1 \
  -ProjectPath E:/projects/directorscut/DirectorsCut/DirectorsCut.uproject \
  -OutputPath "$WINOUT" -SidecarsRoot E:/projects/directorscut/DirectorsCut/Sidecars > "$PROJ/Saved/package.log" 2>&1
RESULT=$?
cat "$PROJ/Saved/package.log"
# The shared helper includes the cook workaround, selected voices and notices.
if [ "$RESULT" -eq 0 ]; then
  echo "$NAME  $(git -C $PROJ log -1 --format=%s)" >> $ROOT/Builds/INDEX.txt
  du -sh $OUT
  echo "Play: $WINOUT/Windows/DirectorsCut.exe"
fi
exit "$RESULT"
