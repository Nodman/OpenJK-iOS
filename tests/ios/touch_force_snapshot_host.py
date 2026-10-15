#!/usr/bin/env python3
"""Exercise decoded MP playerstate through the actual cgame producer and native callback."""
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[2]
build = root / 'build-desktop'
out = build / 'touch-force-snapshot-host'
out.mkdir(parents=True, exist_ok=True)
def flags(directory, target):
    result = {}
    for line in (build / directory / 'CMakeFiles' / (target + '.dir') / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            result[key] = shlex.split(value)
    return result
module = flags('codemp/cgame', 'cgamearm64')
host = flags('codemp', 'openjk.arm64')
common = ['-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-ffunction-sections', '-fdata-sections', '-DOPENJK_IOS', '-UNDEBUG']
def run(args):
    print(shlex.join(args), flush=True)
    subprocess.run(args, cwd=root, check=True)
cgame = out / 'cgame.o'
run(['clang', '-std=c99'] + common + module['C_DEFINES'] + module['C_INCLUDES'] + ['-c', 'tests/ios/touch_force_snapshot_cgame.c', '-o', str(cgame)])
objects = [cgame]
for source, target_flags, cc in [('codemp/game/bg_misc.c', module, 'clang'), ('codemp/client/cl_cgame.cpp', host, 'clang++'), ('codemp/client/cl_input.cpp', host, 'clang++')]:
    obj = out / (Path(source).stem + '.o')
    run([cc, '-std=c99' if cc == 'clang' else '-std=c++11'] + common + target_flags['C_DEFINES' if cc == 'clang' else 'CXX_DEFINES'] + target_flags['C_INCLUDES' if cc == 'clang' else 'CXX_INCLUDES'] + ['-c', source, '-o', str(obj)])
    objects.append(obj)
for source, cc in [('shared/qcommon/q_string.c', 'clang'), ('shared/qcommon/q_math.c', 'clang'), ('codemp/qcommon/q_shared.c', 'clang'), ('codemp/qcommon/huffman.cpp', 'clang++'), ('shared/sys/ios/ios_touch.cpp', 'clang++')]:
    obj = out / (Path(source).stem + '.o')
    run([cc, '-std=c99' if cc == 'clang' else '-std=c++11'] + common + host['CXX_DEFINES'] + host['CXX_INCLUDES'] + ['-c', source, '-o', str(obj)])
    objects.append(obj)
binary = out / 'snapshot'
run(['clang++', '-std=c++11', '-Wno-write-strings', '-Wno-comment', '-Wno-invalid-offsetof'] + common + host['CXX_DEFINES'] + host['CXX_INCLUDES'] + ['tests/ios/touch_force_snapshot_host.cc'] + [str(p) for p in objects] + ['-Wl,-dead_strip', '-o', str(binary)])
run([str(binary)])
