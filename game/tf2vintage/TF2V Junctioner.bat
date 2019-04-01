@echo OFF

set modname=tf2vintage

::Don't edit anything below this.

setlocal ENABLEEXTENSIONS
set KEY_NAME="HKCU\SOFTWARE\VALVE\STEAM"
set VALUE_NAME=SOURCEMODINSTALLPATH

FOR /F "usebackq skip=2 tokens=1-2*" %%A IN (`REG QUERY %KEY_NAME% /v %VALUE_NAME% 2^>nul`) DO (
    set SourcemodsLocation=%%C
)

if defined SourcemodsLocation (
 goto junction
) else (
@echo %KEY_NAME%\%VALUE_NAME% not found.
goto finishbad
)

:junction
set modpath1=%cd%
set sourcemodsmodpath=%SourcemodsLocation%\%modname%
set fullmodpath=%modpath1%
mklink /J "%sourcemodsmodpath%" "%fullmodpath%"
goto finishgood

:finishbad
@echo An error occurred while finding the location of the Sourcemods directory.
pause
exit

:finishgood
taskkill /f /IM "steam.exe"
start steam:
exit

::finishgood
::@echo Your mod has now been junctioned to the sourcemods directory.
::@echo Don't forget to restart Steam to see the mod in your library!
::pause
::exit