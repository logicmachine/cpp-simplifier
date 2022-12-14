cmake_minimum_required(VERSION 3.0)
project(cpp-simplifier)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIR})
link_directories(${LLVM_LINK_DIR})

find_package(Threads REQUIRED)

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB CXX_SOURCES "*.cpp")

add_compile_definitions("DEBUG=$<CONFIG:Debug>")
add_executable(cpp-simplifier ${CXX_SOURCES})
target_link_libraries(
	cpp-simplifier
	-Wl,--start-group
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
	clangTooling
	-Wl,--end-group
	${LLVM_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

install(TARGETS cpp-simplifier RUNTIME DESTINATION bin)
