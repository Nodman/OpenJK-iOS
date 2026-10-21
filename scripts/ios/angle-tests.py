import argparse
import copy
import importlib.util
import json
from pathlib import Path
import plistlib
import shutil
import stat
import subprocess
import zipfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('recipe', Path(__file__).with_name('angle-recipe.py'))
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)
parser = argparse.ArgumentParser(description='Local negative fixtures; release calls are always mocked')
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--fixtures', type=Path, required=True)
a = parser.parse_args()
a.fixtures.mkdir(parents=True)
results = []


def reject(name, function):
    try:
        function()
    except (ValueError, OSError, subprocess.CalledProcessError, KeyError) as error:
        results.append({'case': name, 'result': 'PASS: rejected', 'reason': str(error)})
    else:
        raise RuntimeError('Unexpected acceptance: ' + name)


# No acquisition command or fresh-root creation may precede selector rejection.
for key in ('CUSTOM_CIPD_CLIENT', 'DEPOT_TOOLS_GSUTIL_BIN_DIR', 'ARCH_MAC_OVERRIDE'):
    for value in ('/nonexistent/override', ''):
        with patch.dict(r.ENV, {key: value}):
            fresh = a.fixtures / ('fresh-' + key + ('-empty' if not value else ''))
            for label, action in (
                    ('fresh sync', lambda: r.sync(fresh)),
                    ('fresh bootstrap admission', lambda: r.depot(fresh, bootstrap=True)),
                    ('restored source', lambda: r.verify_source(a.root, patched=True))):
                before = len(results)
                with patch.object(r, 'run', side_effect=AssertionError('Command before selector rejection')) as command:
                    reject(label + ' selector ' + key + (' (empty)' if not value else ''), action)
                    command.assert_not_called()
                assert key in results[before]['reason']
                assert not fresh.exists()

original_pins = copy.deepcopy(r.PINS)
# These mutations are confined to the supplied owned fixture root.
for relative in ('angle/build/config/gclient_args.gni', 'depot_tools/.cipd_bin/vpython3'):
    path = a.root / relative
    saved = path.read_bytes()
    mode = path.stat().st_mode
    try:
        path.chmod(mode | stat.S_IWUSR)
        path.write_bytes(saved + b'corrupt-cache')
        reject('corrupt ' + relative, lambda: r.verify_source(a.root, patched=True))
    finally:
        path.write_bytes(saved)
        path.chmod(mode)
for relative in ('depot_tools/git', 'depot_tools/override.py'):
    path = a.root / relative
    assert not path.exists()
    path.write_text('untrusted override')
    reject('override ' + relative, lambda: r.depot(a.root))
    path.unlink()
for key in ('DEPOT_TOOLS_DIR', 'VPYTHON_BYPASS'):
    saved = r.ENV.get(key)
    r.ENV[key] = 'untrusted'
    reject('environment ' + key, lambda: r.depot(a.root))
    if saved is None:
        del r.ENV[key]
    else:
        r.ENV[key] = saved

for key in ('angle', 'depot_tools'):
    r.PINS[key] = '0' * 40
    reject('wrong ' + key + ' pin', lambda: r.verify_source(a.root) if key == 'angle' else r.depot(a.root))
    r.PINS = copy.deepcopy(original_pins)
original_repo = r.REPO
repo = a.fixtures / 'inputs'
for relative in r.PINS['inputs']:
    dst = repo / relative
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(original_repo / relative, dst)
r.REPO = repo
for relative in ('scripts/ios/angle-args.gn', 'cmake/ios/patches/angle-rgb-batch.patch'):
    path = repo / relative
    data = path.read_bytes()
    path.write_bytes(data + b'\n')
    reject('wrong ' + path.name, r.inputs)
    path.write_bytes(data)
r.REPO = original_repo
package = a.root / 'package-qualified'
reject('output overwrite', lambda: r.build(a.root, 'qualified'))
reject('package overwrite', lambda: subprocess.run(['python3', str(r.HERE / 'angle-package.py'), str(a.root / 'angle'), str(a.root / 'angle/out/ios-arm64-release'), str(package)], check=True, capture_output=True))


def fixture(name):
    dst = a.fixtures / name
    subprocess.run(['cp', '-cR', str(package), str(dst)], check=True)
    return dst


def reseal(pkg, omit=None, member_type=None):
    m = json.loads((pkg / 'manifest.json').read_text())
    for name in r.NAMES:
        xc = pkg / 'xcframeworks' / (name + '.xcframework')
        zpath = pkg / (name + '.xcframework.zip')
        with zipfile.ZipFile(zpath, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for p in sorted(xc.rglob('*')):
                if p.is_file():
                    relative = str(p.relative_to(xc.parent))
                    if omit and relative.endswith(omit):
                        continue
                    if member_type and relative.endswith('/libEGL'):
                        info = zipfile.ZipInfo(relative)
                        info.create_system = 3
                        info.external_attr = (member_type | 0o755) << 16
                        z.writestr(info, p.read_bytes())
                    else:
                        z.write(p, relative)
        m['archives'][zpath.name] = {'sha256': r.sha(zpath), 'bytes': zpath.stat().st_size}
    (pkg / 'SHA256SUMS').write_text(''.join(v['sha256'] + '  ' + k + '\n' for k, v in m['archives'].items()))
    m['payload_sha256'] = r.digest(m['archives'])
    m['identity'] = 'angle-' + r.PINS['angle'][:12] + '-rgb-' + r.PINS['recipe_revision'] + '-' + m['recipe_sha256'][:16] + '-' + m['payload_sha256'][:16] + '-ios-arm64'
    m['inventory'] = {str(p.relative_to(pkg)): r.sha(p) for p in sorted(pkg.rglob('*')) if p.is_file() and p.name != 'manifest.json'}
    r.write(pkg / 'manifest.json', m)


for name in ('simulator', 'missing-slice', 'inner-minimum-os', 'bad-load-path', 'binary-minimum-os'):
    pkg = fixture(name)
    xc = pkg / 'xcframeworks/libEGL.xcframework'
    outer_path = xc / 'Info.plist'
    outer = plistlib.loads(outer_path.read_bytes())
    if name == 'simulator':
        outer['AvailableLibraries'][0]['SupportedPlatformVariant'] = 'simulator'
        outer_path.write_bytes(plistlib.dumps(outer))
    elif name == 'missing-slice':
        outer['AvailableLibraries'] = []
        outer_path.write_bytes(plistlib.dumps(outer))
    elif name == 'inner-minimum-os':
        inner_path = xc / 'ios-arm64/libEGL.framework/Info.plist'
        inner = plistlib.loads(inner_path.read_bytes())
        inner['MinimumOSVersion'] = '16.0'
        inner_path.write_bytes(plistlib.dumps(inner))
    elif name == 'bad-load-path':
        subprocess.run(['xcrun', 'install_name_tool', '-id', '/tmp/unshipped/libEGL', str(xc / 'ios-arm64/libEGL.framework/libEGL')], check=True)
    else:
        binary = xc / 'ios-arm64/libEGL.framework/libEGL'
        subprocess.run(['xcrun', 'vtool', '-set-build-version', 'ios', '16.0', '26.5', '-replace', '-output', str(binary), str(binary)], check=True)
    reseal(pkg)
    reject(name, lambda: r.verify_package(pkg))

for label, omitted in (('executable', '/libEGL'), ('header', '/egl.h'), ('notice', '/Licenses/LICENSE'), ('plist', '.framework/Info.plist')):
    pkg = fixture('omitted-' + label)
    reseal(pkg, omit=omitted)
    reject('omitted ZIP ' + label, lambda: r.verify_package(pkg))
for label, member_type in (('symlink', stat.S_IFLNK), ('fifo', stat.S_IFIFO), ('directory', stat.S_IFDIR)):
    pkg = fixture('member-' + label)
    reseal(pkg, member_type=member_type)
    reject('ZIP member ' + label, lambda: r.verify_package(pkg))
pkg = fixture('missing-tree-notice')
(pkg / 'xcframeworks/libEGL.xcframework/ios-arm64/libEGL.framework/Licenses/LICENSE').unlink()
reseal(pkg)
reject('notice missing from ZIP and tree', lambda: r.verify_package(pkg))

original_run = r.run
for scenario in ('existing-tag', 'existing-release', 'lookup-failure', 'tag-race', 'create-failure', 'new-release', 'original-mutation', 'snapshot-mutation'):
    package = a.root / 'package-qualified'
    if scenario == 'original-mutation':
        package = fixture('publication-original-mutation')
    expected_assets = {n: r.sha(package / n) for n in ('libEGL.xcframework.zip', 'libGLESv2.xcframework.zip', 'manifest.json', 'SHA256SUMS')}
    calls = []
    def mock(*args, **kwargs):
        if args[:3] == ('git', '-C', r.REPO) and args[3] == 'rev-parse':
            relative = str(args[4]).split(':', 1)[1]
            return original_run('git', 'hash-object', r.REPO / relative)
        if args[0] != 'gh':
            if args[:2] == ('git', 'hash-object'):
                global snapshot_path
                snapshot_path = Path(args[2])
                while snapshot_path.name != 'package':
                    snapshot_path = snapshot_path.parent
            return original_run(*args, **kwargs)
        calls.append(list(map(str, args)))
        if args[1] == 'api':
            if '--method' in args:
                if scenario == 'tag-race':
                    raise subprocess.CalledProcessError(1, list(args), 'mock: tag created concurrently')
                return '{}'
            if scenario == 'lookup-failure':
                raise subprocess.CalledProcessError(1, list(args), 'mock: lookup failed')
            if '/git/matching-refs/' in str(args[-1]):
                if scenario == 'original-mutation':
                    (package / 'libEGL.xcframework.zip').write_bytes(b'changed after snapshot validation')
                if scenario == 'snapshot-mutation':
                    # Capture the private snapshot through the local Git hash check below.
                    (snapshot_path / 'libEGL.xcframework.zip').write_bytes(b'changed snapshot')
                return '[[{"ref":"existing"}]]' if scenario == 'existing-tag' else '[[]]'
            tag = json.loads((package / 'manifest.json').read_text())['identity']
            return json.dumps([[{'tag_name': tag}]]) if scenario == 'existing-release' else '[[]]'
        if scenario == 'create-failure':
            raise subprocess.CalledProcessError(1, list(args), 'mock: race/existing release')
        for asset in args[-4:]:
            asset = Path(asset)
            assert asset.parent != package
            assert r.sha(asset) == expected_assets[asset.name]
        return 'MOCK ONLY: release creation captured; validated snapshot bytes matched'
    r.run = mock
    action = lambda: r.publish(package, 'fixture/never-contacted', '2' * 40)
    if scenario in ('new-release', 'original-mutation'):
        action()
        assert len(calls) == 4 and calls[-1][1:3] == ['release', 'create']
        results.append({'case': scenario, 'result': 'PASS: mock only', 'calls': calls})
    else:
        reject(scenario, action)
        if scenario != 'create-failure':
            assert not any(x[1] == 'release' for x in calls)
        if scenario == 'snapshot-mutation':
            assert not any('--method' in x for x in calls)
    r.run = original_run
package = a.root / 'package-qualified'
original_copy = r.shutil.copytree
calls = []
def no_remote(*args, **kwargs):
    if args[0] == 'gh':
        calls.append(args)
        raise RuntimeError('Unexpected gh call during unsafe copy')
    return original_run(*args, **kwargs)
def racing_copy(src, dst, *args, **kwargs):
    result = original_copy(src, dst, *args, **kwargs)
    if Path(src) == package:
        (dst / 'libEGL.xcframework.zip').write_bytes(b'copy corruption')
    return result
r.run = no_remote
r.shutil.copytree = racing_copy
reject('publication copy corruption before remote calls', lambda: r.publish(package, 'fixture/never-contacted', '2' * 40))
assert not calls
r.shutil.copytree = original_copy
r.run = original_run
r.write(a.fixtures / 'results.json', results)
print(json.dumps(results, indent=2))
