#!/bin/bash
# Configure and build a recompiled tree with MSYS2 UCRT64's clang + SDL3.
# RECOMP_DIR picks the tree (default build/recomp, EBA's; build/oe1/recomp,
# build/oe2/recomp); each links the port's own code (cmake/eba.cmake). Extra
# args go to ninja.
# Run from Windows:  C:\msys64\usr\bin\bash.exe -lc "RECOMP_DIR=build/oe1/recomp /f/dxvk_remix/Claude/eba-recomp/tools/build.sh"
set -e
export PATH=/ucrt64/bin:$PATH
cd "$(dirname "$0")/.."
dir=${RECOMP_DIR:-build/recomp}
cmake -S "$dir" -B "$dir/b" -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release \
    -DNDSKIT_EXTRA="$PWD/cmake/eba.cmake" > /dev/null
ninja -C "$dir/b" "$@"
