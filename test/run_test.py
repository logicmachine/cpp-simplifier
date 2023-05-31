#!/usr/bin/env python3
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

import os, sys, re, subprocess, json, difflib, argparse

test_matcher = re.compile(r'^(.*\.in\.c(pp)?)$')

def test_files(test_dir):
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

class Simplifier:

    def __init__(self, simplifier):
        self.simplifier = simplifier
        if not os.path.isfile('compile_commands.json'):
            eprint("Run './test/run_test.py make' first")

    def run(self, input_rel_path):
        cwd = os.getcwd()
        proc = subprocess.Popen([self.simplifier, "--omit-lines", input_rel_path], stdout=subprocess.PIPE)
        result = proc.communicate()[0].decode('utf-8')
        return result

    def output(self, input_rel_path):
        out_dir = self.run(input_rel_path)
        actual_path = os.path.join(out_dir, input_rel_path)
        with open(actual_path, 'r') as actual:
            return (actual.readlines(), actual_path)

def get_diff(simplifier, input_rel_path):
    expect_path = input_rel_path.replace('.in.c', '.out.c')
    with open(expect_path, 'r') as expect:
        actual, actual_path = simplifier.output(input_rel_path)
        return list(difflib.unified_diff(expect.readlines(), actual, expect_path, actual_path))

def eprint(*args, then_exit=True, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    if then_exit:
        exit(1)

def filter_inputs(args):
    inputs = test_files(args.test_dir)
    if args.suffix is not None:
        inputs = list(filter(lambda x : x.endswith(args.suffix), inputs))
        inputs_len = len(inputs)
        if inputs_len > 1:
            eprint(f'More than one file matching *{args.suffix} found in {args.test_dir}', then_exit=False)
            eprint(inputs)
        elif inputs_len == 0:
            eprint(f'*{args.suffix} not found in {args.test_dir}')
    return inputs

def dir_or_file(args):
    failed_tests = 0
    simplifier = Simplifier(args.simplifier)
    for input_rel_path in filter_inputs(args):
        if (diff := get_diff(simplifier, input_rel_path)):
            failed_tests += 1
            sys.stdout.writelines(diff)
        elif not args.quiet:
            print('\033[32m[ PASSED ]\033[m %s' % input_rel_path)
    sys.exit(min(failed_tests, 1))
    return

# top level
parser = argparse.ArgumentParser(description="Script for running c-simplifier tests.")
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
parser_for.add_argument('--simplifier', default='./build/c-simplifier')
parser_for.add_argument('--suffix',
        help='Uniquely identifying suffix of a file in compile_commands.json')
parser_for.add_argument('-q', '--quiet', help='Do not print when tests pass.', action='store_true')
parser_for.set_defaults(func=dir_or_file)

# parse args and call func (as set using set_defaults)
args = parser.parse_args()
args.func(args)

