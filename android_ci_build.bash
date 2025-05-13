!/bin/bash
set -e

function cmake_build {
    local ANDROID_ABI=$1
    local BUILD_DIR="build_${ANDROID_ABI}"
    
    echo "Building for $ANDROID_ABI..."
    mkdir -p $BUILD_DIR
    pushd $BUILD_DIR
    
    cmake "$GITHUB_WORKSPACE" \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_LATEST_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_PLATFORM=android-29 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_MAKE_PROGRAM="$ANDROID_NDK_LATEST_HOME/prebuilt/linux-x86_64/bin/ninja" \
        -GNinja
    
    cmake --build . --target nether_pathfinder -j $(nproc)
    
     Strip debug symbols
    "$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" \
        libnether_pathfinder.so
    
    cp libnether_pathfinder.so ../libnether_pathfinder-${ANDROID_ABI}.so
    popd
    rm -rf $BUILD_DIR
}

cmake_build arm64-v8a