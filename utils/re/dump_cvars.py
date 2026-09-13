#!/usr/bin/env python3
"""
Dump every cvar_t the binary actually contains, and diff it against the fork.

A cvar_t is five words -- {char *name; char *string; int flags; float value;
cvar_t *next} -- so the image can be scanned for the shape directly rather than
walking Cvar_RegisterVariable call sites. That finds cvars the port defines but
never registers as well as the ones it does, which call-site analysis misses.

A record qualifies when name and string both point into the image at
NUL-terminated printable ASCII, the name looks like an identifier, flags is a
small bitmask and next is null or another image pointer. Requiring *both*
strings to resolve removes most false positives; the final name checks exclude
controller bindings and localized menu records with the same five-word shape.

Usage:
  python utils/re/dump_cvars.py                 # summary + differences
  python utils/re/dump_cvars.py --list          # every cvar found in the image
  python utils/re/dump_cvars.py --tsv out.tsv   # write name/default/flags/addr
"""
import os, re, sys, struct, glob, argparse

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
EXE = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE")

IDENT = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
# cvar_t x = { "name", "value" [, flags] };  -- the fork's definition form
DEF = re.compile(r'\bcvar_t\s+(?:\w+\s+)?([A-Za-z_]\w*)\s*=\s*\{\s*"([^"]*)"\s*,\s*"([^"]*)"'
                 r'(?:\s*,\s*([^,}]+))?(?:\s*,\s*([^,}]+))?')
# cvar_t defport = { "port", PORT_SERVER };  -- macro default: the name still
# counts as defined, but the value cannot be compared against the image.
DEF_MACRO = re.compile(r'\bcvar_t\s+(?:\w+\s+)?([A-Za-z_]\w*)\s*=\s*\{\s*"([^"]*)"\s*,\s*'
                       r'([A-Z_][A-Z0-9_]*)\s*[,}]')

REG = re.compile(r'(?:Cvar_RegisterVariable|CVAR_REGISTER)\s*\(\s*&\s*([A-Za-z_]\w*)\s*\)')


def load_image():
    """Map the PE into a flat VA-indexed image."""
    raw = open(EXE, "rb").read()
    pe = struct.unpack_from("<I", raw, 0x3c)[0]
    nsec = struct.unpack_from("<H", raw, pe + 6)[0]
    optsz = struct.unpack_from("<H", raw, pe + 20)[0]
    base = struct.unpack_from("<I", raw, pe + 24 + 28)[0]
    sects = []
    off = pe + 24 + optsz
    for i in range(nsec):
        s = raw[off + i * 40: off + (i + 1) * 40]
        name = s[:8].rstrip(b"\0").decode("ascii", "replace")
        vsz, va, rsz, ptr = struct.unpack_from("<IIII", s, 8)
        sects.append((name, base + va, vsz, ptr, rsz))
    return raw, base, sects


def reader(raw, sects):
    def rd(va, n):
        for _, sva, vsz, ptr, rsz in sects:
            if sva <= va < sva + max(vsz, rsz):
                o = ptr + (va - sva)
                if o + n <= len(raw):
                    return raw[o:o + n]
        return None
    return rd


def cstr(rd, va, limit=64):
    b = rd(va, limit)
    if b is None:
        return None
    i = b.find(b"\0")
    if i < 0:
        return None
    try:
        t = b[:i].decode("ascii")
    except UnicodeDecodeError:
        return None
    if any(ord(c) < 0x20 or ord(c) > 0x7e for c in t):
        return None
    return t


def scan(raw, base, sects):
    rd = reader(raw, sects)
    lo = base
    hi = max(sva + max(vsz, rsz) for _, sva, vsz, ptr, rsz in sects)
    found, seen = [], set()
    for name, sva, vsz, ptr, rsz in sects:
        if name.startswith(".text"):
            continue
        n = min(vsz, rsz) if rsz else vsz
        blob = raw[ptr:ptr + n]
        for off in range(0, max(0, len(blob) - 20), 4):
            npv, spv, flags, val, nxt = struct.unpack_from("<IIIfI", blob, off)
            if not (lo <= npv < hi and lo <= spv < hi):
                continue
            if flags > 0xffff or not (nxt == 0 or lo <= nxt < hi):
                continue
            nm = cstr(rd, npv)
            if not nm or not IDENT.match(nm) or len(nm) < 2:
                continue
            sv = cstr(rd, spv)
            if sv is None:
                continue
            # Console bindings and localized menu entries contain many
            # cvar-shaped five-word records. Engine cvar names are lowercase,
            # and their defaults are never localization tokens.
            if (not (nm[0].islower() or nm[0] == "_") and nm != "HostMap") or sv.startswith("%"):
                continue
            va = sva + off
            if nm in seen:
                continue
            seen.add(nm)
            found.append((nm, sv, flags, val, va))
    found.sort(key=lambda r: r[0].lower())
    return found


def unescape(t):
    """C string literal -> the bytes the compiler emits, so a source-level
    backslash escape compares equal to the single character in the image."""
    out, i = [], 0
    while i < len(t):
        if t[i] == chr(92) and i + 1 < len(t):
            nxt = t[i + 1]
            out.append({'n': chr(10), 't': chr(9), 'r': chr(13), '0': chr(0)}.get(nxt, nxt))
            i += 2
        else:
            out.append(t[i]); i += 1
    return ''.join(out)


FLAG_VALUES = {
    "FCVAR_ARCHIVE": 1,
    "FCVAR_USERINFO": 2,
    "FCVAR_SERVER": 4,
    "FCVAR_EXTDLL": 8,
    "FCVAR_CLIENTDLL": 16,
    "FCVAR_PROTECTED": 32,
    "FCVAR_SPONLY": 64,
    "FCVAR_PRINTABLEONLY": 128,
    "FALSE": 0,
    "TRUE": 1,
}


def source_flags(expr):
    if expr is None:
        return 0
    value = 0
    for term in expr.split("|"):
        term = term.strip()
        if term in FLAG_VALUES:
            value |= FLAG_VALUES[term]
        else:
            try:
                value |= int(term, 0)
            except ValueError:
                return None
    return value


def source_value(expr):
    if expr is None:
        return 0.0
    expr = expr.strip()
    if expr in FLAG_VALUES:
        return float(FLAG_VALUES[expr])
    try:
        return float(expr.rstrip("fF"))
    except ValueError:
        return None


def fork_cvars():
    defs, regs = {}, set()
    for pat in ("src/**/*.c", "src/**/*.C", "src/**/*.cpp"):
        for p in glob.glob(os.path.join(REPO, pat), recursive=True):
            try:
                txt = open(p, encoding="utf-8", errors="replace").read()
            except OSError:
                continue
            rel = os.path.relpath(p, REPO).replace("\\", "/")
            for m in DEF.finditer(txt):
                defs[m.group(2)] = (unescape(m.group(3)), rel, m.group(1),
                                    source_flags(m.group(4)), source_value(m.group(5)))
            for m in DEF_MACRO.finditer(txt):
                defs[m.group(2)] = (None, rel, m.group(1), 0, 0.0)
            for m in REG.finditer(txt):
                regs.add(m.group(1))
    return defs, regs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--tsv")
    args = ap.parse_args()

    raw, base, sects = load_image()
    binc = scan(raw, base, sects)
    defs, regs = fork_cvars()

    print("cvar_t records in the image : %d" % len(binc))
    print("cvar_t definitions in src/  : %d" % len(defs))

    if args.tsv:
        with open(args.tsv, "w", encoding="utf-8", newline="\n") as f:
            f.write("name\tdefault\tflags\taddr\n")
            for nm, sv, fl, val, va in binc:
                f.write("%s\t%s\t%d\t%06x\n" % (nm, sv, fl, va))
        print("wrote", args.tsv)

    if args.list:
        print("\n=== every cvar in the image ===")
        for nm, sv, fl, val, va in binc:
            print("  %06x  %-28s = %-16s flags=%d value=%g" %
                  (va, nm, '"%s"' % sv, fl, val))

    bn = {nm: (sv, fl, val, va) for nm, sv, fl, val, va in binc}

    missing = [r for r in binc if r[0] not in defs]
    if missing:
        print("\n=== in the image, NOT defined in src/ (%d) ===" % len(missing))
        for nm, sv, fl, val, va in missing:
            print("  %06x  %-28s = %-16s flags=%d" % (va, nm, '"%s"' % sv, fl))

    extra = [n for n in defs if n not in bn]
    if extra:
        print("\n=== defined in src/, NOT in the image (%d) ===" % len(extra))
        for n in sorted(extra):
            d, rel, var, fl, val = defs[n]
            print("  %-28s = %-16s %s" % (n, '"%s"' % d if d is not None else "<macro>", rel))

    bad = []
    for n, (d, rel, var, fl, val) in defs.items():
        if d is not None and n in bn and bn[n][0] != d:
            bad.append((n, d, bn[n][0], rel))
    if bad:
        print("\n=== default value differs (%d) ===" % len(bad))
        for n, d, b, rel in sorted(bad):
            print("  %-28s src=%-14s bin=%-14s %s" % (n, '"%s"' % d, '"%s"' % b, rel))

    bad_flags = []
    bad_values = []
    for n, (d, rel, var, fl, val) in defs.items():
        if n not in bn:
            continue
        if fl is not None and fl != bn[n][1]:
            bad_flags.append((n, fl, bn[n][1], rel))
        if val is not None and abs(val - bn[n][2]) > 0.000001:
            bad_values.append((n, val, bn[n][2], rel))
    if bad_flags:
        print("\n=== flags differ (%d) ===" % len(bad_flags))
        for n, src, binary, rel in sorted(bad_flags):
            print("  %-28s src=%-4d bin=%-4d %s" % (n, src, binary, rel))
    if bad_values:
        print("\n=== initial value differs (%d) ===" % len(bad_values))
        for n, src, binary, rel in sorted(bad_values):
            print("  %-28s src=%-10g bin=%-10g %s" % (n, src, binary, rel))

    unreg = [n for n, (d, rel, var, fl, val) in sorted(defs.items()) if var not in regs]
    if unreg:
        print("\n=== defined in src/ but never Cvar_RegisterVariable'd (%d) ===" % len(unreg))
        for n in unreg:
            print("  %-28s %s" % (n, defs[n][1]))


if __name__ == "__main__":
    main()
