#!/usr/bin/env bash

set -e

# 新增NDK构建函数
function do_ndk_build {
    local java_root=$1
    local target=$2
    local output_path=$3
    local ndk_path=$4
    local api_level=$5
    mkdir build
    pushd build
    echo "Building $target (Android API $api_level)..."

    cmake -G Ninja $java_root/.. \
      -DCMAKE_TOOLCHAIN_FILE="$ndk_path/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-$api_level \
      -DCMAKE_C_COMPILER=$ndk_path/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$api_level-clang \
      -DCMAKE_CXX_COMPILER=$ndk_path/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$api_level-clang++ \
      -DCMAKE_BUILD_TYPE=Release

    ninja -j `nproc`
    cp libnether_pathfinder.so ../$output_path
    popd
    rm -rf build
}

# 原Zig编译目标
#do_build $1 x86_64-linux-gnu libnether_pathfinder-x86_64.so
#do_build $1 aarch64-linux-gnu libnether_pathfinder-aarch64.so
#do_build $1 x86_64-macos-none libnether_pathfinder-x86_64.dylib
#do_build $1 aarch64-macos-none libnether_pathfinder-aarch64.dylib
#do_build $1 x86_64-windows-gnu nether_pathfinder-x86_64.dll
#do_build $1 aarch64-windows-gnu nether_pathfinder-aarch64.dll

# 新增Android NDK构建(需设置环境变量ANDROID_NDK和API_LEVEL)
if [ -n "$ANDROID_NDK" ] && [ -n "$API_LEVEL" ]; then
    do_ndk_build $1 aarch64-linux-android libnether_pathfinder-arm64-android.so $ANDROID_NDK $API_LEVEL
else
    echo "Skip Android NDK build: ANDROID_NDK or API_LEVEL not set"
fi
