#!/usr/bin/env python3
"""Recover the game's save/restore export registry from the reference binary.

GameDLL_RegisterModules calls one generated registrar per game source file (plus
a few registrations of its own); each registrar hands Sys_RegisterExport a
two-character code and the function it names.  Save files store those codes, so
the registry has to be rebuilt exactly for a save to load.

Walks the SH-4 code tracking pc-relative literal loads and prints the registry
in construction order: seq, registrar, code, function.
"""
import struct
import sys

PE = "RE/HALFLIFE_DC.EXE"
data = open(PE, "rb").read()

_pe = struct.unpack_from("<I", data, 0x3c)[0]
_nsec = struct.unpack_from("<H", data, _pe + 6)[0]
_opt = struct.unpack_from("<H", data, _pe + 20)[0]
IMAGE_BASE = struct.unpack_from("<I", data, _pe + 24 + 28)[0]
SECTIONS = []
for _i in range(_nsec):
    _o = _pe + 24 + _opt + _i * 40
    _name = data[_o:_o+8].rstrip(b"\0").decode()
    _vsize, _va, _rsize, _raw = struct.unpack_from("<IIII", data, _o + 8)
    SECTIONS.append((_name, IMAGE_BASE + _va, _vsize, _raw, _rsize))

def read(va, n):
    for name, sva, vsize, raw, rsize in SECTIONS:
        if sva <= va < sva + max(vsize, rsize):
            o = raw + (va - sva)
            return data[o:o+n]
    return b""

def dword(va):
    b = read(va, 4)
    return struct.unpack("<I", b)[0] if len(b) == 4 else None

def word(va):
    b = read(va, 2)
    return struct.unpack("<H", b)[0] if len(b) == 2 else None

def cstr(va, maxlen=64):
    b = read(va, maxlen)
    return b.split(b"\0")[0].decode("latin-1") if b else ""

REGISTER_EXPORT = 0x7f330
THUNKS = {REGISTER_EXPORT, 0x123290, 0x1232a4, 0x1232b8, 0x1232cc, 0x1232e0}
REGISTER_MODULES = 0x7f450

def scan(start, limit=0x800):
    """Walk one function from `start` to its rts, yielding ('call', pc, target,
    name, fn) for every jsr whose operands were resolved from literal pools.
    Literal pools sit inline between instructions, so every slot a pc-relative
    load names is remembered and stepped over rather than decoded."""
    reg = {}
    pool = set()
    va = start
    end = start + limit
    while va < end:
        if va in pool:
            va += 4
            continue
        w = word(va)
        if w is None:
            return
        if (w & 0xf000) == 0xd000:                       # mov.l @(disp,PC),Rn
            slot = (va & ~3) + 4 + (w & 0xff) * 4
            pool.add(slot)
            reg[(w >> 8) & 0xf] = dword(slot)
        elif (w & 0xf0ff) == 0x400b:                     # jsr @Rn
            target = reg.get((w >> 8) & 0xf)
            w2 = word(va + 2)                            # delay slot runs first
            if w2 is not None and (w2 & 0xf000) == 0xd000:
                slot = ((va + 2) & ~3) + 4 + (w2 & 0xff) * 4
                pool.add(slot)
                reg[(w2 >> 8) & 0xf] = dword(slot)
            yield ("call", va, target, reg.get(4), reg.get(5))
            va += 4
            continue
        elif w == 0x000b:                                # rts
            return
        va += 2

def registrations(start):
    for kind, pc, target, r4, r5 in scan(start):
        if target in THUNKS and r4 and r5:
            code = cstr(r4, 4)
            if len(code) == 2 and code.isalpha() and code.isupper():
                yield code, r5

def main():
    rows = []
    seq = 0
    for kind, pc, target, r4, r5 in scan(REGISTER_MODULES, 0x400):
        if target is None:
            continue
        if target in THUNKS:                             # registered inline
            code = cstr(r4, 4) if r4 else ""
            if len(code) == 2:
                rows.append((seq, REGISTER_MODULES, code, r5))
                seq += 1
            continue
        for code, fn in registrations(target):
            rows.append((seq, target, code, fn))
            seq += 1
    print("seq\tregistrar\tcode\tfunction")
    for s, reg, code, fn in rows:
        print("%d\t%x\t%s\t%x" % (s, reg, code, fn))
    print("# %d registrations from %d registrars" %
          (len(rows), len(set(r for _, r, _, _ in rows))), file=sys.stderr)

if __name__ == "__main__":
    main()
