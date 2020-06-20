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
    if len(sys.argv) < 2:
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

