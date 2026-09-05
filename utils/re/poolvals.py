"""Resolve every pc-relative operand in an address range to its literal value.

Usage: python utils/re/poolvals.py <start_hex> <end_hex>

Prints one line per mova/mov.w/mov.l @(disp,pc) instruction: the instruction
address, the pool slot it reads and the value there, decoded as int, float and
(when it looks like one) a string.
"""
import re, sys, struct

DIS = 'RE/HALFLIFE_DC.EXE.disasm.txt'
EXE = 'RE/HALFLIFE_DC.EXE'
BASE = 0x10000

# .text starts at 0x11000; the PE maps sections at their virtual addresses, so
# walk the section headers to translate.
img = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', img, 0x3c)[0]
nsec = struct.unpack_from('<H', img, pe + 6)[0]
opt = struct.unpack_from('<H', img, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    name = img[o:o+8].rstrip(b'\0').decode('latin-1')
    vsz, va, rsz, ra = struct.unpack_from('<IIII', img, o + 8)
    secs.append((va + BASE, max(vsz, rsz), ra))

def rd(addr, n):
    for va, sz, ra in secs:
        if va <= addr < va + sz:
            off = ra + (addr - va)
            return img[off:off+n]
    return b''

def u32(a):
    b = rd(a, 4)
    return struct.unpack('<I', b)[0] if len(b) == 4 else None

def u16(a):
    b = rd(a, 2)
    return struct.unpack('<H', b)[0] if len(b) == 2 else None

def s(a):
    b = rd(a, 64)
    z = b.find(b'\0')
    t = b[:z if z >= 0 else 64]
    return t.decode('latin-1') if t and all(32 <= c < 127 for c in t) else None

LINE = re.compile(r'^  ([0-9A-F]{8}): [0-9A-F]{4} (\S+)\s*(.*?)\s*$')
PCREL = re.compile(r'@\(([0-9A-F]{8}),pc\)')

lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
for line in open(DIS, encoding='latin-1'):
    m = LINE.match(line)
    if not m:
        continue
    a = int(m.group(1), 16)
    if not (lo <= a < hi):
        continue
    mn, ops = m.group(2).lower(), m.group(3)
    d = PCREL.search(ops)
    if mn == 'mova':
        d = re.search(r'@\(([0-9A-F]{8}),pc\)', ops)
    if not d:
        continue
    disp = int(d.group(1), 16)
    if mn == 'mov.w':
        tgt = a + 4 + disp
        v = u16(tgt)
        print('%08x %-6s -> %08x  u16=%-6d' % (a, mn, tgt, v))
    else:
        tgt = (a & ~3) + 4 + disp
        v = u32(tgt)
        f = struct.unpack('<f', struct.pack('<I', v))[0] if v is not None else 0.0
        txt = s(v) if v and v > 0x100000 else None
        print('%08x %-6s -> %08x  0x%08x  f=%-14g%s'
              % (a, mn, tgt, v, f, ('  "%s"' % txt) if txt else ''))
