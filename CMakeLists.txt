cmake_minimum_required(VERSION 3.16)
project(nether_pathfinder)

find_package(JNI COMPONENTS JNI_INCLUDE_DIRS)
include_directories(${JNI_INCLUDE_DIRS})

find_package(PkgConfig) # not required
if(PkgConfig_FOUND)
    pkg_check_modules(GBENCHMARK IMPORTED_TARGET benchmark) # not required
endif()

set(CMAKE_C_COMPILER_TARGET ${PATHFINDER_TARGET})
set(CMAKE_CROSSCOMPILING True)
set(WITH_SANITIZER False)
set(ZLIBNG_ENABLE_TESTS False)
set(ZLIB_ENABLE_TESTS False)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(zlib-ng)

enable_language(C)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 20)
set(CAMKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -integrated-as -mllvm -polly -mllvm -polly-vectorizer=stripmine -mllvm -polly-invariant-load-hoisting -mllvm -polly-run-inliner -mllvm -polly-run-dce -mllvm -polly-invariant-load-hoisting -flto=thin -mllvm -polly-run-inliner -mllvm -polly-run-dce -mllvm -polly-parallel -mllvm -polly-scheduling=dynamic -flto=thin -fno-emulated-tls -fwhole-program-vtables -fdata-sections -ffunction-sections -fmerge-all-constants -mllvm -polly-detect-keep-going -mllvm -polly-ast-use-context -march=armv8-a+simd -fvisibility=hidden -Wall -Wextra -fPIC -DPIC -ferror-limit=0 -Wno-return-type -Wno-unused-parameter -Wno-c++98-compat -Wno-c++98-compat-pedantic -DGLM_FORCE_INTRINSICS -target aarch64-linux-android -U_FORTIFY_SOURCE -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -integrated-as -mllvm -polly -mllvm -polly-vectorizer=stripmine -mllvm -polly-invariant-load-hoisting -mllvm -polly-run-inliner -mllvm -polly-run-dce -mllvm -polly-invariant-load-hoisting -flto=thin -mllvm -polly-run-inliner -mllvm -polly-run-dce -mllvm -polly-parallel -mllvm -polly-scheduling=dynamic -flto=thin -fno-emulated-tls -fwhole-program-vtables -fdata-sections -ffunction-sections -fmerge-all-constants -mllvm -polly-detect-keep-going -mllvm -polly-ast-use-context -march=armv8-a+simd -fvisibility=hidden -Wall -Wextra -fPIC -DPIC -ferror-limit=0 -Wno-return-type -Wno-unused-parameter -Wno-c++98-compat -Wno-c++98-compat-pedantic -DGLM_FORCE_INTRINSICS -target aarch64-linux-android -U_FORTIFY_SOURCE -pthread -D_LIBCPP_HAS_NOCOND_CLOCKWAIT")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc -fuse-ld=lld -flto=thin -Wl,-plugin-opt=-emulated-tls=0 -lc++_static -lc++abi")
set(CMAKE_ANDROID_LD "lld")
set(ANDROID_STL c++_static)     # Try newer versions

file(GLOB_RECURSE SRC src/*)
list(REMOVE_ITEM SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/baritone_pather.cpp")

if (TESTING)
    add_executable(nether_pathfinder ${SRC} src/main.cpp)
    if(GBENCHMARK_FOUND)
        #add_executable(nether_pathfinder_benchmark ${SRC} src/benchmark_main.cpp)
        #target_link_libraries(nether_pathfinder_benchmark PkgConfig::GBENCHMARK)
        #target_include_directories(nether_pathfinder_benchmark PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/zlib-ng)
    endif()
else()
    #add_executable(baritone_pather ${SRC} src/baritone_pather.cpp)
    #target_include_directories(baritone_pather PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/zlib-ng)
    #target_link_libraries(baritone_pather PRIVATE zlibstatic)

    add_library(nether_pathfinder SHARED ${SRC})
endif()
target_include_directories(nether_pathfinder PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/zlib-ng)
target_link_libraries(nether_pathfinder PRIVATE zlibstatic)

if (USE_ABSEIL)
    add_subdirectory(abseil-cpp)
    target_link_libraries(nether_pathfinder absl::flat_hash_map)
endif()
