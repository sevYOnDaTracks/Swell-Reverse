#!/bin/bash
# Installe Swell Reverse dans les dossiers plugins de l'utilisateur.
# Usage : bash install_mac.sh [dossier contenant les plugins]  (par défaut : le dossier du script)
set -e
SRC="${1:-$(dirname "$0")}"

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

rm -rf "$VST3_DIR/Swell Reverse.vst3" "$AU_DIR/Swell Reverse.component"
cp -R "$SRC/Swell Reverse.vst3" "$VST3_DIR/"
cp -R "$SRC/Swell Reverse.component" "$AU_DIR/"

# Plugin non signé par Apple : on retire la quarantaine pour que macOS l'accepte
xattr -dr com.apple.quarantine "$VST3_DIR/Swell Reverse.vst3" 2>/dev/null || true
xattr -dr com.apple.quarantine "$AU_DIR/Swell Reverse.component" 2>/dev/null || true

# Force macOS à relire la liste des Audio Units
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "OK : Swell Reverse installé"
echo "  VST3 -> $VST3_DIR"
echo "  AU   -> $AU_DIR"
echo "Dans FL Studio : Options > Manage plugins > Find installed plugins"
