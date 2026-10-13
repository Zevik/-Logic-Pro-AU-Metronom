#!/usr/bin/env bash
# Build BeatCanvas as a universal Audio Unit and install it for Logic Pro.
# Requirements: Xcode command line tools (xcode-select --install) and CMake (brew install cmake).
set -euo pipefail
cd "$(dirname "$0")/.."

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target BeatCanvas_AU ClockTest --parallel

echo "== sync-engine test =="
./build/ClockTest

# JUCE copies the bundle to ~/Library/Audio/Plug-Ins/Components automatically.
AU=~/Library/Audio/Plug-Ins/Components/BeatCanvas.component
codesign --force --deep -s - "$AU"
xattr -cr "$AU"

echo "== auval (Apple's Audio Unit validator) =="
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aufx Bcnv Zvkd

echo
echo "Done. Open Logic Pro -> Audio FX slot -> Audio Units -> Zevik -> BeatCanvas."
echo "If Logic doesn't list it: Logic Pro > Settings > Plug-in Manager > Reset & Rescan Selection."
