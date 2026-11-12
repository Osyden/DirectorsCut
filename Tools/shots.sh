#!/usr/bin/env bash
# Run the game unattended and save screenshots at the given ride times.
# usage: Tools/shots.sh "5,70,150" ["X,Y,Z,Pitch,Yaw"]
OUT=/e/projects/directorscut/DirectorsCut/Saved/Screenshots/Auto
rm -f "$OUT"/*.png
ARGS="-DCAutoShots=$1"
[ -n "$2" ] && ARGS="$ARGS -DCShotCam=$2"
"/e/Utilities/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "E:\projects\directorscut\DirectorsCut\DirectorsCut.uproject" -game -windowed -ResX=${RESX:-1280} -ResY=${RESY:-720} -unattended -nosplash ${SOUND--nosound} $ARGS $EXTRA -log=AutoShots.log >/dev/null 2>&1
ls "$OUT"
grep -c "Blueprint Runtime Error" /e/projects/directorscut/DirectorsCut/Saved/Logs/AutoShots.log
grep "LogDC" /e/projects/directorscut/DirectorsCut/Saved/Logs/AutoShots.log | sed 's/^\[[^]]*\]\[[ 0-9]*\]//' | head -${3:-25}
