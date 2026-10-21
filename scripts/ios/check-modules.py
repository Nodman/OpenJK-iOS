#!/usr/bin/env python3
"""Check exports and native GLES boundaries in complete iOS app bundles."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import plistlib
import re
import subprocess

NATIVE = re.compile(r'^_(gl|egl)[A-Z]')
# SDL 2.32.10 SDL_uikitopenglview.m, verified against the retained P12 EAGL control.
# Control OpenJK-MP SHA256: 6a3e232633ad3a11f57331508a28b25983f75130b550dcddf095336ed4f5e17a.
# Tar SHA256: 5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165.
SDL_GL = '''_glBindFramebuffer
_glBindRenderbuffer
_glBlitFramebuffer
_glCheckFramebufferStatus
_glDeleteFramebuffers
_glDeleteRenderbuffers
_glDiscardFramebufferEXT
_glFramebufferRenderbuffer
_glGenFramebuffers
_glGenRenderbuffers
_glGetIntegerv
_glGetRenderbufferParameteriv
_glInvalidateFramebuffer
_glLabelObjectEXT
_glRenderbufferStorage
_glRenderbufferStorageMultisample
_glResolveMultisampleFramebufferAPPLE'''.splitlines()
MODULES = {
    'MP': {
        'jampgamearm64': (['_GetModuleAPI', '_dllEntry', '_vmMain'], ['_jampgame_GetModuleAPI']),
        'cgamearm64': (['_GetModuleAPI', '_dllEntry', '_vmMain'], ['_cgame_GetModuleAPI']),
        'uiarm64': (['_GetModuleAPI', '_dllEntry', '_vmMain'], ['_ui_GetModuleAPI']),
        'rd-vanilla_arm64': (['_GetRefAPI'], ['_rdvanilla_GetRefAPI']),
    },
    'SP': {
        'jagamearm64': (['_GetGameAPI', '_dllEntry', '_vmMain'],
                       ['_jagame_GetGameAPI', '_jagame_dllEntry', '_jagame_vmMain']),
        'rdsp-vanilla_arm64': (['_GetRefAPI'], ['_rdspvanilla_GetRefAPI']),
    },
}
MAGICS = {bytes.fromhex(x) for x in ('feedface', 'cefaedfe', 'feedfacf', 'cffaedfe',
                                    'cafebabe', 'bebafeca', 'cafebabf', 'bfbafeca')}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def symbols(output, undefined):
    result = set()
    for line in output.splitlines():
        line = line.strip()
        if not line or re.fullmatch(r'.+\.o:|.+\([^()]+\.o\):|.+ \(for architecture [^)]+\):', line):
            continue
        fields = line.split()
        if undefined:
            valid = (len(fields) == 1 or (len(fields) == 2 and fields[0] == 'U')
                     or (len(fields) == 3 and re.fullmatch(r'[0-9a-fA-F]+', fields[0])
                         and fields[1] == 'U'))
        else:
            valid = (len(fields) == 3 and re.fullmatch(r'[0-9a-fA-F]+', fields[0])
                     and len(fields[1]) == 1 and fields[1] != 'U')
        require(valid, 'unrecognized nm row: ' + line)
        result.add(fields[-1])
    return result


def nm(path, undefined):
    run = subprocess.run(['nm', '-u' if undefined else '-gU', str(path)],
                         capture_output=True, text=True)
    require(run.returncode == 0 and not run.stderr.strip(),
            f'nm inspection failed: {path}: {run.stderr.strip()}')
    return symbols(run.stdout, undefined)


def macho(path):
    with path.open('rb') as stream:
        return stream.read(4) in MAGICS


def native_boundary(imports, executable=False):
    native = {s for s in imports if NATIVE.match(s)}
    if executable:
        require(native == set(SDL_GL), f'executable SDL native set mismatch: '
                f'added={sorted(native - set(SDL_GL))} missing={sorted(set(SDL_GL) - native)}')
    else:
        require(not native, f'native GL/EGL imports: {sorted(native)}')


def exports(path, want):
    got = nm(path, False)
    require(got == set(want), f'{path}: exports want={sorted(want)} got={sorted(got)}')
    require(not any('gl4es' in s for s in got), f'{path}: visible gl4es exports')


def angle_qualified(path, name, prepared):
    pins = json.loads(Path(__file__).with_name('angle-consumer.json').read_text())
    pin = pins['frameworks'][name]
    if len(path.read_bytes()) == pin['bytes'] and hashlib.sha256(path.read_bytes()).hexdigest() == pin['sha256']:
        return
    require(prepared is not None, f'{path}: unqualified ANGLE bytes (signed builds need --angle-prepared)')
    spec = importlib.util.spec_from_file_location('angle_consumer', Path(__file__).with_name('angle-consumer.py'))
    consumer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(consumer)
    original = prepared / f'{name}.xcframework/ios-arm64/{name}.framework/{name}'
    require(consumer.sha(original) == pin['sha256'], f'{original}: unqualified prepared ANGLE')
    consumer.audit(path.parent, name)
    consumer.same_signed_payload(path, original)


def inspect_app(app, flavor, mode, mods, prepared):
    require(app.is_dir() and not app.is_symlink(), f'missing app: {app}')
    executable = app / f'OpenJK-{flavor}'
    info = plistlib.loads((app / 'Info.plist').read_bytes())
    require(info['CFBundleExecutable'] == executable.name, f'{app}: wrong executable identity')
    expected = {executable}
    if mode == 'dylib':
        expected.update(app / 'Frameworks' / (t + '.dylib') for t in MODULES[flavor])
    binaries = set()
    paths = []
    def walk_error(error):
        raise error
    for directory, dirs, files in os.walk(app, onerror=walk_error):
        paths.extend(Path(directory) / name for name in dirs + files)
    for path in sorted(paths):
        require(not path.is_symlink(), f'{path}: symlink prevents complete inspection')
        if path.is_dir():
            if path.suffix in ('.framework', '.appex', '.app', '.bundle'):
                inner = plistlib.loads((path / 'Info.plist').read_bytes())
                if 'CFBundleExecutable' in inner:
                    name = inner['CFBundleExecutable']
                    require(Path(name).name == name and name not in ('.', '..'), f'{path}: invalid executable')
                    expected.add(path / name)
                else:
                    require(path.suffix == '.bundle', f'{path}: missing executable metadata')
            continue
        is_macho = macho(path)
        if path.suffix in ('.dylib', '.so'):
            require(is_macho, f'{path}: not Mach-O')
        if is_macho:
            binaries.add(path)
    require(expected <= binaries, f'{app}: missing/non-Mach-O binaries: {sorted(map(str, expected - binaries))}')
    angle = {app / f'Frameworks/{name}.framework/{name}': name for name in ('libEGL', 'libGLESv2')}
    if any(path.parent.exists() for path in angle):
        require(set(angle) <= binaries, f'{app}: incomplete ANGLE framework pair')
    for path in sorted(binaries):
        imports = nm(path, True)
        if path in angle:
            angle_qualified(path, angle[path], prepared)
        else:
            native_boundary(imports, path == executable)
        print(f'ok   native boundary {path.relative_to(app)} ({len(imports)} imports)')
    for t, (dyn, stat) in MODULES[flavor].items():
        path = mods / (t + '.a') if mode == 'static' else app / 'Frameworks' / (t + '.dylib')
        exports(path, stat if mode == 'static' else dyn)
        if mode == 'static':
            native_boundary(nm(path, True))
        print(f'ok   exports {path}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('set', choices=('mp', 'all'))
    parser.add_argument('mode', nargs='?', default='dylib', choices=('dylib', 'static'))
    parser.add_argument('--apps-root', type=Path, help='explicit directory containing the app bundles')
    parser.add_argument('--gl4es', type=Path, help='explicit gl4es libGL.a')
    parser.add_argument('--angle-prepared', type=Path, help='qualified prepared XCFrameworks for signed-payload checks')
    args = parser.parse_args()
    apps = args.apps_root or args.build / 'apps/Release-iphoneos'
    prepared = args.angle_prepared if args.angle_prepared is not None else args.build / 'angle-consumer'
    failures = []
    for flavor in ('MP', 'SP') if args.set == 'all' else ('MP',):
        try:
            inspect_app(apps / f'OpenJK-{flavor}.app', flavor, args.mode,
                        args.build / 'modules/Release-iphoneos', prepared)
        except (ValueError, OSError, KeyError, plistlib.InvalidFileException) as error:
            failures.append(f'{flavor}: {error}')
    gl4es = args.gl4es or args.build / 'gl4es/Release/libGL.a'
    try:
        require(gl4es.read_bytes().startswith(b'!<arch>\n'), f'{gl4es}: missing/invalid archive')
        unresolved = nm(gl4es, True) - nm(gl4es, False)
        native_boundary(unresolved)
        print(f'ok   gl4es archive native-symbol subtraction {gl4es}')
    except (ValueError, OSError) as error:
        failures.append(str(error))
    for failure in failures:
        print('FAIL ' + failure)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
