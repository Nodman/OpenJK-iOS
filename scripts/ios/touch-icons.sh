#!/bin/sh
set -eu
task_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
exec python3 - "$task_root" "$@" <<'PY'
import argparse
import binascii
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zlib

root = Path(sys.argv[1])
parser = argparse.ArgumentParser(description='Regenerate the embedded touch atlas with local headless Chrome.')
parser.add_argument('--check', action='store_true', help='Regenerate and compare without changing outputs.')
args = parser.parse_args(sys.argv[2:])
source = root / 'shared/ios/touch'
names = ('menu', 'chat', 'pad', 'prev', 'saber', 'next', 'force-cycle', 'force-use',
         'console', 'alt-attack', 'attack', 'jump', 'crouch', 'style', 'use', 'plate')
namespace = 'http://www.w3.org/2000/svg'
ET.register_namespace('', namespace)
atlas = ET.Element(f'{{{namespace}}}svg', width='1024', height='1024', viewBox='0 0 1024 1024')
for cell, name in enumerate(names):
    glyph = ET.parse(source / f'{cell:02d}-{name}.svg').getroot()
    if glyph.get('viewBox') != '0 0 256 256':
        raise SystemExit(f'Bad viewBox in cell {cell}')
    for element in glyph.iter():
        if element.tag.rsplit('}', 1)[-1] in ('image', 'script', 'text', 'foreignObject', 'filter'):
            raise SystemExit(f'Unsupported element in cell {cell}')
        for key, value in element.attrib.items():
            if 'href' in key or 'url(' in value:
                raise SystemExit(f'External or referenced content in cell {cell}')
    glyph.set('x', str((cell % 4) * 256))
    glyph.set('y', str((cell // 4) * 256))
    atlas.append(glyph)


def validate(blob):
    if blob[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Missing PNG signature')
    offset, compressed, ihdr, ended = 8, bytearray(), None, False
    while offset < len(blob):
        if len(blob) - offset < 12:
            raise ValueError('Truncated PNG chunk')
        length, kind = struct.unpack_from('>I4s', blob, offset)
        end = offset + 12 + length
        if end > len(blob):
            raise ValueError('Truncated PNG payload')
        data = blob[offset + 8:end - 4]
        crc, = struct.unpack_from('>I', blob, end - 4)
        if binascii.crc32(kind + data) & 0xffffffff != crc:
            raise ValueError('Bad PNG CRC')
        if kind == b'IHDR':
            if ihdr is not None or offset != 8:
                raise ValueError('Bad IHDR order')
            ihdr = struct.unpack('>IIBBBBB', data)
        elif kind == b'IDAT':
            compressed.extend(data)
        elif kind == b'IEND':
            if length or end != len(blob):
                raise ValueError('Bad PNG end')
            ended = True
        offset = end
    if ihdr != (1024, 1024, 8, 6, 0, 0, 0) or not ended:
        raise ValueError(f'Expected 1024x1024 RGBA8, non-interlaced; got {ihdr}')
    raw = zlib.decompress(compressed)
    stride = 1024 * 4
    if len(raw) != (stride + 1) * 1024:
        raise ValueError('Bad decoded PNG length')
    pixels = bytearray(stride * 1024)
    for y in range(1024):
        row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        mode = raw[y * (stride + 1)]
        if mode > 4:
            raise ValueError('Bad PNG filter')
        for x in range(stride):
            left = row[x - 4] if x >= 4 else 0
            above = pixels[(y - 1) * stride + x] if y else 0
            corner = pixels[(y - 1) * stride + x - 4] if y and x >= 4 else 0
            if mode == 1:
                predictor = left
            elif mode == 2:
                predictor = above
            elif mode == 3:
                predictor = (left + above) // 2
            elif mode == 4:
                p = left + above - corner
                distances = (abs(p - left), abs(p - above), abs(p - corner))
                predictor = (left, above, corner)[distances.index(min(distances))]
            else:
                predictor = 0
            row[x] = (row[x] + predictor) & 255
        pixels[y * stride:(y + 1) * stride] = row
    print('PNG: 1024x1024, colour type 6, depth 8, straight RGBA, CRCs valid')
    for cell, name in enumerate(names):
        occupied, partial, bounds = 0, 0, [256, 256, -1, -1]
        for y in range(256):
            for x in range(256):
                pos = (((cell // 4) * 256 + y) * 1024 + (cell % 4) * 256 + x) * 4
                red, green, blue, alpha = pixels[pos:pos + 4]
                if alpha:
                    if (red, green, blue) != (255, 255, 255):
                        raise ValueError(f'Non-white pixel in cell {cell}')
                    occupied += 1
                    partial += alpha < 255
                    bounds = [min(bounds[0], x), min(bounds[1], y), max(bounds[2], x), max(bounds[3], y)]
        if not occupied or not partial or min(bounds[:2]) < 8 or max(bounds[2:]) > 247:
            raise ValueError(f'Cell {cell} fails content/inset/antialias check: {bounds}')
        print(f'Cell {cell:02d} {name}: pixels={occupied}, partial-alpha={partial}, bounds={bounds}')
    def alpha(x, y):
        return pixels[(y * 1024 + x) * 4 + 3]
    if alpha(896, 896) != 255 or alpha(776, 776) != 0 or not 0 < alpha(852, 785) < 255:
        raise ValueError('Round plate circle sample failed')
    if alpha(112, 86) != 255 or alpha(10, 10) != 0:
        raise ValueError('Menu stroke sample failed')
    print('Circle/stroke samples: opaque centres, clear corners and partial-alpha edge pass')


with tempfile.TemporaryDirectory(prefix='openjk-touch-', dir='/private/tmp') as scratch:
    work = Path(scratch)
    svg = work / 'atlas.svg'
    svg.write_bytes(ET.tostring(atlas, encoding='utf-8', xml_declaration=True))
    png = work / 'touch-v2.png'
    command = ['/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
               '--headless=new', '--disable-gpu', '--hide-scrollbars',
               '--default-background-color=00000000', '--window-size=1024,1024',
               '--force-device-scale-factor=1', '--no-first-run', '--no-default-browser-check',
               '--disable-background-networking', '--disable-extensions',
               f'--screenshot={png}', f'--user-data-dir={work / "profile"}', svg.as_uri()]
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    if result.returncode or not png.exists():
        raise SystemExit(f'Chrome failed ({result.returncode}): ' + result.stderr.decode(errors='replace'))
    blob = png.read_bytes()
    validate(blob)
    lines = ['static const unsigned char iosTouchPNG[] = {']
    lines.extend('\t' + ', '.join(f'0x{value:02x}' for value in blob[i:i + 16]) + ',' for i in range(0, len(blob), 16))
    lines.extend(['};', f'static const unsigned int iosTouchPNGSize = {len(blob)}u;', ''])
    outputs = {'touch-v2.png': blob, 'touch-v2.inc': '\n'.join(lines).encode()}
    for name, content in outputs.items():
        destination = source / name
        if args.check:
            if not destination.exists() or destination.read_bytes() != content:
                raise SystemExit(f'Atlas check failed: {destination.relative_to(root)} differs')
        elif not destination.exists() or destination.read_bytes() != content:
            destination.write_bytes(content)
    print(f'Atlas {"check" if args.check else "generation"}: PASS ({len(blob)} PNG bytes)')
PY
