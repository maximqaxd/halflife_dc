#!/usr/bin/env python3
"""
Backport authentic function names from the binary (symbols.tsv, kept in sync with
Ghidra) onto the fork source. For each src file we compile it, code-locate every
function in HALFLIFE_DC.EXE via objdiff (no name reliance), and where the located
binary address carries a real name that differs from the src name, emit a rename.

Output: portdata/rename_map.tsv  (src_name, new_name, match%, file)  -- REVIEW it,
then apply with apply_renames.py.

Usage:
  python backport_names.py src/engine/host.c src/render/decals.c ...
  python backport_names.py --all           # every *.c/*.cpp under the src dirs
"""
import os, re, subprocess, sys, glob

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SYMS = os.path.join(HERE, "symbols.tsv")
BUILD = os.path.join(HERE, "build_obj.sh")
OBJDIFF = os.path.join(HERE, "objdiff.py")
BUILDDIR = os.path.join(HERE, "build")
OUT = os.path.join(HERE, "portdata", "rename_map.tsv")

MIN_MATCH = 88.0        # code-match confidence; below this the correspondence is unsafe
MIN_INSNS = 12          # tiny functions false-match stubs (Cmd_Argc, UI_Draw ...)
SRC_DIRS = ["engine", "render", "network", "common", "util", "client", "audio"]


def load_va2name(path):
    va2name = {}
    for ln in open(path, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 2:
            continue
        try:
            va2name[int(p[1], 16)] = p[0]
        except ValueError:
            pass
    return va2name


def placeholder(nm):
    return nm.startswith(("FUN_", "SUB_", "LAB_", "thunk_"))


def files_from_args(args):
    if args == ["--all"]:
        fs = []
        for d in SRC_DIRS:
            for ext in ("*.c", "*.C", "*.cpp"):
                fs += glob.glob(os.path.join(REPO, "src", d, "**", ext), recursive=True)
        return sorted(fs)
    return [a if os.path.isabs(a) else os.path.join(REPO, a) for a in args]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    va2name = load_va2name(SYMS)
    print("[backport] %d binary symbols" % len(va2name))
    files = files_from_args(sys.argv[1:])
    rows = []
    for f in files:
        rel = os.path.relpath(f, REPO).replace("\\", "/")
        stem = os.path.splitext(os.path.basename(f))[0]
        obj = os.path.join(BUILDDIR, stem + ".obj")
        if not os.path.exists(obj):
            print("  skip (no obj -- build it first): %s" % rel)
            continue
        loc = subprocess.run([sys.executable, OBJDIFF, "--obj", obj, "--locate"],
                             capture_output=True, text=True)
        nfile = 0
        for line in loc.stdout.splitlines():
            p = line.split("\t")
            if len(p) != 4 or p[0] == "obj_function":
                continue
            src_name, insns_s, va, pct_s = p
            try:
                insns, pct = int(insns_s), float(pct_s)
            except ValueError:
                continue
            if va == "-" or pct < MIN_MATCH or insns < MIN_INSNS:
                continue
            if "::" in src_name or "~" in src_name:      # C++ methods: not plain identifiers
                continue
            bn = va2name.get(int(va, 16))
            if not bn or placeholder(bn) or bn == src_name or "::" in bn or "~" in bn:
                continue
            rows.append((src_name, bn, "%.1f" % pct, rel))
            nfile += 1
        print("  %-40s candidates=%d" % (rel, nfile))

    # drop stub magnets: a new_name that several src functions "matched" is a
    # coincidental short-function hit, not a real correspondence.
    from collections import Counter
    newc = Counter(r[1] for r in rows)
    kept = [r for r in rows if newc[r[1]] == 1]
    dropped = len(rows) - len(kept)
    kept.sort(key=lambda r: (r[3], -float(r[2])))
    with open(OUT, "w", encoding="utf-8") as w:
        w.write("src_name\tnew_name\tmatch\tfile\n")
        for r in kept:
            w.write("\t".join(r) + "\n")
    print("[backport] %d candidate renames (%d ambiguous dropped) -> %s"
          % (len(kept), dropped, OUT))


if __name__ == "__main__":
    main()
