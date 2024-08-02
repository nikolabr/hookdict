set(CMAKE_SYSTEM_NAME "Windows")
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(VCPKG_TARGET_TRIPLET "x86-mingw-static")
set(VCPKG_CMAKE_SYSTEM_NAME "")

set(CMAKE_C_COMPILER "/opt/llvm-mingw-20240619-msvcrt-ubuntu-20.04-x86_64/bin/i686-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "/opt/llvm-mingw-20240619-msvcrt-ubuntu-20.04-x86_64/bin/i686-w64-mingw32-clang++")

set(CMAKE_C_FLAGS "-fms-extensions -static -static-libgcc")
set(CMAKE_CXX_FLAGS "-fms-extensions -stdlib=libc++ -static -static-libgcc -static-libstdc++")

set(VCPKG_TARGET_ARCHITECTURE "x86")
set(VCPKG_CRT_LINKAGE "dynamic")

set(VCPKG_HOST_TRIPLET "x64-linux")
set(VCPKG_TARGET_TRIPLET "x86-mingw-static")
