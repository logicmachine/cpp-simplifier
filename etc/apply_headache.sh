#!/usr/bin/env bash
set -euo pipefail

.travis.yml
LICENSE
README.md
cmake/FindLLVM.cmake
src/CMakeLists.txt
src/debug.hpp
src/inclusion_unroller.cpp
src/inclusion_unroller.hpp
src/main.cpp
src/reachability_analyzer.cpp
src/reachability_analyzer.hpp
src/reachability_marker.hpp
src/simplifier.cpp
src/simplifier.hpp

headache -c etc/headache_config -h LICENCE \
    .gitignore .travis.yml README.md cmake/FindLLVM.cmake src/CMakeLists.txt \
    `find src -name '*.cpp'` \
    `find src -name '*.hpp' ! -path 'src/debug.hpp'` \
    `find test -name '.py'`

headache -c etc/headache_config -h etc/BSD-2-Clause src/debug.hpp
