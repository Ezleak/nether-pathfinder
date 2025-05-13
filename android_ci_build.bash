#!/bin/bash
set -e

sudo apt-get update && sudo apt-get install -y ninja-build

function cmake_build {
    local ANDROID_ABI=$1
    local BUILD_DIR="build_${ANDROID_ABI}"
    local NDK_VERSION="25.2.9519653"
    
    export ANDROID_NDK_LATEST_HOME="/usr/local/lib/android/sdk/ndk/${NDK_VERSION}"
    
    if [ ! -f "${ANDROID_NDK_LATEST_HOME}/prebuilt/linux-x86_64/bin/ninja" ]; then
        echo "Error: NDK ninja not found"
        exit 1
    fi

    mkdir -p $BUILD_DIR
    pushd $BUILD_DIR
    
    cmake "$GITHUB_WORKSPACE" \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_LATEST_HOME}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_PLATFORM=android-29 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_MAKE_PROGRAM="${ANDROID_NDK_LATEST_HOME}/prebuilt/linux-x86_64/bin/ninja" \
        -GNinja
    
    "${ANDROID_NDK_LATEST_HOME}/prebuilt/linux-x86_64/bin/ninja" -j $(nproc) nether_pathfinder
    
    llvm-strip libnether_pathfinder.so
    cp libnether_pathfinder.so ../libnether_pathfinder-${ANDROID_ABI}.so
    popd
    rm -rf $BUILD_DIR
}

cmake_build arm64-v8a
