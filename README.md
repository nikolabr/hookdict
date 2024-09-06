# hookdict

A WIP tool for making looking up text inside of untranslated visual novels easier. Currently only text-hooking works.

![Screenshot from 2024-08-26 01-08-21](https://github.com/user-attachments/assets/801149ac-d81b-42c3-874a-3cdf2bf70cfb)

## Building

This project is built using xmake: https://xmake.io/

### Linux

The llvm-mingw toolchain is the recommended toolchain for cross-compiling on Linux.

To configure the xmake platform and toolchain:

```
xmake f -a i386 -p mingw --toolchain=mingw --mingw=<path to llvm-mingw toolchain> -c
```

### Windows
