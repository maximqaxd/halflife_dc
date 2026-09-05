"""Map a hardware exception dump's PC/RA onto a Windows CE ROM module.

An exception dump prints raw addresses. Anything below 0x02000000 that is not in
the exe's own range belongs to a ROM DLL, XIP-mapped at a fixed address by the
kernel. This walks the ROM TOC in the OS image and reports which module an
address lands in, its offset from the module base, and (with capstone) the SH-4
instructions around it.

    python utils/re/cemodmap.py                  # list every module
    python utils/re/cemodmap.py 01c33edc         # locate one address
    python utils/re/cemodmap.py 01c33edc -d 0x40 # ...and disassemble there

Addresses inside HALFLIFE_DC.EXE itself are not ROM modules; look those up in
obj/WCESH4Rel/dc/halflife_dc.map ("Publics by Value") instead.
"""

import argparse, os, struct, sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
IMAGE = os.path.join(REPO, "deploy", "0winceos.bin")

# The image maps physfirst 0x8c010000 at file offset 0x800.
VA_BIAS = 0x8C00F800
ROMHDR = 0x1C69D8
TOC = 0x1C6A2C
# o32 realaddr values in this image carry a +0x02000000 bias over the address
# the module is actually mapped at.
O32_BIAS = 0x02000000


def load(path):
    with open(path, "rb") as f:
        return f.read()


def cstr(buf, off):
    return buf[off:buf.index(b"\0", off)].decode("latin1")


def modules(buf):
    """(name, vbase, entry, [(va, vsize, psize, fileoff, flags), ...]) per module."""
    nummods = struct.unpack_from("<I", buf, ROMHDR + 0x10)[0]
    out = []
    for i in range(nummods):
        b = TOC + i * 32
        _attr, _ftl, _fth, _size, namep, e32, o32, _load = struct.unpack_from("<8I", buf, b)
        e = e32 - VA_BIAS
        objcnt = struct.unpack_from("<H", buf, e)[0]
        entry, vbase = struct.unpack_from("<II", buf, e + 4)
        secs = []
        for k in range(objcnt):
            o = (o32 - VA_BIAS) + k * 24
            vsize, _rva, psize, dataptr, realaddr, flags = struct.unpack_from("<6I", buf, o)
            secs.append((realaddr - O32_BIAS, vsize, psize, dataptr - VA_BIAS, flags))
        out.append((cstr(buf, namep - VA_BIAS), vbase, entry, secs))
    return out


def find(mods, addr):
    for name, vbase, entry, secs in mods:
        for va, vsize, psize, fileoff, flags in secs:
            if va <= addr < va + vsize:
                return name, vbase, entry, (va, vsize, psize, fileoff, flags)
    return None


def disasm(buf, sec, addr, count, back):
    try:
        import capstone
    except ImportError:
        print("  (capstone not installed - no disassembly)")
        return
    va, vsize, psize, fileoff, _flags = sec
    md = capstone.Cs(capstone.CS_ARCH_SH,
                     capstone.CS_MODE_LITTLE_ENDIAN | capstone.CS_MODE_SH4A | capstone.CS_MODE_SHFPU)
    start = max(va, addr - back)
    off = fileoff + (start - va)
    for i in md.disasm(buf[off:off + count + back], start):
        print("  %08x  %-11s %-8s %s%s" % (
            i.address, " ".join("%02x" % b for b in i.bytes), i.mnemonic, i.op_str,
            "   <== " if i.address == addr else ""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("address", nargs="?", help="hex address from the exception dump")
    ap.add_argument("-i", "--image", default=IMAGE)
    ap.add_argument("-d", "--disasm", default="0", help="bytes to disassemble at the address")
    ap.add_argument("-b", "--back", default="0x20", help="bytes of lead-in to disassemble")
    args = ap.parse_args()

    buf = load(args.image)
    mods = modules(buf)

    if not args.address:
        for name, vbase, entry, secs in mods:
            lo = min(s[0] for s in secs)
            hi = max(s[0] + s[1] for s in secs)
            print("%-16s base=%08x entry=%08x  %08x-%08x" % (name, vbase, entry, lo, hi))
        return 0

    addr = int(args.address, 16)
    hit = find(mods, addr)
    if not hit:
        print("%08x is in no ROM module (the exe itself? check halflife_dc.map)" % addr)
        return 1

    name, vbase, entry, sec = hit
    va, vsize, psize, fileoff, flags = sec
    print("%08x  %s+0x%x  (section %08x-%08x, file %06x, flags %08x)"
          % (addr, name, addr - vbase, va, va + vsize, fileoff, flags))

    n = int(args.disasm, 0)
    if n:
        disasm(buf, sec, addr, n, int(args.back, 0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
