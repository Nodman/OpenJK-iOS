import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shutil
import stat
import tempfile
import subprocess
import sys
import time
import zipfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
PINS = json.loads((HERE / 'angle-pins.json').read_text())
PY_CACHE = tempfile.TemporaryDirectory(prefix='angle-python-cache-')
ENV = dict(os.environ, DEPOT_TOOLS_UPDATE='0', PYTHONDONTWRITEBYTECODE='1',
           PYTHONPYCACHEPREFIX=PY_CACHE.name)
GIT = shutil.which('git')
NAMES = ('libEGL', 'libGLESv2')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def run(*args, cwd=None):
    if args[0] == 'git':
        args = (GIT, *args[1:])
    return subprocess.check_output([str(a) for a in args], cwd=cwd, env=ENV, text=True, stderr=subprocess.STDOUT).strip()


def inputs():
    for path, expected in PINS['inputs'].items():
        require(sha(REPO / path) == expected, 'Recipe input mismatch: ' + path)
    recipe_files = ('angle-package.py', 'angle-pins.json', 'angle-recipe.py', 'angle-tests.py')
    return {'scripts/ios/' + name: sha(HERE / name) for name in recipe_files} | PINS['inputs'] | {
        '.github/workflows/angle-apple.yml': sha(REPO / '.github/workflows/angle-apple.yml'),
        'scripts/ios/build-angle-apple.sh': sha(HERE / 'build-angle-apple.sh'),
        'cmake/ios/patches/angle-rgb-batch.patch': sha(REPO / 'cmake/ios/patches/angle-rgb-batch.patch'),
        'cmake/ios/patches/angle-rgb-direct-staging.patch': sha(REPO / 'cmake/ios/patches/angle-rgb-direct-staging.patch')}


def host():
    require(platform.system() == 'Darwin' and platform.machine() == 'arm64', 'Requires native arm64 macOS')
    checks = {
        'xcode': ('xcodebuild', '-version'),
        'sdk': ('xcrun', '--sdk', 'iphoneos', '--show-sdk-version'),
        'sdk_build': ('xcrun', '--sdk', 'iphoneos', '--show-sdk-build-version'),
        'macos': ('sw_vers', '-productVersion'),
        'macos_build': ('sw_vers', '-buildVersion'),
        'python': (sys.executable, '--version'), 'git': ('git', '--version')}
    actual = {k: run(*v) for k, v in checks.items()}
    for key, value in actual.items():
        require(value == PINS['tools'][key], 'Unsupported ' + key + ': ' + value)
    actual.update(architecture=platform.machine(), python=run(sys.executable, '--version'),
                  git=run('git', '--version'), developer_dir=run('xcode-select', '-p'),
                  sdk_path=run('xcrun', '--sdk', 'iphoneos', '--show-sdk-path'))
    return actual


def runtime_files(path, expected=None, tracked=()):
    files = {}
    for parent, dirs, names in os.walk(path, followlinks=True):
        relative = Path(parent).relative_to(path)
        dirs[:] = [d for d in dirs if d not in ('.git', '.cipd', '.versions', '__pycache__', '.cipd_client_cache')]
        for name in names + [d for d in dirs if (Path(parent) / d).is_symlink()]:
            f = Path(parent) / name
            key = str(relative / name)
            if key == 'metrics.cfg':
                continue
            if f.is_symlink():
                if expected is not None and key not in tracked:
                    require(key in expected and os.readlink(f) == expected[key].get('link'), 'Depot runtime symlink mismatch: ' + key)
                require(f.resolve().is_relative_to(path.resolve()), 'External depot runtime symlink: ' + key)
                files[key] = {'link': os.readlink(f), 'sha256': sha(f) if f.is_file() else None}
            else:
                require(f.is_file(), 'Invalid depot runtime type: ' + key)
                files[key] = {'sha256': sha(f)}
    return files


def acquisition_environment():
    for key, value in ENV.items():
        selector = key in ('CUSTOM_CIPD_CLIENT', 'DEPOT_TOOLS_GSUTIL_BIN_DIR', 'ARCH_MAC_OVERRIDE')
        override = key.startswith(('VPYTHON_', 'UV_')) or key in ('DEPOT_TOOLS_DIR', 'DEPOT_TOOLS_UNAME_S', 'CIPD_ROOT', 'PYTHONPATH', 'PYTHONHOME')
        require(not (selector or (override and value)), 'Unsupported depot environment override: ' + key)


def depot(root, bootstrap=False):
    acquisition_environment()
    path = root / 'depot_tools'
    require(run('git', '-C', path, 'rev-parse', 'HEAD') == PINS['depot_tools'], 'Executed depot_tools pin mismatch')
    require(not run('git', '-C', path, 'status', '--porcelain', '--untracked-files=no'), 'Dirty executed depot_tools')
    if bootstrap:
        require(not run('git', '-C', path, 'ls-files', '--others'), 'Bootstrap requires pristine depot checkout')
    else:
        tracked = set(run('git', '-C', path, 'ls-files').splitlines())
        runtime = {k: v for k, v in runtime_files(path, PINS['depot_runtime'], tracked).items() if k not in tracked}
        require(runtime == PINS['depot_runtime'], 'Depot runtime/override mismatch')
    ENV['PATH'] = str(path) + os.pathsep + os.environ['PATH']
    return path


def verify_source(root, patched=False):
    source = root / 'angle'
    depot(root)
    require(sha(source / '.gclient') == PINS['inputs']['scripts/ios/angle.gclient'], '.gclient mismatch')
    deps = json.loads((HERE / 'angle-resolved-deps.json').read_text())
    require(deps['.']['rev'] == PINS['angle'], 'ANGLE recipe pin mismatch')
    require(deps['third_party/depot_tools']['rev'] == PINS['deps_vendored_depot_tools'], 'Vendored depot_tools recipe pin mismatch')
    for path, value in deps.items():
        if value['rev']:
            tree = source / path
            require(run('git', '-C', tree, 'rev-parse', 'HEAD') == value['rev'], 'Git pin mismatch: ' + path)
            changed = run('git', '-C', tree, 'diff', '--name-only', 'HEAD').splitlines()
            expected = sorted(PINS['patched_files']) if patched and path == '.' else []
            require(sorted(changed) == expected, 'Dirty dependency: ' + path + ': ' + repr(changed))
            if path != '.':
                require(not run('git', '-C', tree, 'ls-files', '--others', '--exclude-standard'), 'Untracked dependency files: ' + path)
    allowed = {'.gclient', '.gclient_entries', '.gclient_previous_sync_commits'}
    untracked = run('git', '-C', source, 'ls-files', '--others', '--exclude-standard').splitlines()
    require(set(untracked) <= allowed, 'Unexpected source files: ' + repr(untracked))
    for path, expected in PINS['deps_files'].items():
        require(sha(source / path) == expected, 'DEPS mismatch: ' + path)
    for path, expected in PINS['tool_files'].items():
        require(sha(source / path) == expected, 'Tool bytes mismatch: ' + path)
    for key, path in [('gn', 'buildtools/mac/gn'), ('ninja', 'third_party/ninja/ninja')]:
        require(run(source / path, '--version') == PINS['tools'][key], key + ' version mismatch')
    tool_roots = ('buildtools/mac', 'third_party/ninja', 'third_party/llvm-build/Release+Asserts', 'third_party/cpython3/host')
    tool_files = {str(f.relative_to(source)) for base in tool_roots for f in (source / base).rglob('*')
                  if f.is_file() and '.cipd' not in f.parts and '.versions' not in f.parts
                  and '__pycache__' not in f.parts and not f.name.endswith('.pyc')}
    require(tool_files == set(PINS['tool_files']), 'Unexpected/missing acquired tool files')
    actual_file = root / 'resolved-actual.json'
    run(root / 'depot_tools/gclient', 'revinfo', '--actual', '--output-json', actual_file, cwd=source)
    require(json.loads(actual_file.read_text()) == deps, 'Resolved Git/CIPD/GCS identity mismatch')
    clang = run(source / 'third_party/llvm-build/Release+Asserts/bin/clang', '--version')
    require('clang version 24.0.0git' in clang, 'Clang version mismatch')
    require((source / 'third_party/llvm-build/Release+Asserts/cr_build_revision').read_text().strip() == PINS['tools']['clang_package'], 'Clang package mismatch')
    if patched:
        for path, expected in PINS['patched_files'].items():
            require(sha(source / path) == expected, 'Patched file mismatch: ' + path)
    return {'resolved_dependencies': deps, 'tool_files': PINS['tool_files'], 'clang_version': clang}


def sync(root):
    acquisition_environment()
    require(not (root / 'angle').exists() and not (root / 'depot_tools').exists(), 'Sync requires new source/depot destinations')
    root.mkdir(parents=True, exist_ok=True)
    for directory, url, rev in [('depot_tools', 'https://chromium.googlesource.com/chromium/tools/depot_tools.git', PINS['depot_tools']),
                                ('angle', 'https://chromium.googlesource.com/angle/angle.git', PINS['angle'])]:
        tree = root / directory
        run('git', 'init', tree)
        run('git', '-C', tree, 'remote', 'add', 'origin', url)
        run('git', '-C', tree, 'fetch', '--depth=1', 'origin', rev)
        run('git', '-C', tree, 'checkout', '--detach', 'FETCH_HEAD')
    d = depot(root, bootstrap=True)
    shutil.copyfile(HERE / 'angle.gclient', root / 'angle/.gclient')
    print(run(d / 'gclient', 'sync', '--no-history', '--shallow', '--nohooks', '--jobs=6', cwd=root / 'angle'), flush=True)
    verify_source(root)


def timed(command, cwd, log):
    start = time.monotonic()
    with log.open('w') as f:
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=ENV, stdout=f, stderr=subprocess.STDOUT)
    record = {'command': list(map(str, command)), 'seconds': time.monotonic() - start, 'exit_code': result.returncode}
    write(log.with_suffix('.json'), record)
    require(result.returncode == 0, 'Command failed; see ' + str(log))
    return record


def variant_args(variant):
    text = (HERE / 'angle-args.gn').read_bytes()
    if variant == 'libcxx-on':
        text = text.replace(b'use_custom_libcxx = false', b'use_custom_libcxx = true')
        text = text.replace(b'use_custom_libcxx_for_host = false', b'use_custom_libcxx_for_host = true')
    return text


def build(root, variant):
    recipe_inputs = inputs()
    tools = host()
    require((root / '.angle-recipe-owned').is_file(), 'Build needs an explicitly owned recipe root')
    source = root / 'angle'
    output = source / ('out/ios-arm64-release' if variant == 'qualified' else 'out/ios-arm64-libcxx-on')
    package = root / ('package-' + variant)
    require(not output.exists() and not package.exists(), 'Refusing output/package overwrite')
    patch = REPO / 'cmake/ios/patches/angle-rgb-batch.patch'
    if variant == 'qualified':
        provenance = verify_source(root)
        run('git', '-C', source, 'apply', '--check', patch)
        run('git', '-C', source, 'apply', patch)
        run('git', '-C', source, 'apply', REPO / 'cmake/ios/patches/angle-rgb-direct-staging.patch')
    else:
        provenance = verify_source(root, patched=True)
    for path, expected in PINS['patched_files'].items():
        require(sha(source / path) == expected, 'Patched file mismatch: ' + path)
    evidence = root / ('evidence-' + variant)
    evidence.mkdir()
    timings = []
    timings.append(timed([sys.executable, 'build/util/lastchange.py', '-o', 'build/util/LASTCHANGE'], source, evidence / 'lastchange.log'))
    output.mkdir(parents=True)
    (output / 'args.gn').write_bytes(variant_args(variant))
    timings.append(timed(['buildtools/mac/gn', 'gen', output.relative_to(source), '--fail-on-unused-args'], source, evidence / 'gn.log'))
    timings.append(timed(['third_party/ninja/ninja', '-C', output.relative_to(source), '-j6', 'libEGL', 'libGLESv2'], source, evidence / 'ninja.log'))
    graph = run(source / 'buildtools/mac/gn', 'desc', output, '//:libGLESv2', 'deps', '--all', cwd=source)
    (evidence / 'dependency-graph.txt').write_text(graph + '\n')
    require('renderer/metal' in graph, 'Metal renderer absent')
    for backend in ('vulkan', 'gl', 'd3d', 'null', 'wgpu'):
        require(not re.search(r'renderer/' + backend + r'[:/]', graph), 'Unexpected backend: ' + backend)
    for target in ('swiftshader', 'perfetto', 'protobuf', 'angle_end2end_tests', 'samples'):
        require(target not in graph.lower(), 'Unexpected dependency: ' + target)
    timed([sys.executable, HERE / 'angle-package.py', source, output, package], REPO, evidence / 'package.log')
    shutil.copyfile(output / 'args.gn', package / 'args.gn')
    recipe = package / 'recipe'
    recipe.mkdir()
    for relative in recipe_inputs:
        target = recipe / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(REPO / relative, target)
    archives = {}
    for name in NAMES:
        xc = package / 'xcframeworks' / (name + '.xcframework')
        archive = package / (name + '.xcframework.zip')
        with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for path in sorted(xc.rglob('*')):
                require(not path.is_symlink(), 'Unexpected archive symlink')
                if path.is_file():
                    require(not path.name.startswith('._') and '__MACOSX' not in path.parts, 'Resource fork')
                    z.write(path, path.relative_to(xc.parent))
        archives[archive.name] = {'sha256': sha(archive), 'bytes': archive.stat().st_size}
    (package / 'SHA256SUMS').write_text(''.join(v['sha256'] + '  ' + k + '\n' for k, v in archives.items()))
    recipe_identity = digest({'inputs': recipe_inputs, 'tools': tools, 'variant': variant,
                              'args_sha256': sha(package / 'args.gn')})
    payload_identity = digest(archives)
    identity = 'angle-' + PINS['angle'][:12] + '-rgb-' + PINS['recipe_revision'] + '-' + recipe_identity[:16] + '-' + payload_identity[:16] + '-ios-arm64'
    manifest = {'schema': 1, 'identity': identity, 'variant': variant, 'recipe_sha256': recipe_identity,
                'payload_sha256': payload_identity, 'recipe_inputs': recipe_inputs, 'pins': PINS,
                'tools': tools, 'provenance': provenance, 'args_sha256': sha(package / 'args.gn'),
                'args': (package / 'args.gn').read_text(), 'archives': archives, 'timings': timings,
                'frameworks': json.loads((package / 'audits/framework-verification.json').read_text()),
                'device_qualification': 'Not performed on these rebuilt/repacked bytes',
                'qualified_provenance': PINS['qualified_provenance'],
                'inventory': {str(p.relative_to(package)): sha(p) for p in sorted(package.rglob('*')) if p.is_file()}}
    write(package / 'manifest.json', manifest)
    verify_package(package)
    print(identity, flush=True)


def audit_binary(binary, name):
    require(run('lipo', '-archs', binary) == 'arm64', 'Binary requires arm64 only')
    require('dynamically linked shared library arm64' in run('file', binary), 'Requires dynamic framework')
    version = run('xcrun', 'vtool', '-show-build', binary)
    require(re.search(r'platform IOS\s', version) and re.search(r'minos 17\.0\s', version)
            and re.search(r'sdk 26\.5\s', version), 'Bad Mach-O device/minimum OS/SDK')
    paths = [line.strip().split(' (')[0] for line in run('otool', '-L', binary).splitlines()[1:]]
    require(paths[0] == '@rpath/' + name + '.framework/' + name, 'Bad install name')
    require(all(p.startswith(('/System/Library/', '/usr/lib/')) for p in paths[1:]), 'Non-system load path')
    commands = run('otool', '-l', binary)
    require('LC_CODE_SIGNATURE' not in commands, 'Expected unsigned framework')
    rpaths = re.findall(r'cmd LC_RPATH\n.*?path ([^ ]+)', commands, re.S)
    require(all(p.startswith(('@executable_path/', '@loader_path/', '@rpath/')) for p in rpaths), 'Bad runtime path')


def verify_package(package):
    require(not package.is_symlink(), 'Package symlink')
    require(all(not p.is_symlink() and (p.is_file() or p.is_dir()) for p in package.rglob('*')), 'Package requires regular files/directories')
    manifest = json.loads((package / 'manifest.json').read_text())
    require(manifest['variant'] in ('qualified', 'libcxx-on'), 'Unknown variant')
    require(manifest['pins'] == PINS, 'Manifest pins mismatch')
    for path, expected in manifest['inventory'].items():
        require(sha(package / path) == expected, 'Package file mismatch: ' + path)
    actual_files = {str(p.relative_to(package)) for p in package.rglob('*') if p.is_file()}
    require(actual_files == set(manifest['inventory']) | {'manifest.json'}, 'Unexpected/missing package files')
    for relative, expected in manifest['recipe_inputs'].items():
        require(sha(package / 'recipe' / relative) == expected, 'Bundled recipe mismatch')
    require(sha(package / 'args.gn') == manifest['args_sha256'], 'Packaged args mismatch')
    require((package / 'args.gn').read_bytes() == variant_args(manifest['variant']), 'Unapproved variant args')
    require(manifest['recipe_sha256'] == digest({'inputs': manifest['recipe_inputs'], 'tools': manifest['tools'],
            'variant': manifest['variant'], 'args_sha256': manifest['args_sha256']}), 'Recipe identity mismatch')
    require(manifest['payload_sha256'] == digest(manifest['archives']), 'Payload identity mismatch')
    expected_identity = 'angle-' + PINS['angle'][:12] + '-rgb-' + PINS['recipe_revision'] + '-' + manifest['recipe_sha256'][:16] + '-' + manifest['payload_sha256'][:16] + '-ios-arm64'
    require(manifest['identity'] == expected_identity, 'Release identity mismatch')
    require(set(manifest['archives']) == {n + '.xcframework.zip' for n in NAMES}, 'Archive set mismatch')
    checksums = ''
    for name in NAMES:
        archive = package / (name + '.xcframework.zip')
        entry = manifest['archives'][archive.name]
        require(sha(archive) == entry['sha256'] and archive.stat().st_size == entry['bytes'], 'Archive identity mismatch')
        checksums += entry['sha256'] + '  ' + archive.name + '\n'
        with zipfile.ZipFile(archive) as z:
            require(z.testzip() is None, 'ZIP CRC failure')
            members = z.namelist()
            require(len(members) == len(set(members)), 'Duplicate ZIP members')
            require(all(p.startswith(name + '.xcframework/') and '..' not in Path(p).parts and not p.startswith('/') and '._' not in Path(p).name and '__MACOSX' not in p for p in members), 'Unsafe ZIP layout')
            xc = package / 'xcframeworks' / (name + '.xcframework')
            expected_members = {str(p.relative_to(xc.parent)) for p in xc.rglob('*') if p.is_file()}
            require(set(members) == expected_members, 'Incomplete/unexpected ZIP member set')
            require(all(i.create_system == 3 and stat.S_ISREG(i.external_attr >> 16) for i in z.infolist()), 'ZIP requires regular-file members')
            prefix = name + '.xcframework/ios-arm64/' + name + '.framework/'
            required = {prefix + name, prefix + 'Info.plist', name + '.xcframework/Info.plist'}
            required.update(prefix + 'Headers/' + p for p in PINS['framework_headers'][name])
            notices = ('LICENSE', 'third_party/abseil-cpp/LICENSE', 'third_party/astc-encoder/src/LICENSE.txt',
                       'third_party/zlib/LICENSE', 'third_party/vulkan-headers/LICENSE.txt',
                       'src/common/third_party/xxhash/LICENSE', 'src/third_party/ceval/LICENSE')
            required.update(prefix + 'Licenses/' + p for p in notices)
            require(required <= set(members), 'Missing mandatory framework payload')
            outer = plistlib.loads(z.read(name + '.xcframework/Info.plist'))
            libs = outer['AvailableLibraries']
            require(len(libs) == 1, 'Missing/extra XCFramework slice')
            lib = libs[0]
            require(lib['SupportedPlatform'] == 'ios' and lib['SupportedArchitectures'] == ['arm64'] and 'SupportedPlatformVariant' not in lib, 'Requires device arm64 slice')
            require(lib['LibraryIdentifier'] == 'ios-arm64' and lib['LibraryPath'] == name + '.framework', 'Unexpected slice path')
            prefix = name + '.xcframework/ios-arm64/' + name + '.framework/'
            audit_binary(package / 'xcframeworks' / prefix / name, name)
            inner = plistlib.loads(z.read(prefix + 'Info.plist'))
            require(inner['MinimumOSVersion'] == '17.0' and inner['CFBundleExecutable'] == name and inner['CFBundleShortVersionString'] == '1.0.0' and inner['CFBundleSupportedPlatforms'] == ['iPhoneOS'], 'Bad inner framework plist')
            for member in members:
                file = package / 'xcframeworks' / member
                require(file.is_file() and hashlib.sha256(z.read(member)).hexdigest() == sha(file), 'Archive member differs: ' + member)
    require((package / 'SHA256SUMS').read_text() == checksums, 'SHA256SUMS mismatch')
    return manifest


def package_state(package):
    require(not package.is_symlink(), 'Package symlink')
    state = {}
    for p in package.rglob('*'):
        require(not p.is_symlink(), 'Publication symlink')
        if p.is_dir():
            continue
        require(stat.S_ISREG(p.stat().st_mode), 'Publication requires regular files')
        st = p.stat()
        state[str(p.relative_to(package))] = (st.st_dev, st.st_ino, st.st_size, st.st_mtime_ns, st.st_ctime_ns, sha(p))
    return state


def publish(package, repo, target):
    before = package_state(package)
    with tempfile.TemporaryDirectory(prefix='angle-publication-') as private:
        snapshot = Path(private) / 'package'
        shutil.copytree(package, snapshot, symlinks=True)
        after = package_state(package)
        copied = package_state(snapshot)
        require(before == after and {k: v[-1] for k, v in before.items()} ==
                {k: v[-1] for k, v in copied.items()}, 'Package changed during publication copy')
        manifest = verify_package(snapshot)
        publish_snapshot(snapshot, repo, target, manifest, copied)


def publish_snapshot(package, repo, target, manifest, validated):
    require(manifest['variant'] == 'qualified', 'Comparison variant cannot be published')
    require(manifest['recipe_inputs'] == inputs(), 'Publication helper differs from packaged recipe')
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo), 'Expected owner/repository')
    require(re.fullmatch(r'[0-9a-f]{40}', target), 'Publication requires full reviewed app commit')
    for relative in manifest['recipe_inputs']:
        blob = run('git', '-C', REPO, 'rev-parse', target + ':' + relative)
        require(blob == run('git', 'hash-object', package / 'recipe' / relative), 'Publication target lacks exact recipe: ' + relative)
    tag = manifest['identity']
    # A failed lookup is never treated as proof that a release is absent.
    tags = json.loads(run('gh', 'api', '--paginate', '--slurp', 'repos/' + repo + '/git/matching-refs/tags/' + tag))
    require(not any(page for page in tags), 'Refusing existing tag')
    releases = json.loads(run('gh', 'api', '--paginate', '--slurp', 'repos/' + repo + '/releases?per_page=100'))
    require(not any(r['tag_name'] == tag for page in releases for r in page), 'Refusing existing release/assets')
    require(package_state(package) == validated, 'Publication snapshot changed before remote mutation')
    run('gh', 'api', '--method', 'POST', 'repos/' + repo + '/git/refs',
        '-f', 'ref=refs/tags/' + tag, '-f', 'sha=' + target)
    assets = [package / (n + '.xcframework.zip') for n in NAMES] + [package / 'SHA256SUMS', package / 'manifest.json']
    print(run('gh', 'release', 'create', tag, '--repo', repo, '--verify-tag', '--title', tag,
              '--notes', 'Pinned Metal-only iOS17 arm64 dependency. New payload; device qualification is recorded separately.', *assets))


def main():
    parser = argparse.ArgumentParser(description='Pinned device-only ANGLE recipe; no implicit publication')
    parser.add_argument('command', choices=['preflight', 'cache-key', 'sync', 'verify-source', 'build', 'verify-package', 'publish'])
    parser.add_argument('--root', type=Path)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--variant', choices=['qualified', 'libcxx-on'], default='qualified')
    parser.add_argument('--repo')
    parser.add_argument('--target')
    args = parser.parse_args()
    if args.command == 'preflight':
        print(json.dumps({'inputs': inputs(), 'tools': host()}, indent=2))
    elif args.command == 'cache-key':
        print(digest({'inputs': inputs(), 'tools': host()}))
    elif args.command == 'verify-package':
        print(verify_package(args.package)['identity'])
    elif args.command == 'publish':
        publish(args.package, args.repo, args.target)
    else:
        require(args.root is not None and args.root.is_absolute(), 'Supply absolute --root')
        inputs()
        host()
        if args.command == 'sync':
            sync(args.root)
            (args.root / '.angle-recipe-owned').write_text('Owned dependency recipe root\n')
        elif args.command == 'verify-source':
            verify_source(args.root)
        else:
            build(args.root, args.variant)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, KeyError) as error:
        sys.exit(str(error))
