#!/usr/bin/env bash
set -euo pipefail

headache -c etc/headache_config -r \
    .gitignore .travis.yml README.md \
    cpp/src/CMakeLists.txt \
    `find cpp/src -name '*.cpp'` \
    `find cpp/src -name '*.hpp'` \
    `find cpp/test -name '*.py'` \
    `find lib  -name '*.ml*'` \
    `find bin  -name '*.ml'`  \
    `find test -name '*.ml'`
