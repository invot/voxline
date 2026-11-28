#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: bash build-macos.sh [clean] [build|all|tests|smoke|configure] [jobs]
  build   Build and package the VST3, AU, and standalone app (default).
  all     Alias for build.
  tests   Build and run the headless checks. JUCE is optional.
  smoke   Run the processor and editor smoke checks and write screenshots.
  configure  Configure the build tree without compiling.
  clean   Remove the selected macOS build tree before running the command.

Requires macOS, Xcode Command Line Tools, and CMake 3.22 or later.
Set VX_JUCE_DIR to use a JUCE 8.0.6 checkout.
VX_MACOS_ARCHITECTURES defaults to 'arm64;x86_64'.
MACOSX_DEPLOYMENT_TARGET defaults to 11.0. JOBS defaults to the CPU count.
EOF
}

clean=false
if [[ ${1:-} == clean ]]; then
    clean=true
    shift
fi
action=${1:-build}
case "$action" in
    -h|--help) usage; exit 0 ;;
    build|all|tests|smoke|configure) ;;
    *) usage >&2; exit 2 ;;
esac
if [[ $# -gt 2 ]]; then
    usage >&2
    exit 2
fi

if [[ $(uname -s) != Darwin ]]; then
    echo 'This script requires macOS.' >&2
    exit 1
fi
if ! command -v cmake >/dev/null; then
    echo 'Install CMake 3.22 or later and add it to PATH.' >&2
    exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo 'Install the Xcode Command Line Tools with xcode-select --install.' >&2
    exit 1
fi

jobs=${2:-${JOBS:-$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}}
if [[ ! $jobs =~ ^[1-9][0-9]*$ ]]; then
    echo 'The job count must be a positive integer.' >&2
    exit 2
fi

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$root"
build="$root/build-macos"
juce_dir=${VX_JUCE_DIR:-}
if [[ -z $juce_dir ]]; then
    for candidate in .tools/JUCE-8.0.6 ../sightline/.tools/JUCE-8.0.6 ../SightLine/.tools/JUCE-8.0.6; do
        if [[ -f $candidate/CMakeLists.txt ]]; then
            juce_dir="$root/$candidate"
            break
        fi
    done
fi
juce_dir=${juce_dir:-$root/.tools/JUCE-8.0.6}
if [[ $action != tests && ! -f $juce_dir/CMakeLists.txt ]]; then
    echo "JUCE 8.0.6 was not found at $juce_dir. Clone it into .tools/JUCE-8.0.6 or set VX_JUCE_DIR." >&2
    exit 1
fi

if [[ $clean == true ]]; then
    rm -rf "$build"
fi

architectures=${VX_MACOS_ARCHITECTURES:-arm64;x86_64}
cmake -S "$root" -B "$build" -G 'Unix Makefiles' \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$(xcrun --find clang)" \
    -DCMAKE_CXX_COMPILER="$(xcrun --find clang++)" \
    -DCMAKE_OSX_ARCHITECTURES="$architectures" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}" \
    -DVX_JUCE_DIR="$juce_dir"

case "$action" in
    configure) ;;
    tests)
        cmake --build "$build" --parallel "$jobs" --target VoxlineTests
        "$build/VoxlineTests"
        ;;
    smoke)
        cmake --build "$build" --parallel "$jobs" --target VoxlineSmoke
        mkdir -p "$root/docs/screenshots"
        VX_SNAPSHOT_DIR="$root/docs/screenshots" "$build/VoxlineSmoke"
        ;;
    build|all)
        cmake --build "$build" --parallel "$jobs" --target Voxline_VST3 Voxline_AU Voxline_Standalone

        artifacts="$build/Voxline_artefacts/Release"
        vst3="$artifacts/VST3/voxline.vst3"
        au="$artifacts/AU/voxline.component"
        app="$artifacts/Standalone/voxline.app"
        dist="$root/dist"
        archive="$dist/voxline-macOS.zip"
        mkdir -p "$dist"
        stage=$(mktemp -d "$dist/.macos.XXXXXXXX")
        trap 'rm -rf "$stage"' EXIT
        mkdir "$stage/bundles"
        IFS=';' read -r -a slices <<< "$architectures"

        for bundle in "$vst3" "$au" "$app"; do
            if [[ ! -d $bundle ]]; then
                echo "Missing macOS bundle: $bundle" >&2
                exit 1
            fi
            copy="$stage/bundles/${bundle##*/}"
            ditto "$bundle" "$copy"
            lipo "$copy/Contents/MacOS/voxline" -verify_arch "${slices[@]}"
            codesign --force --sign - "$copy"
            codesign --verify "$copy"
        done

        mkdir "$stage/bundles/licenses"
        for license in "$root/LICENSE" "$root/THIRD_PARTY.txt" "$root"/third_party/*-LICENSE.txt; do
            if [[ -f $license ]]; then
                cp "$license" "$stage/bundles/licenses/"
            fi
        done
        if [[ -f $juce_dir/LICENSE.md ]]; then
            cp "$juce_dir/LICENSE.md" "$stage/bundles/licenses/JUCE-LICENSE.md"
        fi

        ditto -c -k --sequesterRsrc "$stage/bundles" "$stage/release.zip"
        rm -rf "$dist/voxline.component"
        mv "$stage/bundles/voxline.component" "$dist/voxline.component"
        mv -f "$stage/release.zip" "$archive"
        printf 'VST3 %s\nAU %s\nStandalone %s\nCopied AU %s\nmacOS ZIP %s\n' \
            "$vst3" "$au" "$app" "$dist/voxline.component" "$archive"
        echo 'Copy the AU into ~/Library/Audio/Plug-Ins/Components for Logic Pro and GarageBand.'
        ;;
esac
