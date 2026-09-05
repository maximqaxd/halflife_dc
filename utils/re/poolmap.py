"""Resolve SH-4 literal-pool loads to their constants, attributed to the
function containing the referencing instruction.

Usage: python utils/re/poolmap.py <start_hex> <end_hex> [--strings-only]
"""
import re, sys, struct

EXE = 'RE/HALFLIFE_DC.EXE'
DIS = 'RE/HALFLIFE_DC.EXE.disasm.txt'
SYM = 'utils/re/symbols.tsv'


def pe_sections(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    secs, off = [], pe + 24 + optsz
    for _ in range(nsec):
        name = d[off:off + 8].rstrip(b'\0').decode('ascii', 'replace')
        vsz, va, rsz, rptr = struct.unpack_from('<IIII', d, off + 8)
        secs.append((name, base + va, max(vsz, rsz), rptr, rsz))
        off += 40
    return d, base, secs


DATA, BASE, SECS = pe_sections(EXE)


def read(addr, n):
    for name, va, vsz, rptr, rsz in SECS:
        if va <= addr < va + vsz:
            if addr - va >= rsz:
                return None
            fo = rptr + (addr - va)
            return DATA[fo:fo + n]
    return None


def secname(addr):
    for name, va, vsz, rptr, rsz in SECS:
        if va <= addr < va + vsz:
            return name
    return None


def cstring(addr, maxlen=140):
    b = read(addr, maxlen)
    if not b:
        return None
    i = b.find(b'\0')
    if i < 1:
        return None
    s = b[:i]
    if all(32 <= c < 127 or c in (9, 10, 13) for c in s):
        return s.decode('ascii')
    return None


LINE = re.compile(r'^  ([0-9A-F]{8}): ([0-9A-F]{4})(?: ([0-9A-F]{4}))?\s+(\S+)\s*(.*)$')
POOL = re.compile(r'@\(([0-9A-F]{8}),pc\)')


def load():
    hw, insns = {}, []
    for L in open(DIS, errors='ignore'):
        m = LINE.match(L)
        if not m:
            continue
        a = int(m.group(1), 16)
        hw[a] = int(m.group(2), 16)
        insns.append((a, m.group(4), m.group(5)))
    return hw, insns


def load_syms():
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
    return rows


def main():
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    strings_only = '--strings-only' in sys.argv
    hw, insns = load()
    syms = load_syms()

    starts = [s[0] for s in syms]
    import bisect

    def owner(a):
        i = bisect.bisect_right(starts, a) - 1
        if i < 0:
            return '?', 0
        st, sz, nm = syms[i]
        return (nm if a < st + sz else nm + ' [pool/gap]'), st

    cur = None
    for a, mn, ops in insns:
        if not (lo <= a < hi):
            continue
        m = POOL.search(ops)
        if not m:
            continue
        disp = int(m.group(1), 16)
        if mn.startswith('mov.l'):
            slot = ((a + 4) & ~3) + disp
            if slot not in hw or slot + 2 not in hw:
                continue
            v = hw[slot] | (hw[slot + 2] << 16)
        elif mn.startswith('mov.w'):
            slot = a + 4 + disp
            v = hw.get(slot)
            if v is None:
                continue
            v = v - 0x10000 if v & 0x8000 else v
        else:
            continue
        sec = secname(v)
        s = cstring(v) if sec else None
        if strings_only and s is None:
            continue
        nm, st = owner(a)
        if nm != cur:
            print('\n== %s (0x%05x)' % (nm, st))
            cur = nm
        if s is not None:
            print('   %05x  %-8s -> %06x  "%s"' % (a, mn, v, s.replace('\n', '\n')))
        elif sec:
            print('   %05x  %-8s -> %06x  [%s]' % (a, mn, v, sec))
        else:
            print('   %05x  %-8s -> %08x' % (a, mn, v))


main()
