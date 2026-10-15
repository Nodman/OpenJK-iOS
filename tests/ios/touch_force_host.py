#!/usr/bin/env python3
"""Run the Force seam under the configured SP/MP host and module defines."""
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / 'build-desktop'
out = build / 'touch-force-host'
out.mkdir(parents=True, exist_ok=True)
variants = (
    ('sp-host', 'code', 'openjk_sp.arm64'),
    ('sp-module', 'code/game', 'jagamearm64'),
    ('mp-host', 'codemp', 'openjk.arm64'),
    ('mp-module', 'codemp/cgame', 'cgamearm64'),
)
for name, directory, target in variants:
    flags = {}
    for line in (build / directory / 'CMakeFiles' / (target + '.dir') / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    binary = out / name
    command = ['clang++', '-std=c++11', '-g', '-O1', '-fsanitize=address,undefined',
               '-DOPENJK_IOS'] + flags['CXX_DEFINES'] + flags['CXX_INCLUDES'] + [
               '-UNDEBUG', str(root / 'tests/ios/touch_force_host.cc'), '-o', str(binary)]
    print(name + ': ' + shlex.join(command), flush=True)
    subprocess.run(command, check=True, cwd=root)
    subprocess.run([str(binary)], check=True, cwd=root)
