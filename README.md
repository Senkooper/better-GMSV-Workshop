# steamworks.DownloadUGC


## build commands for windows:

g++ -shared -o gmsv_workshop_win64.dll -I "**PATH_TO_SDK**" main.cpp "**PATH_TO_SDK**\steamworks_157\sdk\redistributable_bin\win64\steam_api64.lib"


## build commands for linux:

g++ -shared -fPIC -o "gmsv_workshop_linux64.dll" -I "**PATH_TO_SDK**" main.cpp "**PATH_TO_SDK**/steamworks_157/sdk/redistributable_bin/linux64/libsteam_api.so"

**This requires steam works version 157 and the garry's mod cpp sdk**