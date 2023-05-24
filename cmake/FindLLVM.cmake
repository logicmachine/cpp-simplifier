####################################################################################
#  The following parts of C-simplifier contain new code released under the         #
#  BSD 2-Clause License:                                                           #
#  * `src/debug.hpp`                                                               #
#                                                                                  #
#  Copyright (c) 2022 Dhruv Makwana                                                #
#  All rights reserved.                                                            #
#                                                                                  #
#  This software was developed by the University of Cambridge Computer             #
#  Laboratory as part of the Rigorous Engineering of Mainstream Systems            #
#  (REMS) project. This project has been partly funded by an EPSRC                 #
#  Doctoral Training studentship. This project has been partly funded by           #
#  Google. This project has received funding from the European Research            #
#  Council (ERC) under the European Union's Horizon 2020 research and              #
#  innovation programme (grant agreement No 789108, Advanced Grant                 #
#  ELVER).                                                                         #
#                                                                                  #
#  BSD 2-Clause License                                                            #
#                                                                                  #
#  Redistribution and use in source and binary forms, with or without              #
#  modification, are permitted provided that the following conditions              #
#  are met:                                                                        #
#  1. Redistributions of source code must retain the above copyright               #
#     notice, this list of conditions and the following disclaimer.                #
#  2. Redistributions in binary form must reproduce the above copyright            #
#     notice, this list of conditions and the following disclaimer in              #
#     the documentation and/or other materials provided with the                   #
#     distribution.                                                                #
#                                                                                  #
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''              #
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED               #
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A                 #
#  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR             #
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,                    #
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT                #
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF                #
#  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND             #
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,              #
#  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT              #
#  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF              #
#  SUCH DAMAGE.                                                                    #
#                                                                                  #
#  All other parts involve adapted code, with the new code subject to the          #
#  above BSD 2-Clause licence and the original code subject to its MIT             #
#  licence.                                                                        #
#                                                                                  #
#  The MIT License (MIT)                                                           #
#                                                                                  #
#  Copyright (c) 2016 Takaaki Hiragushi                                            #
#                                                                                  #
#  Permission is hereby granted, free of charge, to any person obtaining a copy    #
#  of this software and associated documentation files (the "Software"), to deal   #
#  in the Software without restriction, including without limitation the rights    #
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       #
#  copies of the Software, and to permit persons to whom the Software is           #
#  furnished to do so, subject to the following conditions:                        #
#                                                                                  #
#  The above copyright notice and this permission notice shall be included in all  #
#  copies or substantial portions of the Software.                                 #
#                                                                                  #
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      #
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        #
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     #
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          #
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   #
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   #
#  SOFTWARE.                                                                       #
####################################################################################

# - Find LLVM
# Find LLVM libraries
# This module defines:
#  LLVM_FOUND, If false, LLVM is unavailable on this system
#  LLVM_INCLUDE_DIR, where to find headers of LLVM
#  LLVM_LINK_DIR, where to find libraries of LLVM
#  LLVM_LIBRARIES, the libraries needed to use LLVM

find_program(
	LLVM_CONFIG_PROGRAM
	NAMES "llvm-config" "llvm-config-12")

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

