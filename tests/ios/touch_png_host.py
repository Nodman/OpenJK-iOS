#!/usr/bin/env python3
"""Decode the embedded touch atlas with the actual SP and MP PNG loaders, including failure paths."""
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[2]
build = root / 'build-desktop'
out = build / 'touch-png-host'
out.mkdir(parents=True, exist_ok=True)


def flags(directory, target):
    result = {}
    for line in (build / directory / 'CMakeFiles' / (target + '.dir') / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            result[key] = shlex.split(value)
    return result


def run(args):
    print(shlex.join(args), flush=True)
    subprocess.run(args, cwd=root, check=True)


# Homebrew libpng stands in for the bundled one, which only builds inside the renderer.
common = ['-std=c++11', '-g', '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-DOPENJK_IOS', '-UNDEBUG',
          '-Wno-write-strings', '-Wno-comment', '-Wno-invalid-offsetof', '-isystem', '/opt/homebrew/include']
for name, directory, target, source, extra in [
        ('sp', 'code/rd-vanilla', 'rdsp-vanilla_arm64', 'code/rd-common/tr_image_png.cpp', ['-DTOUCH_PNG_SP']),
        ('mp', 'codemp/rd-vanilla', 'rd-vanilla_arm64', 'codemp/rd-common/tr_image_png.cpp', [])]:
    f = flags(directory, target)
    includes = [i for i in f['CXX_INCLUDES'] if 'libpng' not in i]
    binary = out / name
    run(['clang++'] + common + extra + f['CXX_DEFINES'] + includes +
        ['-DTOUCH_PNG_SOURCE="' + str(root / source) + '"', '-x', 'c++', str(root / 'tests/ios/touch_png_harness.inc'),
         '-L/opt/homebrew/lib', '-lpng', '-lz', '-o', str(binary)])
    run([str(binary)])
print('PASS: SP and MP LoadPNGFromMemory decode the atlas, reject truncated, corrupt and wrong-header input, and survive allocation failure')
