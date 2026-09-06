#!/usr/bin/env python3
"""
Cross-check binary symbol names against the fork by CODE, not by name.

`objdiff.py --locate` finds where each compiled fork function actually lives in
the exe using an anchored n-gram match, so it does not care what the symbol is
called. Wherever a high-confidence hit lands on an address the database calls
something else, one of the two names is wrong -- and the fork's name usually
comes from real Half-Life source while the binary's is a reverse-engineering
guess. Those are the addresses worth relabelling in Ghidra and symbols.tsv.

This is the counterpart to coverage.py: functions reported "missing" only
because the binary symbol carries a different name show up here instead.

Investigating one specific "missing" function? Drop the floor instead of trusting
it: `objdiff.py --obj <obj> --locate | grep <name>`. A poorly reconstructed
function still lands on the RIGHT address even at 25-35%, because the anchored
n-gram only needs a few matching runs. Several render functions reported
"missing" were really implemented under another name AND diverging badly, which
hides them from both a name match and a 90% locate.

Usage:
  python utils/re/relabel.py                       # every built .obj
  python utils/re/relabel.py dc_rsurf r_studio     # only these
  python utils/re/relabel.py --min 90              # confidence floor
"""
import os, re, sys, glob, subprocess, argparse

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(HERE, "build")
SYMS = os.path.join(HERE, "symbols.tsv")
ROW = re.compile(r"^(\S+)\t(\d+)\t([0-9a-fA-F]+)\t([\d.]+)$")


def load_symbols():
    by_addr = {}
    with open(SYMS, encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3:
                continue
            try:
                by_addr[int(p[1], 16)] = p[0]
            except ValueError:
                pass
    return by_addr


def same(a, b):
    """Names that differ only by decoration, not identity."""
    def key(n):
        n = n.replace("::", "__")
        for pre in ("maybe_", "R_", "SV_", "CL_", "GL_", "DC_", "DCV_"):
            if n.startswith(pre):
                n = n[len(pre):]
        return n.lower().replace("_", "")
    return key(a) == key(b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("objs", nargs="*")
    ap.add_argument("--min", type=float, default=90.0)
    # Short bodies (getters, rts-only stubs, thin wrappers) are structurally
    # identical to dozens of others, so a 100% "match" on them means nothing.
    ap.add_argument("--min-insns", type=int, default=20)
    args = ap.parse_args()

    syms = load_symbols()
    objs = ([os.path.join(BUILD, o if o.endswith(".obj") else o + ".obj")
             for o in args.objs]
            or sorted(glob.glob(os.path.join(BUILD, "*.obj"))))

    hits = []
    for obj in objs:
        if not os.path.exists(obj):
            print("  (no such obj: %s)" % obj)
            continue
        r = subprocess.run([sys.executable, os.path.join(HERE, "objdiff.py"),
                            "--obj", obj, "--locate"],
                           capture_output=True, text=True)
        for line in r.stdout.splitlines():
            m = ROW.match(line.strip())
            if not m:
                continue
            fn, insns, va, score = m.group(1), int(m.group(2)), int(m.group(3), 16), float(m.group(4))
            if score < args.min or insns < args.min_insns:
                continue
            cur = syms.get(va)
            if cur is None:
                hits.append((va, score, fn, "<no symbol at this address>",
                             os.path.basename(obj), insns))
            elif not same(cur, fn):
                hits.append((va, score, fn, cur, os.path.basename(obj), insns))

    # An address claimed by more than one fork function is a generic body, not
    # an identification.
    from collections import Counter
    claims = Counter(h[0] for h in hits)
    ambiguous = {a for a, n in claims.items() if n > 1}
    if ambiguous:
        print("dropped %d ambiguous address(es) claimed by multiple fork functions"
              % len(ambiguous))
    hits = [h for h in hits if h[0] not in ambiguous]

    if not hits:
        print("no relabel candidates at >= %.0f%% confidence" % args.min)
        return

    print("Fork function located at an address the database names differently.")
    print("Verify by decompiling before renaming -- a wrong name is worse than none.\n")
    print("  %-8s %6s %5s  %-34s %-34s %s"
          % ("addr", "score", "insns", "fork name (from src/)", "binary symbol now", "obj"))
    for va, score, fn, cur, obj, insns in sorted(hits, key=lambda h: -h[1]):
        print("  %06x %6.1f %5d  %-34s %-34s %s" % (va, score, insns, fn, cur, obj))
    print("\n%d candidate(s)" % len(hits))


if __name__ == "__main__":
    main()
