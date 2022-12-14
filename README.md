c-simplifier
====

C source simplifier for verification tools.

This tool simplifies C source directories given some root functions,
by retaining only the used declarations.

It is developed to extract and verify progressively larger parts of
pKVM, without having to support irrelevant C constructs.

## Installation

### Prerequisites
- GNU C++ compiler 8
- LLVM/clang 10.0
- CMake 3.0

### How to build
```
mkdir build && pushd -q build
cmake -DCMAKE_BUILD_TYPE=Debug ../src
popd -q && make -C build
```


## To Do

- [x] C constructs required for `kvm_pgtable_walk` in `pgtable.c`
- [x] Command line interface and support for `compile_commands.json`
- [x] Support for reproducing directory structure
- [ ] Preprocess file within tool to retain all comments (including licence headers)
- [ ] Mark licence comments, `#include` and `#define` (especially multiline ones!)
- [ ] Fix up tests (and add new ones)
- [ ] Example for README.md

## Funding

This software was developed by the University of Cambridge Computer
Laboratory as part of the Rigorous Engineering of Mainstream Systems
(REMS) project. This project has been partly funded by an EPSRC
Doctoral Training studentship. This project has been partly funded by
Google. This project has received funding from the European Research
Council (ERC) under the European Union's Horizon 2020 research and
innovation programme (grant agreement No 789108, Advanced Grant
ELVER).

## History

The initial implementation of the Language Server source and end-to-end tests
was taken from [cpp-simplifier](https://github.com/logicmachine/cpp-simplifier/).
