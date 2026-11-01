#!/bin/bash
# Compile Swell Reverse sur ton Mac et l'installe (VST3 + AU).
# Prérequis : outils Xcode (xcode-select --install) et CMake (brew install cmake)
set -e
cd "$(dirname "$0")/.."

command -v cmake >/dev/null || { echo "CMake manquant : brew install cmake"; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Outils Xcode manquants : xcode-select --install"; exit 1; }

[ -d JUCE ] || git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE.git JUCE

rm -f build/CMakeCache.txt
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SwellVerb_VST3 SwellVerb_AU -j "$(sysctl -n hw.ncpu)"

VST3="build/SwellVerb_artefacts/Release/VST3/Swell Reverse.vst3"
AU="build/SwellVerb_artefacts/Release/AU/Swell Reverse.component"
codesign --force --deep -s - "$VST3"
codesign --force --deep -s - "$AU"

mkdir -p dist && rm -rf dist/*
cp -R "$VST3" "$AU" dist/
bash scripts/install_mac.sh dist
