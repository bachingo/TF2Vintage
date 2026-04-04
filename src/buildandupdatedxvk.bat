@echo off
echo Starting the process...

cd .\dxvk

set "SOURCE_BAT=.\package-win-release.bat"
set "LIB_DIR=..\lib\public\x64"
set "GAME_DIR=..\..\game"

echo Running package-win.bat...
call "%SOURCE_BAT%"

if %ERRORLEVEL% EQU 0 (
    echo Batch file executed successfully
) else (
    echo Error running batch file
    exit /b %ERRORLEVEL%
)

if not exist "%LIB_DIR%" mkdir "%LIB_DIR%"
if not exist "%GAME_DIR%" mkdir "%GAME_DIR%"

echo Copying d3d9.lib to %LIB_DIR%...
copy ".\build\d3d9.lib" "%LIB_DIR%" /Y

echo Copying d3d9.dll to %GAME_DIR%...
copy ".\build\src\d3d9\d3d9.dll" "%GAME_DIR%" /Y

if %ERRORLEVEL% EQU 0 (
    echo Files copied successfully
) else (
    echo Error copying files
    exit /b %ERRORLEVEL%
)

echo DXVK updated!
pause