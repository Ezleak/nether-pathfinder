#!/usr/bin/env bash

set -e

function do_build {
    mkdir build
    pushd build
    local java_root=$1
    local target=$2
    local output_path=$3
    echo "Building $target..."

    # 特殊处理Android目标
    if [[ $target == *"android"* ]]; then
        wget https://dl.google.com/android/repository/android-ndk-r25c-linux.zip
        unzip android-ndk-r25c-linux.zip >> /dev/null
        export NDK_ROOT=${NDK_ROOT:-$java_root/../android-ndk}
        $NDK_ROOT/ndk-build \
            NDK_PROJECT_PATH=. \
            NDK_APPLICATION_MK=../Application.mk \
            APP_BUILD_SCRIPT=../Android.mk \
            APP_ABI=$(echo $target | cut -d'-' -f1) \
            APP_PLATFORM=android-21 \
            -j `nproc`
        cp libs/$(echo $target | cut -d'-' -f1)/libnether_pathfinder.so ../$output_path
    else
        # 原有构建逻辑
        CXXFLAGS="-target $target" cmake -G Ninja $java_root/.. \
          -DPATHFINDER_TARGET=$target \
          -DCMAKE_C_COMPILER=$(realpath $java_root/zigcc.sh) -DCMAKE_CXX_COMPILER=$(realpath $java_root/zigcxx.sh) \
          -DCMAKE_AR=$(realpath $java_root/zigar.sh) \
          -DCMAKE_RANLIB=$(realpath $java_root/zigranlib.sh) \
          -DCMAKE_BUILD_TYPE=Release

        ninja -j `nproc`
        cp libnether_pathfinder.so ../$output_path
    fi

    popd
    rm -rf build
}

# 创建必要的Android构建文件
cat > Android.mk <<EOF
LOCAL_PATH := \$(call my-dir)
include \$(CLEAR_VARS)
LOCAL_MODULE := nether_pathfinder
LOCAL_SRC_FILES := src/**.cpp  # 修改为实际源文件路径
include \$(BUILD_SHARED_LIBRARY)
EOF

cat > Application.mk <<EOF
APP_ABI := arm64-v8a
APP_PLATFORM := android-21
APP_STL := c++_static
EOF

# 原有构建目标
do_build $1 aarch64-linux-android libnether_pathfinder-aarch64.so
# 将aarch64-linux-gnu改为Android目标
#do_build $1 arm64-v8a-android libnether_pathfinder-arm64-v8a.so
