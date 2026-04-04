REM Set launch params below, or in steam
SET params=-novid -condebug -nofriendsui -sw -windowed +developer 1 -high -refresh 0 -noasserts -hushasserts -steam -game tf2vintage

SET DXVK_STATE_CACHE=1
SET TFVR_STATE_CACHE_PATH=%~dp0%dxvk-cache
SET DXVK_ASYNC=1
SET DXVK_LOG_LEVEL=info
REM start the game
START tf2vintage_win64.exe %* %params%