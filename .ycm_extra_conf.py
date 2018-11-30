"""This module provides compilation flags used by ycm to enable
semantic code completion."""

import os
import sys

DIR_OF_THIS_SCRIPT = os.path.abspath(os.path.dirname( __file__ ))

INCLUDE = 'src src/util include libs/gl3w libs/imgui libs/nlohmann'
FLAGS = ['`pkg-config --cflags glfw3`', '-Wall', '-Wextra']
LIBS = [
        '-lGL',
        '`pkg-config --static --libs glfw3`',
        '-lboost_system',
        '-lboost_filesystem',
        '-lboost_regex',
        '-lboost_program_options',
        '-lfreeimage']

# for debugging
log = None

def FlagsForFile(filename, **kwargs):
    includes = []
    for item in INCLUDE.split(' '):
        includes.append('-I' + os.path.join(DIR_OF_THIS_SCRIPT, item))


    if log is not None:
        orig_stdout = sys.stdout
        with open(log, 'w') as f:
            sys.stdout = f
            print(kwargs)
            print(kwargs['client_data'])
            for item in kwargs['client_data']:
                print(item, kwargs['client_data'][item])
            print(filename)
            print(includes)
        sys.stdout = orig_stdout

    ctype = []

    return { 'flags': FLAGS + ctype + includes + LIBS }


