@echo off
pushd "%~dp0"
devtools\bin\vpc.exe /tf /define:SOURCESDK +everything /mksln tf2vintage.sln
popd
pause