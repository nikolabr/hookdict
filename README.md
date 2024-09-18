# hookdict

A WIP tool for making looking up text inside of untranslated visual novels easier. Currently only text-hooking works.

![Screenshot from 2024-08-26 01-08-21](https://github.com/user-attachments/assets/801149ac-d81b-42c3-874a-3cdf2bf70cfb)

## Building

This project is built using xmake: https://xmake.io/

### Linux

Create a conan profile for cross-compiling:

```
[settings]
os=Windows
compiler=gcc
compiler.version=14
compiler.libcxx=libstdc++11
compiler.threads=win32
compiler.cppstd=gnu20
arch=x86
build_type=Debug

[buildenv]
CC=i686-w64-mingw32-gcc
CXX=i686-w64-mingw32-g++
LD=i686-w64-mingw32-ld
```

Install the required dependencies with:
`conan install . --build=missing --profile=mingw --output-folder=build`

Configure and build with CMake:
`cmake --preset conan-debug`
`cmake --build build`

### Windows
