$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $projectRoot 'Saved/FogRouteReplay'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'MSVC C++ tools are required for the CPU replay.' }
$devCmd = Join-Path $vsRoot 'Common7/Tools/VsDevCmd.bat'
$source = Join-Path $PSScriptRoot 'FogRouteReplay/replay.cpp'
$headers = Join-Path $projectRoot 'Source/DirectorsCut'
$exe = Join-Path $outDir 'fog-route-replay.exe'
$obj = Join-Path $outDir 'fog-route-replay.obj'
# Fixed compiler command; no engine/build slot, asset loading or GPU required.
$batch = Join-Path $outDir 'compile.cmd'
@"
@echo off
call "$devCmd" -arch=x64 -no_logo
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /I"$headers" "$source" /Fe:"$exe" /Fo:"$obj"
"@ | Set-Content -LiteralPath $batch -Encoding ascii
& $env:ComSpec /d /c $batch
if ($LASTEXITCODE) { throw 'CPU replay compilation failed.' }
& $exe
if ($LASTEXITCODE) { throw 'CPU replay failed.' }


