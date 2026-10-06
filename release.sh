#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
grep -q "^## $VER\$" CHANGELOG.md || { echo "CHANGELOG.md has no ## $VER section"; exit 1; }
./build.sh
./build_installer.sh
OUT=dist/release
rm -rf "$OUT"
mkdir -p "$OUT"
cp xinput1_3.dll CHANGELOG.md dist/FaTrainer-Installer.exe "$OUT/"
printf 'version %s\nsha256 %s\nsize %s\n' "$VER" "$(sha256sum xinput1_3.dll | cut -d' ' -f1)" "$(stat -c%s xinput1_3.dll)" > "$OUT/version.txt"
tar -C dist -czf "$OUT/FaTrainer-Installer-Linux.tar.gz" FaTrainer-Installer
awk -v v="## $VER" '$0 == v {on = 1; next} /^## / {on = 0} on' CHANGELOG.md > "$OUT/notes.md"
if [ "${1:-}" = "--publish" ]; then
  nix-shell -p gh --run "gh release create v$VER $OUT/FaTrainer-Installer.exe $OUT/FaTrainer-Installer-Linux.tar.gz $OUT/xinput1_3.dll $OUT/version.txt $OUT/CHANGELOG.md --title 'FaTrainer $VER' --notes-file $OUT/notes.md --latest"
fi
echo "release files: $OUT"
