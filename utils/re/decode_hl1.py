#!/usr/bin/env python
"""Decode a Dreamcast .HL1 level-state save and report what the token table holds.

Usage:
    python utils/re/decode_hl1.py <file.HL1> [more.HL1 ...]
    python utils/re/decode_hl1.py --dir "<path to SAVE/DICTS>"

Handles both a plain .HL1 and one wrapped by Zip_CompressFile as
[int uncompressedSize][zlib deflate stream].

Layout (confirmed against UnzipSaveFile @0x175edc):
    int   tag        'VALV'
    int   version    0x71
    int   size       ETABLE + entity data, in bytes
    int   tableCount entity table entries
    int   tokenCount hash buckets in the symbol table
    int   tokenSize  bytes of NUL-separated token text
    char  tokens[tokenSize]
    ...   ETABLE block, then the entity data, `size` bytes plus long padding

Record shapes (Zap_ParseKeyvalueSection @0x1761c4):
    record: short size; short slot; int keyCount      (8 bytes)
    key:    short size; short slot; byte data[size]   (4 byte head)
"""
import struct
import sys
import os
import zlib

SAVEFILE_HEADER = 0x564C4156  # 'VALV'

# Every field name zapsave.c's Zap_MapKeyFieldSlot binds to a token slot.
KNOWN_KEYS = [
    "id", "location", "size", "flags", "classname",
    "skillLevel", "entityCount", "connectionCount", "lightStyleCount", "time",
    "mapName", "skyName", "skyColor_r", "skyColor_g", "skyColor_b",
    "skyVec_x", "skyVec_y", "skyVec_z",
    "adjacentMapName", "landmarkName", "pentLandmark", "vecLandmarkOrigin",
    "index", "style",
]
# Zap_MapKeyFieldSlot binds adjacentMapName from the same "mapName" token as
# mapName -- the two ifs are not chained, so one token sets both globals.
ALIASED = {"adjacentMapName": "mapName"}


def load(path):
    raw = open(path, "rb").read()
    if len(raw) < 4:
        raise ValueError("too short")
    if struct.unpack_from("<I", raw, 0)[0] == SAVEFILE_HEADER:
        return raw, False
    declared = struct.unpack_from("<i", raw, 0)[0]
    out = zlib.decompress(raw[4:])
    if len(out) != declared:
        print("    ! length header says %d, inflate produced %d" % (declared, len(out)))
    return out, True


def tokens_of(buf, off, token_size, token_count):
    blob = buf[off:off + token_size]
    names, start = [], 0
    for _ in range(token_count):
        end = blob.find(b"\0", start)
        if end < 0:
            break
        names.append(blob[start:end].decode("latin-1"))
        start = end + 1
    return names


def walk_etable(buf, off, count, names):
    """Walk the ETABLE records; return (end offset, {token slot: hits})."""
    used = {}
    for _ in range(count):
        if off + 8 > len(buf):
            break
        _sz, _slot, keys = struct.unpack_from("<hhi", buf, off)
        off += 8
        for _ in range(keys):
            if off + 4 > len(buf):
                return off, used
            ksize, kslot = struct.unpack_from("<hh", buf, off)
            off += 4 + ksize
            used[kslot] = used.get(kslot, 0) + 1
    return off, used


def report(path):
    print("=" * 78)
    print(path)
    try:
        buf, packed = load(path)
    except Exception as exc:
        print("    ! cannot read: %s" % exc)
        return
    if len(buf) < 24:
        print("    ! truncated")
        return

    tag, ver, size, tables, tcount, tsize = struct.unpack_from("<6i", buf, 0)
    print("    %-9s tag=%08x ver=%d size=%d table=%d tokens=%d tokenSize=%d"
          % ("packed" if packed else "plain", tag, ver, size, tables, tcount, tsize))
    if tag != SAVEFILE_HEADER:
        print("    ! not a VALV save")
        return

    expect = 24 + tsize + size
    print("    file=%d bytes, header+tokens+size=%d (pad %+d)"
          % (len(buf), expect, len(buf) - expect))

    names = tokens_of(buf, 24, tsize, tcount)
    nonempty = [(i, n) for i, n in enumerate(names) if n]
    print("    %d token slots, %d named" % (len(names), len(nonempty)))

    where = {}
    for i, n in nonempty:
        where.setdefault(n, []).append(i)

    missing = []
    for key in KNOWN_KEYS:
        probe = ALIASED.get(key, key)
        slots = where.get(probe, [])
        ok = bool(slots)
        if key in ALIASED:
            note = "shares the %s slot %s" % (probe, slots[0] if slots else "-")
        else:
            note = "slot %s" % (slots[0] if slots else "-")
        if not ok:
            missing.append(key)
        print("      %-18s %-4s %s" % (key, "ok" if ok else "MISS", note))

    if missing:
        print("    ** absent from this save: %s" % ", ".join(missing))

    end, used = walk_etable(buf, 24 + tsize, tables, names)
    print("    ETABLE ends at +%d (%d bytes)" % (end, end - (24 + tsize)))
    unknown = sorted(s for s in used
                     if 0 <= s < len(names) and names[s] and names[s] not in where)
    hits = sorted(used.items(), key=lambda kv: -kv[1])[:16]
    print("    key tokens used: " +
          ", ".join("%s x%d" % (names[s] if 0 <= s < len(names) and names[s]
                                else "slot%d" % s, c) for s, c in hits))
    if unknown:
        print("    ** unnamed key slots: %s" % unknown)

    unhandled = set()
    handled = set(ALIASED.get(k, k) for k in KNOWN_KEYS)
    for s in used:
        if 0 <= s < len(names) and names[s] and names[s] not in handled:
            unhandled.add(names[s])
    if unhandled:
        print("    ** ETABLE keys our parser ignores: %s"
              % ", ".join(sorted(unhandled)))


def main(argv):
    paths = []
    if len(argv) >= 3 and argv[1] == "--dir":
        for f in sorted(os.listdir(argv[2])):
            if f.lower().endswith(".hl1"):
                paths.append(os.path.join(argv[2], f))
    else:
        paths = argv[1:]
    if not paths:
        print(__doc__)
        return 1
    for p in paths:
        report(p)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
