#!/usr/bin/env python3
"""Symbol-gate fixtures; no app products or devices are modified."""
import contextlib
import importlib.util
import io
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('gate', ROOT / 'scripts/ios/check-modules.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class Symbols(unittest.TestCase):
    def test_undefined_layouts(self):
        for row in ('_glFlush', '    _glFlush', ' U _glFlush', '0000000000000000 U _glFlush'):
            self.assertEqual(gate.symbols(row, True), {'_glFlush'})
        self.assertEqual(gate.symbols('archive.a(object.o):\n  U _eglSwapBuffers\n', True), {'_eglSwapBuffers'})
        self.assertEqual(gate.symbols('_objc_msgSend$fixture:', True), {'_objc_msgSend$fixture:'})

    def test_export_layout(self):
        self.assertEqual(gate.symbols('lib.a(obj.o):\n0000000000000010 T _GetRefAPI\n', False), {'_GetRefAPI'})

    def test_bad_layouts(self):
        for undefined, row in ((True, 'garbage incomplete nm output'), (False, '_GetRefAPI'),
                               (False, '00000000 U _GetRefAPI'), (False, 'incomplete:')):
            with self.assertRaises(ValueError):
                gate.symbols(row, undefined)

    def test_exact_sdl(self):
        self.assertEqual(gate.SDL_GL, sorted(gate.SDL_GL))
        gate.native_boundary(set(gate.SDL_GL), True)
        for delta in ({'_glFlush'}, {'_eglSwapBuffers'}):
            with self.assertRaises(ValueError):
                gate.native_boundary(set(gate.SDL_GL) | delta, True)
        with self.assertRaises(ValueError):
            gate.native_boundary(set(gate.SDL_GL[1:]), True)

    def test_gl4es_namespace(self):
        gate.native_boundary({'_gl4es_glFlush', '_gl4es_eglSwapBuffers', '_gl4es_GetProcAddress'})
        for symbol in ('_glFlush', '_eglSwapBuffers'):
            with self.assertRaises(ValueError):
                gate.native_boundary({symbol})

    def test_incomplete_inspection(self):
        for result in (subprocess.CompletedProcess([], 1, '', 'failure'),
                       subprocess.CompletedProcess([], 0, '', 'warning: incomplete'),
                       subprocess.CompletedProcess([], 0, '0000 U\n', '')):
            with patch.object(gate.subprocess, 'run', return_value=result):
                with self.assertRaises(ValueError):
                    gate.nm(Path('fixture'), True)


class Bundles(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='openjk-symbol-fixtures-')
        self.addCleanup(self.temp.cleanup)
        self.build = Path(self.temp.name)
        self.app = self.build / 'apps/Release-iphoneos/OpenJK-MP.app'
        self.app.mkdir(parents=True)
        (self.app / 'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable': 'OpenJK-MP'}))
        self.imports, self.defined = {}, {}
        self.binary('OpenJK-MP', gate.SDL_GL)
        for name, (dyn, _) in gate.MODULES['MP'].items():
            self.binary('Frameworks/' + name + '.dylib', [], dyn)

    def binary(self, relative, imports=(), defined=('_fixture',)):
        path = self.app / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(bytes.fromhex('cffaedfe') + b'fixture')
        self.imports[path] = set(imports)
        self.defined[path] = set(defined)
        return path

    def inspect(self, mode='dylib'):
        def nm(path, undefined):
            require = self.imports if undefined else self.defined
            if path not in require:
                raise OSError('fixture missing inspection: ' + str(path))
            return require[path]
        with patch.object(gate, 'nm', side_effect=nm), contextlib.redirect_stdout(io.StringIO()):
            gate.inspect_app(self.app, 'MP', mode, self.build / 'modules/Release-iphoneos', None)

    def test_exact_pass(self):
        self.inspect()

    def run_cli(self, *options):
        archive = self.build / 'gl4es/Release/libGL.a'
        archive.parent.mkdir(parents=True, exist_ok=True)
        archive.write_bytes(b'!<arch>\n')
        def nm(path, undefined):
            if path == archive:
                return set()
            return (self.imports if undefined else self.defined)[path]
        with patch.object(sys, 'argv', ['check-modules', str(self.build), 'mp', *map(str, options)]), \
                patch.object(gate, 'nm', side_effect=nm), contextlib.redirect_stdout(io.StringIO()):
            return gate.main()

    def test_cli_prepared_default_and_override(self):
        for name in ('libEGL', 'libGLESv2'):
            path = self.binary(f'Frameworks/{name}.framework/{name}', ['_eglGetDisplay'])
            (path.parent / 'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable': name}))
        default = self.build / 'angle-consumer'
        external = self.build / 'external-prepared'
        for prepared in (default, external):
            prepared.mkdir()
            (prepared / 'qualified-fixture').touch()
        def qualify(path, name, prepared):
            if prepared is None or not (prepared / 'qualified-fixture').is_file():
                raise ValueError('missing qualified fixture')
        with patch.object(gate, 'angle_qualified', side_effect=qualify) as qualified:
            self.assertEqual(self.run_cli(), 0)
            self.assertEqual([call.args[2] for call in qualified.call_args_list], [default, default])
            (default / 'qualified-fixture').unlink()
            qualified.reset_mock()
            self.assertEqual(self.run_cli('--angle-prepared', external), 0)
            self.assertEqual([call.args[2] for call in qualified.call_args_list], [external, external])
            (default / 'qualified-fixture').touch()
            self.assertEqual(self.run_cli('--angle-prepared', self.build / 'missing'), 1)

    def test_cli_eagl_needs_no_prepared_directory(self):
        self.assertFalse((self.build / 'angle-consumer').exists())
        self.assertEqual(self.run_cli(), 0)
        self.assertEqual(self.run_cli('--angle-prepared', self.build / 'missing'), 0)

    def test_app_additions(self):
        for symbol in ('_glFlush', '_eglGetDisplay'):
            self.imports[self.app / 'OpenJK-MP'].add(symbol)
            with self.assertRaisesRegex(ValueError, 'SDL native set mismatch'):
                self.inspect()
            self.imports[self.app / 'OpenJK-MP'].remove(symbol)

    def test_ui_and_renderer_native(self):
        for name in ('uiarm64', 'rd-vanilla_arm64'):
            for symbol in ('_glFlush', '_eglGetDisplay'):
                path = self.app / ('Frameworks/' + name + '.dylib')
                self.imports[path].add(symbol)
                with self.assertRaisesRegex(ValueError, 'native GL/EGL'):
                    self.inspect()
                self.imports[path].clear()

    def test_hidden_gl4es(self):
        path = self.app / 'Frameworks/rd-vanilla_arm64.dylib'
        self.imports[path].add('_gl4es_glFlush')
        self.inspect()
        self.defined[path].add('_gl4es_glFlush')
        with self.assertRaisesRegex(ValueError, 'exports'):
            self.inspect()

    def test_nested_unexpected_and_impostor(self):
        for name in ('Extras/not-a-dylib', 'Nested/OpenJK-MP', 'Nested/libEGL.framework/libEGL'):
            path = self.binary(name, ['_glFlush'])
            if path.parent.suffix == '.framework':
                (path.parent / 'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable': path.name}))
            with self.assertRaisesRegex(ValueError, 'native GL/EGL'):
                self.inspect()
            self.imports[path].clear()
        self.inspect()

    def test_missing_and_non_macho(self):
        path = self.app / 'Frameworks/uiarm64.dylib'
        path.unlink()
        with self.assertRaisesRegex(ValueError, 'missing/non-Mach-O'):
            self.inspect()
        path.write_bytes(b'not macho')
        with self.assertRaisesRegex(ValueError, 'not Mach-O'):
            self.inspect()

    def test_incomplete_unexpected_framework(self):
        path = self.app / 'Extras/Other.framework'
        path.mkdir(parents=True)
        (path / 'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable': 'Missing'}))
        with self.assertRaisesRegex(ValueError, 'missing/non-Mach-O'):
            self.inspect()

    def test_uninspectable_and_symlink(self):
        path = self.binary('Extra')
        del self.imports[path]
        with self.assertRaisesRegex(OSError, 'missing inspection'):
            self.inspect()
        path.unlink()
        path.symlink_to(self.app / 'OpenJK-MP')
        with self.assertRaisesRegex(ValueError, 'symlink'):
            self.inspect()

    def test_walk_error(self):
        def failed_walk(*args, **kwargs):
            kwargs['onerror'](PermissionError('unreadable directory'))
            return []
        with patch.object(gate.os, 'walk', side_effect=failed_walk):
            with self.assertRaisesRegex(PermissionError, 'unreadable'):
                self.inspect()

    def test_two_angle_exemptions(self):
        qualified = []
        for name in ('libEGL', 'libGLESv2'):
            path = self.binary(f'Frameworks/{name}.framework/{name}', ['_glFlush', '_eglGetDisplay'])
            (path.parent / 'Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable': name}))
        with patch.object(gate, 'angle_qualified', side_effect=lambda p, n, _: qualified.append((p, n))):
            self.inspect()
        self.assertEqual([n for _, n in qualified], ['libEGL', 'libGLESv2'])
        with self.assertRaisesRegex(ValueError, 'unqualified ANGLE'):
            self.inspect()
        (self.app / 'Frameworks/libGLESv2.framework/libGLESv2').unlink()
        with self.assertRaisesRegex(ValueError, 'missing/non-Mach-O|incomplete ANGLE'):
            self.inspect()

    def test_static_archives(self):
        for name, (_, stat) in gate.MODULES['MP'].items():
            path = self.build / 'modules/Release-iphoneos' / (name + '.a')
            self.imports[path], self.defined[path] = set(), set(stat)
        self.inspect('static')
        renderer = self.build / 'modules/Release-iphoneos/rd-vanilla_arm64.a'
        for symbol in ('_glFlush', '_eglGetDisplay'):
            self.imports[renderer] = {symbol}
            with self.assertRaisesRegex(ValueError, 'native GL/EGL'):
                self.inspect('static')

    @unittest.skipUnless(sys.platform == 'darwin' and shutil.which('clang'), 'requires host Mach-O tools')
    def test_real_nm_and_throwaway_nonrenderer(self):
        def compile_binary(path, source, library=True):
            src = self.build / (path.name + '.c')
            src.write_text(source)
            subprocess.run(['clang', str(src), '-o', str(path), '-Wl,-undefined,dynamic_lookup']
                           + (['-dynamiclib'] if library else []), check=True, capture_output=True)
        calls = [symbol[1:] for symbol in gate.SDL_GL]
        app_source = ('\n'.join('extern void ' + s + '(void);' for s in calls)
                      + '\nint main(void) {' + ''.join(s + '();' for s in calls) + 'return 0;}')
        compile_binary(self.app / 'OpenJK-MP', app_source, False)
        for name, (dyn, _) in gate.MODULES['MP'].items():
            compile_binary(self.app / ('Frameworks/' + name + '.dylib'),
                           '\n'.join('void ' + s[1:] + '(void) {}' for s in dyn))
        archive = self.build / 'gl4es/Release/libGL.a'
        archive.parent.mkdir(parents=True)
        source = self.build / 'gl4es.c'
        source.write_text('extern void glLocal(void); void gl4es_test(void) { glLocal(); }\n')
        obj = self.build / 'gl4es.o'
        subprocess.run(['clang', '-c', str(source), '-o', str(obj)], check=True)
        definition = self.build / 'definition.c'
        definition.write_text('void glLocal(void) {}\n')
        definition_obj = self.build / 'definition.o'
        subprocess.run(['clang', '-c', str(definition), '-o', str(definition_obj)], check=True)
        subprocess.run(['ar', 'rcs', str(archive), str(obj), str(definition_obj)], check=True)
        self.assertIn('_glLocal', gate.nm(archive, True))
        self.assertIn('_glLocal', gate.nm(archive, False))
        command = [str(ROOT / 'scripts/ios/check-modules.sh'), str(self.build), 'mp']
        def run(expected, message):
            result = subprocess.run(command, text=True, capture_output=True)
            self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
            self.assertIn(message, result.stdout)
        run(0, 'gl4es archive native-symbol subtraction')
        for symbol in ('glFlush', 'eglGetDisplay'):
            compile_binary(self.app / 'OpenJK-MP', 'extern void ' + symbol + '(void);\n'
                           + app_source.replace('return 0;', symbol + '(); return 0;'), False)
            run(1, 'SDL native set mismatch')
        compile_binary(self.app / 'OpenJK-MP', app_source, False)
        for symbol in ('glFlush', 'eglGetDisplay'):
            compile_binary(self.app / 'Frameworks/uiarm64.dylib',
                           'extern void ' + symbol + '(void); void GetModuleAPI(void) { '
                           + symbol + '(); }\nvoid dllEntry(void) {} void vmMain(void) {}\n')
            run(1, "native GL/EGL imports: ['_" + symbol + "']")
        compile_binary(self.app / 'Frameworks/uiarm64.dylib',
                       'void GetModuleAPI(void) {} void dllEntry(void) {} void vmMain(void) {}')
        extra = self.app / 'Unexpected'
        compile_binary(extra, 'extern void glFlush(void); void fixture(void) { glFlush(); }')
        run(1, "native GL/EGL imports: ['_glFlush']")
        extra.unlink()
        run(0, 'gl4es archive native-symbol subtraction')
        source.write_text('extern void eglGetDisplay(void); void gl4es_test(void) { eglGetDisplay(); }\n')
        subprocess.run(['clang', '-c', str(source), '-o', str(obj)], check=True)
        subprocess.run(['ar', 'rcs', str(archive), str(obj)], check=True)
        run(1, "native GL/EGL imports: ['_eglGetDisplay']")


if __name__ == '__main__':
    unittest.main(verbosity=2)
