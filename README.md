<!--                                                                                  -->
<!--  The following parts of C-simplifier contain new code released under the         -->
<!--  BSD 2-Clause License:                                                           -->
<!--  * `src/debug.hpp`                                                               -->
<!--                                                                                  -->
<!--  Copyright (c) 2022 Dhruv Makwana                                                -->
<!--  All rights reserved.                                                            -->
<!--                                                                                  -->
<!--  This software was developed by the University of Cambridge Computer             -->
<!--  Laboratory as part of the Rigorous Engineering of Mainstream Systems            -->
<!--  (REMS) project. This project has been partly funded by an EPSRC                 -->
<!--  Doctoral Training studentship. This project has been partly funded by           -->
<!--  Google. This project has received funding from the European Research            -->
<!--  Council (ERC) under the European Union's Horizon 2020 research and              -->
<!--  innovation programme (grant agreement No 789108, Advanced Grant                 -->
<!--  ELVER).                                                                         -->
<!--                                                                                  -->
<!--  BSD 2-Clause License                                                            -->
<!--                                                                                  -->
<!--  Redistribution and use in source and binary forms, with or without              -->
<!--  modification, are permitted provided that the following conditions              -->
<!--  are met:                                                                        -->
<!--  1. Redistributions of source code must retain the above copyright               -->
<!--     notice, this list of conditions and the following disclaimer.                -->
<!--  2. Redistributions in binary form must reproduce the above copyright            -->
<!--     notice, this list of conditions and the following disclaimer in              -->
<!--     the documentation and/or other materials provided with the                   -->
<!--     distribution.                                                                -->
<!--                                                                                  -->
<!--  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''              -->
<!--  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED               -->
<!--  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A                 -->
<!--  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR             -->
<!--  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,                    -->
<!--  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT                -->
<!--  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF                -->
<!--  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND             -->
<!--  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,              -->
<!--  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT              -->
<!--  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF              -->
<!--  SUCH DAMAGE.                                                                    -->
<!--                                                                                  -->
<!--  All other parts involve adapted code, with the new code subject to the          -->
<!--  above BSD 2-Clause licence and the original code subject to its MIT             -->
<!--  licence.                                                                        -->
<!--                                                                                  -->
<!--  The MIT License (MIT)                                                           -->
<!--                                                                                  -->
<!--  Copyright (c) 2016 Takaaki Hiragushi                                            -->
<!--                                                                                  -->
<!--  Permission is hereby granted, free of charge, to any person obtaining a copy    -->
<!--  of this software and associated documentation files (the "Software"), to deal   -->
<!--  in the Software without restriction, including without limitation the rights    -->
<!--  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       -->
<!--  copies of the Software, and to permit persons to whom the Software is           -->
<!--  furnished to do so, subject to the following conditions:                        -->
<!--                                                                                  -->
<!--  The above copyright notice and this permission notice shall be included in all  -->
<!--  copies or substantial portions of the Software.                                 -->
<!--                                                                                  -->
<!--  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      -->
<!--  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        -->
<!--  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     -->
<!--  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          -->
<!--  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   -->
<!--  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   -->
<!--  SOFTWARE.                                                                       -->
<!--                                                                                  -->

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

### Build

```
mkdir build && pushd -q build
cmake -DCMAKE_BUILD_TYPE=Debug ../src
popd -q && make -C build
```

You can type `make -C help` for more info too.

## Usage

See `/path/to/c-simplifier --help` for full list of options.

Suppose you have a `compile_commands.json` file like the one below in some `/repo/root`.
(Note: for projects which do not use build systems that natively support the generation
of such a file, use [Bear](https://github.com/rizsotto/Bear/).)

```json
[
    {
        directory: "/repo/root",
        command: "/prebuilts/linux-x86/clang-r433403b/bin/clang -Wp,-MMD,arch/arm64/kvm/hyp/nvhe/../.pgtable.nvhe.o.d  -nostdinc -isystem /prebuilts/linux-x86/clang-r433403b/lib64/clang/13.0.3/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h -D__KERNEL__ -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT= -Qunused-arguments -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu -integrated-as -Werror=unknown-warning-option -mgeneral-regs-only -DCONFIG_CC_HAS_K_CONSTRAINT=1 -Wno-psabi -fno-asynchronous-unwind-tables -fno-unwind-tables -mbranch-protection=pac-ret+leaf+bti -Wa,-march=armv8.5-a -DARM64_ASM_ARCH='\"armv8.5-a\"' -DKASAN_SHADOW_SCALE_SHIFT= -fno-delete-null-pointer-checks -Wno-frame-address -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -mno-global-merge -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Wno-initializer-overrides -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-pointer-to-enum-cast -Wno-tautological-constant-out-of-range-compare -mstack-protector-guard=sysreg -mstack-protector-guard-reg=sp_el0 -mstack-protector-guard-offset=1272 -I./arch/arm64/kvm/hyp/include -fno-stack-protector -DDISABLE_BRANCH_PROFILING -D__KVM_NVHE_HYPERVISOR__ -D__DISABLE_EXPORTS -D__DISABLE_TRACE_MMIO__    -DKBUILD_MODFILE='\"arch/arm64/kvm/hyp/nvhe/pgtable.nvhe\"' -DKBUILD_BASENAME='\"pgtable.nvhe\"' -DKBUILD_MODNAME='\"pgtable.nvhe\"' -D__KBUILD_MODNAME=kmod_pgtable.nvhe -c -o arch/arm64/kvm/hyp/nvhe/../pgtable.nvhe.o -x cpp-output preprocessed.c",
        file: "preprocessed.c"
    },
    {
        directory: "/repo/root",
        command: "/prebuilts/linux-x86/clang-r433403b/bin/clang -Wp,-MMD,arch/arm64/kvm/hyp/nvhe/../.pgtable.nvhe.o.d  -nostdinc -isystem /prebuilts/linux-x86/clang-r433403b/lib64/clang/13.0.3/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h -D__KERNEL__ -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT= -Qunused-arguments -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu -integrated-as -Werror=unknown-warning-option -mgeneral-regs-only -DCONFIG_CC_HAS_K_CONSTRAINT=1 -Wno-psabi -fno-asynchronous-unwind-tables -fno-unwind-tables -mbranch-protection=pac-ret+leaf+bti -Wa,-march=armv8.5-a -DARM64_ASM_ARCH='\"armv8.5-a\"' -DKASAN_SHADOW_SCALE_SHIFT= -fno-delete-null-pointer-checks -Wno-frame-address -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -mno-global-merge -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Wno-initializer-overrides -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-pointer-to-enum-cast -Wno-tautological-constant-out-of-range-compare -mstack-protector-guard=sysreg -mstack-protector-guard-reg=sp_el0 -mstack-protector-guard-offset=1272 -I./arch/arm64/kvm/hyp/include -fno-stack-protector -DDISABLE_BRANCH_PROFILING -D__KVM_NVHE_HYPERVISOR__ -D__DISABLE_EXPORTS -D__DISABLE_TRACE_MMIO__    -DKBUILD_MODFILE='\"arch/arm64/kvm/hyp/nvhe/pgtable.nvhe\"' -DKBUILD_BASENAME='\"pgtable.nvhe\"' -DKBUILD_MODNAME='\"pgtable.nvhe\"' -D__KBUILD_MODNAME=kmod_pgtable.nvhe -c -o arch/arm64/kvm/hyp/nvhe/../pgtable.nvhe.o arch/arm64/kvm/hyp/nvhe/../pgtable.c",
        file: "arch/arm64/kvm/hyp/nvhe/../pgtable.c"
    }
]
```

Then simply run the below commands in `/repo/root`

```sh
/path/to/c-simplifier/build/c-simplifier -d -r kvm_pgtable_walk -o preprocessed.cutdown.c preprocessed.c  --extra-arg=-Wno-unused-value --extra-arg=-Wno-misleading-indentation 2> preprocessed.cutdown.log
```

```sh
/path/to/c-simplifier/build/c-simplifier -d -r kvm_pgtable_walk -o pgtable.cutdown.c arch/arm64/kvm/hyp/nvhe/../pgtable.c  --extra-arg=-Wno-unused-value --extra-arg=-Wno-misleading-indentation 2> pgtable.cutdown.log
```

In the `/tmp` directory you will then find:

```sh
# grep "Tmp dir:" *.cutdown.log ==> /tmp/c-simplifier-55b854
# find /tmp/c-simplifier-55b854 -type f
/tmp/c-simplifier-55b854/arch/arm64/include/asm/kvm_pgtable.h
/tmp/c-simplifier-55b854/arch/arm64/include/asm/kvm_host.h
/tmp/c-simplifier-55b854/arch/arm64/include/asm/cpufeature.h
/tmp/c-simplifier-55b854/arch/arm64/include/asm/kvm_asm.h
/tmp/c-simplifier-55b854/arch/arm64/kvm/hyp/include/nvhe/memory.h
/tmp/c-simplifier-55b854/arch/arm64/kvm/hyp/pgtable.c
/tmp/c-simplifier-55b854/arch/arm64/kvm/hyp/inline_funcptr.h
/tmp/c-simplifier-55b854/include/asm-generic/int-ll64.h
/tmp/c-simplifier-55b854/include/linux/types.h
/tmp/c-simplifier-55b854/include/linux/stddef.h
/tmp/c-simplifier-55b854/include/uapi/asm-generic/int-ll64.h
```

## To Do

- [x] C constructs required for `kvm_pgtable_walk` in `pgtable.c`
- [x] Command line interface and support for `compile_commands.json`
- [x] Support for reproducing directory structure
- [ ] Mark `#include` and `#define` (especially multiline ones!)
- [ ] Preprocess file within tool to retain all comments (including licence headers)
- [ ] Example for README.md
- [ ] Fix up tests (and add new ones)
- [ ] Update .travis.yml

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

The initial implementation and tests
were taken from [cpp-simplifier](https://github.com/logicmachine/cpp-simplifier/).
