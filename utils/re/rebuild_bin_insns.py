#!/usr/bin/env python
"""Rebuild bin_insns.tsv blocks for an address range from the cached exe
disassembly (RE/HALFLIFE_DC.EXE.disasm.txt), replacing whatever blocks the
range currently has.

Why: ExportInsns.java blocks are sometimes TRUNCATED -- Ghidra's function body
can stop at a mid-function literal pool (code resumes after the pool via a
bra/bt over it), so objdiff scores against a partial binary side. The exe
disasm is flat and complete; objdiff's trim_pool() drops the pool words that
dumpbin decodes as code (execution can only resume at a branch target).

Function extents = symbols.tsv start .. next symbol start (gap-to-next),
which is immune to Ghidra body truncation.

Usage: python utils/re/rebuild_bin_insns.py 0x16626c 0x16a1c0 [more lo hi ...]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from objdiff import parse_disasm, trim_pool  # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
DISASM = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE.disasm.txt")
SYMBOLS = os.path.join(HERE, "symbols.tsv")
BININSNS = os.path.join(HERE, "bin_insns.tsv")


def main():
    args = [int(a, 16) for a in sys.argv[1:]]
    if not args or len(args) % 2:
        print(__doc__)
        return 1
    ranges = list(zip(args[0::2], args[1::2]))

    # all symbol starts (sorted) for gap-to-next extents; sizes for the
    # insn-count cap (Ghidra body size is pool-free, SH-4 insns are 2 bytes,
    # so a function has exactly size/2 instructions)
    starts = set()
    sizes = {}
    for line in open(SYMBOLS, encoding="latin-1"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2:
            try:
                a = int(p[1], 16)
            except ValueError:
                continue
            starts.add(a)
            if len(p) >= 3:
                try:
                    sizes[a] = max(sizes.get(a, 0), int(p[2]))
                except ValueError:
                    pass
    order = sorted(starts)

    exe = parse_disasm(open(DISASM, encoding="latin-1").read())
    by_va = {va: i for i, (va, m, o) in enumerate(exe)}

    entries = [a for a in order if any(lo <= a < hi for lo, hi in ranges)]
    blocks = {}
    for a in entries:
        nxt = next((s for s in order if s > a), a + 4)
        i = by_va.get(a)
        if i is None:
            print("WARN: no disasm line at %x" % a)
            continue
        j = i
        while j < len(exe) and exe[j][0] < nxt:
            j += 1
        insns = trim_pool(exe[i:j])
        # cap at the Ghidra body's insn count: the gap to the next SYMBOL can
        # overshoot into unnamed neighbor functions
        n = sizes.get(a, 0) // 2
        if n and len(insns) > n:
            insns = insns[:n]
        # sanity: a complete function ends in a transfer + delay slot
        if len(insns) >= 2 and insns[-2][1].lower() not in ("rts", "jmp", "bra", "rte"):
            print("WARN: %x does not end on a return (last: %s %s) -- "
                  "Ghidra body may be truncated; verify with disassemble_function"
                  % (a, insns[-1][1], insns[-1][2]))
        blocks["%x" % a] = insns

    keep = []
    for line in open(BININSNS, encoding="latin-1"):
        key = line.split("\t", 1)[0]
        try:
            if any(lo <= int(key, 16) < hi for lo, hi in ranges):
                continue
        except ValueError:
            pass
        keep.append(line.rstrip("\n"))
    for key in sorted(blocks, key=lambda k: int(k, 16)):
        for va, m, o in blocks[key]:
            keep.append("%s\t%08x\t%s %s" % (key, va, m, o) if o
                        else "%s\t%08x\t%s" % (key, va, m))
    with open(BININSNS, "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(keep) + "\n")
    print("rebuilt %d blocks in %s" % (len(blocks), BININSNS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
