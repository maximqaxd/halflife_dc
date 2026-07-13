#!/usr/bin/env python3
"""Build bin_insns.tsv (objdiff's per-function pool-free listing) from the cached
dumpbin disassembly of the exe + symbols.tsv, WITHOUT the gated ExportInsns.java.

For each function in symbols.tsv we slice its [entry, entry+size) instruction
window out of RE/HALFLIFE_DC.EXE.disasm.txt and emit `entryHex<TAB>va<TAB>mnem ops`.

SH-4 emits literal pools both at the END of a function (after the last rts) and
MID-function (for large functions, since `mov.l @(disp,pc)` reaches only +/-1KB).
dumpbin disassembles those pool constants as garbage instructions. The obj side
(our compiled source) places its pools differently, so leaving binary pools in the
listing injects phantom instructions that wreck the objdiff alignment/score. We
strip both: the trailing pool (after the last rts+delay), and every mid-function
pool (the region after an unconditional bra/jmp/rts+delay slot, up to the next
instruction that is a branch target -- i.e. where real code resumes).
"""
import os, re, sys

RE_DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(RE_DIR))
DISASM = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE.disasm.txt")
SYMS = os.path.join(RE_DIR, "symbols.tsv")
OUT = os.path.join(RE_DIR, "bin_insns.tsv")

# dumpbin line:  "  0013789C: 2FE6 mov.l   r14,@-r15"  (addr, opcode-hex, mnem ops)
LINE = re.compile(r"^\s*([0-9A-Fa-f]{6,8}):\s+([0-9A-Fa-f]{2,8})\s+(\S.*?)\s*$")


def _sext(val, bits):
    sign = 1 << (bits - 1)
    return (val ^ sign) - sign


def branch_target(addr, op):
    """If op is a PC-relative branch (bra/bsr/bt/bf/bt.s/bf.s), return its target
    address, else None. Used to find where real code resumes after a pool."""
    if (op & 0xF000) in (0xA000, 0xB000):           # bra / bsr : 12-bit disp
        return addr + 4 + _sext(op & 0x0FFF, 12) * 2
    if (op & 0xF000) == 0x8000 and (op & 0x0F00) in (0x0900, 0x0B00, 0x0D00, 0x0F00):
        return addr + 4 + _sext(op & 0x00FF, 8) * 2  # bt / bf / bt.s / bf.s : 8-bit
    return None


def is_unconditional(op):
    """SH-4 unconditional control transfers that can be followed by a literal pool
    (each has a delay slot). bra, jmp, braf, rts, rte."""
    if (op & 0xF000) == 0xA000:            # bra
        return True
    if (op & 0xF0FF) == 0x402B:            # jmp @Rn
        return True
    if (op & 0xF0FF) == 0x0023:            # braf Rn
        return True
    if op == 0x000B or op == 0x002B:       # rts / rte
        return True
    return False


def load_syms(path):
    syms = []
    with open(path) as f:
        for ln in f:
            p = ln.rstrip("\n").split("\t")
            if len(p) != 3:
                continue
            try:
                start = int(p[1], 16); size = int(p[2])
            except ValueError:
                continue
            if size <= 0:
                continue
            syms.append((start, size, p[1].lower()))
    syms.sort()
    return syms


def main():
    syms = load_syms(SYMS)
    # parse disasm into ordered (addr, opcode, text)
    insns = []
    with open(DISASM, encoding="latin-1") as f:
        for ln in f:
            m = LINE.match(ln)
            if not m:
                continue
            try:
                op = int(m.group(2), 16)
            except ValueError:
                continue
            insns.append((int(m.group(1), 16), op, m.group(3)))
    insns.sort()
    import bisect
    addrs = [a for a, _, _ in insns]
    n = trailing = midpools = 0
    with open(OUT, "w", encoding="latin-1") as out:
        for start, size, entry in syms:
            lo = bisect.bisect_left(addrs, start)
            hi = start + size
            endi = lo
            while endi < len(insns) and insns[endi][0] < hi:
                endi += 1

            # Trim the TRAILING pool: cut after the LAST rts(+delay slot).
            last_rts = -1
            for k in range(lo, endi):
                if insns[k][1] == 0x000B:
                    last_rts = k
            if last_rts >= 0 and last_rts + 2 < endi:
                endi = last_rts + 2
                trailing += 1

            # Collect every in-window PC-relative branch target: these are the
            # addresses where real code resumes, so they bound each pool.
            targets = set()
            for k in range(lo, endi):
                t = branch_target(insns[k][0], insns[k][1])
                if t is not None:
                    targets.add(t)

            # Emit, skipping mid-function pools. After an unconditional transfer
            # and its delay slot, skip until the next instruction whose address is
            # a branch target (real code resumes there).
            i = lo
            skipping = False
            skipped = 0
            while i < endi:
                a, op, text = insns[i]
                if skipping:
                    if a in targets:
                        skipping = False
                    else:
                        i += 1
                        skipped += 1
                        continue
                out.write("%s\t%08x\t%s\n" % (entry, a, text))
                n += 1
                if is_unconditional(op):
                    # emit the delay slot, then treat the rest as pool
                    i += 1
                    if i < endi:
                        a2, _, text2 = insns[i]
                        out.write("%s\t%08x\t%s\n" % (entry, a2, text2))
                        n += 1
                        i += 1
                    skipping = True
                    continue
                i += 1
            if skipped:
                midpools += 1
    print("[build_bininsns] %d functions, %d instruction lines "
          "(%d trailing pools trimmed, %d fns with mid pools stripped) -> %s" %
          (len(syms), n, trailing, midpools, OUT))


if __name__ == "__main__":
    main()
