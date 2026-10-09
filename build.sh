#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[ -f src/items.h ] || python3 gen_data.py
IM=third_party/imgui
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
NUM=${VER%% *}
RC=$(mktemp -d)
trap 'rm -rf "$RC"' EXIT
version_header() {
  mkdir -p "$RC/$1"
  printf '#define VER_MAJOR %s\n#define VER_MINOR %s\n#define VER_STRING "%s"\n#define VER_FILETYPE %s\n#define VER_DESCRIPTION "%s"\n#define VER_INTERNAL "%s"\n#define VER_FILENAME "%s"\n' \
    "${NUM%%.*}" "${NUM#*.}" "$VER" "$2" "$3" "$4" "$5" > "$RC/$1/version_values.h"
}
version_header dll VFT_DLL "FaTrainer | Dying Light" xinput1_3 xinput1_3.dll
DLL=xinput1_3.dll EDITION= NET=-lwinhttp TESTS=1
if [ "${1:-}" = "--nexus" ]; then
  DLL=dist/nexus/xinput1_3.dll EDITION=-DFATRAINER_OFFLINE NET= TESTS=0
  mkdir -p dist/nexus
fi
export RC
nix-shell -p pkgsCross.mingwW64.buildPackages.gcc --run "
  set -e
  x86_64-w64-mingw32-windres src/version.rc -O coff -o \$RC/dll.res -I\$RC/dll
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -shared -o $DLL $EDITION \
    src/dllmain.cpp $IM/imgui.cpp $IM/imgui_draw.cpp $IM/imgui_tables.cpp $IM/imgui_widgets.cpp \
    $IM/backends/imgui_impl_dx11.cpp $IM/backends/imgui_impl_win32.cpp src/xinput1_3.def \$RC/dll.res \
    -I$IM -Isrc -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_USER_CONFIG=\\\"imconfig_dlt.h\\\" \
    -static -static-libgcc -static-libstdc++ -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -limm32 -luser32 -ldinput8 -ldxguid $NET
  [ $TESTS = 0 ] && exit 0
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/selftest.exe test/selftest.cpp -Isrc -static -ldinput8 -ldxguid
  x86_64-w64-mingw32-g++ -O2 -std=c++17 -o test/preview.exe test/preview.cpp -Isrc -static -ld3d11 -ldxgi
"
echo "built: $(pwd)/$DLL"
