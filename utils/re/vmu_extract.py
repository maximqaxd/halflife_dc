#!/usr/bin/env python3
"""Pull the game's save files out of a VMU image and unwrap them.

Usage: python utils/re/vmu_extract.py <vmu_save_A1.bin> [outdir]

Each file on the card is a VMS file: a 640-byte header (description, icon
palette and one icon bitmap) followed by the payload the game wrote.  The
payload is a four-character tag, the compressed and uncompressed lengths, and a
zlib stream:

    HLS3   a save slot        JSAV bundle
    HLC2   the config         plain text
    BSS3   a save slot        JSAV bundle

The JSAV bundle is the five-int header from SaveGameSlot -- tag, version, size,
tokenCount, tokenSize -- then the token blob, the slot data, and one entry per
embedded file: char name[MAX_PATH], int size, contents.  The embedded .hl4 is a
level state wrapped by Zip_CompressFile as [int uncompressedSize][zlib stream].

Everything is written out at each stage, so the .HL1 that falls out the end can
be handed to decode_hl1.py.
"""
import os
import struct
import sys
import zlib

BLOCK = 512
MAX_PATH = 260
EYECATCH = {0: 0, 1: 8064, 2: 4544, 3: 2048}


def read_card(path):
    """the directory blocks (253 down to 241) and the FAT (254)"""
    data = open(path, "rb").read()
    blk = lambda n: data[n * BLOCK:(n + 1) * BLOCK]
    fat = blk(254)
    directory = b"".join(blk(b) for b in range(253, 241, -1))
    return data, blk, fat, directory


def chain(fat, start):
    out, n = [], start
    while len(out) < 256:
        out.append(n)
        n = struct.unpack_from("<H", fat, n * 2)[0]
        if n >= 0xfffa:
            break
    return out


def files(path):
    data, blk, fat, directory = read_card(path)
    for i in range(0, len(directory), 32):
        e = directory[i:i + 32]
        if e[0] not in (0xcc, 0x33):
            continue
        first = struct.unpack_from("<H", e, 2)[0]
        name = e[4:16].split(b"\0")[0].decode("latin-1").strip()
        yield name, b"".join(blk(b) for b in chain(fat, first))


def payload(vms):
    """strip the VMS header"""
    icons = struct.unpack_from("<H", vms, 0x40)[0]
    eye = struct.unpack_from("<H", vms, 0x44)[0]
    size = struct.unpack_from("<I", vms, 0x48)[0]
    off = 0x80 + 512 * icons + EYECATCH.get(eye, 0)
    return vms[off:off + size]


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    out = argv[2] if len(argv) > 2 else "vmu"
    for name, vms in files(argv[1]):
        base = os.path.join(out, name.replace(" ", "_").replace("/", "_"))
        os.makedirs(os.path.dirname(base) or ".", exist_ok=True)
        body = payload(vms)
        tag = body[:4]
        print("%-14s %6d bytes  tag=%s" % (name, len(body), tag.decode("latin-1", "replace")))
        if tag not in (b"HLS3", b"BSS3") or len(body) < 12:
            open(base, "wb").write(body)      # config, or another game's file
            continue
        try:
            slot = zlib.decompress(body[12:])
        except zlib.error as e:
            open(base, "wb").write(body)
            print("    will not inflate (%s)" % e)
            continue
        open(base + ".sav", "wb").write(slot)
        if slot[:4] != b"JSAV":
            continue
        _, version, size, tokenCount, tokenSize = struct.unpack_from("<5i", slot, 0)
        print("    JSAV version=%d size=%d tokenCount=%d tokenSize=%d"
              % (version, size, tokenCount, tokenSize))
        off = 20 + tokenSize + size
        while off + MAX_PATH + 4 <= len(slot):
            inner = slot[off:off + MAX_PATH].split(b"\0")[0].decode("latin-1")
            off += MAX_PATH
            n = struct.unpack_from("<i", slot, off)[0]
            off += 4
            blob = slot[off:off + n]
            off += n
            os.makedirs(base + ".files", exist_ok=True)
            open(os.path.join(base + ".files", inner), "wb").write(blob)
            note = ""
            if inner.lower().endswith(".hl4") and len(blob) > 4:
                try:
                    plain = zlib.decompress(blob[4:])
                    open(os.path.join(base + ".files", inner[:-4] + ".HL1"), "wb").write(plain)
                    note = " -> %d bytes of level state" % len(plain)
                except zlib.error:
                    pass
            print("    %-16s %6d%s" % (inner, n, note))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
