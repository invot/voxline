#!/usr/bin/env bash
# Builds voxline with the bundled MinGW + CMake + JUCE toolchain.
#
#   ./build.sh              configure (if needed) and build the plugin
#   ./build.sh tests        build and run the headless DSP checks
#   ./build.sh smoke        build and run the smoke/snapshot harness
#   ./build.sh icon         redraw Source/platform/voxline.ico from the UI mark
#   ./build.sh clean        wipe build/ first
#
# Outputs:
#   build/Voxline_artefacts/Release/VST3/voxline.vst3
#   build/Voxline_artefacts/Release/Standalone/voxline.exe

set -euo pipefail
cd "$(dirname "$0")"

ROOT="$PWD"
ROOT_W="$(cd "$ROOT" && pwd -W 2>/dev/null || echo "$ROOT")"
TOOLS="$ROOT/../SightLine/.tools"

if [ ! -d "$TOOLS" ]; then
    echo "Bundled toolchain not found at $TOOLS" >&2
    exit 1
fi

export PATH="$TOOLS/cmake-3.31.6-windows-x86_64/bin:$TOOLS/mingw64/bin:$PATH"
unset RC

# The bundled JUCE checkout carries a MinGW fix that gives its juceaide helper
# a Common Controls v6 manifest. It looks for this variable by name.
export SIGHTLINE_MANIFEST_RC="$ROOT_W/Source/platform/win_manifest.rc"

# GCC 16 no longer pulls the C headers JUCE assumes, and JUCE 8's UIA text
# provider needs one enum MinGW does not declare. Both are force-included.
export CXXFLAGS="-include utility -include cstring -include cstdint -include cstdio -include $ROOT_W/Source/platform/mingw_uia_shim.h -static"

if [ "${1:-}" = "clean" ]; then
    rm -rf build
    shift
fi

CONFIGURE_ARGS=(
    -S "$ROOT" -B "$ROOT/build" -G "MinGW Makefiles"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_C_COMPILER="$TOOLS/mingw64/bin/gcc.exe"
    -DCMAKE_CXX_COMPILER="$TOOLS/mingw64/bin/c++.exe"
    -DCMAKE_RC_COMPILER="$TOOLS/mingw64/bin/windres.exe"
    -DCMAKE_MAKE_PROGRAM="$TOOLS/mingw64/bin/mingw32-make.exe"
    -DCMAKE_CXX_FLAGS="$CXXFLAGS"
    -DVX_JUCE_DIR="$TOOLS/JUCE-8.0.6"
)

if [ ! -f build/Makefile ]; then
    cmake "${CONFIGURE_ARGS[@]}"
fi

JOBS="$(nproc 2>/dev/null || echo 4)"

case "${1:-}" in
    tests)
        cmake --build build --target VoxlineTests -j"$JOBS"
        ./build/VoxlineTests.exe
        ;;
    smoke)
        cmake --build build --target VoxlineSmoke -j"$JOBS"
        mkdir -p docs/screenshots
        VX_SNAPSHOT_DIR="$ROOT/docs/screenshots" ./build/VoxlineSmoke.exe
        ;;
    icon)
        cmake --build build --target VoxlineIcon -j"$JOBS"
        ./build/VoxlineIcon.exe Source/platform/voxline.ico docs/screenshots/icon.png
        ;;
    *)
        cmake --build build -j"$JOBS"
        echo
        echo "Built:"
        ls -1d build/Voxline_artefacts/Release/VST3/voxline.vst3 2>/dev/null || true
        ls -1 build/Voxline_artefacts/Release/Standalone/voxline.exe 2>/dev/null || true
        ls -1 build/VoxlineSmoke.exe 2>/dev/null || true
        ;;
esac
