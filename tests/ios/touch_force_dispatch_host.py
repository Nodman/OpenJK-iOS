#!/usr/bin/env python3
"""Exercise actual client input, key expansion, command execution and packet seams."""
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / 'build-desktop'
out = build / 'touch-force-dispatch-host'
out.mkdir(parents=True, exist_ok=True)
strings = out / 'q_string.o'
subprocess.run(['clang', '-std=c99', '-ffunction-sections', '-fdata-sections',
                '-Ishared', '-c', 'shared/qcommon/q_string.c', '-o', str(strings)],
               cwd=root, check=True)
for tree, target in [('code', 'openjk_sp.arm64'), ('codemp', 'openjk.arm64')]:
    flags = {}
    for line in (build / tree / 'CMakeFiles' / (target + '.dir') / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    binary = out / tree
    command = ['clang++', '-std=c++11', '-g', '-O1', '-fsanitize=address,undefined',
               '-ffunction-sections', '-fdata-sections', '-Wno-write-strings', '-Wno-comment',
               '-Wno-invalid-offsetof', '-DOPENJK_IOS'] + flags['CXX_DEFINES'] + flags['CXX_INCLUDES']
    command += ['-I' + tree + '/qcommon', '-UNDEBUG']
    if tree == 'codemp':
        command += ['-DTEST_MP']
    command += ['tests/ios/touch_force_dispatch_host.cc', str(strings), '-Wl,-dead_strip', '-o', str(binary)]
    print(tree + ': ' + shlex.join(command), flush=True)
    subprocess.run(command, cwd=root, check=True)
    subprocess.run([str(binary)], cwd=root, check=True)
