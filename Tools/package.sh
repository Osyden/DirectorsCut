#!/usr/bin/env bash
# Package a Windows build into E:\projects\directorscut\Builds. Close the editor first.
cmd //c "E:\Utilities\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=E:\projects\directorscut\DirectorsCut\DirectorsCut.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=E:\projects\directorscut\Builds -nop4 -utf8output -unattended -noP4" > /e/projects/directorscut/DirectorsCut/Saved/package.log 2>&1
echo "exit $?"; grep -E "BUILD SUCCESSFUL|BUILD FAILED|AutomationTool exiting|Error:" /e/projects/directorscut/DirectorsCut/Saved/package.log | sort | uniq -c | head -20
# The local AI sidecars are not Unreal content; copy them next to the packaged project.
if grep -q "BUILD SUCCESSFUL" /e/projects/directorscut/DirectorsCut/Saved/package.log; then
  mkdir -p /e/projects/directorscut/Builds/Windows/DirectorsCut/Sidecars
  rm -rf /e/projects/directorscut/Builds/Windows/DirectorsCut/Sidecars/bin /e/projects/directorscut/Builds/Windows/DirectorsCut/Sidecars/models
  cp -ru /e/projects/directorscut/DirectorsCut/Sidecars/bin /e/projects/directorscut/DirectorsCut/Sidecars/models /e/projects/directorscut/Builds/Windows/DirectorsCut/Sidecars/
  du -sh /e/projects/directorscut/Builds/Windows
fi
