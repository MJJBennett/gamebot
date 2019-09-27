#!/usr/bin/env python3
from os import walk
from os.path import isfile, isdir, abspath, samefile
import pathlib
import json

# Note: This is a file I use across multiple projects to get autocomplete
# to work properly & consistently (I have not yet found a better and simpler
# solution) if anyone finds this feel free to use it.
# Execute this with:
# ./fix-compile-commands.py include src deps/mydep1
# etc for each folder that contains files you want to ensure there are 
# compile commands for.
# If you need non-standard file extensions, edit the get_files_recurse
# method below to add them following the pattern.

def get_files_recurse(d):
    # input : directory
    l = [x for x in pathlib.Path(d).glob('**/*.cpp')]
    l.extend([x for x in pathlib.Path(d).glob('**/*.h')])
    l.extend([x for x in pathlib.Path(d).glob('**/*.hpp')])
    return l

def is_present(file_path, cmds, debug):
    for o in cmds:
        if samefile(o["file"], file_path):
            if debug:
                print("Found same file:", o["file"], file_path)
            return True
    return False

def ensure_all_present(files, debug):
    with open("compile_commands.json", 'r') as file:
        cmds = json.load(file)
    not_in_cmds = []
    for f in files:
        file_path = abspath(f)
        if not is_present(file_path, cmds, debug):
            not_in_cmds.append(file_path)
    default = cmds[0]
    if len(not_in_cmds) > 0:
        print("Adding commands for files:", not_in_cmds)
    for c in not_in_cmds:
        d_copy = dict(default)
        d_copy["file"] = abspath(c)
        cmds.append(d_copy)
    with open("compile_commands.json", 'w') as file:
        json.dump(cmds, file, indent=2)

if __name__ == '__main__':
    import sys
    args = sys.argv[1:]
    debug = False
    if '--debug' in args:
        args.remove('--debug')
        debug = True
    files = []
    for d in args:
        l = get_files_recurse(d)
        files.extend(l if l is not None else [])
        if debug:
            print('For directory:', d, '\n' + str(files))
    ensure_all_present(files, debug) 
