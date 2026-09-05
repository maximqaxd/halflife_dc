"""Find function starts in a .text range by subtracting literal-pool slots.

Every 4-byte pool slot is the resolved target of a `mov.l @(disp,pc)` /
`mova @(disp,pc)` / `mov.w @(disp,pc)`. Whatever is left in a gap between
known functions is code, and each such run begins a function.

Usage: python utils/re/findfns.py <start_hex> <end_hex>
"""
import re, sys, bisect

DIS = 'RE/HALFLIFE_DC.EXE.disasm.txt'
SYM = 'utils/re/symbols.tsv'
LINE = re.compile(r'^  ([0-9A-F]{8}): ([0-9A-F]{4})(?: [0-9A-F]{4})?\s+(\S+)\s*(.*)$')
POOL = re.compile(r'@\(([0-9A-F]{8}),pc\)')

hw, ins = {}, {}
for L in open(DIS, errors='ignore'):
    m = LINE.match(L)
    if not m:
        continue
    a = int(m.group(1), 16)
    hw[a] = int(m.group(2), 16)
    ins[a] = (m.group(3), m.group(4))

lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)

pool = set()
for a in range(lo - 0x400, hi + 0x800, 2):
    r = ins.get(a)
    if not r:
        continue
    mn, ops = r
    m = POOL.search(ops)
    if not m:
        continue
    d = int(m.group(1), 16)
    if mn.startswith('mov.l') or mn == 'mova':
        s = ((a + 4) & ~3) + d
        pool.update((s, s + 2))
    elif mn.startswith('mov.w'):
        pool.add(a + 4 + d)

rows = []
for L in open(SYM):
    p = L.rstrip('\n').split('\t')
    if len(p) < 3:
        continue
    try:
        rows.append((int(p[1], 16), int(p[2]), p[0]))
    except ValueError:
        pass
rows.sort()
known = {r[0] for r in rows}

# addresses in [lo,hi) that are neither inside a known function nor a pool slot
covered = set()
for st, sz, nm in rows:
    for a in range(max(st, lo), min(st + sz, hi), 2):
        covered.add(a)

runs, cur = [], None
for a in range(lo, hi, 2):
    free = a not in covered and a not in pool and a in ins
    if free:
        if cur is None:
            cur = a
    else:
        if cur is not None:
            runs.append((cur, a))
            cur = None
if cur is not None:
    runs.append((cur, hi))

for s, e in runs:
    if e - s < 4:
        continue
    mn, ops = ins.get(s, ('?', ''))
    print('%05x-%05x (%3d)  %s %s' % (s, e, e - s, mn, ops))
