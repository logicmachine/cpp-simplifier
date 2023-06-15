###########################################################################
#  Copyright (c) 2022 Dhruv Makwana                                       #
#  All rights reserved.                                                   #
#                                                                         #
#  This software was developed by the University of Cambridge Computer    #
#  Laboratory as part of the Rigorous Engineering of Mainstream Systems   #
#  (REMS) project. This project has been partly funded by an EPSRC        #
#  Doctoral Training studentship. This project has been partly funded by  #
#  Google. This project has received funding from the European Research   #
#  Council (ERC) under the European Union's Horizon 2020 research and     #
#  innovation programme (grant agreement No 789108, Advanced Grant        #
#  ELVER).                                                                #
#                                                                         #
#  BSD 2-Clause License                                                   #
#                                                                         #
#  Redistribution and use in source and binary forms, with or without     #
#  modification, are permitted provided that the following conditions     #
#  are met:                                                               #
#  1. Redistributions of source code must retain the above copyright      #
#     notice, this list of conditions and the following disclaimer.       #
#  2. Redistributions in binary form must reproduce the above copyright   #
#     notice, this list of conditions and the following disclaimer in     #
#     the documentation and/or other materials provided with the          #
#     distribution.                                                       #
#                                                                         #
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''     #
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED      #
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A        #
#  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR    #
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,           #
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       #
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF       #
#  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND    #
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     #
#  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT     #
#  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF     #
#  SUCH DAMAGE.                                                           #
###########################################################################

# This is helpful: https://alereinking.com/blog/how-to-use-cmake-without-the-agonizing-pain-part-1.html

cmake_minimum_required(VERSION 3.16.3)
project(clang-tree-carver)

find_package(LLVM "12.0.0" REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

set(CLANG_LIBS
    clangAnalysis
    clangAST
    clangBasic
    clangDriver
    clangEdit
    clangFrontend
    clangLex
    clangParse
    clangSema
    clangSerialization
    clangTooling)

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB CXX_SOURCES "*.cpp")

# # Whilst it is do-able, and not too icky to link directly with
# # the library instead of calling out to a different process,
# # there the linker-flags had to be copied over into a dune file manually
# # and the library & executable sizes ended up being stupidly large.

# # For binding via OCaml
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#
# add_library(file_objs OBJECT ${CXX_SOURCES})
#
# # For binding via OCaml
# add_library(clang-tree-carver-static STATIC $<TARGET_OBJECTS:file_objs>)
# target_link_libraries(clang-tree-carver-static ${CLANG_LIBS})
# set_target_properties(clang-tree-carver-static
#     PROPERTIES OUTPUT_NAME clang-tree-carver)
#
# # For binding via OCaml
# add_library(clang-tree-carver-shared SHARED $<TARGET_OBJECTS:file_objs>)
# target_link_libraries(clang-tree-carver-shared ${CLANG_LIBS})
# set_target_properties(clang-tree-carver-shared
#     PROPERTIES OUTPUT_NAME clang-tree-carver)

# For testing/transition/standalone use
add_executable(clang-tree-carve ${CXX_SOURCES})
target_link_libraries(clang-tree-carve ${CLANG_LIBS})
