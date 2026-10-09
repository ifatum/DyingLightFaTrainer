#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
VER=$(sed -n 's/^#define INSTALLER_VERSION "\(.*\)"/\1/p' src/version.h)
grep -q "^## $VER\$" installer/CHANGELOG.md || { echo "installer/CHANGELOG.md has no ## $VER section"; exit 1; }
./build_installer.sh
OUT=dist/release
mkdir -p "$OUT"
WIN=FaTrainer-Fatum-Version-Installer.exe LIN=FaTrainer-Fatum-Version-Installer
cp "dist/$WIN" "dist/$LIN" "$OUT/"
tar -C dist -czf "$OUT/$LIN-Linux.tar.gz" "$LIN"
info() { printf 'version %s\nsha256 %s\nsize %s\n' "$VER" "$(sha256sum "$OUT/$1" | cut -d' ' -f1)" "$(stat -c%s "$OUT/$1")"; }
info "$WIN" > "$OUT/installer-windows.txt"
info "$LIN" > "$OUT/installer-linux.txt"
awk -v v="## $VER" '$0 == v {on = 1; next} /^## / {on = 0} on' installer/CHANGELOG.md > "$OUT/installer-notes.md"
if [ "${1:-}" = "--publish" ]; then
  FILES="$OUT/$WIN $OUT/$LIN $OUT/$LIN-Linux.tar.gz $OUT/installer-windows.txt $OUT/installer-linux.txt"
  nix-shell -p gh --run "
    gh release view installer >/dev/null 2>&1 || gh release create installer --title 'FaTrainer Installer $VER' --notes-file $OUT/installer-notes.md --latest=false
    gh release upload installer $FILES --clobber
    gh release edit installer --title 'FaTrainer Installer $VER' --notes-file $OUT/installer-notes.md --latest=false"
fi
echo "installer $VER files: $OUT"
