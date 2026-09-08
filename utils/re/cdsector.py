#!/usr/bin/env python3
"""Mode 1 CD sector error detection and correction.

A raw sector is 2352 bytes: twelve bytes of sync, a four byte address and mode,
2048 bytes of data, a four byte checksum, eight zeroes, and then the two parity
blocks the drive uses to repair a scratched sector.  Rewriting the data means
recomputing all three, or the disc no longer verifies.

Run this file directly to check the implementation against an image whose
sectors are known good:

  python utils/re/cdsector.py <track.bin> [sectors to test]
"""

from __future__ import annotations

import sys
from pathlib import Path

RAW_SECTOR_SIZE = 2352
USER_DATA_SIZE = 2048
USER_DATA_START = 16
EDC_START = 2064
P_PARITY_START = 2076
Q_PARITY_START = 2248

# the checksum runs over the address, the data and the checksum's own zeroes
EDC_LENGTH = 2064
# the P parity covers the address, the data, the checksum and its zeroes; the Q
# parity then runs over all of that plus the P parity that was just written
ECC_START = 12
P_SOURCE_LENGTH = 86 * 24
Q_SOURCE_LENGTH = 52 * 43


def _build_tables():
    edc = []
    for i in range(256):
        value = i
        for _ in range(8):
            value = (value >> 1) ^ (0xD8018001 if value & 1 else 0)
        edc.append(value & 0xFFFFFFFF)

    forward = [0] * 256
    back = [0] * 256
    for i in range(256):
        j = ((i << 1) ^ (0x11D if i & 0x80 else 0)) & 0xFF
        forward[i] = j
        back[i ^ j] = i
    return edc, forward, back


EDC_TABLE, ECC_F, ECC_B = _build_tables()


def _parity_indices(major_count: int, minor_count: int, major_mult: int, minor_inc: int):
    """The fixed gather pattern each parity byte reads, worked out once."""
    size = major_count * minor_count
    table = []
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        row = []
        for _ in range(minor_count):
            row.append(index)
            index += minor_inc
            if index >= size:
                index -= size
        table.append(row)
    return table


P_INDICES = _parity_indices(86, 24, 2, 86)
Q_INDICES = _parity_indices(52, 43, 86, 88)


def compute_edc(sector: bytes | bytearray) -> int:
    edc = 0
    for b in sector[0:EDC_LENGTH]:
        edc = (edc >> 8) ^ EDC_TABLE[(edc ^ b) & 0xFF]
    return edc & 0xFFFFFFFF


def _parity(source: memoryview, indices, out: bytearray, offset: int) -> None:
    count = len(indices)
    forward = ECC_F
    back = ECC_B
    for major, row in enumerate(indices):
        a = 0
        b = 0
        for index in row:
            temp = source[index]
            a ^= temp
            b ^= temp
            a = forward[a]
        a = back[forward[a] ^ b]
        out[offset + major] = a
        out[offset + major + count] = a ^ b


def rebuild(sector: bytearray) -> bytearray:
    """Puts the checksum and both parity blocks back in step with the data."""
    edc = compute_edc(sector)
    sector[EDC_START:EDC_START + 4] = edc.to_bytes(4, "little")
    sector[EDC_START + 4:EDC_START + 12] = b"\0" * 8

    source = memoryview(bytes(sector[ECC_START:ECC_START + P_SOURCE_LENGTH]))
    _parity(source, P_INDICES, sector, P_PARITY_START)
    source = memoryview(bytes(sector[ECC_START:ECC_START + Q_SOURCE_LENGTH]))
    _parity(source, Q_INDICES, sector, Q_PARITY_START)
    return sector


def verify(sector: bytes) -> tuple[bool, bool]:
    """Returns whether the checksum and the parity match what is stored."""
    stored_edc = int.from_bytes(sector[EDC_START:EDC_START + 4], "little")
    edc_ok = stored_edc == compute_edc(sector)

    rebuilt = bytearray(sector)
    source = memoryview(bytes(rebuilt[ECC_START:ECC_START + P_SOURCE_LENGTH]))
    _parity(source, P_INDICES, rebuilt, P_PARITY_START)
    source = memoryview(bytes(rebuilt[ECC_START:ECC_START + Q_SOURCE_LENGTH]))
    _parity(source, Q_INDICES, rebuilt, Q_PARITY_START)
    ecc_ok = bytes(rebuilt[P_PARITY_START:]) == bytes(sector[P_PARITY_START:])
    return edc_ok, ecc_ok


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    path = Path(sys.argv[1])
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    total = path.stat().st_size // RAW_SECTOR_SIZE
    step = max(1, total // count)

    good = bad = skipped = 0
    with path.open("rb") as f:
        for n in range(0, total, step):
            f.seek(n * RAW_SECTOR_SIZE)
            sector = f.read(RAW_SECTOR_SIZE)
            if sector[15] != 1:
                skipped += 1
                continue
            edc_ok, ecc_ok = verify(sector)
            if edc_ok and ecc_ok:
                good += 1
            else:
                bad += 1
                print("sector %d: checksum %s, parity %s"
                      % (n, "ok" if edc_ok else "WRONG", "ok" if ecc_ok else "WRONG"))

    print("%s: %d sectors reproduce exactly, %d do not, %d not mode 1"
          % (path.name, good, bad, skipped))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
