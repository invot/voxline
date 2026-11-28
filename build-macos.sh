#!/bin/bash
# Build universal macOS VST3 and standalone bundles with Apple Clang and JUCE 8.
set -euo pipefail
cd "$(dirname "$0")"

usage() {
    cat <<'EOF'
Usage: bash ./build-macos.sh [build|tests|smoke|clean]
  build   Build the VST3 and standalone app (default).
  tests   Build and run the headless DSP checks; JUCE is optional.
  smoke   Build and run the processor/editor smoke checks and screenshots.
  clean   Remove build-macos/ before building (may precede tests or smoke).

Requires macOS, Xcode Command Line Tools, CMake 3.22+, and JUCE 8.0.6.
Set VX_JUCE_DIR to use an existing JUCE checkout.
VX_MACOS_ARCHITECTURES defaults to 'arm64;x86_64'.
MACOSX_DEPLOYMENT_TARGET defaults to 11.0; JOBS defaults to the CPU count.
EOF
}

CLEAN=false
if [ "${1:-}" = clean ]; then
    CLEAN=true
    shift
fi
ACTION="${1:-build}"
case "$ACTION" in
    -h|--help) usage; exit 0 ;;
    build|tests|smoke) ;;
    *) usage >&2; exit 1 ;;
esac
if [ "$#" -gt 1 ]; then
    usage >&2
    exit 1
fi

if [ "$(uname -s)" != Darwin ]; then
    echo "This script requires macOS. Use ./build.sh for Windows." >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake 3.22+ is required. Install it with: brew install cmake" >&2
    exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo "Apple's developer tools are required. Run: xcode-select --install" >&2
    exit 1
fi

ROOT="$PWD"
BUILD_DIR="$ROOT/build-macos"
JUCE_DIR="${VX_JUCE_DIR:-$ROOT/.tools/JUCE-8.0.6}"
if [ -z "${VX_JUCE_DIR:-}" ] && [ ! -f "$JUCE_DIR/CMakeLists.txt" ] &&
   [ -f "$ROOT/../SightLine/.tools/JUCE-8.0.6/CMakeLists.txt" ]; then
    JUCE_DIR="$ROOT/../SightLine/.tools/JUCE-8.0.6"
fi
if [ "$ACTION" != tests ] && [ ! -f "$JUCE_DIR/CMakeLists.txt" ]; then
    echo "JUCE not found at $JUCE_DIR." >&2
    echo "Clone JUCE 8.0.6 into .tools/JUCE-8.0.6 or set VX_JUCE_DIR." >&2
    exit 1
fi

JOBS="${JOBS:-$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}"
if [ "$CLEAN" = true ]; then
    rm -rf "$BUILD_DIR"
fi

# Use a separate tree so the Windows build and its cached toolchain stay usable.
# Configure every time so changed JUCE paths and architecture settings take effect.
cmake -S "$ROOT" -B "$BUILD_DIR" -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$(xcrun --find clang)" \
    -DCMAKE_CXX_COMPILER="$(xcrun --find clang++)" \
    -DCMAKE_OSX_ARCHITECTURES="${VX_MACOS_ARCHITECTURES:-arm64;x86_64}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}" \
    -DVX_JUCE_DIR="$JUCE_DIR"

case "$ACTION" in
    tests)
        cmake --build "$BUILD_DIR" --target VoxlineTests --parallel "$JOBS"
        "$BUILD_DIR/VoxlineTests"
        ;;
    smoke)
        cmake --build "$BUILD_DIR" --target VoxlineSmoke --parallel "$JOBS"
        mkdir -p "$ROOT/docs/screenshots"
        VX_SNAPSHOT_DIR="$ROOT/docs/screenshots" "$BUILD_DIR/VoxlineSmoke"
        ;;
    build)
        cmake --build "$BUILD_DIR" --target Voxline_VST3 Voxline_Standalone --parallel "$JOBS"
        echo
        echo "Built:"
        printf '%s\n' \
            "$BUILD_DIR/Voxline_artefacts/Release/VST3/voxline.vst3" \
            "$BUILD_DIR/Voxline_artefacts/Release/Standalone/voxline.app"
        ;;
esac
