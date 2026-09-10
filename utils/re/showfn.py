#!/usr/bin/env python3
"""Print the instructions at an address from the cached exe disassembly."""
import sys, re
CACHE = "RE/HALFLIFE_DC.EXE.disasm.txt"
text = open(CACHE, encoding="latin-1").read().splitlines()
rows = []
for ln in text:
    m = re.match(r"\s*([0-9A-Fa-f]{8}):\s+(.*)$", ln)
    if m:
        rows.append((int(m.group(1), 16), m.group(2).rstrip()))
idx = {a: i for i, (a, _) in enumerate(rows)}
for arg in sys.argv[1:]:
    a = int(arg, 16)
    i = idx.get(a)
    print("==== %08x" % a)
    if i is None:
        print("   not in listing")
        continue
    for va, t in rows[i:i+int(__import__("os").environ.get("N", "26"))]:
        print("   %08x  %s" % (va, t))
