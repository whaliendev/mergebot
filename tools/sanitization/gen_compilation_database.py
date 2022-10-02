#!/usr/bin/env python3

# This is based on the script on the Envoy project 
# https://github.com/envoyproxy/envoy/blob/master/tools/gen_compilation_database.py

import argparse
import json
from pathlib import Path
import re
import subprocess
import os

RE_INCLUDE_SYSTEM = re.compile('\s*-I\s+/usr/[^ ]+')

def generateCompilationDatabase(args):
    # generate build/CMakeCache.txt
    subprocess.check_call(['cmake', '-Bbuild'])

    # TODO(hwa): there may be a better way to get the project top level dir
    bin_dir = subprocess.check_output(
        ['grep', '-i', 'BINARY_DIR', 'build/CMakeCache.txt']).decode().split('=')[1].replace('\n', '')

    return json.loads(Path(os.path.join(bin_dir, 'compile_commands.json')).read_text())

def isHeader(filename):
    for ext in ('.h', '.hh', '.hpp', '.hxx'):
        if filename.endswith(ext):
            return True
    return False

def isCompileTarget(target, args):
    filename = target['file']
    if not args.include_headers and isHeader(filename):
        return False
    return True

def modifyCompileCommand(target, args):
    cc, options = target['command'].split(" ", 1)

    # we have added C++17 options in cmake script. Those doesn't affect build
    #  itself but clang-tidy will misinterpret them.
    options = options.replace('-std=c++0x ', '')
    options = options.replace('-std=c++14', '')
    options = options.replace('-std=c++17', '')

    # add -DNDEBUG so that editors show the correct size information for structs
    options += " -DNDEBUG"

    if args.ignore_system_headers:
        # remove all include options for /usr/* dir
        options = RE_INCLUDE_SYSTEM.sub('', options)

    target['command'] = ' '.join([cc, options])

    return target

def fixCompilationDatabase(args, db):
    db = [
        modifyCompileCommand(target, args)
        for target in db
        if isCompileTarget(target, args)
    ]

    with open('compile_commands.json', 'w') as db_file: 
        json.dump(db, db_file, indent=2)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generate JSON compilation database'
    )
    parser.add_argument('--ignore_system_headers', action='store_true')
    parser.add_argument('--include_headers', action='store_true')
    args = parser.parse_args()
    fixCompilationDatabase(args, generateCompilationDatabase(args))
    