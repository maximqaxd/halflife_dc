#!/usr/bin/env python3
"""
Binary-side coverage: which functions in the exe does the fork actually account
for? Complements scoreboard.py, which only reports on what already compiles --
this walks the other direction, starting from every symbol in the binary, so
whole translation units the fork has never reproduced show up instead of being
silently absent.

Each binary function lands in one of four buckets:

  scored   - a fork file compiles it and objdiff matched it at its address
  present  - defined somewhere in src/, but never scored (file does not compile,
             or objdiff could not match the name)
  alias    - not found under this name, but src/ defines it under a decorated
             variant (R_ / SV_ / CL_ / DC_ / DCV_ / GL_ prefix, or _Neo suffix)
  missing  - no definition anywhere in src/
  unnamed  - still FUN_xxxx in the database, so not even identified yet

Ranges owned by src/client and src/halflife are skipped by default; pass --all
to include them.

Usage:
  python utils/re/coverage.py                 # summary + worst clusters
  python utils/re/coverage.py --list missing  # every uncovered function
  python utils/re/coverage.py --tu            # roll up by owning TU/region
"""
import os, sys, csv, re, glob, argparse

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

SYMS = os.path.join(HERE, "symbols.tsv")
SCORE = os.path.join(HERE, "scoreboard.csv")

# .text regions, from the link-order map in CLAUDE.md. Excluded ones belong to
# the statically linked game DLL and the client HUD, which are out of scope here.
REGIONS = [
    (0x011000, 0x01615c, "audio",            True),
    (0x016260, 0x01f000, "client/hud.cpp",   False),
    (0x01f000, 0x040000, "engine (client)",  True),
    (0x040000, 0x06a4ac, "engine (host/sv)", True),
    (0x06a4ac, 0x10c320, "halflife game dll", False),
    (0x10c320, 0x126000, "engine + network", True),
    (0x126000, 0x15b000, "render",           True),
    (0x15b000, 0x178558, "util / view / CRT", True),
]

DEF_RE = re.compile(
    r'^[A-Za-z_][A-Za-z0-9_ \t\*&:<>,]*?'
    r'\b([A-Za-z_][A-Za-z0-9_]*(?:::~?[A-Za-z_][A-Za-z0-9_]*)?)\s*\([^;]*$')


def norm(n):
    # Ghidra's provisional-name prefix is not part of the real symbol, and
    # C++ methods are spelled both ways across the exports.
    if n.startswith("maybe_"):
        n = n[6:]
    return n.replace("::", "__")


def alias_key(n):
    """Name with module decoration stripped, so R_StudioGetAttachment and the
    fork's GetAttachment collapse together. Used only as a fallback -- an exact
    match always wins -- because it is deliberately lossy."""
    for pre in ("R_", "SV_", "CL_", "GL_", "DC_", "DCV_", "Mod_", "Studio"):
        if n.startswith(pre) and len(n) > len(pre) + 3:
            n = n[len(pre):]
    for suf in ("_Neo",):
        if n.endswith(suf):
            n = n[:-len(suf)]
    return n.lower().replace("_", "")


def region_of(addr):
    for lo, hi, name, inscope in REGIONS:
        if lo <= addr < hi:
            return name, inscope
    return "?", False


def load_symbols():
    out = []
    with open(SYMS, encoding="utf-8", errors="replace") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3:
                continue
            try:
                out.append((p[0], int(p[1], 16), int(p[2])))
            except ValueError:
                continue
    out.sort(key=lambda r: r[1])
    return out


def load_scored():
    scored = {}
    if not os.path.exists(SCORE):
        return scored
    with open(SCORE, encoding="utf-8", errors="replace") as f:
        for r in csv.DictReader(f):
            scored[norm(r["func"])] = (r["file"], float(r["match"]))
    return scored


def index_source_defs(dirs):
    """Every identifier that looks like a function definition under src/."""
    defs = {}
    for d in dirs:
        for ext in ("*.c", "*.C", "*.cpp", "*.cc"):
            for p in glob.glob(os.path.join(REPO, d, "**", ext), recursive=True):
                try:
                    txt = open(p, encoding="utf-8", errors="replace").read()
                except OSError:
                    continue
                rel = os.path.relpath(p, REPO).replace("\\", "/")
                for line in txt.split("\n"):
                    if line[:1] in (" ", "\t", "#", "/", "*", "}") or "(" not in line:
                        continue
                    m = DEF_RE.match(line)
                    if m:
                        defs.setdefault(norm(m.group(1)), rel)
    return defs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", choices=["missing", "present", "alias", "unnamed", "all"])
    ap.add_argument("--tu", action="store_true", help="roll up by region")
    ap.add_argument("--all", action="store_true", help="include client + game dll")
    ap.add_argument("--min-size", type=int, default=0)
    args = ap.parse_args()

    syms = load_symbols()
    scored = load_scored()
    defs = index_source_defs(["src"])
    scored_alias = {}
    for k, v in scored.items():
        scored_alias.setdefault(alias_key(k), v)
    defs_alias = {}
    for k, v in defs.items():
        defs_alias.setdefault(alias_key(k), v)

    rows = []
    for name, addr, size in syms:
        region, inscope = region_of(addr)
        if not inscope and not args.all:
            continue
        if size < args.min_size:
            continue
        n = norm(name)
        if n.startswith("FUN_"):
            bucket = "unnamed"
        elif n in scored:
            bucket = "scored"
        elif n in defs:
            bucket = "present"
        elif alias_key(n) in scored_alias or alias_key(n) in defs_alias:
            bucket = "alias"
        else:
            bucket = "missing"
        if bucket == "alias":
            src = (scored_alias.get(alias_key(n)) or (defs_alias[alias_key(n)],))[0]
        else:
            src = scored.get(n, (defs.get(n, ""), None))[0]
        rows.append((addr, size, name, region, bucket, src))

    if not rows:
        print("no symbols in scope -- is symbols.tsv present?")
        return

    order = ["scored", "present", "alias", "missing", "unnamed"]
    print("=== coverage by function count / bytes ===")
    tot_n = len(rows)
    tot_b = sum(r[1] for r in rows)
    for b in order:
        sel = [r for r in rows if r[4] == b]
        nb = sum(r[1] for r in sel)
        print("  %-8s %5d fns (%5.1f%%)   %8d bytes (%5.1f%%)"
              % (b, len(sel), 100.0 * len(sel) / tot_n, nb, 100.0 * nb / tot_b))
    print("  %-8s %5d fns              %8d bytes" % ("TOTAL", tot_n, tot_b))

    if args.tu:
        print("\n=== by region ===")
        print("  %-20s %6s %7s %7s %6s %7s %7s  %s"
              % ("region", "fns", "scored", "present", "alias", "missing", "unnamed",
                 "uncovered bytes"))
        for lo, hi, rname, inscope in REGIONS:
            if not inscope and not args.all:
                continue
            sel = [r for r in rows if r[3] == rname]
            if not sel:
                continue
            c = {b: sum(1 for r in sel if r[4] == b) for b in order}
            unc = sum(r[1] for r in sel if r[4] in ("missing", "unnamed"))
            print("  %-20s %6d %7d %7d %6d %7d %7d  %d"
                  % (rname, len(sel), c["scored"], c["present"], c["alias"],
                     c["missing"], c["unnamed"], unc))

    # Contiguous runs of uncovered functions: a long run is an unreproduced TU.
    print("\n=== largest uncovered clusters (missing/unnamed runs) ===")
    clusters, cur = [], []
    for r in sorted(rows):
        if r[4] in ("missing", "unnamed"):
            cur.append(r)
        else:
            if cur:
                clusters.append(cur)
            cur = []
    if cur:
        clusters.append(cur)
    clusters.sort(key=lambda c: sum(x[1] for x in c), reverse=True)
    for c in clusters[:20]:
        nb = sum(x[1] for x in c)
        print("  0x%06x-0x%06x  %3d fns  %6d bytes  [%s]  e.g. %s"
              % (c[0][0], c[-1][0] + c[-1][1], len(c), nb, c[0][3],
                 ", ".join(x[2] for x in c[:4])))

    if args.list:
        print("\n=== %s ===" % args.list)
        for r in sorted(rows):
            if args.list != "all" and r[4] != args.list:
                continue
            print("  0x%06x %6d  %-10s %-20s %-44s %s"
                  % (r[0], r[1], r[4], r[3], r[2], r[5]))


if __name__ == "__main__":
    main()
