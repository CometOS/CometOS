#!/usr/bin/env python3

from subprocess import Popen, PIPE
import os
import sys


class Violation:
    errors = {}

    def __init__(self, line, path, file_name, include_line, include_value, fixed_include = None):
        self.line = line
        self.path = path
        self.file_name = file_name
        self.include_line = include_line
        self.include_value = include_value
        self.fixed_include = fixed_include

        matches = []
        if self.fixed_include is None:
            include_file = os.path.basename(self.include_value)
            for root, subfolders, files in os.walk('.'):
                if include_file in files:
                    matches.append(os.path.join(root, include_file))

            if len(matches) == 1:
                self.fixed_include = os.path.relpath(matches[0], self.path)
                if self.fixed_include[0] != '.':
                    self.fixed_include = './' + self.fixed_include
            else:
                if not include_file in Violation.errors:
                    Violation.errors[include_file] = 0
                Violation.errors[include_file] = Violation.errors[include_file] + 1
                #print(include_file, matches, file=sys.stderr)

    def patch(self):
        if self.fixed_include is not None:
            a = os.path.join('a', 'src', self.path, self.file_name)
            b = os.path.join('b', 'src', self.path, self.file_name)
            print('diff', a, b)
            print('---', a)
            print('+++', b)
            print('@@', '-' + str(self.include_line) + ',1', '+' + str(self.include_line) + ',1', '@@')
            print('-' + self.line)
            print('+' + '#include "' + self.fixed_include + '"')

def main():
    output, errors = Popen(['egrep -rn --include \*.h --include \*.cc "^#include \\".*\\""'], shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE).communicate()
    error_message = errors.decode()
    if error_message != '':
        print(error_message)

    violations = []
    include_dict = {}

    lines = output.decode().splitlines()
    lines.sort()
    for line in lines:

        parts = line.split(':')

        filename = os.path.basename(parts[0])
        pathname = os.path.dirname(parts[0])
        include = parts[2].split('"')[1]

        combined = os.path.normpath(os.path.join(pathname, include))

        if not os.path.isfile(combined):
            violations.append(Violation(parts[2], pathname, filename, int(parts[1]), include))
            continue

        relative = os.path.relpath(combined, pathname)
        if relative[0] != '.':
            relative = './' + relative

        if not include == relative:
            violations.append(Violation(parts[2], pathname, filename, int(parts[1]), include, relative))

    for violation in violations:
        violation.patch()

    for key, value in Violation.errors.items():
        print(key, value, file=sys.stderr)

if __name__ == "__main__":
    main()
