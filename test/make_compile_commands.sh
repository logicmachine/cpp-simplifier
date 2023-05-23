#!/usr/bin/env bash

# ONLY FOR USE FOR THIS TEST-SUITE. Please configure your build system (or use
# a tool like Bear) for your own project.

# Helper script to generate compile_commands.json
# Test harness (`run_test.py`) creates compile_commands.json already.
# This is only a helper for running individual test outside of test harness.

set -euo pipefail

DIRECTORY=$(dirname $PWD)

echo "[" > compile_commands.json

for input_file in cxx_record/*.in.cpp  function/*.in.cpp  record/*.in.cpp  simple/*.in.cpp typedef/*.in.cpp; do
  echo '    {' >> compile_commands.json
  echo "        directory: \"$DIRECTORY\"," >> compile_commands.json
  echo "        command: \"clang -c test/$input_file\"," >> compile_commands.json
  echo "        file: \"test/$input_file\"" >> compile_commands.json
  echo '    },' >> compile_commands.json
done

echo '    {' >> compile_commands.json
echo '        directory: "/home/dhruv/c-simplifier",' >> compile_commands.json
echo "        command: \"clang -c last_doesnt_exist.cpp\"," >> compile_commands.json
echo "        file: \"last_doesnt_exist.cpp\"" >> compile_commands.json
echo '    }' >> compile_commands.json
echo ']' >> compile_commands.json
