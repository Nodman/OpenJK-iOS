#!/usr/bin/env python3
"""Focused consumer checks against an explicitly supplied qualified package."""
import argparse
import importlib.util
import json
import pathlib
import plistlib
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('consumer', pathlib.Path(__file__).with_name('angle-consumer.py'))
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('assets', type=pathlib.Path)
inputs, remaining = parser.parse_known_args()
ASSETS = inputs.assets.resolve()
sys.argv[1:] = remaining


class ConsumerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='angle-consumer-test-')
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)

    def rejects(self, text, function, *args):
        with self.assertRaisesRegex((ValueError, FileNotFoundError), text):
            function(*args)

    def fixture(self):
        result = self.root / 'assets'
        result.mkdir()
        for name in json.loads(c.PINS.read_text())['assets']:
            shutil.copyfile(ASSETS / name, result / name)
        return result

    def extracted(self):
        return c.extract(ASSETS / 'libEGL.xcframework.zip', self.root, 'libEGL')

    def test_actual_pair_and_reuse(self):
        dst = self.root / 'prepared'
        c.prepare(dst, ASSETS)
        c.prepare(dst, ASSETS)
        for name in c.NAMES:
            self.assertFalse((dst / 'Frameworks' / (name + '.framework') / 'Headers').exists())
            self.assertTrue((dst / (name + '.xcframework') / 'ios-arm64' / (name + '.framework') / 'Headers').exists())

    def test_missing_release(self):
        self.rejects('unresolved: release_commit', c.prepare, self.root / 'prepared')
        self.assertFalse((self.root / 'prepared').exists())

    def test_missing_asset(self):
        src = self.fixture()
        (src / 'libGLESv2.xcframework.zip').unlink()
        self.rejects('libGLESv2', c.prepare, self.root / 'prepared', src)
        self.assertFalse((self.root / 'prepared').exists())

    def test_bad_sha(self):
        src = self.fixture()
        with (src / 'libEGL.xcframework.zip').open('ab') as f:
            f.write(b'bad')
        self.rejects('bad SHA256', c.prepare, self.root / 'prepared', src)
        self.assertFalse((self.root / 'prepared').exists())

    def test_comparison_manifest(self):
        src = self.fixture()
        m = json.loads((src / 'manifest.json').read_text())
        m['variant'] = 'libcxx-on'
        (src / 'manifest.json').write_text(json.dumps(m))
        self.rejects('bad SHA256: manifest', c.prepare, self.root / 'prepared', src)

    def test_reused_package_tampering(self):
        dst = self.root / 'prepared'
        c.prepare(dst, ASSETS)
        (dst / 'Frameworks/libEGL.framework/libEGL').write_bytes(b'bad')
        self.rejects('prepared package changed', c.prepare, dst, ASSETS)

    def signed_copy(self, name):
        fw = c.extract(ASSETS / (name + '.xcframework.zip'), self.root, name)
        original = self.root / (name + '-original')
        shutil.copyfile(fw / name, original)
        subprocess.run(['codesign', '--force', '--sign', '-', str(fw)], check=True)
        subprocess.run(['codesign', '--verify', '--strict', str(fw)], check=True)
        c.audit(fw, name)
        c.same_signed_payload(fw / name, original)
        return fw / name, original

    def linkedit(self, data):
        offset = 32
        for _ in range(struct.unpack_from('<I', data, 16)[0]):
            kind, length = struct.unpack_from('<II', data, offset)
            if kind == 0x19 and data[offset + 8:offset + 24].rstrip(b'\0') == b'__LINKEDIT':
                return offset
            offset += length
        self.fail('fixture has no LINKEDIT')

    def test_adhoc_signature_preserves_payload(self):
        for name in c.NAMES:
            binary, original = self.signed_copy(name)
            data = bytearray(binary.read_bytes())
            data[20000] ^= 1
            binary.write_bytes(data)
            self.rejects('signed payload changed', c.same_signed_payload, binary, original)

    def test_independent_linkedit_size_changes(self):
        for name in c.NAMES:
            binary, original = self.signed_copy(name)
            baseline = binary.read_bytes()
            link = self.linkedit(baseline)
            for field in (32, 48):
                value = struct.unpack_from('<Q', baseline, link + field)[0]
                for changed in (0, value - 1, value + 1, value - 16384, value + 16384, 1 << 32):
                    with self.subTest(name=name, field=field, changed=changed):
                        data = bytearray(baseline)
                        struct.pack_into('<Q', data, link + field, changed)
                        binary.write_bytes(data)
                        self.rejects('LINKEDIT geometry', c.same_signed_payload, binary, original)

    def test_linkedit_start_and_other_fields(self):
        binary, original = self.signed_copy('libEGL')
        baseline = binary.read_bytes()
        link = self.linkedit(baseline)
        for field, fmt in ((24, '<Q'), (40, '<Q'), (56, '<I'), (60, '<I'), (68, '<I')):
            with self.subTest(field=field):
                data = bytearray(baseline)
                value = struct.unpack_from(fmt, data, link + field)[0]
                struct.pack_into(fmt, data, link + field, value + 16384)
                binary.write_bytes(data)
                self.rejects('LINKEDIT geometry|segment layout|load commands changed',
                             c.same_signed_payload, binary, original)

    def test_linkedit_must_be_unique_and_terminal(self):
        binary, original = self.signed_copy('libEGL')
        baseline = binary.read_bytes()
        link = self.linkedit(baseline)
        for layout in ('missing', 'duplicate', 'nonterminal'):
            with self.subTest(layout=layout):
                data = bytearray(baseline)
                if layout != 'missing':
                    data[40:56] = b'__LINKEDIT'.ljust(16, b'\0')
                if layout != 'duplicate':
                    data[link + 8:link + 24] = b'__OTHER'.ljust(16, b'\0')
                binary.write_bytes(data)
                self.rejects('unique terminal LINKEDIT', c.same_signed_payload, binary, original)

    def test_missing_binary(self):
        fw = self.extracted()
        (fw / 'libEGL').unlink()
        with self.assertRaises(subprocess.CalledProcessError):
            c.audit(fw, 'libEGL')

    def test_wrong_inner_minos(self):
        fw = self.extracted()
        path = fw / 'Info.plist'
        info = c.plist(path)
        info['MinimumOSVersion'] = '14.0'
        path.write_bytes(plistlib.dumps(info))
        self.rejects('minimum OS', c.audit, fw, 'libEGL')

    def test_wrong_loadpath(self):
        fw = self.extracted()
        subprocess.run(['install_name_tool', '-id', '/tmp/libEGL', str(fw / 'libEGL')], check=True)
        self.rejects('install name', c.audit, fw, 'libEGL')

    def test_wrong_slice(self):
        archive = self.root / 'wrong.zip'
        with zipfile.ZipFile(ASSETS / 'libEGL.xcframework.zip') as src, zipfile.ZipFile(archive, 'w') as dst:
            for info in src.infolist():
                data = src.read(info)
                if info.filename == 'libEGL.xcframework/Info.plist':
                    p = plistlib.loads(data)
                    p['AvailableLibraries'][0]['SupportedPlatformVariant'] = 'simulator'
                    data = plistlib.dumps(p)
                dst.writestr(info, data)
        self.rejects('wrong XCFramework slice', c.extract, archive, self.root / 'out', 'libEGL')

    def test_traversal(self):
        archive = self.root / 'bad.zip'
        with zipfile.ZipFile(archive, 'w') as dst:
            dst.writestr('libEGL.xcframework/../../escape', 'bad')
        self.rejects('unsafe archive path', c.extract, archive, self.root / 'out', 'libEGL')

    def test_duplicate_pair(self):
        app = self.root / 'app'
        for name in c.NAMES:
            source = ASSETS / 'frameworks' / (name + '.framework')
            target = app / 'Frameworks' / source.name
            shutil.copytree(source, target)
            shutil.rmtree(target / 'Headers')
        shutil.copytree(app / 'Frameworks/libEGL.framework', app / 'extra/libEGL.framework')
        self.rejects('duplicate pair', c.check_app, app, self.root, False)

    def test_missing_pair(self):
        self.rejects('missing or duplicate pair', c.check_app, self.root / 'empty.app', self.root, False)


if __name__ == '__main__':
    unittest.main()
