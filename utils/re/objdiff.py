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

The binary side is sliced out of the image disassembly, from a function's symbol
up to the next one, with literal pools dropped and the tail cut at the epilogue.
Ghidra's exported listing (--listing) is only a cross-check: wherever a literal
pool splits a function, Ghidra's body stops at the pool and the code after it
goes missing, which reads as a large false divergence.

Usage:
  python objdiff.py --obj build/combat.obj [--func CBaseMonster::Killed]
  python objdiff.py --obj build/console.obj --all      # score every fn in obj
  python objdiff.py --obj build/host_cmd.obj --func Host_Kick_f --full

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


# Registers with fixed ABI/architectural roles -- NOT part of the allocator's
# choices, so they stay put under canonicalization.
# Allocatable GP registers r1-r13 (NOT r0/r14/r15, which have fixed roles). These
# are the compiler's free choices; abstracting them makes a register-renaming
# difference invisible.
_ALLOC_RE = re.compile(r"\br(?:1[0-3]|[1-9])\b")
# SP-relative displacements are spill/local slots whose offsets shift with how
# many callee-saved regs were pushed -- i.e. regalloc-dependent, not structural.
_SP_DISP_RE = re.compile(r"@\((-?(?:0x)?[0-9a-f]+),r15\)")


def canon_stream(norm_insns):
    """Per-instruction abstraction that isolates instruction-set STRUCTURE from
    the register allocator's free choices: every allocatable register (r1-r13)
    collapses to 'rX' and every SP-relative spill slot to '@(sp)'. r0/r14/r15 and
    @(disp,rN) struct-field offsets are kept (they're meaningful, not regalloc).

    Because abstraction only ever MERGES distinct instructions, the resulting
    'struct' ratio is always >= the exact ratio; the gap is how much of the miss
    is pure register/spill assignment (noise we can't steer from C) vs. real
    structural divergence still worth fixing."""
    out = []
    for s in norm_insns:
        s = _SP_DISP_RE.sub("@(sp)", s)
        s = _ALLOC_RE.sub("rX", s)
        out.append(s)
    return out


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


# pc-relative pool loads name their own operand: mov.l @(disp,pc) reads 4 bytes
# at ((va & ~3) + 4 + disp), mov.w @(disp,pc) reads 2 bytes at (va + 4 + disp).
POOL_LOAD = re.compile(r"^mov\.(l|w)$")
POOL_OPERAND = re.compile(r"@\((-?[0-9a-f]+),pc\)", re.I)


def pool_slots(insns):
    """Addresses covered by literal pools, taken from the loads that read them.
    Exact, unlike inferring pool extent from control flow."""
    data = set()
    for va, m, o in insns:
        mm = POOL_LOAD.match(m.lower())
        if not mm:
            continue
        g = POOL_OPERAND.search(o)
        if not g:
            continue
        try:
            disp = int(g.group(1), 16)
        except ValueError:
            continue
        if mm.group(1) == "l":
            base, width = (va & ~3) + 4, 4
        else:
            base, width = va + 4, 2
        for k in range(0, width, 2):
            data.add(base + disp + k)
    return data


def trim_pool(insns, known_data=None):
    """Drop literal-pool words that dumpbin disassembles as code.

    Two passes. The structural one first: SHCL places a pool after an
    unconditional transfer and execution can only resume at a branch target, so
    anything between the transfer's delay slot and the next target is data, and
    the tail after the final rts is data outright. It runs to a fixed point,
    because pool words that happen to decode as branches seed targets that end a
    pool early and leave the rest of it in; recomputing the targets from only the
    survivors drops those.

    Then the exact one, which catches pools the first pass can't see (one wedged
    between two branch targets, say): every pc-relative load names the address it
    reads, so those addresses are data by definition."""
    prev = None
    while prev != len(insns):
        prev = len(insns)
        insns = _trim_pool_once(insns)
    data = pool_slots(insns)
    if known_data:
        data |= known_data
    if data:
        insns = [t for t in insns if t[0] not in data]
    return insns


def _trim_pool_once(insns):
    targets = set()
    for va, m, o in insns:
        if m.lower() in ("bt", "bf", "bt/s", "bf/s", "bra", "bsr"):
            # dumpbin prints the branch displacement (relative to va+4), not
            # the target address; FFFFxxxx values are negative.
            tok = o.strip().split(",")[0]
            try:
                d = int(tok, 16)
            except ValueError:
                continue
            if d >= 0x80000000:
                d -= 0x100000000
            targets.add(va + 4 + d)
    out = []
    i = 0
    n = len(insns)
    while i < n:
        out.append(insns[i])
        if insns[i][1].lower() in ("bra", "jmp", "rts"):
            if i + 1 < n:
                out.append(insns[i + 1])   # delay slot
            i += 2
            while i < n and insns[i][0] not in targets:
                i += 1                     # pool word
            continue
        i += 1
    return out


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


def load_exe_disasm(dumpbin, exe):
    """Flat dumpbin disassembly of the image, cached next to the exe."""
    cache = exe + ".disasm.txt"
    if os.path.exists(cache) and os.path.getmtime(cache) >= os.path.getmtime(exe):
        text = open(cache, encoding="latin-1").read()
    else:
        text = run_dumpbin(dumpbin, "-disasm:nobytes", exe)
        open(cache, "w", encoding="latin-1").write(text)
    return parse_disasm(text)


def cut_at_epilogue(insns, min_len=0):
    """A function ends at its epilogue rts plus the delay slot; everything after
    is literal pool or the next (possibly unnamed) function. trim_pool alone
    can't tell, because pool words that decode as fake branches seed bogus
    targets. min_len skips over an early-return rts when the function is known
    to be longer than that."""
    for i, t in enumerate(insns):
        if t[1].lower() == "rts" and i + 2 >= min_len:
            return insns[:i + 2]
    return insns


def exe_function_extents(syms):
    """Function start -> end, where end is the next symbol start. Ghidra bodies
    stop short whenever a literal pool splits a function, so the gap to the next
    named symbol is the reliable extent."""
    starts = sorted(set(a for a, _ in syms.values()))
    nxt = {}
    for i, a in enumerate(starts):
        nxt[a] = starts[i + 1] if i + 1 < len(starts) else a + 0x400
    return nxt


def diff_one(name, obj_insns, bin_insns, full=False):
    a, b = norm_stream(obj_insns), norm_stream(bin_insns)
    # autojunk=False: the >200-element 'popular element' heuristic silently drops
    # repeated instructions (esp. after abstraction), distorting large functions.
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    ratio = sm.ratio()
    # register-allocation-invariant score: same instruction set/structure up to a
    # consistent register renaming scores as a match.
    rn = difflib.SequenceMatcher(None, canon_stream(a), canon_stream(b),
                                 autojunk=False).ratio()
    print("\n=== %s ===  obj=%d insns  bin=%d insns  match=%.1f%%  struct=%.1f%% (regalloc gap %.1f)"
          % (name, len(a), len(b), ratio * 100, rn * 100, (rn - ratio) * 100))
    if full:
        # side-by-side of the whole function: '=' kept, '-' obj-only, '+' bin-only
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                for k in range(i1, i2):
                    print("  = %-34s | %s" % (a[k], b[j1 + k - i1]))
            else:
                for k in range(i1, i2):
                    print("  - %-34s |" % a[k])
                for k in range(j1, j2):
                    print("  + %-34s | %s" % ("", b[k]))
    elif ratio < 1.0:
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
    return (ratio, rn)


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
    ap.add_argument("--scores", help="write name/obj/bin/exact%/struct% TSV to this path")
    ap.add_argument("--listing", action="store_true",
                    help="binary side = Ghidra's exported listing instead of the "
                         "exe disassembly (bodies split by a literal pool come "
                         "out truncated, so scores read low)")
    ap.add_argument("--full", action="store_true",
                    help="print the whole obj-vs-binary instruction diff")
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

    exe_dis = exe_ext = exe_data = None
    if not args.listing:
        exe_dis = load_exe_disasm(args.dumpbin, args.exe)
        exe_ext = exe_function_extents(syms)
        exe_data = None

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
        # tab-separated, FULL names (backport_names.py parses this)
        print("obj_function\tinsns\tbinary_va\tmatch")
        for sym, n, va, r in rows:
            print("%s\t%d\t%s\t%.1f" % (sym, n, ("%08x" % va) if va else "-", r * 100))
        return

    results = []
    score_rows = []
    for sym, insns in ofuncs.items():
        if not insns:
            continue
        # obj_functions() keys are already Ghidra names (label_to_gname strips the
        # single compiler-added leading underscore). Do NOT strip again here or a
        # Valve name that is itself underscore-prefixed (e.g. _FreeBlock_Coalesce,
        # emitted as __FreeBlock_Coalesce) loses its real leading underscore.
        gname = sym
        if args.func and gname != args.func:
            continue
        if gname not in syms:
            if args.func:
                print("!! %s (%s) not found in symbols.tsv" % (gname, sym))
            continue
        start, size = syms[gname]
        if not args.listing:
            # extent = gap to the next named symbol, cut back to the epilogue;
            # the Ghidra listing's length is a floor so a function with an
            # early-return rts isn't truncated.
            end = exe_ext.get(start, start + size)
            gh = bininsns.get(format(start, "x"))
            bin_insns = cut_at_epilogue(
                trim_pool(slice_exe(exe_dis, start, end - start), exe_data),
                len(gh or ()))
            r, rn = diff_one(gname, insns, bin_insns, full=args.full)
            results.append((gname, r, rn))
            score_rows.append((gname, len(insns), len(bin_insns), r * 100, rn * 100))
            continue
        bkey = format(start, "x")
        if bkey not in bininsns:
            if args.func:
                print("!! %s @ %s not in Ghidra listing (re-run ExportInsns.java)" % (gname, bkey))
            continue
        bin_insns = bininsns[bkey]
        r, rn = diff_one(gname, insns, bin_insns, full=args.full)
        results.append((gname, r, rn))
        score_rows.append((gname, len(insns), len(bin_insns), r * 100, rn * 100))
        if not (args.all or args.func):
            pass

    if args.scores and score_rows:
        score_rows.sort(key=lambda x: x[0])
        with open(args.scores, "w", encoding="latin-1") as fh:
            for nm, o, b, m, st in score_rows:
                fh.write("%s\t%d\t%d\t%.1f\t%.1f\n" % (nm, o, b, m, st))
        print("[objdiff] wrote %d scores -> %s" % (len(score_rows), args.scores))

    if results:
        results.sort(key=lambda x: x[1])
        matched = sum(1 for _, r, _ in results if r == 1.0)
        structex = sum(1 for _, _, rn in results if rn == 1.0)
        print("\n[objdiff] %d/%d exact; %d/%d struct-exact (same code mod regalloc); "
              "mean %.1f%%  struct-mean %.1f%%"
              % (matched, len(results), structex, len(results),
                 100 * sum(r for _, r, _ in results) / len(results),
                 100 * sum(rn for _, _, rn in results) / len(results)))


if __name__ == "__main__":
    main()
