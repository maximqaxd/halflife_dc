#!/usr/bin/env python3
"""
Cross-binary symbol port: carry authentic function names from the symboled
hlds_l (x86 Linux, the fork parent) onto the unsymboled HALFLIFE_DC.EXE (SH-4).

Different architectures -> no byte/instruction matching. We correlate the two
purely by what the shared source guarantees is identical:
  1. rarity-weighted shared STRING references (strong seeds)
  2. iterative CALL-GRAPH propagation from those seeds

Inputs : portdata/hlds_l_fp.tsv, portdata/HALFLIFE_DC.EXE_fp.tsv
          (from ExportFingerprints.java: addr, name, is_default, strings, callees)
Output : portdata/hlds_names.tsv  ->  dc_addr_hex \t hlds_name \t score \t method
"""
import os, re, sys
from collections import defaultdict

D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "portdata")
HLDS = "hlds_l_fp.tsv"
DC = "HALFLIFE_DC.EXE_fp.tsv"

# string-weight cap: ignore strings referenced by so many functions on either
# side that a shared occurrence carries almost no identifying information.
PAIR_CAP = 40
MIN_LEN = 4
MIN_SEED = 0.40         # min rarity-weighted string score to seed a match
PROP_MIN_SHARED = 3     # call-graph: min matched neighbours in common
PROP_MIN_JACCARD = 0.50
PROP_MARGIN = 0.15      # winner must beat runner-up by this
MAX_GAP = 12            # align a run of N unmatched fns between two anchors when
                        # both sides have exactly N (source order preserved)


class Fn:
    __slots__ = ("addr", "name", "isdef", "strs", "callees")

    def __init__(self, addr, name, isdef, strs, callees):
        self.addr = addr
        self.name = name
        self.isdef = isdef
        self.strs = strs
        self.callees = callees


def load(path):
    fns = {}
    for ln in open(os.path.join(D, path), encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        while len(p) < 5:
            p.append("")
        addr, name, isdef, strs, cals = p[:5]
        fns[addr] = Fn(addr, name, int(isdef or 0),
                       set(s for s in strs.split("\x01") if len(s) >= MIN_LEN) if strs else set(),
                       [c for c in cals.split(",") if c] if cals else [])
    return fns


def placeholder(nm):
    return nm.startswith(("FUN_", "SUB_", "LAB_", "thunk_"))


def demangle_gcc2(nm):
    """Minimal GCC 2.x demangle for the names hlds_l carries: free functions
    (foo__Fii -> foo) and members (meth__11CBaseEntity -> CBaseEntity::meth).
    Ghidra's demangler doesn't handle this old '__F' style; leave anything else."""
    if "__" not in nm:
        return nm
    m = re.match(r"^(\w+)__F[A-Za-z0-9_]*$", nm)
    if m:
        return m.group(1)
    m = re.match(r"^(\w+)__(?:C)?(\d+)([A-Za-z_]\w*)$", nm)
    if m:
        base, ln, rest = m.group(1), int(m.group(2)), m.group(3)
        if len(rest) >= ln:
            return rest[:ln] + "::" + base
    return nm


def main():
    H = load(HLDS)
    Dc = load(DC)
    print("hlds fns %d, dc fns %d" % (len(H), len(Dc)))

    # string -> set(addr) for each side
    def str_index(fns):
        idx = defaultdict(set)
        for a, f in fns.items():
            for s in f.strs:
                idx[s].add(a)
        return idx
    hidx, didx = str_index(H), str_index(Dc)

    # candidate score[(h_addr,d_addr)] = sum rarity weights over shared strings
    score = defaultdict(float)
    for s, hset in hidx.items():
        dset = didx.get(s)
        if not dset:
            continue
        if len(hset) * len(dset) > PAIR_CAP:
            continue                       # too common: skip
        w = 1.0 / (len(hset) * len(dset))  # rarity-1 both -> 1.0
        for ha in hset:
            for da in dset:
                score[(ha, da)] += w

    # greedy mutual assignment by descending score (string seeds)
    pairs = sorted(score.items(), key=lambda kv: -kv[1])
    h2d, d2h = {}, {}
    method = {}
    for (ha, da), sc in pairs:
        if sc < MIN_SEED:
            break
        if ha in h2d or da in d2h:
            continue
        h2d[ha] = da
        d2h[da] = ha
        method[da] = ("string", sc)
    print("string-seed matches: %d" % len(h2d))

    # name-equality seeds: every function DC already shares a (unique) authentic
    # name with hlds is a high-confidence call-graph anchor. This is what makes
    # re-running after an apply pass propagate further.
    hname, dname = defaultdict(list), defaultdict(list)
    for a, f in H.items():
        if not placeholder(f.name):
            hname[f.name].append(a)
    for a, f in Dc.items():
        if not placeholder(f.name):
            dname[f.name].append(a)
    neq = 0
    for nm, has in hname.items():
        das = dname.get(nm)
        if das and len(has) == 1 and len(das) == 1:
            ha, da = has[0], das[0]
            if ha not in h2d and da not in d2h:
                h2d[ha] = da
                d2h[da] = ha
                method[da] = ("name", 1.0)
                neq += 1
    print("name-equality seeds: %d (total seeds %d)" % (neq, len(h2d)))

    # call-graph propagation --------------------------------------------------
    # neighbour sets in matched space: a fn -> the set of matched partner addrs
    # among its callees and callers.
    hcallers = defaultdict(set)
    for a, f in H.items():
        for c in f.callees:
            hcallers[c].add(a)
    dcallers = defaultdict(set)
    for a, f in Dc.items():
        for c in f.callees:
            dcallers[c].add(a)

    def neigh_h(a):
        return set(H[a].callees if a in H else []) | hcallers.get(a, set())

    def neigh_d(a):
        return set(Dc[a].callees if a in Dc else []) | dcallers.get(a, set())

    def order_align():
        # For each matched caller pair, use its already-matched callees as ordered
        # anchors; a gap between two anchors holding exactly one unmatched callee on
        # each side pins that pair by call-site position (great for string-less fns).
        added = 0
        for ha, da in list(h2d.items()):
            hc = H[ha].callees
            dc = Dc[da].callees
            dpos = {}
            for j, c in enumerate(dc):
                dpos.setdefault(c, j)
            anchors = []
            prevj = -1
            ok = True
            for i, c in enumerate(hc):
                if c in h2d:
                    j = dpos.get(h2d[c])
                    if j is None or j <= prevj:
                        ok = False
                        break
                    anchors.append((i, j))
                    prevj = j
            if not ok or len(anchors) < 2:
                continue
            bounds = [(-1, -1)] + anchors + [(len(hc), len(dc))]
            for k in range(len(bounds) - 1):
                i0, j0 = bounds[k]
                i1, j1 = bounds[k + 1]
                gh = [hc[x] for x in range(i0 + 1, i1) if hc[x] not in h2d and hc[x] in H]
                gd = [dc[x] for x in range(j0 + 1, j1) if dc[x] not in d2h and dc[x] in Dc]
                if len(gh) == len(gd) and 1 <= len(gh) <= MAX_GAP:
                    # a run of N string-less callees between two anchors aligns 1:1
                    # by position (source order preserved). Fill-only (0.9).
                    for hi, dj in zip(gh, gd):
                        if hi not in h2d and dj not in d2h:
                            h2d[hi] = dj
                            d2h[dj] = hi
                            method[dj] = ("callorder", 0.9)
                            added += 1
        return added

    H_sorted = sorted(H.keys(), key=lambda a: int(a, 16))
    D_sorted = sorted(Dc.keys(), key=lambda a: int(a, 16))
    hpos = {a: i for i, a in enumerate(H_sorted)}
    dpos = {a: i for i, a in enumerate(D_sorted)}

    def addr_align():
        # Link-order alignment: functions are emitted in source order per object
        # file, so between two consecutive matched anchors that keep the same order
        # on both sides, a lone unmatched function in the gap aligns by position.
        added = 0
        anchors = [(hpos[a], dpos[h2d[a]]) for a in H_sorted
                   if a in h2d and h2d[a] in dpos]
        for k in range(len(anchors) - 1):
            hi0, dj0 = anchors[k]
            hi1, dj1 = anchors[k + 1]
            if dj1 <= dj0:
                continue                      # not monotonic in DC -> different region
            gh = [H_sorted[x] for x in range(hi0 + 1, hi1) if H_sorted[x] not in h2d]
            gd = [D_sorted[x] for x in range(dj0 + 1, dj1) if D_sorted[x] not in d2h]
            if len(gh) == len(gd) and 1 <= len(gh) <= MAX_GAP:
                # a run of N functions between two anchors aligns 1:1 by position
                # when both sides have the same count (link order preserved).
                for hi, dj in zip(gh, gd):
                    if hi not in h2d and dj not in d2h:
                        h2d[hi] = dj
                        d2h[dj] = hi
                        method[dj] = ("addrorder", 0.9)   # fill-only: never clobbers
                        added += 1
        return added

    passes = 0
    while True:
        passes += 1
        # for each unmatched hlds fn, project neighbours into dc space via h2d
        added = order_align() + addr_align()
        # precompute matched-dc-neighbour set per unmatched d for scoring
        for ha, hf in H.items():
            if ha in h2d:
                continue
            hn = neigh_h(ha)
            proj = set(h2d[x] for x in hn if x in h2d)   # expected dc neighbours
            if len(proj) < PROP_MIN_SHARED:
                continue
            best = None
            bestj = 0.0
            second = 0.0
            # candidate dc fns: those sharing >=1 neighbour with proj
            cand = set()
            for dn in proj:
                cand |= neigh_d(dn)          # dc fns adjacent to expected neighbours
            for da in cand:
                if da in d2h:
                    continue
                dn = neigh_d(da)
                inter = len(dn & proj)
                if inter < PROP_MIN_SHARED:
                    continue
                uni = len(dn | proj)
                j = inter / uni if uni else 0
                if j > bestj:
                    second = bestj
                    bestj = j
                    best = da
                elif j > second:
                    second = j
            if best is not None and best in Dc and bestj >= PROP_MIN_JACCARD and bestj - second >= PROP_MARGIN:
                h2d[ha] = best
                d2h[best] = ha
                method[best] = ("callgraph", bestj)
                added += 1
        print("  propagation pass %d: +%d" % (passes, added))
        if added == 0 or passes >= 8:
            break

    # write results -----------------------------------------------------------
    out = os.path.join(D, "hlds_names.tsv")
    rows = []
    renamed = same = 0
    for da, ha in d2h.items():
        hn = demangle_gcc2(H[ha].name)
        if placeholder(hn):
            continue
        dn = Dc[da].name
        if hn == dn:
            same += 1
            continue
        m, sc = method[da]
        rows.append((da, hn, "%.3f" % sc, m, dn))
        renamed += 1
    rows.sort(key=lambda r: r[3] + r[0])
    with open(out, "w", encoding="utf-8") as w:
        w.write("dc_addr\thlds_name\tscore\tmethod\tdc_old_name\n")
        for r in rows:
            w.write("\t".join(r) + "\n")
    print("total matched %d | would-rename %d | already-same %d" % (len(d2h), renamed, same))
    print("by method:", {m: sum(1 for r in rows if r[3] == m) for m in ("string", "callgraph")})
    print("-> %s" % out)
    # preview
    print("\nsample renames:")
    for r in rows[:25]:
        print("  %-8s %-28s %s %s (was %s)" % (r[0], r[1], r[2], r[3], r[4]))

    # ---- diagnostic: which hlds NAMED functions did not match, by prefix ----
    unm = [(H[a].name, len(H[a].strs), len(H[a].callees))
           for a in H if a not in h2d and not placeholder(H[a].name)]

    def pfx(nm):
        i = nm.find("_")
        return (nm[:i + 1] if 0 < i <= 6 else nm[:5])
    hist = {}
    for nm, _, _ in unm:
        hist[pfx(nm)] = hist.get(pfx(nm), 0) + 1
    print("\nUNMATCHED hlds named fns: %d" % len(unm))
    for p, c in sorted(hist.items(), key=lambda kv: -kv[1])[:35]:
        print("   %-14s %d" % (p, c))
    with open(os.path.join(D, "unmatched_hlds.tsv"), "w", encoding="utf-8") as w:
        for nm, ns, nc in sorted(unm):
            w.write("%s\t%d\t%d\n" % (nm, ns, nc))

    # ---- GLOBALS: match by referencing-function-set overlap (needs *_glob.tsv) ----
    gh_file = os.path.join(D, "hlds_l_glob.tsv")
    gd_file = os.path.join(D, "HALFLIFE_DC.EXE_glob.tsv")
    if os.path.exists(gh_file) and os.path.exists(gd_file):
        def load_glob(path):
            g = {}
            for ln in open(path, encoding="utf-8"):
                p = ln.rstrip("\n").split("\t")
                while len(p) < 4:
                    p.append("")
                ga, name, isdef, refs = p[:4]
                g[ga] = (name, int(isdef or 0), set(r for r in refs.split(",") if r) if refs else set())
            return g
        HG = load_glob(gh_file)
        DG = load_glob(gd_file)
        dfn2g = defaultdict(set)          # dc fn addr -> dc globals it references
        for ga, (nm, isd, refs) in DG.items():
            for f in refs:
                dfn2g[f].add(ga)
        cands = []                        # (jaccard, hlds_gaddr, dc_gaddr)
        for gh, (hnm, hisd, hrefs) in HG.items():
            if hisd:
                continue                  # only globals hlds actually names
            proj = set(h2d[f] for f in hrefs if f in h2d)
            if len(proj) < 2:
                continue
            pool = set()
            for f in proj:
                pool |= dfn2g.get(f, set())
            best = None; bestj = 0.0; second = 0.0
            for gd in pool:
                drefs = DG[gd][2]
                inter = len(drefs & proj)
                if inter < 2:
                    continue
                j = inter / len(drefs | proj)
                if j > bestj:
                    second = bestj; bestj = j; best = gd
                elif j > second:
                    second = j
            if best is not None and bestj >= 0.50 and bestj - second >= 0.10:
                cands.append((bestj, gh, best))
        cands.sort(reverse=True)
        g_h2d, g_d2h, gscore = {}, {}, {}
        for j, gh, gd in cands:
            if gh in g_h2d or gd in g_d2h:
                continue
            g_h2d[gh] = gd; g_d2h[gd] = gh; gscore[gd] = j
        outg = os.path.join(D, "glob_names.tsv")
        gn = gsame = 0
        with open(outg, "w", encoding="utf-8") as w:
            w.write("dc_gaddr\thlds_gname\tscore\tdc_old\n")
            for gd, gh in sorted(g_d2h.items(), key=lambda kv: -gscore[kv[0]]):
                hnm = HG[gh][0]; dnm = DG[gd][0]
                if hnm == dnm:
                    gsame += 1; continue
                w.write("%s\t%s\t%.3f\t%s\n" % (gd, hnm, gscore[gd], dnm))
                gn += 1
        print("\nglobals matched %d | would-rename %d | already-same %d -> %s"
              % (len(g_d2h), gn, gsame, outg))


if __name__ == "__main__":
    main()
