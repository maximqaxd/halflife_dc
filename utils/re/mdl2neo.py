#!/usr/bin/env python3
"""Convert Half-Life studio models to the Dreamcast's Neo format.

A Neo model is marked with a lower case "idst" and differs from a normal studio
model in two places: attachments drop the name and type nothing reads back, and
animation values are written as a bit stream instead of a word each.  Both
change size, so the file is rebuilt and every offset in it moved to match.

Sequences in a demand-loaded group live in a separate file, so a model is always
converted together with the group files its sequence groups name.

Usage:
  python utils/re/mdl2neo.py <model.mdl> [more.mdl ...]     convert in place
  python utils/re/mdl2neo.py --check <model.mdl>            report, change nothing
  python utils/re/mdl2neo.py --pak <pak0.pak>               convert every model in a pak
"""

from __future__ import annotations

import argparse
import shutil
import struct
from pathlib import Path

IDSTUDIOHEADER = b"IDST"
IDSTUDIONEOHEADER = b"idst"

SEQDESC_SIZE = 176
SEQGROUP_SIZE = 104
BODYPART_SIZE = 76
MODEL_SIZE = 112
MESH_SIZE = 20
TEXTURE_SIZE = 80
ANIM_SIZE = 12
ATTACHMENT_SIZE = 88
ATTACHMENT_NEO_SIZE = 52

# studiohdr_t offsets that hold a position in the file
HEADER_OFFSETS = (144, 152, 160, 168, 176, 184, 188, 200, 208, 216, 240)
H_LENGTH = 72
H_NUMBONES = 140
H_NUMHITBOXES = 156
H_NUMSEQ = 164
H_SEQINDEX = 168
H_NUMSEQGROUPS = 172
H_SEQGROUPINDEX = 176
H_NUMTEXTURES = 180
H_TEXTUREINDEX = 184
H_NUMBODYPARTS = 204
H_BODYPARTINDEX = 208
H_NUMATTACHMENTS = 212
H_ATTACHMENTINDEX = 216

# mstudioseqdesc_t
S_NUMEVENTS = 48
S_EVENTINDEX = 52
S_NUMFRAMES = 56
S_NUMPIVOTS = 60
S_PIVOTINDEX = 64
S_NUMBLENDS = 120
S_ANIMINDEX = 124
S_SEQGROUP = 156

# mstudioseqgroup_t
G_NAME = 32
G_DATA = 100

# studioseqhdr_t
Q_LENGTH = 72

NEO_MAX_DELTA_BITS = 15
NEO_ROTATION_SHIFT = 4


class ConversionError(Exception):
    pass


# ---------------------------------------------------------------- bit stream


class BitWriter:
    """Writes bits from bit 0 of each byte upwards, least significant first."""

    def __init__(self) -> None:
        self.out = bytearray()
        self.used = 0

    def write(self, value: int, count: int) -> None:
        while count > 0:
            if self.used == 0:
                self.out.append(0)
            take = min(count, 8 - self.used)
            self.out[-1] |= (value & ((1 << take) - 1)) << self.used
            value >>= take
            count -= take
            self.used = (self.used + take) & 7

    def bytes(self) -> bytes:
        return bytes(self.out)


def delta_bits(delta: int) -> int:
    """How many bits a difference needs, signed."""
    for bits in range(NEO_MAX_DELTA_BITS + 1):
        limit = 1 << bits
        if -(limit // 2) <= delta < limit // 2:
            return bits
    return NEO_MAX_DELTA_BITS + 1


# ------------------------------------------------------------- relocation


class Relocator:
    """Splices replacement blocks into a file and moves every offset to suit."""

    def __init__(self, data: bytes) -> None:
        self.data = bytearray(data)
        self.edits: list[tuple[int, int, bytes]] = []

    def replace(self, start: int, oldlen: int, new: bytes) -> None:
        self.edits.append((start, oldlen, bytes(new)))

    def finish(self) -> tuple[bytearray, "Relocator"]:
        self.edits.sort()
        last = -1
        for start, oldlen, _ in self.edits:
            if start < last:
                raise ConversionError("overlapping edits")
            last = start + oldlen

        out = bytearray()
        pos = 0
        self.shifts: list[tuple[int, int]] = []   # (end of old block, delta so far)
        delta = 0
        for start, oldlen, new in self.edits:
            out += self.data[pos:start]
            out += new
            pos = start + oldlen
            delta += len(new) - oldlen
            self.shifts.append((pos, delta))
        out += self.data[pos:]
        self.result = out
        return out, self

    def map(self, offset: int) -> int:
        """Where an offset ends up.  Offsets inside a replaced block are a bug."""
        if offset == 0:
            return 0
        delta = 0
        for start, oldlen, new in self.edits:
            if offset >= start + oldlen:
                delta += len(new) - oldlen
            elif offset > start:
                raise ConversionError("offset %d lands inside a rewritten block" % offset)
        return offset + delta


# ------------------------------------------------------------------ packing


def read_runs(data: bytes, offset: int, numframes: int) -> tuple[list, int]:
    """Pulls one bone axis out of a normal model.  Returns the runs and length."""
    runs = []
    pos = offset
    frames = 0
    while frames < numframes:
        if pos + 2 > len(data):
            raise ConversionError("animation runs off the end of the file")
        valid = data[pos]
        total = data[pos + 1]
        if total == 0:
            raise ConversionError("animation run covers no frames")
        values = list(struct.unpack_from("<%dh" % valid, data, pos + 2))
        runs.append((valid, total, values))
        pos += 2 + valid * 2
        frames += total
    return runs, pos - offset


def pack_axis(runs: list, rotation: bool) -> bytes:
    """Rewrites one bone axis into the packed form a Neo model carries."""
    packed = []
    for valid, total, values in runs:
        if rotation:
            values = [v >> NEO_ROTATION_SHIFT for v in values]

        # a run whose values move too far apart to pack is split so each half
        # can carry its own first value
        used: list[int] = []
        remaining = total
        for v in values:
            if used and delta_bits(v - used[-1]) > NEO_MAX_DELTA_BITS:
                packed.append((len(used), len(used), used))
                remaining -= len(used)
                used = []
            used.append(v)
        packed.append((len(used), remaining, used))

    out = bytearray()
    for valid, total, values in packed:
        if valid > 255 or total > 255:
            raise ConversionError("animation run is longer than a byte can hold")
        out.append(valid)
        out.append(total)

    bits = BitWriter()
    for valid, total, values in packed:
        width = 0
        for i in range(1, len(values)):
            width = max(width, delta_bits(values[i] - values[i - 1]))

        bits.write(width, 4)
        if rotation:
            bits.write(values[0] & 0xFFF, 12)
        else:
            bits.write(values[0] & 0xFFFF, 16)
        for i in range(1, len(values)):
            bits.write(values[i] - values[i - 1], width)

    return bytes(out) + bits.bytes()


def pack_anim_block(reloc: Relocator, base: int, numbones: int, numblends: int,
                    numframes: int) -> tuple[int, int]:
    """Repacks one sequence's animation.  Returns (was, now) value bytes."""
    data = reloc.data
    count = numbones * numblends
    table = base + count * ANIM_SIZE

    axes = []
    end = table
    for i in range(count):
        anim = base + i * ANIM_SIZE
        for k, offset in enumerate(struct.unpack_from("<6H", data, anim)):
            if offset == 0:
                axes.append((i, k, None))
                continue
            runs, length = read_runs(data, anim + offset, numframes)
            axes.append((i, k, (runs, k >= 3)))
            end = max(end, anim + offset + length)

    # every block in a studio model starts on a long boundary, so the block
    # reaches to the padding after its last value and has to keep doing so
    end = min((end + 3) & ~3, len(data))

    out = bytearray()
    for n, (i, k, axis) in enumerate(axes):
        anim = base + i * ANIM_SIZE
        if axis is None:
            offset = 0
        else:
            runs, rotation = axis
            offset = table + len(out) - anim
            out += pack_axis(runs, rotation)
        if offset > 0xFFFF:
            raise ConversionError("animation offset no longer fits in a short")
        struct.pack_into("<H", data, anim + k * 2, offset)

    packed = len(out)
    while len(out) & 3:
        out.append(0)

    reloc.replace(table, end - table, out)
    return end - table, packed


def pack_attachments(reloc: Relocator) -> int:
    data = reloc.data
    count = struct.unpack_from("<i", data, H_NUMATTACHMENTS)[0]
    index = struct.unpack_from("<i", data, H_ATTACHMENTINDEX)[0]
    if count <= 0 or index <= 0:
        return 0

    out = bytearray()
    for i in range(count):
        src = index + i * ATTACHMENT_SIZE
        out += data[src + 36:src + 40]                       # bone
        out += data[src + 40:src + ATTACHMENT_SIZE]          # org and vectors
    reloc.replace(index, count * ATTACHMENT_SIZE, out)
    return count


# ------------------------------------------------------------------- models


def sequence_group_names(header: bytes) -> list[str]:
    count = struct.unpack_from("<i", header, H_NUMSEQGROUPS)[0]
    index = struct.unpack_from("<i", header, H_SEQGROUPINDEX)[0]
    names = []
    for i in range(count):
        base = index + i * SEQGROUP_SIZE
        names.append(header[base + G_NAME:base + G_NAME + 64].split(b"\0")[0]
                     .decode("ascii", "replace"))
    return names


def remap_model(reloc: Relocator, groups: dict[int, Relocator]) -> None:
    """Moves every offset in the rebuilt model to where its block now sits."""
    data = reloc.result

    def fix(at: int, mapper=None) -> None:
        old = struct.unpack_from("<i", data, at)[0]
        struct.pack_into("<i", data, at, (mapper or reloc).map(old))

    numseq = struct.unpack_from("<i", reloc.data, H_NUMSEQ)[0]
    seqindex = struct.unpack_from("<i", reloc.data, H_SEQINDEX)[0]
    numgroups = struct.unpack_from("<i", reloc.data, H_NUMSEQGROUPS)[0]
    groupindex = struct.unpack_from("<i", reloc.data, H_SEQGROUPINDEX)[0]
    numbodyparts = struct.unpack_from("<i", reloc.data, H_NUMBODYPARTS)[0]
    bodypartindex = struct.unpack_from("<i", reloc.data, H_BODYPARTINDEX)[0]
    numtextures = struct.unpack_from("<i", reloc.data, H_NUMTEXTURES)[0]
    textureindex = struct.unpack_from("<i", reloc.data, H_TEXTUREINDEX)[0]

    # bodyparts, models and meshes, walked before their indices move
    for b in range(numbodyparts):
        bp = reloc.map(bodypartindex) + b * BODYPART_SIZE
        nummodels = struct.unpack_from("<i", data, bp + 64)[0]
        modelindex = struct.unpack_from("<i", data, bp + 72)[0]
        for m in range(nummodels):
            md = reloc.map(modelindex) + m * MODEL_SIZE
            nummesh = struct.unpack_from("<i", data, md + 72)[0]
            meshindex = struct.unpack_from("<i", data, md + 76)[0]
            for e in range(nummesh):
                mesh = reloc.map(meshindex) + e * MESH_SIZE
                fix(mesh + 4)                                # triindex
            for at in (76, 84, 88, 96, 100):                 # mesh, vert, norm
                fix(md + at)
        fix(bp + 72)

    for t in range(numtextures):
        fix(reloc.map(textureindex) + t * TEXTURE_SIZE + 76)

    for i in range(numseq):
        seq = reloc.map(seqindex) + i * SEQDESC_SIZE
        seqgroup = struct.unpack_from("<i", data, seq + S_SEQGROUP)[0]
        if struct.unpack_from("<i", data, seq + S_NUMEVENTS)[0]:
            fix(seq + S_EVENTINDEX)
        if struct.unpack_from("<i", data, seq + S_NUMPIVOTS)[0]:
            fix(seq + S_PIVOTINDEX)
        fix(seq + S_ANIMINDEX, groups.get(seqgroup, reloc) if seqgroup else reloc)

    for g in range(numgroups):
        base = reloc.map(groupindex) + g * SEQGROUP_SIZE
        if struct.unpack_from("<i", data, base + G_DATA)[0]:
            fix(base + G_DATA)

    for at in HEADER_OFFSETS:
        fix(at)
    struct.pack_into("<i", data, H_LENGTH, len(data))


def convert_model(model: bytes, groups: dict[int, bytes], name: str
                  ) -> tuple[bytes, dict[int, bytes], dict]:
    if model[:4] == IDSTUDIONEOHEADER:
        return model, groups, {"skipped": "already neo"}
    if model[:4] != IDSTUDIOHEADER:
        raise ConversionError("not a studio model")

    reloc = Relocator(model)
    grelocs = {i: Relocator(g) for i, g in groups.items()}

    numbones = struct.unpack_from("<i", model, H_NUMBONES)[0]
    numseq = struct.unpack_from("<i", model, H_NUMSEQ)[0]
    seqindex = struct.unpack_from("<i", model, H_SEQINDEX)[0]
    group0 = 0
    if struct.unpack_from("<i", model, H_NUMSEQGROUPS)[0]:
        group0 = struct.unpack_from(
            "<i", model, struct.unpack_from("<i", model, H_SEQGROUPINDEX)[0] + G_DATA)[0]

    was = now = 0
    for i in range(numseq):
        seq = seqindex + i * SEQDESC_SIZE
        numframes = struct.unpack_from("<i", model, seq + S_NUMFRAMES)[0]
        numblends = struct.unpack_from("<i", model, seq + S_NUMBLENDS)[0]
        animindex = struct.unpack_from("<i", model, seq + S_ANIMINDEX)[0]
        seqgroup = struct.unpack_from("<i", model, seq + S_SEQGROUP)[0]
        if numframes <= 0 or numblends <= 0:
            continue

        if seqgroup == 0:
            target, base = reloc, group0 + animindex
        else:
            if seqgroup not in grelocs:
                raise ConversionError("sequence group %d is missing" % seqgroup)
            target, base = grelocs[seqgroup], animindex

        a, b = pack_anim_block(target, base, numbones, numblends, numframes)
        was += a
        now += b

    attachments = pack_attachments(reloc)
    reloc.data[0:4] = IDSTUDIONEOHEADER

    reloc.finish()
    for g in grelocs.values():
        g.finish()
    remap_model(reloc, grelocs)
    for g in grelocs.values():
        struct.pack_into("<i", g.result, Q_LENGTH, len(g.result))

    return (bytes(reloc.result),
            {i: bytes(g.result) for i, g in grelocs.items()},
            {"sequences": numseq, "attachments": attachments,
             "anim_was": was, "anim_now": now,
             "size_was": len(model), "size_now": len(reloc.result)})


# ---------------------------------------------------------------------- pak


def pak_entries(data: bytes):
    if data[:4] != b"PACK":
        raise ConversionError("not a pak file")
    ofs, length = struct.unpack_from("<ii", data, 4)
    for i in range(length // 64):
        base = ofs + i * 64
        name = data[base:base + 56].split(b"\0")[0].decode("ascii", "replace")
        filepos, size = struct.unpack_from("<ii", data, base + 56)
        yield name, filepos, size


def wanted(name: str, blob: bytes, skip_groups: bool, only: list) -> bool:
    if only and not any(o.replace("\\", "/").lower() in name.replace("\\", "/").lower()
                        for o in only):
        return False
    if skip_groups and struct.unpack_from("<i", blob, H_NUMSEQGROUPS)[0] > 1:
        return False
    return True


def convert_pak(path: Path, check: bool, skip_groups: bool = False, only: list = ()) -> int:
    data = path.read_bytes()
    entries = list(pak_entries(data))
    by_name = {name.replace("\\", "/").lower(): (fp, sz) for name, fp, sz in entries}

    # a sequence group can be shared by more than one model, so reads always
    # come from the file as it was and only writes land in the new content
    original = {name: data[fp:fp + sz] for name, fp, sz in entries}
    content = dict(original)
    written: dict[str, bytes] = {}

    models = [n for n, fp, sz in entries
              if data[fp:fp + 4] == IDSTUDIOHEADER
              and wanted(n, data[fp:fp + sz], skip_groups, list(only))]
    print("%d studio models to convert" % len(models))

    converted = failed = 0
    was = now = 0
    for name in models:
        blob = content[name]
        groups: dict[int, bytes] = {}
        keys: dict[int, str] = {}
        missing = None
        for i, gname in enumerate(sequence_group_names(blob)):
            if not i:
                continue
            key = gname.replace("\\", "/").lower()
            if key not in by_name:
                key = "models/" + Path(key).name
            if key not in by_name:
                missing = gname
                break
            real = next(n for n in original if n.replace("\\", "/").lower() == key)
            groups[i] = original[real]
            keys[i] = real
        if missing:
            print("  %-42s missing sequence group %s" % (name, missing))
            failed += 1
            continue

        try:
            newmodel, newgroups, info = convert_model(blob, groups, name)
        except ConversionError as exc:
            print("  %-42s %s" % (name, exc))
            failed += 1
            continue
        if "skipped" in info:
            continue

        clash = next((keys[i] for i, blob2 in newgroups.items()
                      if keys[i] in written and written[keys[i]] != blob2), None)
        if clash:
            print("  %-42s wants %s packed differently to another model"
                  % (name, clash))
            failed += 1
            continue

        content[name] = newmodel
        for i, blob2 in newgroups.items():
            content[keys[i]] = blob2
            written[keys[i]] = blob2
        was += info["anim_was"]
        now += info["anim_now"]
        converted += 1

    print("converted %d, failed %d" % (converted, failed))
    if was:
        print("animation %d -> %d bytes (%.0f%% of the original)"
              % (was, now, 100.0 * now / was))
    if check or not converted:
        return failed

    # rebuild the pak around whatever changed size
    out = bytearray(b"PACK" + b"\0" * 8)
    directory = bytearray()
    for name, _, _ in entries:
        blob = content[name]
        while len(out) & 3:
            out.append(0)
        directory += name.encode("ascii").ljust(56, b"\0")
        directory += struct.pack("<ii", len(out), len(blob))
        out += blob
    struct.pack_into("<ii", out, 4, len(out), len(directory))
    out += directory

    spare = path.with_suffix(path.suffix + ".orig")
    if not spare.exists():
        shutil.copy2(path, spare)
        print("original saved as %s" % spare.name)
    path.write_bytes(bytes(out))
    print("%s: %d -> %d bytes" % (path.name, len(data), len(out)))
    return failed


# --------------------------------------------------------------------- main


def convert_file(path: Path, check: bool) -> int:
    model = path.read_bytes()
    if model[:4] != IDSTUDIOHEADER:
        return 0

    groups: dict[int, bytes] = {}
    paths: dict[int, Path] = {}
    for i, gname in enumerate(sequence_group_names(model)):
        if not i:
            continue
        gpath = path.parent / Path(gname.replace("\\", "/")).name
        if not gpath.exists():
            print("%s: sequence group %s not found" % (path.name, gname))
            return 1
        groups[i] = gpath.read_bytes()
        paths[i] = gpath

    try:
        newmodel, newgroups, info = convert_model(model, groups, path.name)
    except ConversionError as exc:
        print("%s: %s" % (path.name, exc))
        return 1
    if "skipped" in info:
        return 0

    print("%-28s %3d sequences, %d attachments, animation %7d -> %7d, file %8d -> %8d"
          % (path.name, info["sequences"], info["attachments"], info["anim_was"],
             info["anim_now"], info["size_was"], info["size_now"]))

    if not check:
        path.write_bytes(newmodel)
        for i, gpath in paths.items():
            gpath.write_bytes(newgroups[i])
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("targets", nargs="+")
    parser.add_argument("--check", action="store_true",
                        help="report what would change and write nothing")
    parser.add_argument("--pak", action="store_true",
                        help="the targets are pak files")
    parser.add_argument("--skip-groups", action="store_true",
                        help="leave models that demand-load sequence groups alone")
    parser.add_argument("--only", action="append", default=[],
                        help="convert only models whose name contains this")
    args = parser.parse_args()

    failed = 0
    for target in args.targets:
        path = Path(target)
        if args.pak:
            failed += convert_pak(path, args.check, args.skip_groups, args.only)
        elif path.is_dir():
            for model in sorted(path.glob("*.mdl")):
                failed += convert_file(model, args.check)
        else:
            failed += convert_file(path, args.check)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
