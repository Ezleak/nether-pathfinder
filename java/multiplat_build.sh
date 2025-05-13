#!/usr/bin/env bash

set -e

function do_build {
    mkdir build
    pushd build
    local java_root=$1
    local target=$2
    local output_path=$3
    echo "Building $target..."

    CXXFLAGS="-target $target" cmake -G Ninja $java_root/.. \
      -DPATHFINDER_TARGET=$target \
      -DCMAKE_C_COMPILER=$(realpath $java_root/zigcc.sh) -DCMAKE_CXX_COMPILER=$(realpath $java_root/zigcxx.sh) \
      -DCMAKE_AR=$(realpath $java_root/zigar.sh) \
      -DCMAKE_RANLIB=$(realpath $java_root/zigranlib.sh) \
      -DCMAKE_BUILD_TYPE=Release

    ninja -j `nproc`

    cp libnether_pathfinder.so ../$output_path
    popd
    rm -rf build
}

do_build $1 aarch64-linux-gnu libnether_pathfinder-aarch64.so
do_build $1 aarch64-Android libnether_pathfinder-aarch64.so
