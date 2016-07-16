cpp-simplifier
====

C++ source simplifier for competitive programming.

This tool expands double-quoted inclusions and removes unused declarations.

## Installation

### Prerequisites
- GNU C++ compiler 4.9
- LLVM/clang 3.8
- CMake 3.0
- Boost 1.55

### How to build
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src
make
```

