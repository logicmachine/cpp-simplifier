#!/usr/bin/env bash
set -euo pipefail

headache -c etc/headache_config -h LICENCE \
    .gitignore .travis.yml README.md \
    cpp/src/reachability_analyzer.hpp \
    cpp/src/simplifier.cpp \
    cpp/src/reachability_analyzer.cpp \
    cpp/src/main.cpp \
    cpp/src/kept_lines.hpp \
    cpp/src/source_marker.hpp \
    cpp/src/simplifier.hpp \
    cpp/src/traverser.hpp \
    `find cpp/test -name '*.py'`

headache -c etc/headache_config -h etc/BSD-2-Clause \
    cpp/src/CMakeLists.txt \
    cpp/src/debug.hpp \
    cpp/src/debug_printers.cpp \
    cpp/src/debug_printers.hpp \
    cpp/src/source_range_hash.hpp \
    `find lib  -name '*.ml*'` \
    `find bin  -name '*.ml'`  \
    `find test -name '*.ml'`
