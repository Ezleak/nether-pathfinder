#!/bin/bash
# set -e

cmake_build () {
  ANDROID_ABI=$1
  mkdir -p build
  cd build
  cmake $GITHUB_WORKSPACE -DPATHFINDER_TARGET=aarch64-linux-android -DCMAKE_C_COMPILER=$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang -DCMAKE_CXX_COMPILER=$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang++ -DCMAKE_AR=$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar -DCMAKE_RANLIB=$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib -DANDROID_PLATFORM=24 -DANDROID_ABI=$ANDROID_ABI -DCMAKE_SYSTEM_NAME=Android -DANDROID_TOOLCHAIN=clang -DANDROID_ARM_MODE=arm -DCMAKE_MAKE_PROGRAM=$ANDROID_NDK_LATEST_HOME/prebuilt/linux-x86_64/bin/make -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_LATEST_HOME/build/cmake/android.toolchain.cmake 
  cmake --build . --config Release --parallel 4
  $ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip $GITHUB_WORKSPACE/build/libnether_pathfinder.so
}

cmake_build arm64-v8a
