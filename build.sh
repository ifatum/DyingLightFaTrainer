#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[ -f src/items.h ] && [ -f src/font.h ] || python3 gen_data.py
IM=third_party/imgui
nix-shell -p pkgsCross.mingwW64.buildPackages.gcc --run "
  set -e
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -shared -o xinput1_3.dll \
    src/dllmain.cpp $IM/imgui.cpp $IM/imgui_draw.cpp $IM/imgui_tables.cpp $IM/imgui_widgets.cpp \
    $IM/backends/imgui_impl_dx11.cpp $IM/backends/imgui_impl_win32.cpp src/xinput1_3.def \
    -I$IM -Isrc -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_USER_CONFIG=\\\"imconfig_dlt.h\\\" \
    -static -static-libgcc -static-libstdc++ -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -limm32 -luser32
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/selftest.exe test/selftest.cpp -Isrc -static
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/preview.exe test/preview.cpp -Isrc -static -ld3d11 -ldxgi
"
echo "built: $(pwd)/xinput1_3.dll"
