#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
SLUG=${VER// /-}
grep -q "^## $VER\$" CHANGELOG.md || { echo "CHANGELOG.md has no ## $VER section"; exit 1; }
./build.sh
./build_installer.sh
OUT=dist/release
mkdir -p "$OUT"
rm -f "$OUT/xinput1_3.dll" "$OUT/CHANGELOG.md" "$OUT/version.txt" "$OUT/notes.md"
cp xinput1_3.dll CHANGELOG.md "$OUT/"
printf 'version %s\nsha256 %s\nsize %s\n' "$VER" "$(sha256sum xinput1_3.dll | cut -d' ' -f1)" "$(stat -c%s xinput1_3.dll)" > "$OUT/version.txt"
awk -v v="## $VER" '$0 == v {on = 1; next} /^## / {on = 0} on' CHANGELOG.md > "$OUT/notes.md"
NEXUS=$(mktemp -d)
mkdir -p "$NEXUS/manual"
cp dist/nexus/FaTrainer-Nexus-Version-Installer.exe dist/nexus/FaTrainer-Nexus-Version-Installer "$NEXUS/"
cp dist/nexus/xinput1_3.dll "$NEXUS/manual/"
[ -f nexus/readme.txt ] && cp nexus/readme.txt "$NEXUS/"
rm -f "dist/nexus/FaTrainer-$SLUG-Nexus-Version.zip"
ZIP="$(pwd)/dist/nexus/FaTrainer-$SLUG-Nexus-Version.zip"
(cd "$NEXUS" && nix-shell -p zip --run "zip -qX -r '$ZIP' .")
rm -rf "$NEXUS"
if [ "${1:-}" = "--publish" ]; then
  nix-shell -p gh --run "gh release create v$SLUG $OUT/xinput1_3.dll $OUT/version.txt $OUT/CHANGELOG.md --title 'FaTrainer $VER' --notes-file $OUT/notes.md --latest"
fi
echo "release files: $OUT, Nexus Version zip: dist/nexus/FaTrainer-$SLUG-Nexus-Version.zip"
