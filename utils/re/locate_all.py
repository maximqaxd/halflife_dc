#!/usr/bin/env python3
"""Locate every game-DLL function in the reference binary, in one pass.

objdiff --locate loads and normalizes the whole exe disassembly per invocation,
which is far too slow to run over a hundred source files.  This drives the same
matcher with the binary side loaded once.

Usage: python utils/re/locate_all.py [src/halflife/*.cpp ...]
Writes utils/re/build/locate_all.tsv: file, function, insns, va, match%.
"""
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import objdiff

OUT = "utils/re/build/locate_all.tsv"

def main(argv):
    files = argv[1:] or sorted(glob.glob("src/halflife/*.cpp"))
    objs = []
    for f in files:
        base = os.path.splitext(os.path.basename(f))[0]
        obj = "utils/re/build/%s.obj" % base
        if not os.path.exists(obj) or os.path.getmtime(obj) < os.path.getmtime(f):
            r = subprocess.run(["bash", "utils/re/build_obj.sh", f],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if r.returncode != 0:
                print("[skip-build] %s" % base, file=sys.stderr)
                continue
        objs.append((base, obj))

    cache = objdiff.EXE_DEFAULT + ".disasm.txt"
    text = open(cache, encoding="latin-1").read()
    exe_dis = objdiff.parse_disasm(text)
    exe_norm = objdiff.norm_stream(exe_dis)
    print("[locate] %d exe instructions, %d objs" % (len(exe_dis), len(objs)), file=sys.stderr)

    # one index pass over the binary instead of a full scan per function
    anchors = {}
    for i in range(len(exe_norm) - 3):
        anchors.setdefault((exe_norm[i], exe_norm[i+1], exe_norm[i+2]), []).append(i)

    import difflib
    with open(OUT, "a" if argv[1:] else "w") as fh:
        for base, obj in objs:
            try:
                ofuncs = objdiff.obj_functions(objdiff.DUMPBIN_DEFAULT, obj)
            except Exception as e:
                print("[skip-obj] %s (%s)" % (base, e), file=sys.stderr)
                continue
            for sym, insns in ofuncs.items():
                a = objdiff.norm_stream(insns)
                if len(a) < 3:
                    continue
                best_va, best_r = None, 0.0
                for i in anchors.get(tuple(a[:3]), ()):
                    r = difflib.SequenceMatcher(None, a, exe_norm[i:i+len(a)]).ratio()
                    if r > best_r:
                        best_r, best_va = r, exe_dis[i][0]
                if best_va:
                    fh.write("%s\t%s\t%d\t%08x\t%.1f\n" % (base, sym, len(insns), best_va, best_r * 100))
            fh.flush()
            print("[done] %s" % base, file=sys.stderr)

if __name__ == "__main__":
    main(sys.argv)
