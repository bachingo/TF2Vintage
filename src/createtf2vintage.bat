@echo off
pushd "%~dp0"
devtools\bin\vpc.exe /tf /define:LTCG /define:SOURCESDK +everything /mksln tf2vintage.sln
popd
pause