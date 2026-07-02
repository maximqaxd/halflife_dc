#!/usr/bin/env python3
"""
Phase 2 reconstruction dashboard. Compile each given fork source file and score
every function against the binary, producing a per-file / per-function match
table. Drives the attack: highest-value files = many functions, low mean match.

Usage:
  python utils/re/scoreboard.py src/engine/*.c
  python utils/re/scoreboard.py src/render          # a whole dir
Writes utils/re/scoreboard.csv (file,func,obj_insns,bin_insns,match) and prints
a per-file summary sorted by mean match (worst first).
"""
import os, sys, glob, subprocess, csv, re

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
BUILD = os.path.join(HERE, "build")
RES = re.compile(r"=== (.+?) ===\s+obj=(\d+) insns\s+bin=(\d+) insns\s+match=([\d.]+)%")

def expand(args):
    files = []
    for a in args:
        if os.path.isdir(a):
            for ext in ("*.c", "*.C", "*.cpp"):
                files += glob.glob(os.path.join(a, ext))
        else:
            files += glob.glob(a)
    return sorted(set(files))

def main():
    files = expand(sys.argv[1:])
    if not files:
        print("no files"); return
    rows = []
    per_file = []
    for src in files:
        base = os.path.splitext(os.path.basename(src))[0]
        b = subprocess.run(["bash", os.path.join(HERE, "build_obj.sh"), src],
                           capture_output=True, text=True)
        obj = os.path.join(BUILD, base + ".obj")
        if b.returncode != 0 or not os.path.exists(obj):
            # capture the missing-include or error briefly
            err = ""
            for ln in (b.stdout + b.stderr).splitlines():
                if "error" in ln.lower(): err = ln.strip()[:80]; break
            per_file.append((src, None, 0, 0, err or "compile failed"))
            continue
        d = subprocess.run([sys.executable, os.path.join(HERE, "objdiff.py"),
                            "--obj", obj, "--all"], capture_output=True, text=True)
        fns = []
        for m in RES.finditer(d.stdout):
            name, oi, bi, mt = m.group(1), int(m.group(2)), int(m.group(3)), float(m.group(4))
            rows.append([src, name, oi, bi, mt]); fns.append(mt)
        if fns:
            mean = sum(fns) / len(fns)
            exact = sum(1 for x in fns if x >= 99.9)
            per_file.append((src, mean, len(fns), exact, ""))
        else:
            per_file.append((src, None, 0, 0, "no matched functions"))

    with open(os.path.join(HERE, "scoreboard.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["file", "func", "obj_insns", "bin_insns", "match"])
        w.writerows(rows)

    print("\n%-34s %6s %5s %5s  %s" % ("file", "mean%", "fns", "100%", "note"))
    print("-" * 72)
    def key(r): return (r[1] if r[1] is not None else -1)
    for src, mean, nf, exact, note in sorted(per_file, key=key):
        rel = os.path.relpath(src, REPO)
        ms = ("%.1f" % mean) if mean is not None else "  -"
        print("%-34s %6s %5d %5d  %s" % (rel[-34:], ms, nf, exact, note))
    tot = [r[4] for r in rows]
    if tot:
        print("\nTOTAL functions=%d  exact=%d  mean=%.1f%%"
              % (len(tot), sum(1 for x in tot if x >= 99.9), sum(tot) / len(tot)))

if __name__ == "__main__":
    main()
