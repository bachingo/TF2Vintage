@echo off
pushd "%~dp0"
devtools\bin\vpc.exe /tf2vintage /define:SOURCESDK,TF +game /mksln TF2vintage.sln
popd
pause