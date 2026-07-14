#!/usr/bin/env python3
"""
Align fork src *.c/*.cpp files to the binary's real .obj layout, so files can be
renamed / merged / split to match. Three inputs:

  1. src function definitions  -> name -> fork src file  (which file DEFINES each fn)
  2. symbols.tsv               -> name -> binary address  (where that fn lives)
  3. assert-path strings       -> binary filename -> address it's referenced from
                                  (the authoritative name for that .obj)

Outputs, worst-first:
  * OVERLAP  : two fork files whose binary address footprints interleave
               => the fork split one .obj into two (merge) OR mis-filed a fn.
  * RENAME   : an assert says file X owns an address, but the fork file defining
               the fn there is Y != X  => rename/move.
"""
import os, re, glob
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SYMS = os.path.join(HERE, "symbols.tsv")
FP = os.path.join(HERE, "portdata", "HALFLIFE_DC.EXE_fp.tsv")
SRC_DIRS = ["engine", "render", "network", "common", "util", "client", "audio", "halflife"]
HDR = re.compile(r"([A-Za-z_]\w*(?:\s*::\s*~?[A-Za-z_]\w*)?)\s*\([^;{}]*\)\s*(?:const)?\s*$")
KW = ("if", "for", "while", "switch", "return", "sizeof", "else", "do", "catch")
APATH = re.compile(r"halflifedc/+src/+([A-Za-z0-9_]+)/+([A-Za-z0-9_]+\.(?:cpp|hpp|c|h))\b", re.I)


def strip(t):
    out = []; i = 0; n = len(t); s = c = lc = bc = False
    while i < n:
        ch = t[i]; two = t[i:i+2]
        if lc:
            if ch == "\n": lc = False; out.append(ch)
            i += 1; continue
        if bc:
            if two == "*/": bc = False; i += 2; continue
            i += 1; continue
        if s:
            if ch == "\\": i += 2; continue
            if ch == '"': s = False
            i += 1; continue
        if c:
            if ch == "\\": i += 2; continue
            if ch == "'": c = False
            i += 1; continue
        if two == "//": lc = True; i += 2; continue
        if two == "/*": bc = True; i += 2; continue
        if ch == '"': s = True; i += 1; continue
        if ch == "'": c = True; i += 1; continue
        out.append(ch); i += 1
    return "".join(out)


def src_index():
    """name -> fork src file (only names defined in exactly one file)."""
    name2files = defaultdict(set)
    for d in SRC_DIRS:
        for ext in ("*.c", "*.C", "*.cpp"):
            for path in glob.glob(os.path.join(REPO, "src", d, "**", ext), recursive=True):
                rel = os.path.relpath(path, os.path.join(REPO, "src")).replace("\\", "/")
                try:
                    t = strip(open(path, encoding="latin-1").read())
                except Exception:
                    continue
                depth = i = 0; n = len(t)
                while i < n:
                    ch = t[i]
                    if ch == "{":
                        if depth == 0:
                            j = i - 1
                            while j > 0 and t[j] not in ";}{":
                                j -= 1
                            m = HDR.search(t[j+1:i].strip())
                            if m:
                                nm = re.sub(r"\s+", "", m.group(1))
                                if nm.split("::")[0] not in KW:
                                    name2files[nm].add(rel)
                        depth += 1; i += 1; continue
                    if ch == "}":
                        depth = max(0, depth-1); i += 1; continue
                    i += 1
    return {k: next(iter(v)) for k, v in name2files.items() if len(v) == 1}


def main():
    n2f = src_index()
    va2name = {}
    for ln in open(SYMS, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) == 2:
            try: va2name[int(p[1], 16)] = p[0]
            except ValueError: pass

    # fork src file -> its binary address footprint
    foot = defaultdict(list)
    for va, nm in va2name.items():
        f = n2f.get(nm)
        if f:
            foot[f].append(va)
    filerange = {f: (min(v), max(v), len(v)) for f, v in foot.items()}

    # asserts: binary filename -> addresses it is referenced from
    asserts = defaultdict(list)
    for ln in open(FP, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 4: continue
        try: a = int(p[0], 16)
        except ValueError: continue
        for m in APATH.finditer(p[3].replace("\\", "/")):
            asserts[(m.group(1)+"/"+m.group(2)).lower()].append(a)

    print("=== OVERLAP: fork files whose binary footprints interleave (merge/split candidates) ===")
    items = sorted(filerange.items(), key=lambda kv: kv[1][0])
    for i in range(len(items)):
        fa, (la, ha, ca) = items[i]
        for fb, (lb, hb, cb) in items[i+1:]:
            if lb > ha: break
            if lb <= ha:  # b starts before a ends -> interleave
                print("  %-26s [%06x-%06x n=%d]  overlaps  %-26s [%06x-%06x n=%d]"
                      % (fa, la, ha, ca, fb, lb, hb, cb))

    print("\n=== RENAME/MOVE: assert filename != fork file defining the fn at that address ===")
    for bfile, addrs in sorted(asserts.items()):
        for a in sorted(set(addrs)):
            nm = va2name.get(a, "?")
            forkf = n2f.get(nm)
            if forkf and forkf.lower() != bfile:
                print("  assert %-26s @%06x (%-24s) but fork defines it in %s"
                      % (bfile, a, nm, forkf))

    print("\n=== per fork-file footprint (link order) ===")
    for f, (lo, hi, c) in items:
        print("  %-28s %06x-%06x  fns=%d" % (f, lo, hi, c))


if __name__ == "__main__":
    main()
