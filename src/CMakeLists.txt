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
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
if(COMPILER_SUPPORTS_CXX14)
	add_definitions("-std=c++14")
elseif(COMPILER_SUPPORTS_CXX1Y)
	add_definitions("-std=c++1y")
endif()

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
