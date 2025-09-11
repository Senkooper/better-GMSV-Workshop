wsl g++ -shared -fPIC -o "/mnt/c/gmod_projs/workshop_v2/gmsv_workshop_linux64.dll" \
    -I "/mnt/c/sdk" \
    -I "$HOME/glibc-2.31/include" \
    "/mnt/c/gmod_projs/workshop_v2/main.cpp" \
    "/mnt/c/sdk/steamworks_157/sdk/redistributable_bin/linux64/libsteam_api.so" \
    -L "$HOME/glibc-2.31/lib" -Wl,-rpath,"$HOME/glibc-2.31/lib"

g++ -shared -fPIC -o "/home/testserver2/serverfiles/garrysmod/lua/bin/gmsv_workshop_linux64.dll" -I "sdk" main.cpp "sdk/steamworks_157/sdk/redistributable_bin/linux64/libsteam_api.so"
g++ -shared -fPIC -o "gmsv_workshop_linux64.dll" -I "sdk" main.cpp "sdk/steamworks_157/sdk/redistributable_bin/linux64/libsteam_api.so"

set /p var=Hit ENTER to continue...

lua_openscript linux-wor