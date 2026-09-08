#!/usr/bin/env python3
"""Check that a converted Neo model still says what the original said.

Reads the original with the normal studio rules and the converted copy with the
engine's Neo rules, then compares what each one hands the renderer: the value
every bone axis holds on every frame, the attachment points, the texture pixels,
and the geometry each mesh draws.  Rotations are allowed to differ by the four
bits the Neo format drops and nothing else.

Usage:
  python utils/re/verify_neo.py <original dir> <converted dir>
  python utils/re/verify_neo.py --pak <original pak> <converted pak>
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from mdl2neo import (ANIM_SIZE, BODYPART_SIZE, G_DATA, G_NAME, H_ATTACHMENTINDEX,
                     H_BODYPARTINDEX, H_NUMATTACHMENTS, H_NUMBODYPARTS, H_NUMBONES,
                     H_NUMSEQ, H_NUMSEQGROUPS, H_NUMTEXTURES, H_SEQGROUPINDEX,
                     H_SEQINDEX, H_TEXTUREINDEX, MESH_SIZE, MODEL_SIZE,
                     NEO_ROTATION_SHIFT, S_ANIMINDEX, S_NUMBLENDS, S_NUMFRAMES,
                     S_SEQGROUP, SEQDESC_SIZE, SEQGROUP_SIZE, TEXTURE_SIZE,
                     ATTACHMENT_SIZE, ATTACHMENT_NEO_SIZE, pak_entries,
                     sequence_group_names)

HIGH = [0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF]
LOW = [0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF]


class BitReader:
    """StudioReadBits_Neo, transcribed."""

    def __init__(self, data: bytes, pos: int) -> None:
        self.d = data
        self.p = pos
        self.rem = 8

    def _read(self, count: int) -> tuple[int, int]:
        shift = 0
        result = 0
        while count >= self.rem:
            bits = self.rem
            v = (self.d[self.p] & HIGH[bits]) >> (8 - bits)
            self.p += 1
            result |= v << shift
            shift += bits
            count -= bits
            self.rem = 8
        if count:
            bits = self.rem
            v = ((self.d[self.p] & HIGH[bits]) >> (8 - bits)) & LOW[count]
            result |= v << shift
            shift += count
            self.rem = bits - count
            if self.rem == 0:
                self.rem = 8
                self.p += 1
        return result, shift

    def unsigned(self, count: int) -> int:
        return self._read(count)[0]

    def signed(self, count: int) -> int:
        result, shift = self._read(count)
        if result:
            sign = 1 << (shift - 1)
            result |= -2 * (sign & result)
        return result


def neo_axis(data: bytes, offset: int, numframes: int, rotation: bool) -> list[int]:
    """What the Neo reader ends up with for one bone axis, frame by frame."""
    runs = []
    pos = offset
    frames = 0
    while frames < numframes:
        valid, total = data[pos], data[pos + 1]
        runs.append((valid, total))
        pos += 2
        frames += total

    bits = BitReader(data, pos)
    out: list[int] = []
    for valid, total in runs:
        width = bits.unsigned(4)
        values = []
        if rotation:
            v = (bits.unsigned(12) << 4) & 0xFFFF
            values.append(v - 0x10000 if v >= 0x8000 else v)
            for _ in range(1, valid):
                values.append(values[-1] + (bits.signed(width) << 4))
        else:
            v = bits.unsigned(16)
            values.append(v - 0x10000 if v >= 0x8000 else v)
            for _ in range(1, valid):
                values.append(values[-1] + bits.signed(width))
        for k in range(total):
            out.append(values[k] if k < valid else values[-1])
    return out


def stock_axis(data: bytes, offset: int, numframes: int) -> list[int]:
    """The same, from a normal model."""
    out: list[int] = []
    pos = offset
    frames = 0
    while frames < numframes:
        valid, total = data[pos], data[pos + 1]
        values = list(struct.unpack_from("<%dh" % valid, data, pos + 2))
        for k in range(total):
            out.append(values[k] if k < valid else values[-1])
        pos += 2 + valid * 2
        frames += total
    return out


def group0_base(model: bytes) -> int:
    if not struct.unpack_from("<i", model, H_NUMSEQGROUPS)[0]:
        return 0
    return struct.unpack_from(
        "<i", model, struct.unpack_from("<i", model, H_SEQGROUPINDEX)[0] + G_DATA)[0]


def geometry(model: bytes) -> list:
    """Everything a mesh actually draws, pulled out through the offsets."""
    out = []
    nbp = struct.unpack_from("<i", model, H_NUMBODYPARTS)[0]
    bpi = struct.unpack_from("<i", model, H_BODYPARTINDEX)[0]
    for b in range(nbp):
        bp = bpi + b * BODYPART_SIZE
        nm = struct.unpack_from("<i", model, bp + 64)[0]
        mi = struct.unpack_from("<i", model, bp + 72)[0]
        for m in range(nm):
            md = mi + m * MODEL_SIZE
            numverts = struct.unpack_from("<i", model, md + 80)[0]
            numnorms = struct.unpack_from("<i", model, md + 92)[0]
            vertindex = struct.unpack_from("<i", model, md + 88)[0]
            normindex = struct.unpack_from("<i", model, md + 100)[0]
            vertinfo = struct.unpack_from("<i", model, md + 84)[0]
            norminfo = struct.unpack_from("<i", model, md + 96)[0]
            out.append(model[vertindex:vertindex + numverts * 12])
            out.append(model[normindex:normindex + numnorms * 12])
            out.append(model[vertinfo:vertinfo + numverts])
            out.append(model[norminfo:norminfo + numnorms])
            nmesh = struct.unpack_from("<i", model, md + 72)[0]
            meshindex = struct.unpack_from("<i", model, md + 76)[0]
            for e in range(nmesh):
                mesh = meshindex + e * MESH_SIZE
                tri = struct.unpack_from("<i", model, mesh + 4)[0]
                pos = tri
                while True:
                    n = struct.unpack_from("<h", model, pos)[0]
                    pos += 2
                    if n == 0:
                        break
                    pos += abs(n) * 8
                out.append(model[tri:pos])
    return out


def textures(model: bytes) -> list:
    out = []
    n = struct.unpack_from("<i", model, H_NUMTEXTURES)[0]
    index = struct.unpack_from("<i", model, H_TEXTUREINDEX)[0]
    for t in range(n):
        base = index + t * TEXTURE_SIZE
        w, h, idx = struct.unpack_from("<iii", model, base + 68)
        if model[idx:idx + 4] == b"GBIX":
            size = 32 + struct.unpack_from("<i", model, idx + 0x14)[0] - 8
        else:
            size = w * h + 768
        out.append(model[idx:idx + size])
    return out


def attachments(model: bytes, neo: bool) -> list:
    n = struct.unpack_from("<i", model, H_NUMATTACHMENTS)[0]
    index = struct.unpack_from("<i", model, H_ATTACHMENTINDEX)[0]
    stride = ATTACHMENT_NEO_SIZE if neo else ATTACHMENT_SIZE
    skip = 0 if neo else 36
    out = []
    for i in range(n):
        base = index + i * stride + skip
        out.append(struct.unpack_from("<i16s", model, base))
    return out


HEADER_OFFSETS = (144, 152, 160, 168, 176, 184, 188, 200, 208, 216, 240)


def misalignments(model: bytes) -> list[str]:
    """Everything the engine reads as a long has to sit on a long boundary.

    The SH-4 faults on an unaligned load, and the studio loader checks the
    texture offsets itself before it gets that far.
    """
    problems = []

    def check(label: str, offset: int) -> None:
        if offset & 3:
            problems.append("%s sits at %d, off a long boundary" % (label, offset))

    for at in HEADER_OFFSETS:
        check("header offset at %d" % at, struct.unpack_from("<i", model, at)[0])

    n = struct.unpack_from("<i", model, H_NUMTEXTURES)[0]
    index = struct.unpack_from("<i", model, H_TEXTUREINDEX)[0]
    for t in range(n):
        check("texture %d" % t,
              struct.unpack_from("<i", model, index + t * TEXTURE_SIZE + 76)[0])

    nbp = struct.unpack_from("<i", model, H_NUMBODYPARTS)[0]
    bpi = struct.unpack_from("<i", model, H_BODYPARTINDEX)[0]
    for b in range(nbp):
        bp = bpi + b * BODYPART_SIZE
        nm = struct.unpack_from("<i", model, bp + 64)[0]
        mi = struct.unpack_from("<i", model, bp + 72)[0]
        check("bodypart %d models" % b, mi)
        for m in range(nm):
            md = mi + m * MODEL_SIZE
            for at, label in ((76, "meshes"), (84, "vertex bones"), (88, "vertices"),
                              (96, "normal bones"), (100, "normals")):
                check("model %d %s" % (m, label),
                      struct.unpack_from("<i", model, md + at)[0])
            nmesh = struct.unpack_from("<i", model, md + 72)[0]
            meshindex = struct.unpack_from("<i", model, md + 76)[0]
            for e in range(nmesh):
                check("mesh %d triangles" % e,
                      struct.unpack_from("<i", model, meshindex + e * MESH_SIZE + 4)[0])

    numseq = struct.unpack_from("<i", model, H_NUMSEQ)[0]
    seqindex = struct.unpack_from("<i", model, H_SEQINDEX)[0]
    for i in range(numseq):
        seq = seqindex + i * SEQDESC_SIZE
        check("sequence %d animation" % i,
              struct.unpack_from("<i", model, seq + S_ANIMINDEX)[0])
        if struct.unpack_from("<i", model, seq + 48)[0]:
            check("sequence %d events" % i,
                  struct.unpack_from("<i", model, seq + 52)[0])
        if struct.unpack_from("<i", model, seq + 60)[0]:
            check("sequence %d pivots" % i,
                  struct.unpack_from("<i", model, seq + 64)[0])

    return problems[:6]


def compare(name: str, orig: bytes, ogroups: dict, new: bytes, ngroups: dict) -> list[str]:
    problems = []

    if new[:4] != b"idst":
        problems.append("not marked as a Neo model")
        return problems

    for field, label in ((H_NUMBONES, "bones"), (H_NUMSEQ, "sequences"),
                         (H_NUMBODYPARTS, "bodyparts"), (H_NUMTEXTURES, "textures"),
                         (H_NUMATTACHMENTS, "attachments")):
        a = struct.unpack_from("<i", orig, field)[0]
        b = struct.unpack_from("<i", new, field)[0]
        if a != b:
            problems.append("%s count changed %d -> %d" % (label, a, b))

    if struct.unpack_from("<i", new, 72)[0] != len(new):
        problems.append("header length does not match the file")

    problems += misalignments(new)

    if geometry(orig) != geometry(new):
        problems.append("geometry differs")
    if textures(orig) != textures(new):
        problems.append("texture data differs")
    if attachments(orig, False) != attachments(new, True):
        problems.append("attachments differ")

    numbones = struct.unpack_from("<i", orig, H_NUMBONES)[0]
    numseq = struct.unpack_from("<i", orig, H_NUMSEQ)[0]
    oseq = struct.unpack_from("<i", orig, H_SEQINDEX)[0]
    nseq = struct.unpack_from("<i", new, H_SEQINDEX)[0]
    obase0, nbase0 = group0_base(orig), group0_base(new)

    for i in range(numseq):
        o = oseq + i * SEQDESC_SIZE
        n = nseq + i * SEQDESC_SIZE
        numframes = struct.unpack_from("<i", orig, o + S_NUMFRAMES)[0]
        numblends = struct.unpack_from("<i", orig, o + S_NUMBLENDS)[0]
        seqgroup = struct.unpack_from("<i", orig, o + S_SEQGROUP)[0]
        if numframes <= 0 or numblends <= 0:
            continue

        oanim = struct.unpack_from("<i", orig, o + S_ANIMINDEX)[0]
        nanim = struct.unpack_from("<i", new, n + S_ANIMINDEX)[0]
        if seqgroup == 0:
            odata, ndata = orig, new
            ostart, nstart = obase0 + oanim, nbase0 + nanim
        else:
            odata, ndata = ogroups[seqgroup], ngroups[seqgroup]
            ostart, nstart = oanim, nanim

        for slot in range(numbones * numblends):
            oa = ostart + slot * ANIM_SIZE
            na = nstart + slot * ANIM_SIZE
            ooff = struct.unpack_from("<6H", odata, oa)
            noff = struct.unpack_from("<6H", ndata, na)
            for k in range(6):
                if (ooff[k] == 0) != (noff[k] == 0):
                    problems.append("sequence %d bone %d axis %d appeared or vanished"
                                    % (i, slot % numbones, k))
                    continue
                if ooff[k] == 0:
                    continue
                want = stock_axis(odata, oa + ooff[k], numframes)
                got = neo_axis(ndata, na + noff[k], numframes, k >= 3)
                if k >= 3:
                    want = [(v >> NEO_ROTATION_SHIFT) << NEO_ROTATION_SHIFT for v in want]
                if want != got:
                    where = next((f for f, (x, y) in enumerate(zip(want, got)) if x != y), -1)
                    problems.append(
                        "sequence %d bone %d axis %d differs at frame %d (%s vs %s)"
                        % (i, slot % numbones, k, where,
                           want[where] if where >= 0 else "?",
                           got[where] if where >= 0 else "?"))
                    return problems
    return problems


def load_groups(model: bytes, folder: Path) -> dict:
    out = {}
    for i, gname in enumerate(sequence_group_names(model)):
        if i:
            out[i] = (folder / Path(gname.replace("\\", "/")).name).read_bytes()
    return out


def verify_dirs(original: Path, converted: Path) -> int:
    bad = 0
    checked = 0
    for path in sorted(original.glob("*.mdl")):
        orig = path.read_bytes()
        if orig[:4] != b"IDST":
            continue
        new = (converted / path.name).read_bytes()
        problems = compare(path.name, orig, load_groups(orig, original),
                           new, load_groups(orig, converted))
        checked += 1
        if problems:
            bad += 1
            print("%s:" % path.name)
            for p in problems[:4]:
                print("   %s" % p)
    print("\n%d models checked, %d with problems" % (checked, bad))
    return bad


def verify_paks(original: Path, converted: Path) -> int:
    od = original.read_bytes()
    nd = converted.read_bytes()
    oc = {n: od[fp:fp + sz] for n, fp, sz in pak_entries(od)}
    nc = {n: nd[fp:fp + sz] for n, fp, sz in pak_entries(nd)}

    if set(oc) != set(nc):
        print("the two paks do not hold the same files")
        return 1

    bad = checked = unchanged = repacked = 0
    for name, blob in oc.items():
        if blob[:4] != b"IDST":
            if blob == nc[name]:
                unchanged += 1
            elif blob[:4] == b"IDSQ":
                # a sequence group carries packed animation now; the model that
                # owns it reads it back below
                repacked += 1
            else:
                print("%s: changed but is neither a model nor a sequence group" % name)
                bad += 1
            continue
        groups = {}
        ngroups = {}
        for i, gname in enumerate(sequence_group_names(blob)):
            if not i:
                continue
            key = next((n for n in oc if n.replace("\\", "/").lower()
                        == gname.replace("\\", "/").lower()), None)
            groups[i] = oc[key]
            ngroups[i] = nc[key]
        problems = compare(name, blob, groups, nc[name], ngroups)
        checked += 1
        if problems:
            bad += 1
            print("%s:" % name)
            for p in problems[:4]:
                print("   %s" % p)
    print("\n%d models checked, %d with problems" % (checked, bad))
    print("%d sequence groups repacked, %d other files untouched" % (repacked, unchanged))
    return bad


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original")
    parser.add_argument("converted")
    parser.add_argument("--pak", action="store_true")
    args = parser.parse_args()

    if args.pak:
        return 1 if verify_paks(Path(args.original), Path(args.converted)) else 0
    return 1 if verify_dirs(Path(args.original), Path(args.converted)) else 0


if __name__ == "__main__":
    raise SystemExit(main())
