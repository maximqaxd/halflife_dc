"""Print the message layout CL_ParseTEnt expects for every TE_ opcode.

A temp entity message carries no length, so a handler that reads the wrong
number of bytes desynchronises the whole stream and the next parse dies with
"CL_ParseServerMessage: illegible server message". This recovers the layout
straight from the image: CL_ParseTEnt dispatches through a 128-entry jump table
of 16-bit braf displacements, so each opcode's body can be sliced out and its
MSG_Read* calls listed in order.

    python utils/re/te_opcodes.py            # every opcode
    python utils/re/te_opcodes.py --check    # compare against src/engine/CL_TENT.C

Opcodes whose table entry is the default body are unhandled and not listed.
"""

import os, re, sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LISTING = os.path.join(REPO, "RE", "HALFLIFE_DC.EXE.disasm.txt")
SYMBOLS = os.path.join(REPO, "utils", "re", "symbols.tsv")
CONST_H = os.path.join(REPO, "src", "engine", "const.h")
PARSER = os.path.join(REPO, "src", "engine", "CL_TENT.C")

# CL_ParseTEnt runs to the next symbol, not to the length symbols.tsv records.
FN_LO, FN_HI = 0x31750, 0x3366C
JUMP_TABLE = 0x317C4          # mova operand of the dispatch
BRAF_BASE = 0x3178A           # braf's pc+4
NUM_OPCODES = 0x80

LINE = re.compile(r"^  ([0-9A-F]{8}): ([0-9A-F]{4})")


def read_image():
    mem = {}
    for ln in open(LISTING, encoding="latin1"):
        m = LINE.match(ln)
        if not m:
            continue
        a = int(m.group(1), 16)
        if FN_LO - 0x100 <= a <= FN_HI + 0x400:
            mem[a] = int(m.group(2), 16)
    return mem


def symbols():
    out = {}
    for ln in open(SYMBOLS, encoding="latin1"):
        f = ln.rstrip("\n").split("\t")
        if len(f) >= 2:
            try:
                out[int(f[1], 16)] = f[0]
            except ValueError:
                pass
    return out


def opcode_names():
    out = {}
    for ln in open(CONST_H, encoding="latin1"):
        m = re.match(r"#define\s+(TE_[A-Z0-9_]+)\s+(\d+)", ln)
        if m and "EXPLFLAG" not in m.group(1) and not m.group(1).startswith("TE_BOUNCE"):
            out.setdefault(int(m.group(2)), m.group(1))
    return out


def binary_layouts():
    mem = read_image()
    syms = symbols()

    def rd32(a):
        return mem.get(a, 0) | (mem.get(a + 2, 0) << 16)

    targets = {op: BRAF_BASE + mem.get(JUMP_TABLE + op * 2, 0) for op in range(NUM_OPCODES)}
    bodies = sorted(set(targets.values()))
    # the body every unhandled opcode shares is the default (Sys_Error)
    counts = {}
    for t in targets.values():
        counts[t] = counts.get(t, 0) + 1
    default = max(counts, key=lambda t: counts[t])

    regs, reads = {}, {}
    for a in sorted(mem):
        w = mem[a]
        if (w & 0xF000) == 0xD000:                       # mov.l @(disp,pc),rN
            regs[(w >> 8) & 0xF] = rd32((a & ~3) + 4 + (w & 0xFF) * 4)
        elif (w & 0xF0FF) == 0x400B:                     # jsr @rN
            name = syms.get(regs.get((w >> 8) & 0xF), "")
            if name.startswith("MSG_Read"):
                reads[a] = name[len("MSG_Read"):]

    out = {}
    for op, t in targets.items():
        if t == default:
            continue
        i = bodies.index(t)
        end = bodies[i + 1] if i + 1 < len(bodies) else FN_HI
        out[op] = [reads[a] for a in sorted(reads) if t <= a < end]
    return out


def source_layouts():
    """Per-case MSG_Read* order as written in CL_ParseTEnt."""
    src = open(PARSER, encoding="latin1").read().splitlines()
    start = next(i for i, l in enumerate(src) if l.startswith("void CL_ParseTEnt"))
    out, labels, reads, depth, opened, body = {}, [], [], 0, False, False
    case_depth = None
    for l in src[start:]:
        depth += l.count("{") - l.count("}")
        if "{" in l:
            opened = True
        m = re.match(r"\s*case (TE_\w+):", l)
        if m and case_depth is None:
            case_depth = depth
        # a label deeper than the dispatch switch belongs to a nested switch
        if m and depth != case_depth:
            m = None
        if m:
            # consecutive labels share one body; a label after code starts a new one
            if body:
                for n in labels:
                    out[n] = reads
                labels, reads, body = [], [], False
            labels.append(m.group(1))
        elif labels:
            reads = reads + re.findall(r"MSG_Read(\w+)\s*\(", l)
            if l.strip() not in ("", "{", "}") and not l.strip().startswith("//"):
                body = True
        if opened and depth == 0:
            break
    for n in labels:
        out[n] = reads
    return out


def main():
    names = opcode_names()
    binary = binary_layouts()

    if "--check" not in sys.argv:
        for op in sorted(binary):
            print("%-3d %-26s %s" % (op, names.get(op, "?"), " ".join(binary[op])))
        return 0

    src = source_layouts()
    bad = 0
    for op in sorted(binary):
        n = names.get(op)
        if n not in src:
            print("%-26s not handled in CL_ParseTEnt" % n)
            bad += 1
            continue
        if src[n] != binary[op]:
            print("%-26s\n   image:  %s\n   source: %s" % (n, " ".join(binary[op]), " ".join(src[n])))
            bad += 1
    print("%d opcode(s) differ" % bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
