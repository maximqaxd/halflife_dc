#!/usr/bin/env python3
"""
SH-4 object/binary diff harness for the Half-Life Dreamcast RE.

Compares functions compiled from the fork source (a COFF .obj produced by
shcl.exe) against the same functions in the reference binary
(RE/HALFLIFE_DC.EXE), using dumpbin -disasm on both sides and a normalized
instruction-stream comparison that ignores link-time differences
(literal-pool displacements, absolute call targets) while keeping everything
structural (mnemonics, registers, struct/stack offsets, branch shape).

A 100% normalized match means the compiled code has the same instructions in
the same order as the binary -- i.e. the source reproduces that function.

Usage:
  python objdiff.py --obj build/combat.obj [--func CBaseMonster::Killed]
  python objdiff.py --obj build/console.obj --all      # score every fn in obj

Requires: symbols.tsv (from ExportSymbols.java), dumpbin.exe on PATH or via
--dumpbin, and RE/HALFLIFE_DC.EXE.
"""
import argparse, os, re, subprocess, sys, difflib

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DUMPBIN_DEFAULT = r"C:\Windows CE Tools\WCE212\bin\DUMPBIN.EXE"
EXE_DEFAULT = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE")
SYMS_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "symbols.tsv")
BININSNS_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "bin_insns.tsv")

# Call idioms — near (bsr), far (bsrf), indirect (jsr): all collapse to "call"
# so link-distance choices (which the linked binary makes but a standalone .obj
# can't) don't register as differences.
CALL = {"bsr", "bsrf", "jsr"}
# Unconditional jumps
JUMP = {"bra", "braf", "jmp"}
# Conditional branches — keep the mnemonic, drop the (link-relative) target.
COND = {"bt", "bf", "bt/s", "bf/s"}

# dumpbin -disasm line:  "  0001100c: 8900 bt      00000000"  (with bytes)
#                    or  "  0001100c: bt      00000000"        (nobytes)
# SH-4 mnemonics can contain '.' (mov.l) and '/' (cmp/gt, bf/s) -> include both
LINE = re.compile(r"^\s*([0-9A-Fa-f]{4,8}):\s+(?:[0-9A-Fa-f]{2,4}\s+)?([a-z][a-z0-9._/]*)\s*(.*)$")


def run_dumpbin(dumpbin, *args):
    # dash-form flags; MSYS/quoting-safe since we exec directly (no shell).
    out = subprocess.run([dumpbin, *args], capture_output=True, text=True)
    return out.stdout


def parse_disasm(text):
    """Return ordered list of (va:int, mnem:str, ops:str)."""
    insns = []
    for ln in text.splitlines():
        m = LINE.match(ln)
        if not m:
            continue
        va = int(m.group(1), 16)
        insns.append((va, m.group(2), m.group(3).strip()))
    return insns


def _canon_num(tok):
    """Canonicalize an immediate/displacement to a signed-32 decimal string.
    Handles dumpbin (#FFFFFFF0, 00000004) and Ghidra (#-0x10, #0x1, 0x4)."""
    s = tok.strip()
    neg = s.startswith("-")
    if neg:
        s = s[1:]
    try:
        if s.lower().startswith("0x"):
            v = int(s, 16)
        else:
            v = int(s, 16)  # dumpbin bare hex
    except ValueError:
        return tok
    if neg:
        v = -v
    v &= 0xFFFFFFFF
    if v >= 0x80000000:
        v -= 0x100000000
    return str(v)


def normalize(mnem, ops):
    """Format-agnostic canonical form for a dumpbin OR Ghidra instruction, so the
    obj (dumpbin) and binary (Ghidra, pool-free) sides compare on structure."""
    mnem = mnem.lstrip("_").lower()          # drop Ghidra delay-slot prefix
    if mnem in CALL:
        return "call"
    if mnem in JUMP:
        return "jump"
    o = ops.strip().lower()
    if mnem in COND:
        return mnem                          # drop link-relative target
    # pool loads: dumpbin @(disp,pc) and Ghidra bare absolute addr -> @(pc)
    o = re.sub(r"@\(-?(?:0x)?[0-9a-f]+,pc\)", "@(pc)", o)
    o = re.sub(r"(?<![#\w])0x[0-9a-f]{5,8}\b", "@(pc)", o)
    # canonicalize @(disp,rN) displacements
    o = re.sub(r"@\((-?(?:0x)?[0-9a-f]+),(r\d+|pc|gbr)\)",
               lambda m: "@(" + _canon_num(m.group(1)) + "," + m.group(2) + ")", o)
    # canonicalize # immediates
    o = re.sub(r"#(-?(?:0x)?[0-9a-f]+)", lambda m: "#" + _canon_num(m.group(1)), o)
    return mnem + " " + o if o else mnem


def norm_stream(insns):
    return [normalize(m, o) for (va, m, o) in insns]


def load_bin_insns(path):
    """entry_hex -> [(va, mnem, ops)] from bin_insns.tsv (Ghidra's pool-free
    instruction listing). Lines: entryHex<TAB>vaHex<TAB>mnem ops."""
    funcs = {}
    with open(path, encoding="latin-1") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3:
                continue
            entry, va, insn = p[0], p[1], p[2]
            parts = insn.split(None, 1)
            mnem = parts[0] if parts else ""
            ops = parts[1] if len(parts) > 1 else ""
            funcs.setdefault(entry, []).append((int(va, 16), mnem, ops))
    return funcs


def load_symbols(path):
    """name -> (start, size). Prefer Ghidra's recorded body size; fall back to
    the gap-to-next-symbol only when the recorded size is a truncated stub
    (<=8 bytes, typical of vtable-swept functions Ghidra didn't fully analyze).
    Always clamp so a function never overruns into the next symbol."""
    raw = {}
    with open(path) as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) == 3:
                raw[p[0]] = (int(p[1], 16), int(p[2]))
    starts = sorted({s for s, _ in raw.values()})
    nextof = {}
    for i, s in enumerate(starts):
        nextof[s] = starts[i + 1] if i + 1 < len(starts) else s + 0x400
    out = {}
    for n, (s, size) in raw.items():
        gap = nextof[s] - s
        out[n] = (s, gap if size <= 8 else min(size, gap))
    # maybe_-prefixed Ghidra names are unconfirmed identifications; alias the
    # bare name (what the source uses) when it doesn't collide.
    for n in list(out):
        if n.startswith("maybe_") and n[6:] not in out:
            out[n[6:]] = out[n]
    return out


def demangle(sym):
    """obj symbol -> Ghidra function name. Handles C (_foo) and simple
    MSVC C++ (?meth@class@@...) -> class::meth."""
    if sym.startswith("?"):
        body = sym[1:]
        parts = body.split("@")
        if len(parts) >= 2 and parts[1]:
            meth, cls = parts[0], parts[1]
            return cls + "::" + meth
        return parts[0]
    if sym.startswith("_"):
        return sym[1:]
    return sym


# label forms:  "_name:"  or  "?mangled@... (undecorated sig):"
LABEL_C = re.compile(r"^([A-Za-z_?][\w?@$.]*):$")
LABEL_CPP = re.compile(r"^\S+\s+\((.*)\):$")
# inside an undecorated sig, the qualified fn name right before its arg list:
CDECL = re.compile(r"(?:__cdecl|__thiscall|__stdcall|__fastcall)\s+([\w:~]+)\s*\(")


def label_to_gname(line):
    """Derive the Ghidra function name from a dumpbin label line, or None."""
    line = line.strip()
    m = LABEL_CPP.match(line)
    if m:
        sig = m.group(1)
        c = CDECL.search(sig)
        if c:
            return c.group(1)          # e.g. CBarney::Save
        # no calling-convention token (free C++ fn); take name before first '('
        c2 = re.search(r"([\w:~]+)\s*\(", sig)
        return c2.group(1) if c2 else None
    m = LABEL_C.match(line)
    if m:
        s = m.group(1)
        return s[1:] if s.startswith("_") else s   # strip C leading underscore
    return None


def trim_pool(insns):
    """Cut the trailing literal pool: obj disassembly runs the PC-relative
    constant pool right after a function's final `rts` (+ its delay slot).
    Everything after the last rts's delay slot is data, not code."""
    last = -1
    for i, (_, m, _) in enumerate(insns):
        if m == "rts":
            last = i
    if last >= 0:
        return insns[:last + 2]  # keep rts + its delay-slot instruction
    return insns


def obj_functions(dumpbin, obj):
    """Map Ghidra function name -> list-of-insns, from the obj disassembly."""
    funcs = {}
    cur = None
    for ln in run_dumpbin(dumpbin, "-disasm", obj).splitlines():
        g = label_to_gname(ln)
        if g is not None:
            cur = g
            funcs[cur] = []
            continue
        m = LINE.match(ln)
        if m and cur is not None:
            funcs[cur].append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return {k: trim_pool(v) for k, v in funcs.items()}


def slice_exe(exe_dis, start, size):
    return [t for t in exe_dis if start <= t[0] < start + size]


def diff_one(name, obj_insns, bin_insns):
    a, b = norm_stream(obj_insns), norm_stream(bin_insns)
    sm = difflib.SequenceMatcher(None, a, b)
    ratio = sm.ratio()
    print("\n=== %s ===  obj=%d insns  bin=%d insns  match=%.1f%%"
          % (name, len(a), len(b), ratio * 100))
    if ratio < 1.0:
        # show first divergence
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                continue
            print("  first diff @ obj[%d] bin[%d]:" % (i1, j1))
            for k in range(i1, min(i1 + 4, len(a))):
                print("    obj: " + a[k])
            for k in range(j1, min(j1 + 4, len(b))):
                print("    bin: " + b[k])
            break
    return ratio


def locate(obj_insns, exe_dis, exe_norm):
    """Find the best-matching window in the binary for an unnamed obj function,
    anchored on the first 3 normalized instructions. Returns (va, ratio)."""
    a = norm_stream(obj_insns)
    if len(a) < 3:
        return (None, 0.0)
    anchor = tuple(a[:3])
    n = len(a)
    best_va, best_r = None, 0.0
    for i in range(len(exe_norm) - 3):
        if (exe_norm[i], exe_norm[i + 1], exe_norm[i + 2]) != anchor:
            continue
        window = exe_norm[i:i + n]
        r = difflib.SequenceMatcher(None, a, window).ratio()
        if r > best_r:
            best_r, best_va = r, exe_dis[i][0]
    return (best_va, best_r)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--obj", required=True)
    ap.add_argument("--exe", default=EXE_DEFAULT)
    ap.add_argument("--symbols", default=SYMS_DEFAULT)
    ap.add_argument("--dumpbin", default=DUMPBIN_DEFAULT)
    ap.add_argument("--func", help="diff only this Ghidra function name")
    ap.add_argument("--all", action="store_true", help="score every matched fn")
    ap.add_argument("--locate", action="store_true",
                    help="find each obj fn in the binary by code (no symbols needed)")
    args = ap.parse_args()

    syms = load_symbols(args.symbols)
    # binary side = Ghidra's pool-free per-function listing (from ExportInsns.java)
    bininsns = load_bin_insns(BININSNS_DEFAULT)
    print("[objdiff] %d binary symbols; %d binary functions (Ghidra listing)"
          % (len(syms), len(bininsns)))

    ofuncs = obj_functions(args.dumpbin, args.obj)
    print("[objdiff] obj functions: %d" % len(ofuncs))

    if args.locate:
        # --locate still needs the flat exe disassembly (dumpbin, cached)
        cache = args.exe + ".disasm.txt"
        if os.path.exists(cache) and os.path.getmtime(cache) >= os.path.getmtime(args.exe):
            text = open(cache, encoding="latin-1").read()
        else:
            text = run_dumpbin(args.dumpbin, "-disasm:nobytes", args.exe)
            open(cache, "w", encoding="latin-1").write(text)
        exe_dis = parse_disasm(text)
        exe_norm = norm_stream(exe_dis)
        rows = []
        for sym, insns in ofuncs.items():
            if len(insns) < 3:
                continue
            va, r = locate(insns, exe_dis, exe_norm)
            rows.append((sym, len(insns), va, r))
        rows.sort(key=lambda x: -x[3])
        print("\n  %-22s %5s  %-10s %s" % ("obj function", "insns", "binary@", "match"))
        for sym, n, va, r in rows:
            print("  %-22s %5d  %-10s %.1f%%"
                  % (sym[:22], n, ("%08x" % va) if va else "-", r * 100))
        return

    results = []
    for sym, insns in ofuncs.items():
        if not insns:
            continue
        gname = demangle(sym)
        if args.func and gname != args.func:
            continue
        if gname not in syms:
            if args.func:
                print("!! %s (%s) not found in symbols.tsv" % (gname, sym))
            continue
        start, size = syms[gname]
        bkey = format(start, "x")
        if bkey not in bininsns:
            if args.func:
                print("!! %s @ %s not in Ghidra listing (re-run ExportInsns.java)" % (gname, bkey))
            continue
        bin_insns = bininsns[bkey]
        r = diff_one(gname, insns, bin_insns)
        results.append((gname, r))
        if not (args.all or args.func):
            pass

    if results:
        results.sort(key=lambda x: x[1])
        matched = sum(1 for _, r in results if r == 1.0)
        print("\n[objdiff] %d/%d functions exact-match; mean %.1f%%"
              % (matched, len(results),
                 100 * sum(r for _, r in results) / len(results)))


if __name__ == "__main__":
    main()
