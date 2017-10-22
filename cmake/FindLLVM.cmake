# - Find LLVM
# Find LLVM libraries
# This module defines:
#  LLVM_FOUND, If false, LLVM is unavailable on this system
#  LLVM_INCLUDE_DIR, where to find headers of LLVM
#  LLVM_LINK_DIR, where to find libraries of LLVM
#  LLVM_LIBRARIES, the libraries needed to use LLVM

find_program(
	LLVM_CONFIG_PROGRAM
	NAMES "llvm-config-4.0" "llvm-config")

if(LLVM_CONFIG_PROGRAM)
	# Extract include directories from cxxflags
	execute_process(
		COMMAND ${LLVM_CONFIG_PROGRAM} "--cxxflags"
		OUTPUT_VARIABLE LLVM_CXXFLAGS)
	string(
		REGEX MATCHALL "-I([^ \r\n\"]+|\"[^\"]+\")"
		LLVM_INCLUDE_DIR_OPTIONS
		"${LLVM_CXXFLAGS}")
	foreach(MATCH ${LLVM_INCLUDE_DIR_OPTIONS})
		string(REGEX REPLACE "(^-I\"?|\"$)" "" TRIMMED ${MATCH})
		set(LLVM_INCLUDE_DIR ${LLVM_INCLUDE_DIR} ${TRIMMED})
	endforeach()
	# Extract library directories from ldflags
	execute_process(
		COMMAND ${LLVM_CONFIG_PROGRAM} "--ldflags"
		OUTPUT_VARIABLE LLVM_LDFLAGS)
	string(
		REGEX MATCHALL "-L([^ \r\n\"]+|\"[^\"]+\")"
		LLVM_LINK_DIR_OPTIONS
		"${LLVM_LDFLAGS}")
	foreach(MATCH ${LLVM_LINK_DIR_OPTIONS})
		string(REGEX REPLACE "(^-L\"?|\"$)" "" TRIMMED ${MATCH})
		set(LLVM_LINK_DIR ${LLVM_INCLUDE_DIR} ${TRIMMED})
	endforeach()
	# Extract library names from libs
	execute_process(
		COMMAND ${LLVM_CONFIG_PROGRAM} "--libs"
		OUTPUT_VARIABLE LLVM_LIBS)
	string(
		REGEX MATCHALL "-l([^ \r\n\"]+|\"[^\"]+\")"
		LLVM_LIBRARY_OPTIONS
		"${LLVM_LIBS}")
	foreach(MATCH ${LLVM_LIBRARY_OPTIONS})
		string(REGEX REPLACE "(^-l\"?|\"$)" "" TRIMMED ${MATCH})
		find_library(LIBRARY_PATH NAMES ${TRIMMED} PATHS ${LLVM_LINK_DIR})
		set(LLVM_LIBRARIES ${LLVM_LIBRARIES} ${LIBRARY_PATH})
		unset(LIBRARY_PATH CACHE)
	endforeach()
	# Extract library names from system-libs
	execute_process(
		COMMAND ${LLVM_CONFIG_PROGRAM} "--system-libs"
		OUTPUT_VARIABLE LLVM_LIBS)
	string(
		REGEX MATCHALL "-l([^ \r\n\"]+|\"[^\"]+\")"
		LLVM_LIBRARY_OPTIONS
		"${LLVM_LIBS}")
	foreach(MATCH ${LLVM_LIBRARY_OPTIONS})
		string(REGEX REPLACE "(^-l\"?|\"$)" "" TRIMMED ${MATCH})
		set(LLVM_LIBRARIES ${LLVM_LIBRARIES} ${TRIMMED})
	endforeach()
	# Set the flag
	set(LLVM_FOUND "YES")
else()
	# Unset the flag
	set(LLVM_FOUND "NO")
endif()

