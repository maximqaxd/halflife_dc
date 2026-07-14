import re
PATH = re.compile(r"halflifedc/+src/+([A-Za-z0-9_]+)/+([A-Za-z0-9_.]+\.(?:c|C|cpp|h))", re.I)
hits = {}
for ln in open('utils/re/portdata/HALFLIFE_DC.EXE_fp.tsv', encoding='utf-8'):
    p = ln.rstrip('\n').split('\t')
    if len(p) < 4:
        continue
    try:
        addr = int(p[0], 16)
    except ValueError:
        continue
    norm = p[3].replace('\\', '/')
    for m in PATH.finditer(norm):
        key = m.group(1) + '/' + m.group(2)
        if 0x165000 <= addr < 0x177000:
            hits.setdefault(key, []).append((addr, p[1]))
print("=== assert-path source files referenced by fns in 0x165000-0x177000 ===")
for k in sorted(hits):
    fns = sorted(hits[k])
    print("\n%s  (%d fns)" % (k, len(fns)))
    for a, nm in fns:
        print("   %06x %s" % (a, nm))
