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
DUMPBIN_DEFAULT = r"C:\Windows CE Tools\wce211\bin\DUMPBIN.EXE"
EXE_DEFAULT = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE")
SYMS_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "symbols.tsv")

# All PC-relative control flow: dumpbin prints targets relative-to-function in
# objs but absolute in the exe, so the displacement isn't comparable across the
# two. Strip to the mnemonic; scoring rests on opcodes + data operands
# (registers, immediates, @(disp,rN) struct/stack offsets, pool loads), which
# still captures register-allocation and instruction-selection divergence.
CFLOW = {"bt", "bf", "bt.s", "bf.s", "bra", "bra.s", "bsr", "bsr.s"}

# dumpbin -disasm line:  "  0001100c: 8900 bt      00000000"  (with bytes)
#                    or  "  0001100c: bt      00000000"        (nobytes)
LINE = re.compile(r"^\s*([0-9A-Fa-f]{4,8}):\s+(?:[0-9A-Fa-f]{2,4}\s+)?([a-z][a-z0-9._]*)\s*(.*)$")


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


def normalize(va, mnem, ops):
    """Collapse link-time-variable operands; keep structure."""
    o = ops
    # literal-pool loads: @(disp,pc) -> @(pc)
    o = re.sub(r"@\(([0-9A-Fa-f]+),pc\)", "@(pc)", o)
    # control-flow target is not comparable across obj/exe dumps -> mnemonic only
    if mnem in CFLOW:
        o = ""
    return mnem + " " + o if o else mnem


def norm_stream(insns):
    return [normalize(va, m, o) for (va, m, o) in insns]


def load_symbols(path):
    """name -> (start, size). size is recomputed as the gap to the next
    function start, because vtable-swept functions have truncated bodies in
    Ghidra and their recorded size is unreliable for slicing."""
    raw = {}
    with open(path) as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) == 3:
                raw[p[0]] = int(p[1], 16)
    starts = sorted(set(raw.values()))
    nextof = {}
    for i, s in enumerate(starts):
        nextof[s] = starts[i + 1] if i + 1 < len(starts) else s + 0x400
    return {n: (s, nextof[s] - s) for n, s in raw.items()}


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
    return funcs


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
    # cache the (slow) 25MB exe disassembly next to the exe; reuse if fresh
    cache = args.exe + ".disasm.txt"
    if os.path.exists(cache) and os.path.getmtime(cache) >= os.path.getmtime(args.exe):
        text = open(cache, encoding="latin-1").read()
    else:
        print("[objdiff] disassembling exe (caching)...")
        text = run_dumpbin(args.dumpbin, "-disasm:nobytes", args.exe)
        open(cache, "w", encoding="latin-1").write(text)
    exe_dis = parse_disasm(text)
    print("[objdiff] %d binary symbols; exe .text: %d instructions"
          % (len(syms), len(exe_dis)))

    ofuncs = obj_functions(args.dumpbin, args.obj)
    print("[objdiff] obj functions: %d" % len(ofuncs))

    if args.locate:
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
        bin_insns = slice_exe(exe_dis, start, size)
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
