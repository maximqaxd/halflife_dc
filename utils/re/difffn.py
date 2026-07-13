#!/usr/bin/env python3
"""Show the full aligned obj-vs-binary normalized instruction diff for ONE function.
Usage: difffn.py <obj> <GhidraFuncName> [--struct]
--struct diffs the register/spill-canonicalized streams: every remaining '!' line
is REAL structural divergence (regalloc noise is invisible in this view).
Reuses objdiff's dumpbin + normalization so scores line up."""
import sys, os, difflib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import objdiff as O

def main():
    obj, fn = sys.argv[1], sys.argv[2]
    struct_mode = "--struct" in sys.argv
    syms = O.load_symbols(O.SYMS_DEFAULT)
    bininsns = O.load_bin_insns(O.BININSNS_DEFAULT)
    ofuncs = O.obj_functions(O.DUMPBIN_DEFAULT, obj)
    # find obj symbol whose demangled name == fn
    match = None
    for sym, insns in ofuncs.items():
        if O.demangle(sym) == fn:
            match = insns; break
    if match is None:
        print("obj function not found:", fn); return
    start, size = syms[fn]
    b = bininsns[format(start, "x")]
    a = O.norm_stream(match)
    bb = O.norm_stream(b)
    if struct_mode:
        a, bb = O.canon_stream(a), O.canon_stream(bb)
    sm = difflib.SequenceMatcher(None, a, bb, autojunk=False)
    print("=== %s [%s]  obj=%d bin=%d  match=%.1f%% ===" %
          (fn, "STRUCT" if struct_mode else "exact", len(a), len(bb), sm.ratio()*100))
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for k in range(i1, i2):
                print("    %s" % a[k])
        else:
            m = max(i2 - i1, j2 - j1)
            for k in range(m):
                l = a[i1 + k] if i1 + k < i2 else ""
                r = bb[j1 + k] if j1 + k < j2 else ""
                mark = "!" if l != r else " "
                print("  %s %-34s | %s" % (mark, l, r))

if __name__ == "__main__":
    main()
