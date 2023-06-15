#!/usr/bin/env python3
####################################################################################
#  The following parts of c-tree-carver contain new code released under the        #
#  BSD 2-Clause License:                                                           #
#  * `bin`                                                                         #
#  * `cpp/src/debug.hpp`                                                           #
#  * `cpp/src/debug_printers.cpp`                                                  #
#  * `cpp/src/debug_printers.hpp`                                                  #
#  * `cpp/src/source_range_hash.hpp`                                               #
#  * `lib`                                                                         #
#  * `test`                                                                        #
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

import os, sys, re, subprocess, json, difflib, argparse

show_actual = False
test_matcher = re.compile(r'^(.*\.in\.c(pp)?)$')

def eprint(*args, then_exit=True, **kwargs):
    print('Error:', *args, file=sys.stderr, **kwargs)
    if then_exit:
        exit(1)

def test_files(test_dir):
    if not os.path.isdir(test_dir):
        eprint(f"'{test_dir}' not a directory")
    for root, _, files in os.walk(test_dir):
        for filename in files:
            if test_matcher.match(filename) is not None:
                yield os.path.join(root, filename)

def make_dict(filename):
    return {
            "directory": os.getcwd(),
            "command": 'clang -c ' + filename,
            "file": filename
            }

def write_compile_commands(args):
    dicts = [ make_dict(filename) for filename in test_files(args.test_dir) ]
    with open('compile_commands.json', 'w') as f:
        json.dump(dicts, f, indent=4)

class Carver:

    def __init__(self, carver):
        self.carver = carver
        if not os.path.isfile('compile_commands.json'):
            eprint("run './test/run_test.py make' first")

    def run(self, input_rel_path):
        cwd = os.getcwd()
        completed = subprocess.run([self.carver, input_rel_path], capture_output=True, text=True)
        return completed.stdout if completed.stdout != None else completed.stderr

    def output(self, input_rel_path):
        out_dir = self.run(input_rel_path)
        actual_path = os.path.join(out_dir, input_rel_path)
        with open(actual_path, 'r') as actual:
            return { 'content' : actual.readlines(), 'path': actual_path }

def get_diff(carver, input_rel_path):
    actual = carver.output(input_rel_path)
    expect_path = input_rel_path.replace('.in.c', '.out.c')
    with open(expect_path, 'r') as expect:
        return list(difflib.unified_diff(expect.readlines(), actual['content'], expect_path,
            expect_path if not show_actual else actual['path']))

def filter_inputs(test_dir, suffix):
    inputs = test_files(test_dir)
    if suffix is not None:
        inputs = list(filter(lambda x : x.endswith(suffix), inputs))
        inputs_len = len(inputs)
        if inputs_len > 1:
            eprint(f'more than one file matching *{suffix} found in {test_dir}', then_exit=False)
            eprint(inputs)
        elif inputs_len == 0:
            eprint(f'*{suffix} not found in {test_dir}')
    return inputs

def run_tests(args):
    failed_tests = 0
    carver = Carver(args.carver)
    for input_rel_path in filter_inputs(args.test_dir, args.suffix):
        diff = get_diff(carver, input_rel_path)
        if args.patch:
            if diff:
                failed_tests += 1
                sys.stdout.writelines(diff)
        elif diff:
            failed_tests += 1
            print('\033[31m[ FAILED ]\033[m %s' % input_rel_path)
        else:
            print('\033[32m[ PASSED ]\033[m %s' % input_rel_path)
    sys.exit(min(failed_tests, 1))
    return

# top level
parser = argparse.ArgumentParser(description="Script for running clang-tree-carve tests.")
parser.set_defaults(func=(lambda _: parser.parse_args(['-h'])))
parser.add_argument('--test_dir', help='Directory of tests.', default='test')

# subparsers
subparsers = parser.add_subparsers(title='subcommands', description='Choose which action to execute', help='Additional help')

# make subcommand
parser_make = subparsers.add_parser('make',
        help='Generate a compile_commands.json file.')
parser_make.set_defaults(func=write_compile_commands)

# for subcommand
parser_for = subparsers.add_parser('for',
        help='Run the tests for a directory or file.')
parser_for.add_argument('--carver', default='../_build/default/cpp/clang-tree-carve.exe')
parser_for.add_argument('--suffix',
        help='Uniquely identifying suffix of a file in compile_commands.json')
parser_for.add_argument('--patch', help='Output unified format patches for the failing tests.',
        action='store_true')
parser_for.set_defaults(func=run_tests)

# parse args and call func (as set using set_defaults)
args = parser.parse_args()
args.func(args)

