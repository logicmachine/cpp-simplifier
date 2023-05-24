#!/usr/bin/env bash

# ONLY FOR USE FOR THIS TEST-SUITE. Please configure your build system (or use
# a tool like Bear) for your own project.

# Helper script to generate compile_commands.json
# Test harness (`run_test.py`) creates compile_commands.json already.
# This is only a helper for running individual test outside of test harness.

set -euo pipefail

echo "[" > compile_commands.json

readarray -t files < <(find test -name '*.c*')

for input_file in ${files[@]}; do
  input_file="${input_file/\.\//}"
  echo "    {" >> compile_commands.json
  echo "        \"directory\": \"$PWD\"," >> compile_commands.json
  echo "        \"command\": \"clang -c ${input_file}\"," >> compile_commands.json
  echo "        \"file\": \"${input_file}\"" >> compile_commands.json
  echo "    }," >> compile_commands.json
done

echo "    {" >> compile_commands.json
echo "        \"directory\": \"$PWD\"" >> compile_commands.json
echo "        \"command\": \"clang -c last_doesnt_exist.cpp\"," >> compile_commands.json
echo "        \"file\": \"last_doesnt_exist.cpp\"" >> compile_commands.json
echo "    }" >> compile_commands.json
echo "]" >> compile_commands.json
