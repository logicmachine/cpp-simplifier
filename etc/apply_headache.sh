#!/usr/bin/env bash
set -euo pipefail

headache -c etc/headache_config -h LICENCE \
    .gitignore .travis.yml README.md cmake/FindLLVM.cmake src/CMakeLists.txt \
    `find src -name '*.cpp'` \
    `find src -name '*.hpp' ! -path 'src/debug.hpp'` \
    `find test -name '*.py'`

headache -c etc/headache_config -h etc/BSD-2-Clause src/debug.hpp
