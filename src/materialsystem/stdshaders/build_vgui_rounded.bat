@echo off
setlocal

rem Build just our VGUI rounded shader for fast iteration

set BUILD_SHADER=call buildshaders.bat
set GAME_DIR="..\..\..\game\tfvr"
set SRC_DIR="..\..\"

rem Build SDK shaders which includes our vgui_rounded shaders
%BUILD_SHADER% sdkshaders_dx9_20b -game %GAME_DIR% -source %SRC_DIR%
