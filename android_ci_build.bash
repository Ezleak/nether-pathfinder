#!/bin/bash
set -eo pipefail

sudo apt-get update && sudo apt-get install -y ninja-build

function cmake_build {
    local ANDROID_ABI=$1
    local BUILD_DIR="build_${ANDROID_ABI}"
    local NDK_VERSION="25.2.9519653"
    local NDK_PATH="/usr/local/lib/android/sdk/ndk/${NDK_VERSION}"
    local TOOLCHAIN_FILE="${NDK_PATH}/build/cmake/android.toolchain.cmake"
    local SYSTEM_NINJA=$(which ninja)

    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        sdkmanager --install "ndk;${NDK_VERSION}" --channel=3
        export NDK_PATH="/usr/local/lib/android/sdk/ndk/${NDK_VERSION}"
        TOOLCHAIN_FILE="${NDK_PATH}/build/cmake/android.toolchain.cmake"
    fi

    mkdir -p $BUILD_DIR
    pushd $BUILD_DIR
    
    cmake "$GITHUB_WORKSPACE" \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_PLATFORM=android-29 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_MAKE_PROGRAM="$SYSTEM_NINJA" \
        -GNinja
    
    $SYSTEM_NINJA -j $(( $(nproc) * 2 )) nether_pathfinder
    
    llvm-strip libnether_pathfinder.so
    cp libnether_pathfinder.so ../libnether_pathfinder-${ANDROID_ABI}.so
    
    popd
    rm -rf $BUILD_DIR
}

cmake_build arm64-v8a
