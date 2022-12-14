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

import os, sys, re, subprocess;
import clang.cindex

def print_success(name):
    print('\033[32m[ PASSED ]\033[m %s' % name)

def print_failed(name):
    print('\033[31m[ FAILED ]\033[m %s' % name)

def tokenize(input_path):
    index = clang.cindex.Index.create()
    tu = index.parse(input_path, args=['-std=c++11'])
    tokens = tu.get_tokens(extent=tu.cursor.extent)
    return '\n'.join([t.spelling for t in tokens]) + '\n'

def run_simplify(minifier_path, input_path):
    proc = subprocess.Popen([minifier_path, input_path], stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def run_tokenized_simplify(minifier_path, input_path):
    source = tokenize(input_path)
    proc = subprocess.Popen(
        [minifier_path, '-'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    return proc.communicate(source.encode('utf-8'))[0].decode('utf-8')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: python %s minifier_path [test_directory]' % sys.argv[0])
        quit()
    minifier_path = sys.argv[1]
    test_directory = os.path.dirname(os.path.abspath(__file__))
    if len(sys.argv) >= 3:
        test_directory = sys.argv[2]
    test_directory = os.path.abspath(test_directory)
    if test_directory[-1] != '/':
        test_directory += '/'
    matcher = re.compile(r'^(.*)\.in\.cpp$');
    test_inputs = []
    for directory in os.walk(test_directory):
        dir_path = directory[0]
        for filename in directory[2]:
            filepath = os.path.abspath(dir_path + '/' + filename)
            match = matcher.match(filepath)
            if match is not None:
                test_inputs.append(match.group(1))
    passed_tests = []
    failed_tests = []
    for filepath in test_inputs:
        test_name = filepath[len(test_directory):]
        input_path = filepath + '.in.cpp'
        expect_path = filepath + '.out.cpp'
        # normal
        expect = ''.join([s.decode('utf-8') for s in open(expect_path, 'rb').readlines()])
        actual = run_simplify(minifier_path, input_path)
        if expect == actual:
            print_success(test_name)
            passed_tests.append(test_name)
        else:
            print_failed(test_name)
            failed_tests.append(test_name)
        # tokenized
        test_name += ' (tokenized)'
        expect = tokenize(expect_path)
        actual = run_tokenized_simplify(minifier_path, input_path)
        if expect == actual:
            print_success(test_name)
            passed_tests.append(test_name)
        else:
            print_failed(test_name)
            failed_tests.append(test_name)
            print('---- expect ----')
            print(expect)
            print('---- actual ----')
            print(actual)
    if len(failed_tests) == 0:
        print_success('%d tests.' % len(passed_tests))
    else:
        print_failed('Passed %d tests and failed %d tests.' % (len(passed_tests), len(failed_tests)))
    sys.exit(min(len(failed_tests), 1))

