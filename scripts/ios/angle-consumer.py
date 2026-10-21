#!/usr/bin/env python3
"""Consume the qualified ANGLE release; never acquire or build its sources."""
import argparse
import hashlib
import json
import pathlib
import plistlib
import re
import shutil
import stat
import struct
import subprocess
import tempfile
import urllib.request
import zipfile

NAMES = ('libEGL', 'libGLESv2')
PINS = pathlib.Path(__file__).with_name('angle-consumer.json')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()


def plist(path):
    return plistlib.loads(path.read_bytes())


def audit(framework, name):
    binary = framework / name
    info = plist(framework / 'Info.plist')
    require(info['CFBundleExecutable'] == name, 'wrong framework executable')
    require(info['CFBundleIdentifier'] == 'org.chromium.ost.' + name, 'wrong bundle identity')
    require(info['MinimumOSVersion'] == '17.0', 'wrong inner minimum OS')
    require(info['CFBundleShortVersionString'] == '1.0.0', 'wrong framework version')
    require(info['CFBundleSupportedPlatforms'] == ['iPhoneOS'], 'wrong inner platform')
    require(run('lipo', '-archs', str(binary)) == 'arm64', 'wrong architecture')
    version = run('xcrun', 'vtool', '-show-build', str(binary))
    require(re.search(r'platform IOS\s', version), 'wrong Mach-O platform')
    require(re.search(r'minos 17\.0\s', version), 'wrong Mach-O minimum OS')
    paths = [s.strip().split(' (')[0] for s in run('otool', '-L', str(binary)).splitlines()[1:]]
    require(paths[0] == f'@rpath/{name}.framework/{name}', 'wrong install name')
    require(all(p.startswith(('/System/Library/', '/usr/lib/')) for p in paths[1:]), 'non-system dependency')
    commands = run('otool', '-l', str(binary))
    rpaths = re.findall(r'cmd LC_RPATH\n.*?path ([^ ]+)', commands, re.S)
    require(all(p.startswith(('@executable_path/', '@loader_path/', '@rpath/')) for p in rpaths), 'unsafe framework rpath')
    return binary


def extract(archive, destination, name):
    root = name + '.xcframework'
    with zipfile.ZipFile(archive) as z:
        members = z.infolist()
        require(len({i.filename for i in members}) == len(members), 'duplicate archive entry')
        for i in members:
            p = pathlib.PurePosixPath(i.filename)
            mode = i.external_attr >> 16
            require(p.parts and p.parts[0] == root and '..' not in p.parts and not p.is_absolute(), 'unsafe archive path')
            require(not stat.S_ISLNK(mode) and stat.S_IFMT(mode) in (0, stat.S_IFREG, stat.S_IFDIR), 'unsafe archive type')
        require(z.testzip() is None, 'bad archive CRC')
        z.extractall(destination)
        for i in members:
            mode = (i.external_attr >> 16) & 0o777
            if mode:
                (destination / i.filename).chmod(mode)
    xc = destination / root
    libraries = plist(xc / 'Info.plist')['AvailableLibraries']
    require(len(libraries) == 1, 'expected one device slice')
    lib = libraries[0]
    require(lib['SupportedArchitectures'] == ['arm64'] and lib['SupportedPlatform'] == 'ios'
            and 'SupportedPlatformVariant' not in lib, 'wrong XCFramework slice')
    require(lib['LibraryIdentifier'] == 'ios-arm64' and lib['LibraryPath'] == name + '.framework', 'wrong slice path')
    return xc / lib['LibraryIdentifier'] / lib['LibraryPath']


def prepare(destination, local_assets=None):
    pins = json.loads(PINS.read_text())
    if local_assets is None:
        commit, url = pins['release_commit'], pins['release_base_url']
        require(isinstance(commit, str) and re.fullmatch(r'[0-9a-f]{40}', commit),
                'immutable release inputs unresolved: release_commit')
        require(url == 'https://github.com/Nodman/OpenJK-iOS/releases/download/' + pins['identity'],
                'immutable release inputs unresolved: release_base_url')
    require(not destination.is_symlink(), 'symlink package destination')
    destination = destination.resolve()
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='angle-consume-', dir=destination.parent) as temp:
        stage = pathlib.Path(temp)
        for name, expected in pins['assets'].items():
            target = stage / name
            if local_assets is not None:
                shutil.copyfile(local_assets / name, target)
            else:
                with urllib.request.urlopen(pins['release_base_url'] + '/' + name, timeout=120) as response:
                    require(response.url.startswith('https://'), 'insecure asset redirect')
                    target.write_bytes(response.read())
            require(sha(target) == expected, 'bad SHA256: ' + name)
        manifest = json.loads((stage / 'manifest.json').read_text())
        for key in ('identity', 'recipe_sha256', 'payload_sha256', 'args_sha256'):
            require(manifest[key] == pins[key], 'wrong manifest ' + key)
        require(manifest['variant'] == 'qualified', 'comparison variant forbidden')
        expected_sums = ''.join(pins['assets'][n + '.xcframework.zip'] + '  ' + n + '.xcframework.zip\n' for n in NAMES)
        require((stage / 'SHA256SUMS').read_text() == expected_sums, 'wrong archive checksum set')
        for name in NAMES:
            fw = extract(stage / (name + '.xcframework.zip'), stage, name)
            binary = audit(fw, name)
            require(sha(binary) == pins['frameworks'][name]['sha256'], 'wrong qualified binary')
            (stage / 'Frameworks').mkdir(exist_ok=True)
            embedded = stage / 'Frameworks' / fw.name
            shutil.copytree(fw, embedded)
            shutil.rmtree(embedded / 'Headers')
        # Compare the complete prepared tree on every configure, including reused inputs.
        if destination.exists():
            def inventory(root):
                require(not any(p.is_symlink() for p in root.rglob('*')), 'symlink in prepared package')
                return {p.relative_to(root).as_posix(): (sha(p), stat.S_IMODE(p.stat().st_mode))
                        for p in root.rglob('*') if p.is_file()}
            require(inventory(destination) == inventory(stage), 'prepared package changed; use a fresh destination')
        else:
            stage.rename(destination)
    print(json.dumps({'identity': pins['identity'], 'source': 'local fixture' if local_assets else 'immutable release',
                      'frameworks': pins['frameworks']}, indent=2))


def same_signed_payload(binary, original):
    expected, actual = original.read_bytes(), binary.read_bytes()

    # Signing adds LC_CODE_SIGNATURE and extends only __LINKEDIT.
    def commands(data):
        require(struct.unpack_from('<I', data)[0] == 0xfeedfacf, 'expected 64-bit Mach-O')
        count, size = struct.unpack_from('<II', data, 16)
        offset, result, signature, segments = 32, [], None, []
        for _ in range(count):
            kind, length = struct.unpack_from('<II', data, offset)
            require(length >= 8 and offset + length <= 32 + size, 'invalid load command')
            command = bytearray(data[offset:offset + length])
            if kind == 0x1d:
                require(signature is None and length == 16, 'invalid signature command')
                signature = struct.unpack_from('<II', command, 8)
            else:
                if kind == 0x19:
                    require(length >= 72, 'invalid segment command')
                    segments.append((len(result), struct.unpack_from('<16sQQQQiiII', command, 8)))
                result.append(command)
            offset += length
        require(offset == 32 + size, 'invalid load command size')
        return result, signature, offset, segments

    original_commands, unsigned_signature, original_end, original_segments = commands(expected)
    signed_commands, signature, signed_end, signed_segments = commands(actual)
    require(unsigned_signature is None and signature is not None, 'unexpected signature layout')
    offset, size = signature
    require(size > 0 and offset >= len(expected) and offset + size == len(actual)
            and not any(actual[len(expected):offset]), 'unexpected signature extent')
    # The qualified arm64 device layout uses 16 KiB segment pages, not CodeDirectory hash pages.
    page = 1 << 14
    require(struct.unpack_from('<I', expected, 4)[0] == 0x100000c, 'expected arm64 Mach-O')
    link_commands = []
    for data, load_commands, segments in ((expected, original_commands, original_segments),
                                          (actual, signed_commands, signed_segments)):
        links = [(index, segment) for index, segment in segments if segment[0].rstrip(b'\0') == b'__LINKEDIT']
        require(len(links) == 1 and links[0] == segments[-1], 'expected unique terminal LINKEDIT')
        index, (_, vmaddr, vmsize, fileoff, filesize, _, _, nsects, _) = links[0]
        require(nsects == 0 and vmaddr % page == 0 and fileoff % page == 0
                and fileoff + filesize == len(data)
                and vmsize == (filesize + page - 1) // page * page,
                'unexpected LINKEDIT geometry')
        require(all(segment[1] % page == 0 and segment[2] % page == 0
                    and segment[3] % page == 0
                    and segment[1] + segment[2] <= vmaddr
                    and segment[3] + segment[4] <= fileoff
                    for _, segment in segments[:-1]), 'unexpected segment layout')
        link_commands.append(load_commands[index])
    for command in link_commands:
        command[32:40] = b'\0' * 8
        command[48:56] = b'\0' * 8
    require(original_commands == signed_commands, 'signed load commands changed')
    require(expected[:16] == actual[:16] and expected[24:32] == actual[24:32], 'signed Mach-O header changed')
    require(signed_end >= original_end and not any(expected[original_end:signed_end]), 'signature overwrote payload')
    require(expected[signed_end:] == actual[signed_end:len(expected)], 'signed payload changed')


def check_app(app, prepared, signed):
    pins = json.loads(PINS.read_text())
    for name in NAMES:
        matches = list(app.rglob(name + '.framework'))
        fw = app / 'Frameworks' / (name + '.framework')
        require(matches == [fw], 'missing or duplicate pair: ' + name)
        require(not (fw / 'Headers').exists(), 'bundled provider headers')
        binary = audit(fw, name)
        source = prepared / 'Frameworks' / fw.name
        def resources(root):
            require(not any(p.is_symlink() for p in root.rglob('*')), 'symlink in embedded framework')
            return {p.relative_to(root).as_posix(): sha(p) for p in root.rglob('*')
                    if p.is_file() and p.name != name and p.relative_to(root).parts[0] != '_CodeSignature'}
        require(resources(fw) == resources(source), 'embedded framework resources changed')
        original = source / name
        require(sha(original) == pins['frameworks'][name]['sha256'], 'prepared binary changed')
        if signed:
            run('codesign', '--verify', '--strict', str(fw))
            same_signed_payload(binary, original)
            def team(path):
                output = run('codesign', '-d', '--verbose=4', str(path))
                match = re.search(r'^TeamIdentifier=(.+)$', output, re.M)
                require(match and match[1] != 'not set', 'missing signing team')
                return match[1]
            require(team(fw) == team(app), 'framework/app signing team mismatch')
        else:
            require(sha(binary) == pins['frameworks'][name]['sha256'], 'embedded binary changed')
            require('LC_CODE_SIGNATURE' not in run('otool', '-l', str(binary)), 'unexpected signed framework')
    executable = app / plist(app / 'Info.plist')['CFBundleExecutable']
    commands = run('otool', '-l', str(executable))
    require('@executable_path/Frameworks' in re.findall(r'cmd LC_RPATH\n.*?path ([^ ]+)', commands, re.S), 'missing app rpath')
    require(not re.search(r'lib(?:EGL|GLESv2)\.framework', run('otool', '-L', str(executable))), 'hard-linked provider')
    require(not list(app.rglob('*.pk3')), 'game assets in app')
    if signed:
        run('codesign', '--verify', '--deep', '--strict', str(app))
    print('PASS:', app, 'signed' if signed else 'unsigned', pins['frameworks'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    fetch = sub.add_parser('prepare')
    fetch.add_argument('--destination', type=pathlib.Path, required=True)
    fetch.add_argument('--local-assets', type=pathlib.Path, help='Explicit offline fixture; identical pin validation applies')
    check = sub.add_parser('check-app')
    check.add_argument('--app', type=pathlib.Path, required=True)
    check.add_argument('--prepared', type=pathlib.Path, required=True)
    check.add_argument('--signed', action='store_true')
    args = parser.parse_args()
    if args.command == 'prepare':
        prepare(args.destination, args.local_assets)
    else:
        check_app(args.app, args.prepared, args.signed)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as error:
        raise SystemExit('ANGLE consumer: ' + str(error)) from error
