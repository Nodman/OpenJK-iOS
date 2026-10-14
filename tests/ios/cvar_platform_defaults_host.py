#!/usr/bin/env python3
"""Compile and run the real SP/MP cvar implementations with host service stubs."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
out = root / 'build-desktop' / 't11-16-host'
out.mkdir(parents=True, exist_ok=True)
strings = out / 'q_string.o'
subprocess.run(['cc', '-std=c99', '-ffunction-sections', '-fdata-sections',
                '-I' + str(root / 'shared'), '-c', str(root / 'shared/qcommon/q_string.c'),
                '-o', str(strings)], check=True, cwd=root)
for tree in ('code', 'codemp'):
    binary = out / (tree + '-cvar')
    command = ['c++', '-std=c++11', '-g', '-O1', '-Wall', '-Wextra',
               '-Wno-write-strings', '-Wno-comment', '-ffunction-sections',
               '-fdata-sections', '-I' + str(root / tree),
               '-I' + str(root / 'shared'), '-I' + str(root / 'lib/gsl-lite/include'),
               str(root / 'tests/ios/cvar_platform_defaults_host.cc'), str(strings), '-o', str(binary)]
    command += ['-Wl,-dead_strip'] if sys.platform == 'darwin' else ['-Wl,--gc-sections']
    if tree == 'codemp':
        command += ['-DTEST_MP']
    subprocess.run(command, check=True, cwd=root)
    print(tree + ':', flush=True)
    subprocess.run([str(binary)], check=True, cwd=root)
