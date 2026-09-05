#!/usr/bin/env python3
"""Extract the Dreamcast per-map precache manifests from the reference image."""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

import pefile


IMAGE = Path(__file__).resolve().parents[2] / "RE" / "HALFLIFE_DC.EXE"
TABLE_ADDRESS = 0x1CE8A8


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--c", action="store_true", help="emit C data tables")
    parser.add_argument("--output", type=Path, help="write output to this file")
    args = parser.parse_args()

    if args.output:
        sys.stdout = args.output.open("w", encoding="ascii", newline="\n")

    pe = pefile.PE(str(IMAGE), fast_load=True)
    image_base = pe.OPTIONAL_HEADER.ImageBase

    def read(address: int, size: int) -> bytes:
        return pe.get_data(address - image_base, size)

    def read_u32(address: int) -> int:
        return struct.unpack("<I", read(address, 4))[0]

    def read_string(address: int) -> str | None:
        data = bytearray()
        while True:
            byte = read(address + len(data), 1)
            if not byte or byte == b"\0":
                try:
                    return data.decode("ascii")
                except UnicodeDecodeError:
                    return None
            if byte[0] < 0x20 or byte[0] > 0x7E:
                return None
            data += byte

    def read_manifest(address: int) -> list[int]:
        entries: list[int] = []
        cursor = address
        while True:
            value = read_u32(cursor)
            cursor += 4
            if value == 0:
                return entries
            entries.append(value)

    maps: list[tuple[int, int]] = []
    table = TABLE_ADDRESS
    while True:
        map_name_address = read_u32(table)
        manifest_address = read_u32(table + 4)
        if map_name_address == 0:
            break

        maps.append((map_name_address, manifest_address))
        table += 8

    if args.c:
        manifests: dict[int, list[int]] = {}
        strings: dict[int, str] = {}

        def collect_manifest(address: int) -> None:
            if address in manifests:
                return

            entries = read_manifest(address)
            manifests[address] = entries

            previous = ""
            for value in entries:
                if previous == "-":
                    collect_manifest(value)
                    previous = ""
                    continue
                if value < image_base:
                    previous = ""
                    continue

                string = read_string(value)
                if string is None:
                    raise ValueError(f"invalid string pointer 0x{value:x}")
                strings[value] = string
                previous = string

        for map_name_address, manifest_address in maps:
            map_name = read_string(map_name_address)
            if map_name is None:
                raise ValueError(f"invalid map name pointer 0x{map_name_address:x}")
            strings[map_name_address] = map_name
            collect_manifest(manifest_address)

        emitted: set[int] = set()
        manifest_order: list[int] = []

        def order_manifest(address: int) -> None:
            if address in emitted:
                return
            emitted.add(address)

            previous = ""
            for value in manifests[address]:
                if previous == "-":
                    order_manifest(value)
                    previous = ""
                    continue
                if value >= image_base:
                    previous = strings[value]
                else:
                    previous = ""

            manifest_order.append(address)

        for _, manifest_address in maps:
            order_manifest(manifest_address)

        def identifier(value: str) -> str:
            value = re.sub(r"[^a-zA-Z0-9]+", "_", value).strip("_").lower()
            if not value or value[0].isdigit():
                value = "map_" + value
            return value

        def unique_name(base: str, used: set[str]) -> str:
            name = base
            suffix = 2
            while name in used:
                name = f"{base}_{suffix}"
                suffix += 1
            used.add(name)
            return name

        used_names: set[str] = set()
        manifest_names: dict[int, str] = {}
        for map_name_address, manifest_address in maps:
            if manifest_address not in manifest_names:
                map_name = strings[map_name_address]
                manifest_names[manifest_address] = unique_name(
                    f"dc_precache_{identifier(map_name)}", used_names
                )

        shared_number = 1
        for manifest_address in manifest_order:
            if manifest_address not in manifest_names:
                manifest_names[manifest_address] = unique_name(
                    f"dc_precache_shared_{shared_number}", used_names
                )
                shared_number += 1

        decal_names: dict[int, str] = {}
        for manifest_address in manifest_order:
            entries = manifests[manifest_address]
            for index, value in enumerate(entries[:-1]):
                if value >= image_base and strings[value] == ":":
                    decal_address = entries[index + 1]
                    if decal_address not in decal_names:
                        decal_names[decal_address] = unique_name(
                            f"dc_decal_{identifier(strings[decal_address])}", used_names
                        )

        print("/* Per-map cache manifests. Generated by utils/re/extract_dc_precache.py. */")
        print()
        print('static char dc_precache_anim_group[] = "+";')
        print('static char dc_precache_decal_sequence[] = ":";')
        print('static char dc_precache_shared_tail[] = "-";')
        print()
        print("#define DC_ANIM_GROUP(group) \\")
        print("\tdc_precache_anim_group, (char *)(group)")
        print("#define DC_DECAL_SEQUENCE(name) \\")
        print("\tdc_precache_decal_sequence, name")
        print("#define DC_SHARED_MANIFEST(manifest) \\")
        print("\tdc_precache_shared_tail, (char *)(manifest)")

        if decal_names:
            print()
            for address, name in sorted(decal_names.items(), key=lambda item: item[1]):
                print(f"static char {name}[] = {json.dumps(strings[address])};")

        for address in manifest_order:
            print()
            print(f"static char *{manifest_names[address]}[] =")
            print("{")
            entries = manifests[address]
            index = 0
            while index < len(entries):
                value = entries[index]
                if value < image_base:
                    print(f"\t(char *){value},")
                    index += 1
                    continue

                entry = strings[value]
                if entry == "+" and index + 1 < len(entries):
                    print(f"\tDC_ANIM_GROUP({entries[index + 1]}),")
                    index += 2
                elif entry == ":" and index + 1 < len(entries):
                    print(f"\tDC_DECAL_SEQUENCE({decal_names[entries[index + 1]]}),")
                    index += 2
                elif entry == "-" and index + 1 < len(entries):
                    print(
                        f"\tDC_SHARED_MANIFEST({manifest_names[entries[index + 1]]}),"
                    )
                    index += 2
                else:
                    print(f"\t{json.dumps(entry)},")
                    index += 1
            print("\tNULL")
            print("};")

        print()
        print("static dc_precache_map_t dc_precache_maps[] =")
        print("{")
        for map_name_address, manifest_address in maps:
            print(
                f"\t{{{json.dumps(strings[map_name_address])}, "
                f"{manifest_names[manifest_address]}}},"
            )
        print("\t{NULL, NULL}")
        print("};")
        print()
        print("#undef DC_SHARED_MANIFEST")
        print("#undef DC_DECAL_SEQUENCE")
        print("#undef DC_ANIM_GROUP")
        return

    map_count = 0
    for map_name_address, manifest_address in maps:

        map_name = read_string(map_name_address)
        entries: list[str] = []
        for value in read_manifest(manifest_address):
            if value < image_base:
                entries.append(str(value))
            else:
                string = read_string(value)
                if string is None:
                    entries.append(f"@0x{value:x}")
                else:
                    entries.append(repr(string))

        print(f"{map_name}: {', '.join(entries)}")
        map_count += 1

    print(f"maps: {map_count}")


if __name__ == "__main__":
    main()
