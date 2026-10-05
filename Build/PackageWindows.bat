@echo off
setlocal
REM AFTERLIGHT — one-click Win64 package (run on Windows with UE 5.8 installed)
set ENGINE=C:\Program Files\Epic Games\UE_5.8
if not exist "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" (
  echo UE 5.8 not found at "%ENGINE%".
  echo Set ENGINE= to your Unreal Engine 5.8 path and re-run.
  exit /b 1
)
set PROJECT=%~dp0..\Afterlight.uproject
set OUT=%~dp0..\Dist\itch\Windows
mkdir "%OUT%" 2>nul
call "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%PROJECT%" ^
  -noP4 -platform=Win64 -clientconfig=Development ^
  -cook -build -stage -pak -archive ^
  -archivedirectory="%OUT%" ^
  -utf8output ^
  -map=/Game/System/FrontEnd/Maps/L_LyraFrontEnd+/ShooterMaps/Maps/L_Expanse+/ShooterCore/Maps/L_ShooterGym
if errorlevel 1 exit /b %errorlevel%
echo.
echo Packaged to %OUT%
echo Rename LyraGame.exe folder/app branding to Afterlight as needed.
