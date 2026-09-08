#!/usr/bin/env python3
"""Replace a file inside a Dreamcast GDI disc image.

The image is a set of raw 2352 byte tracks, so a file lives in the data area of
a run of sectors and each sector carries a checksum and two parity blocks that
have to be put back in step afterwards.

The replacement has to fit the room the original had, and is padded out with
zeroes to the same length so the disc's own directory never changes.  A pak
does not mind the padding: it finds its contents through its own header.

Usage:
  python utils/re/gdi_inject.py <disc.gdi> <path/in/image> <replacement>
  python utils/re/gdi_inject.py <disc.gdi> <path/in/image> --extract <output>
  python utils/re/gdi_inject.py <disc.gdi> --list
"""

from __future__ import annotations

import argparse
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from cdsector import RAW_SECTOR_SIZE, USER_DATA_SIZE, USER_DATA_START, rebuild

PVD_OFFSET = 16


class Image:
    """A GDI and the data tracks it names, addressed by absolute sector."""

    def __init__(self, gdi: Path) -> None:
        self.root = gdi.parent
        self.tracks = []
        lines = gdi.read_text().splitlines()
        for line in lines[1:]:
            parts = line.split()
            if len(parts) < 5:
                continue
            index, start, kind, size, name = (int(parts[0]), int(parts[1]),
                                              int(parts[2]), int(parts[3]), parts[4])
            if size != RAW_SECTOR_SIZE:
                raise SystemExit("track %d is not a raw 2352 byte track" % index)
            path = self.root / name
            self.tracks.append({"index": index, "start": start, "data": kind == 4,
                                "path": path,
                                "sectors": path.stat().st_size // RAW_SECTOR_SIZE})
        self.tracks.sort(key=lambda t: t["start"])
        self.first_data = next(t for t in self.tracks if t["data"] and t["start"] >= 45000)

    def locate(self, lba: int):
        for track in self.tracks:
            if track["data"] and track["start"] <= lba < track["start"] + track["sectors"]:
                return track, lba - track["start"]
        raise KeyError("sector %d is not in any data track" % lba)

    def read(self, lba: int, count: int = 1) -> bytes:
        out = bytearray()
        done = 0
        while done < count:
            track, offset = self.locate(lba + done)
            run = min(count - done, track["sectors"] - offset)
            with track["path"].open("rb") as f:
                f.seek(offset * RAW_SECTOR_SIZE)
                # read in batches rather than a sector at a time
                left = run
                while left:
                    batch = min(left, 2048)
                    raw = f.read(batch * RAW_SECTOR_SIZE)
                    for i in range(batch):
                        base = i * RAW_SECTOR_SIZE + USER_DATA_START
                        out += raw[base:base + USER_DATA_SIZE]
                    left -= batch
            done += run
        return bytes(out)

    def walk(self, lba: int, length: int, prefix: str = ""):
        data = self.read(lba, (length + USER_DATA_SIZE - 1) // USER_DATA_SIZE)
        pos = 0
        while pos < length:
            record = data[pos]
            if record == 0:
                pos = (pos // USER_DATA_SIZE + 1) * USER_DATA_SIZE
                continue
            extent = struct.unpack_from("<I", data, pos + 2)[0]
            size = struct.unpack_from("<I", data, pos + 10)[0]
            flags = data[pos + 25]
            name = data[pos + 33:pos + 33 + data[pos + 32]].decode("ascii", "replace")
            pos += record
            if name in ("\x00", "\x01"):
                continue
            full = prefix + name.split(";")[0]
            if flags & 2:
                yield from self.walk(extent, size, full + "/")
            else:
                yield full, extent, size

    def contents(self):
        pvd = self.read(self.first_data["start"] + PVD_OFFSET)
        if pvd[1:6] != b"CD001":
            raise SystemExit("no ISO9660 volume descriptor where one was expected")
        root = pvd[156:190]
        return list(self.walk(struct.unpack_from("<I", root, 2)[0],
                              struct.unpack_from("<I", root, 10)[0]))

    def find(self, wanted: str):
        wanted = wanted.replace("\\", "/").upper().lstrip("/")
        for name, extent, size in self.contents():
            if name.upper() == wanted:
                return name, extent, size
        raise SystemExit("%s is not in the image" % wanted)


def extract(image: Image, path: str, out: Path) -> int:
    name, extent, size = image.find(path)
    data = image.read(extent, (size + USER_DATA_SIZE - 1) // USER_DATA_SIZE)
    out.write_bytes(data[:size])
    print("%s: %d bytes -> %s" % (name, size, out))
    return 0


def inject(image: Image, path: str, source: Path, backup: bool = True) -> int:
    name, extent, size = image.find(path)
    payload = source.read_bytes()

    if len(payload) > size:
        raise SystemExit("%s is %d bytes; the image only has room for %d"
                         % (source.name, len(payload), size))

    count = (size + USER_DATA_SIZE - 1) // USER_DATA_SIZE
    track, first = image.locate(extent)
    if first + count > track["sectors"]:
        raise SystemExit("the file runs past the end of %s" % track["path"].name)

    print("%s: %d bytes at sector %d of %s" % (name, size, first, track["path"].name))
    if len(payload) < size:
        print("padding %s out by %d bytes to keep the image's directory unchanged"
              % (source.name, size - len(payload)))
        payload = payload + b"\0" * (size - len(payload))

    if backup:
        spare = track["path"].with_suffix(track["path"].suffix + ".orig")
        if not spare.exists():
            print("saving the original track as %s" % spare.name)
            shutil.copy2(track["path"], spare)

    written = 0
    with track["path"].open("r+b") as f:
        for i in range(count):
            base = (first + i) * RAW_SECTOR_SIZE
            f.seek(base)
            sector = bytearray(f.read(RAW_SECTOR_SIZE))
            chunk = payload[i * USER_DATA_SIZE:(i + 1) * USER_DATA_SIZE]
            chunk = chunk + b"\0" * (USER_DATA_SIZE - len(chunk))
            if sector[USER_DATA_START:USER_DATA_START + USER_DATA_SIZE] == chunk:
                continue
            sector[USER_DATA_START:USER_DATA_START + USER_DATA_SIZE] = chunk
            rebuild(sector)
            f.seek(base)
            f.write(sector)
            written += 1
            if written % 20000 == 0:
                print("   %d sectors rewritten" % written)

    print("%d of %d sectors rewritten" % (written, count))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gdi")
    parser.add_argument("path", nargs="?")
    parser.add_argument("replacement", nargs="?")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--extract")
    args = parser.parse_args()

    image = Image(Path(args.gdi))

    if args.list:
        for name, extent, size in image.contents():
            print("%-40s lba %-8d %d bytes" % (name, extent, size))
        return 0

    if args.extract:
        return extract(image, args.path, Path(args.extract))

    if not args.replacement:
        parser.error("give a replacement file, --extract or --list")
    return inject(image, args.path, Path(args.replacement))


if __name__ == "__main__":
    raise SystemExit(main())
