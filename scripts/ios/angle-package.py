import hashlib
import json
import pathlib
import plistlib
import re
import shutil
import subprocess
import sys

if sys.flags.optimize:
    raise SystemExit("Packaging audits require Python without -O")
source, output, package = map(pathlib.Path, sys.argv[1:])
if package.exists():
    raise SystemExit("Refusing existing package: " + str(package))
package.mkdir(parents=True)
root = source.parent
evidence = package / 'audits'
evidence.mkdir()
shutil.copytree(source / 'include', package / 'include')
def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT)
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

notices = package / 'licenses'
license_files = [
    'LICENSE',
    'third_party/abseil-cpp/LICENSE',
    'third_party/astc-encoder/src/LICENSE.txt',
    'third_party/zlib/LICENSE',
    'third_party/vulkan-headers/LICENSE.txt',
    'src/common/third_party/xxhash/LICENSE',
    'src/third_party/ceval/LICENSE',
]
for relative in license_files:
    target = notices / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / relative, target)

frameworks = []
for name in ('libEGL', 'libGLESv2'):
    original = output / (name + '.framework')
    destination = package / 'frameworks' / original.name
    shutil.copytree(original, destination, symlinks=True)
    binary = destination / name
    raw_bytes = binary.stat().st_size
    raw_sha = sha(binary)
    strip_log = run('xcrun', 'strip', '-S', '-x', str(binary))
    (evidence / (name + '-strip.txt')).write_text(strip_log)
    plist_path = destination / 'Info.plist'
    plist = plistlib.loads(plist_path.read_bytes())
    assert plist['MinimumOSVersion'] == '17.0'
    assert plist['CFBundleExecutable'] == name
    assert plist['CFBundleIdentifier'] == {'libEGL': 'org.chromium.ost.libEGL', 'libGLESv2': 'org.chromium.ost.libGLESv2'}[name]
    assert plist['CFBundleSupportedPlatforms'] == ['iPhoneOS']
    plist['CFBundleShortVersionString'] = '1.0.0'
    plist_path.write_bytes(plistlib.dumps(plist))
    arch = run('lipo', '-archs', str(binary)).strip()
    assert arch == 'arm64', arch
    file_info = run('file', str(binary))
    assert 'dynamically linked shared library arm64' in file_info
    dependencies = run('otool', '-L', str(binary))
    load_paths = [line.strip().split(' (')[0] for line in dependencies.splitlines()[1:]]
    install_name = '@rpath/' + name + '.framework/' + name
    assert load_paths[0] == install_name
    assert all(path.startswith(('/System/Library/', '/usr/lib/')) for path in load_paths[1:]), load_paths
    build_version = run('xcrun', 'vtool', '-show-build', str(binary))
    assert re.search(r'platform IOS\s', build_version)
    assert re.search(r'minos 17\.0\s', build_version)
    assert re.search(r'sdk 26\.5\s', build_version)
    load_commands = run('otool', '-l', str(binary))
    rpaths = re.findall(r'cmd LC_RPATH\n.*?path ([^ ]+)', load_commands, re.S)
    assert all(path.startswith(('@executable_path/', '@loader_path/', '@rpath/')) for path in rpaths)
    assert all(str(root) not in path for path in load_paths)
    assert 'LC_CODE_SIGNATURE' not in load_commands
    nm = run('nm', '-gU', str(binary))
    required = ['eglGetPlatformDisplay', 'eglInitialize', 'eglChooseConfig', 'eglCreateContext', 'eglCreateWindowSurface', 'eglMakeCurrent', 'eglSwapBuffers', 'eglGetProcAddress'] if name == 'libEGL' else ['glGetString', 'glBindFramebuffer', 'glReadPixels', 'glFinish', 'glDiscardFramebufferEXT']
    for symbol in required:
        assert re.search(r'\b_' + symbol + r'\s*$', nm, re.M), symbol
    for suffix, content in [('file', file_info), ('otool-L', dependencies), ('vtool', build_version), ('otool-l', load_commands), ('exports', nm)]:
        (evidence / (name + '-' + suffix + '.txt')).write_text(content)
    (destination / 'Headers').mkdir(exist_ok=True)
    items = ['EGL', 'KHR'] if name == 'libEGL' else ['angle_gl.h', 'GLES', 'GLES2', 'GLES3', 'KHR']
    for item in items:
        src = source / 'include' / item
        dst = destination / 'Headers' / item
        if src.is_dir():
            shutil.copytree(src, dst, dirs_exist_ok=True)
        else:
            shutil.copyfile(src, dst)
    shutil.copytree(notices, destination / 'Licenses', dirs_exist_ok=True)
    xcframework = package / 'xcframeworks' / (name + '.xcframework')
    xcframework.parent.mkdir(exist_ok=True)
    xc_log = run('xcodebuild', '-create-xcframework', '-framework', str(destination), '-output', str(xcframework))
    (evidence / (name + '-create-xcframework.txt')).write_text(xc_log)
    outer = plistlib.loads((xcframework / 'Info.plist').read_bytes())
    libs = outer['AvailableLibraries']
    assert len(libs) == 1 and libs[0]['SupportedArchitectures'] == ['arm64']
    assert libs[0]['SupportedPlatform'] == 'ios'
    assert 'SupportedPlatformVariant' not in libs[0]
    xc_binary = xcframework / libs[0]['LibraryIdentifier'] / (name + '.framework') / name
    assert sha(xc_binary) == sha(binary)
    frameworks.append(dict(name=name, framework=str(destination), xcframework=str(xcframework), architecture=arch, minimum_os=plist['MinimumOSVersion'], sdk='26.5', install_name=install_name, dynamic_load_dependencies=load_paths[1:], rpaths=rpaths, raw_binary_bytes=raw_bytes, raw_binary_sha256=raw_sha, stripped_binary_bytes=binary.stat().st_size, stripped_binary_sha256=sha(binary), packaged_framework_bytes=sum(p.stat().st_size for p in destination.rglob('*') if p.is_file()), required_exports_verified=required))
    print(name, binary.stat().st_size, sha(binary), flush=True)

(evidence / 'framework-verification.json').write_text(json.dumps(frameworks, indent=2) + '\n')
header_probe = evidence / 'header-probe.c'
header_probe.write_text('#include <EGL/egl.h>\n#include <EGL/eglext.h>\n#include <angle_gl.h>\n_Static_assert(EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE == 0x3489, "Metal backend token");\n')
sdk = run('xcrun', '--sdk', 'iphoneos', '--show-sdk-path').strip()
header_log = run('xcrun', '--sdk', 'iphoneos', 'clang', '-target', 'arm64-apple-ios17.0', '-isysroot', sdk, '-I', str(package / 'include'), '-fsyntax-only', str(header_probe))
(evidence / 'header-verification.txt').write_text(header_log + 'PASS: C headers parse for arm64-apple-ios17.0\n')
