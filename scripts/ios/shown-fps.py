#!/usr/bin/env python3
# Real on-screen fps of an iOS app: records a short Metal System Trace over USB and counts the app's displayed frames.
# usage: shown-fps.py [--device <xctrace id or name part>] [--seconds 3] [--app OpenJK-MP] [--keep]
# The in-game cg_drawFPS counts engine frames; this counts frames the display actually showed.
import argparse, collections, os, re, shutil, statistics, subprocess, sys, tempfile, time
import xml.etree.ElementTree as ET

def xctrace(*args, **kw):
    return subprocess.run(['xcrun', 'xctrace', *args], capture_output=True, text=True, **kw)

def pick_device(want):
    out = xctrace('list', 'devices').stdout
    devs = [m.groups() for m in re.finditer(r'^(.+?) \((\d+(?:\.\d+)*)\) \(([0-9A-F-]+)\)$', out, re.M)]  # (name, os, id)
    devs = [d for d in devs if not d[0].endswith('Simulator')]
    if want:
        devs = [d for d in devs if any(want in x for x in d)]
    if len(devs) != 1:
        sys.exit('pick one device with --device (name, OS version or id):\n' + '\n'.join(f'  {n} ({v})  {i}' for n, v, i in devs) if devs else 'no iOS device on USB')
    return f'{devs[0][0]} ({devs[0][1]})', devs[0][2]

def export_rows(trace, schema, tries=4):
    xpath = f'/trace-toc/run[@number="1"]/data/table[@schema="{schema}"]'
    for _ in range(tries):  # the export sometimes comes back empty
        r = xctrace('export', '--input', trace, '--xpath', xpath)
        if len(r.stdout) > 1000:
            return parse(r.stdout)
        time.sleep(2)
    sys.exit(f'export of {schema} is empty')

def parse(xml):
    ids, cols, rows = {}, [], []
    root = ET.fromstring(xml)
    for e in root.iter():
        if e.get('id'):
            ids[e.get('id')] = e
    for el in root.iter():
        if el.tag == 'schema':
            cols = [c.findtext('mnemonic') for c in el.findall('col')]
        elif el.tag == 'row':
            row = {}
            for name, c in zip(cols, list(el)):
                c = ids.get(c.get('ref'), c) if c.get('ref') else c
                row[name] = (c.text, c.get('fmt', ''))
            rows.append(row)
    return rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--device')
    ap.add_argument('--seconds', type=int, default=3)
    ap.add_argument('--app', default='OpenJK-MP')
    ap.add_argument('--keep', action='store_true', help='keep the .trace next to the cwd')
    a = ap.parse_args()
    name, dev = pick_device(a.device)
    tmp = tempfile.mkdtemp(prefix='shown-fps-')
    trace = os.path.join(tmp, 'disp.trace')
    r = xctrace('record', '--template', 'Metal System Trace', '--device', dev, '--all-processes',
                '--time-limit', f'{a.seconds}s', '--output', trace)
    if r.returncode:
        sys.exit(f'xctrace record failed on {name}: {r.stderr.strip().splitlines()[-1:]}\n(plug the device in by USB and unlock it)')
    rows = export_rows(trace, 'displayed-surfaces-interval')
    shown = sorted(int(x['start'][0]) for x in rows if a.app in (x['event-label'][1] or ''))
    if a.keep:
        shutil.move(trace, os.path.abspath('disp.trace'))
    shutil.rmtree(tmp, ignore_errors=True)
    if len(shown) < 2:
        sys.exit(f'{name}: no {a.app} frames on the display in {len(rows)} rows (app in background or not in a map?)')
    span = (shown[-1] - shown[0]) / 1e9
    gaps = sorted((shown[i + 1] - shown[i]) / 1e6 for i in range(len(shown) - 1))
    hist = collections.Counter(round(g / 8.333) for g in gaps)
    print(f'{name}: {a.app} shown {len(shown)} frames in {span:.2f} s = {len(gaps) / span:.1f} fps')  # gaps/span: no endpoint bias
    print(f'frame gap p50 {statistics.median(gaps):.2f} ms  p95 {gaps[int(len(gaps) * 0.95)]:.2f} ms  max {gaps[-1]:.2f} ms')
    print('gaps by 120 Hz refresh count: ' + '  '.join(f'{k}x={v}' for k, v in sorted(hist.items())))

if __name__ == '__main__':
    main()
