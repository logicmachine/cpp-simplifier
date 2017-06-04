cpp-simplifier
====

C++ source simplifier for competitive programming.

This tool expands double-quoted inclusions and removes unused declarations.

For example, when there are two C++ source/header files:

```c++
// foo.hpp
struct A {
    int foo() const { return 10; }
    int bar() const { return 20; }
};
```

```c++
// main.cpp
#include <vector>
#include "foo.hpp"
int main(){
    std::vector<A> v(2);
    for(const auto& a : v){
        a.foo();
    }
    return 0;
}
```

`/path/to/cpp-simplifier main.cpp` generates the code in below:

```c++
#include <vector>
struct A {
    int foo() const { return 10; }
};
int main(){
    std::vector<A> v(2);
    for(const auto& a : v){
        a.foo();
    }
    return 0;
}
```

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

