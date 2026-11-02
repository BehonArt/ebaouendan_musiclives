#!/bin/bash
# Build eba-recomp's launcher (launcher/) into build/launcher with MSYS2 UCRT64's clang + SDL3.
# Run from Windows:  C:\msys64\usr\bin\bash.exe -lc "/f/dxvk_remix/Claude/eba-recomp/tools/build_launcher.sh"
set -e
export PATH=/ucrt64/bin:$PATH
cd "$(dirname "$0")/.."
cmake -S launcher -B build/launcher -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release ${EBA_VERSION:+-DEBA_VERSION=$EBA_VERSION} > /dev/null
ninja -C build/launcher "$@"
