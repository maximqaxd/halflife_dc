#!/usr/bin/env python3
"""
Per-source-file "unnamed functions" checklist for HALFLIFE_DC.EXE, excluding the
statically-linked game DLL region (0x6a4ac-0x10c320, halflife/*.cpp).

File attribution comes from two anchor sources, then link order fills the gaps:
  1. src definitions -- a NAMED binary function whose name is defined in exactly
     one src/*.c/*.cpp file is attributed to that file (dense, precise).
  2. assert paths    -- the "halflifedc\\src\\<dir>\\<file>" strings a function
     references (covers files not yet in src).
Each file's region = [min anchored addr, next file's min). Unanchored functions
(incl. subsystems missing from src, e.g. audio) fall into the preceding region.

Reads portdata/HALFLIFE_DC.EXE_fp.tsv; writes portdata/region_checklist.tsv.
"""
import os, re
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
FP = os.path.join(HERE, "portdata", "HALFLIFE_DC.EXE_fp.tsv")
OUT = os.path.join(HERE, "portdata", "region_checklist.tsv")

GAME_LO, GAME_HI = 0x6a4ac, 0x10c320
PATH = re.compile(r"halflifedc/src/([A-Za-z0-9_]+)/([A-Za-z0-9_]+\.(?:c|C|cpp))")
SRC_DIRS = ["engine", "render", "network", "common", "util", "client", "audio"]
HDR = re.compile(
    r"([A-Za-z_]\w*(?:\s*::\s*~?[A-Za-z_]\w*)?)\s*\([^;{}]*\)\s*(?:const)?\s*$")
KW = ("if", "for", "while", "switch", "return", "sizeof", "else", "do")


def strip(text):
    out = []; i = 0; n = len(text); s = c = lc = bc = False
    while i < n:
        ch = text[i]; two = text[i:i + 2]
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
    name2files = defaultdict(set)
    files = []
    for d in SRC_DIRS:
        for ext in ("*.c", "*.C", "*.cpp"):
            import glob
            files += glob.glob(os.path.join(REPO, "src", d, "**", ext), recursive=True)
    for path in files:
        rel = os.path.relpath(path, os.path.join(REPO, "src")).replace("\\", "/")
        try:
            t = strip(open(path, encoding="latin-1").read())
        except Exception:
            continue
        depth = 0; i = 0; n = len(t)
        while i < n:
            ch = t[i]
            if ch == "{":
                if depth == 0:
                    j = i - 1
                    while j > 0 and t[j] not in ";}{":
                        j -= 1
                    m = HDR.search(t[j + 1:i].strip())
                    if m:
                        nm = re.sub(r"\s+", "", m.group(1))
                        if nm.split("::")[0] not in KW:
                            name2files[nm].add(rel)
                depth += 1; i += 1; continue
            if ch == "}":
                depth = max(0, depth - 1); i += 1; continue
            i += 1
    # keep only unambiguous (defined in exactly one file)
    return {k: next(iter(v)) for k, v in name2files.items() if len(v) == 1}


def main():
    s2f = src_index()
    print("[src] %d uniquely-defined functions indexed" % len(s2f))

    funcs = []
    file_min = {}
    for ln in open(FP, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 4:
            continue
        try:
            addr = int(p[0], 16)
        except ValueError:
            continue
        name, isdef, strs = p[1], int(p[2] or 0), re.sub(r"\\+", "/", p[3])
        anchor = s2f.get(name)                          # src definition (precise)
        if anchor is None:
            paths = set(a + "/" + b for a, b in PATH.findall(strs))
            if len(paths) == 1:
                anchor = next(iter(paths))              # assert-path fallback
        funcs.append([addr, name, isdef])
        if anchor and addr < file_min.get(anchor, 1 << 32):
            file_min[anchor] = addr
    funcs.sort(key=lambda x: x[0])

    starts = sorted(file_min.items(), key=lambda kv: kv[1])

    def region_of(addr):
        cur = "(startup/pre-anchor)"
        for f, s in starts:
            if addr >= s:
                cur = f
            else:
                break
        return cur

    tot = defaultdict(int); named = defaultdict(int); unn = defaultdict(int)
    lo = defaultdict(lambda: 1 << 32); hi = defaultdict(int); ua = defaultdict(list)
    for addr, name, isdef in funcs:
        if GAME_LO <= addr < GAME_HI:
            continue
        k = region_of(addr)
        tot[k] += 1
        if isdef:
            unn[k] += 1; ua[k].append("%x" % addr)
        else:
            named[k] += 1
        lo[k] = min(lo[k], addr); hi[k] = max(hi[k], addr)

    order = sorted(tot, key=lambda k: -unn[k])
    print("%-30s %-8s %-8s %4s %5s %4s %6s" %
          ("file/region", "lo", "hi", "tot", "named", "UNN", "%named"))
    print("-" * 74)
    tU = tT = 0
    for k in order:
        print("%-30s %08x %08x %4d %5d %4d %5.0f%%" %
              (k, lo[k], hi[k], tot[k], named[k], unn[k],
               100.0 * named[k] / tot[k] if tot[k] else 0))
        tU += unn[k]; tT += tot[k]
    print("-" * 74)
    print("%-30s %8s %8s %4d %5d %4d %5.0f%%" %
          ("TOTAL (non-game)", "", "", tT, tT - tU, tU,
           100.0 * (tT - tU) / tT if tT else 0))

    with open(OUT, "w", encoding="utf-8") as w:
        w.write("region\tlo\thi\ttotal\tnamed\tunnamed\tunnamed_addrs\n")
        for k in order:
            w.write("%s\t%08x\t%08x\t%d\t%d\t%d\t%s\n" %
                    (k, lo[k], hi[k], tot[k], named[k], unn[k], ",".join(ua[k])))
    print("\n-> %s" % OUT)


if __name__ == "__main__":
    main()
