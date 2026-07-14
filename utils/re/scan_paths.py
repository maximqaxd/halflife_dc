import re
PATH = re.compile(r"halflifedc/+src/+([A-Za-z0-9_]+)/+([A-Za-z0-9_]+\.(?:cpp|hpp|c|h))\b", re.I)
hits = {}
for ln in open('utils/re/portdata/HALFLIFE_DC.EXE_fp.tsv', encoding='utf-8'):
    p = ln.rstrip('\n').split('\t')
    if len(p) < 4:
        continue
    try:
        a = int(p[0], 16)
    except ValueError:
        continue
    norm = p[3].replace('\\', '/')
    for m in PATH.finditer(norm):
        k = (m.group(1) + '/' + m.group(2)).lower()
        d = hits.setdefault(k, [1 << 32, 0, 0])
        d[0] = min(d[0], a); d[1] = max(d[1], a); d[2] += 1
print("=== authentic source files by assert path (link order) ===")
for k, (lo, hi, c) in sorted(hits.items(), key=lambda kv: kv[1][0]):
    print("%-34s %06x-%06x  refs=%d" % (k, lo, hi, c))
print("total distinct assert-path files:", len(hits))
