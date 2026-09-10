#!/usr/bin/env python3
"""Join the shipped export registry to source identities.

Inputs:
  utils/re/saverestore/bin_registry.tsv  seq, registrar, code, function   (from the binary)
  utils/re/build/locate_all.tsv          file, function, insns, va, match (objdiff --locate)

Emits utils/re/saverestore/code_map.tsv: seq, code, class, method, file, how,
function, registrar.

Each registrar belongs to one class, so the class is settled first, by vote over
the whole block; only then is each entry matched.  That keeps a handler whose
body has drifted from the binary -- a poor instruction match, but the only
candidate of the right class -- from being dropped or confused with the folded
copy of an identical body somewhere else.
"""
import bisect
import collections
import os
import re

REG = "utils/re/saverestore/bin_registry.tsv"
LOC = "utils/re/build/locate_all.tsv"
OUT = "utils/re/saverestore/code_map.tsv"
OVER = "utils/re/saverestore/overrides.tsv"

reg = []
for line in open(REG):
    if line.startswith(("seq", "#")):
        continue
    seq, registrar, code, fn = line.split()
    reg.append((int(seq), int(registrar, 16), code, int(fn, 16)))

cands = collections.defaultdict(list)
for line in open(LOC):
    p = line.rstrip("\n").split("\t")
    if len(p) != 5 or p[3] == "-":
        continue
    f, name, insns, va, match = p
    if name.startswith("SR_Register_") or "::" not in name:
        continue                       # our own registrars, and file-scope helpers
    cands[int(va, 16)].append((f, name, int(insns), float(match)))

# the EXPORT set: only these may carry a saved function pointer
exports = set()
for path in sorted(os.listdir("src/halflife")):
    if path.endswith((".cpp", ".h")):
        txt = open("src/halflife/" + path, encoding="latin-1").read()
        exports.update(m.group(1) for m in re.finditer(r"EXPORT\s+(\w+)\s*\(", txt))

over = {}
for line in open(OVER):
    if line.startswith("#") or not line.strip():
        continue
    p = line.rstrip("\n").split("\t")
    over[p[0]] = (p[1], p[2])

# which source file owns a given binary address: a registrar sits among the
# functions of its own translation unit, so vote with the nearest ones
placed = sorted((va, f) for va, lst in cands.items()
                for f, name, insns, match in lst if match >= 99.0)
placed_va = [va for va, _ in placed]

def file_at(va):
    i = bisect.bisect_left(placed_va, va)
    near = placed[max(0, i - 4):i + 4]
    if not near:
        return "-"
    vote = collections.Counter()
    for a, f in near:
        vote[f] += 1.0 / (1 + abs(a - va))
    return vote.most_common(1)[0][0]

blocks = collections.OrderedDict()
for seq, registrar, code, fn in reg:
    blocks.setdefault(registrar, []).append((seq, code, fn))

rows = []
for registrar, block in blocks.items():
    # settle the class this registrar belongs to
    score = collections.Counter()
    for seq, code, fn in block:
        for f, name, insns, match in cands.get(fn, []):
            cls, _, meth = name.partition("::")
            if meth in exports:
                score[cls] += match
    lead = score.most_common(1)[0][0] if score else None

    for seq, code, fn in block:
        if code in over:
            cls, meth = over[code]
            rows.append((seq, code, cls, meth, file_at(fn), "binary", "%x" % fn,
                         "%x" % registrar, file_at(registrar)))
            continue
        picks = cands.get(fn, [])
        mine = [c for c in picks if c[1].split("::")[0] == lead]
        pool = mine or [c for c in picks if c[1].split("::")[-1] in exports] or picks
        if not pool:
            rows.append((seq, code, "?", "?", "?", "unresolved", "%x" % fn,
                         "%x" % registrar, file_at(registrar)))
            continue
        best = max(pool, key=lambda c: c[3])
        cls, _, meth = best[1].partition("::")
        tier = "exact" if best[3] >= 99 else ("high" if best[3] >= 85 else "weak")
        how = tier + ("" if mine else "/other-class")
        rows.append((seq, code, cls, meth, best[0], how, "%x" % fn,
                     "%x" % registrar, file_at(registrar)))

with open(OUT, "w") as fh:
    fh.write("seq\tcode\tclass\tmethod\tfile\thow\tfunction\tregistrar\n")
    for r in rows:
        fh.write("\t".join(str(x) for x in r) + "\n")

tiers = collections.Counter(r[5] for r in rows)
print("%d registrations: %s" % (len(rows), dict(tiers)))
for r in rows:
    if r[5] == "unresolved":
        print("   %s  fn=%s registrar=%s" % (r[1], r[6], r[7]))
