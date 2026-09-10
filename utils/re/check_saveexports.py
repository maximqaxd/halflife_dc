#!/usr/bin/env python3
"""Check the generated registry against the one the reference binary builds.

Reads the dispatcher's call order and each registrar's registrations out of the
source, and compares the resulting code sequence with bin_registry.tsv.  A save
written by one build only loads in the other while these agree.
"""
import glob
import re
import sys

want = [l.split()[2] for l in open("utils/re/saverestore/bin_registry.tsv")
        if not l.startswith(("seq", "#"))]

bodies = {}
for path in glob.glob("src/halflife/*.cpp"):
    txt = open(path, encoding="latin-1").read()
    for m in re.finditer(r"void\s+(SR_Register_\w+)\s*\(\s*void\s*\)\s*\{(.*?)\n\}", txt, re.S):
        bodies[m.group(1)] = re.findall(r'SR_REGISTER\(\s*"(\w\w)"\s*,\s*(\w+)\s*,\s*(\w+)\s*\)',
                                        m.group(2))

order = re.findall(r"\t(SR_Register_\w+)\(\);",
                   open("src/halflife/saveexports.cpp", encoding="latin-1").read())

got, named = [], []
for name in order:
    for code, cls, fn in bodies.get(name, []):
        got.append(code)
        named.append("%s::%s" % (cls, fn))

if got == want:
    print("registry matches the binary: %d registrations in order" % len(got))
    sys.exit(0)

print("MISMATCH: %d registrations, binary has %d" % (len(got), len(want)))
for i in range(max(len(got), len(want))):
    a = got[i] if i < len(got) else "-"
    b = want[i] if i < len(want) else "-"
    if a != b:
        print("  first difference at %d: source %s (%s), binary %s"
              % (i, a, named[i] if i < len(named) else "-", b))
        break
sys.exit(1)
