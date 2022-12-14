#!/usr/bin/env bash
set -euo pipefail

headache -c etc/headache_config -r \
    .gitignore .travis.yml README.md cmake/FindLLVM.cmake src/CMakeLists.txt \
    `find src -name '*.cpp'` \
    `find src -name '*.hpp' ! -path 'src/debug.hpp'` \
    `find test -name '*.py'` \
    src/debug.hpp
