#!/usr/bin/env python3
"""Compile and run the real SP/MP cvar implementations with host service stubs."""
from pathlib import Path
import argparse
import subprocess
import sys
import re

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device-source', type=Path,
                    default=root / 'shared/sys/ios/sys_ios_device.mm',
                    help='Production device-default source for before/after control')
args = parser.parse_args()
out = root / 'build-desktop' / 't11-16-host'
out.mkdir(parents=True, exist_ok=True)
strings = out / 'q_string.o'
subprocess.run(['cc', '-std=c99', '-ffunction-sections', '-fdata-sections',
                '-I' + str(root / 'shared'), '-c', str(root / 'shared/qcommon/q_string.c'),
                '-o', str(strings)], check=True, cwd=root)
# Extract complete production functions, retaining the panel-dependent branch in
# the before control. Only OS services and config/command transport are host stubs.
def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

device = args.device_source.read_text()
(out / 'device_defaults.inc').write_text(function(device, 'const char *Sys_IOS_DeviceDefaults('))
failures = 0
for tree in ('code', 'codemp'):
    common = (root / tree / 'qcommon/common.cpp').read_text()
    (out / 'startup_defaults.inc').write_text(
        function(common, 'void Com_ExecuteCfg(') +
        function(common, 'void Com_StartupVariable('))
    # Verify the exercised startup sequence is also the actual Com_Init order.
    init = common[common.index('void Com_Init('):]
    assert init.index('Com_ExecuteCfg();') < init.rindex('Com_StartupVariable( NULL );')
    registrations = re.findall(r'com_maxfps = (Cvar_Get[^;]+;)',
                               (root / 'shared/sys/sys_main.cpp').read_text())
    assert len(registrations) == 2
    (out / 'rate_registration.inc').write_text(
        'static void RegisterRate() { ' + registrations[tree == 'codemp'] + ' }\n')
    binary = out / (tree + '-cvar')
    command = ['c++', '-std=c++11', '-g', '-O1', '-Wall', '-Wextra',
               '-Wno-write-strings', '-Wno-comment', '-ffunction-sections',
               '-fdata-sections', '-DOPENJK_IOS', '-I' + str(out), '-I' + str(root / tree),
               '-I' + str(root / 'shared'), '-I' + str(root / 'lib/gsl-lite/include'),
               str(root / 'tests/ios/cvar_platform_defaults_host.cc'), str(strings), '-o', str(binary)]
    if sys.platform == 'darwin':
        command[1:1] = ['-x', 'objective-c++']
        command += ['-framework', 'Foundation']
        # q_string.o is already compiled C, not Objective-C++ source.
        command[command.index(str(strings)):command.index(str(strings))] = ['-x', 'none']
    command += ['-Wl,-dead_strip'] if sys.platform == 'darwin' else ['-Wl,--gc-sections']
    if tree == 'codemp':
        command += ['-DTEST_MP']
    subprocess.run(command, check=True, cwd=root)
    print(tree + ':', flush=True)
    failures += subprocess.run([str(binary)], cwd=root).returncode != 0
sys.exit(bool(failures))
