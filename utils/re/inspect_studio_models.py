#!/usr/bin/env python3
"""Identify Half-Life Dreamcast studio-model files.

The retail executable dispatches the signatures as follows:

  IDST  Mod_LoadStudioModel      (standard studio model)
  idst  Mod_LoadStudioNeoModel   (Dreamcast Neo studio model)

IDSQ is a studio sequence-group sidecar.  It is not a Neo model and is not
sent to either renderer as a standalone model.

Usage:
  python utils/re/inspect_studio_models.py path/to/model.mdl
  python utils/re/inspect_studio_models.py path/to/models
  python utils/re/inspect_studio_models.py --json path/to/models
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable


STUDIO_VERSION = 10
STUDIO_HEADER_SIZE = 0xF4
SEQUENCE_HEADER_SIZE = 0x4C

MODEL_MAGICS = {
    b"IDST": ("standard", "r_studio.c", "Mod_LoadStudioModel"),
    b"idst": ("neo", "r_studio_neo.c", "Mod_LoadStudioNeoModel"),
}


def _cstring(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")


def _i32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]


def _range_is_valid(offset: int, count: int, stride: int, limit: int) -> bool:
    return offset >= 0 and count >= 0 and offset + count * stride <= limit


def _inspect_textures(data: bytes, header: dict[str, Any]) -> dict[str, int]:
    count = header["numtextures"]
    offset = header["textureindex"]
    result: Counter[str] = Counter()

    if count == 0:
        return {}
    if not _range_is_valid(offset, count, 80, len(data)):
        return {"invalid_directory": count}

    for index in range(count):
        texture = offset + index * 80
        payload = _i32(data, texture + 76)
        if payload == 0:
            result["external"] += 1
        elif payload < 0 or payload + 4 > len(data):
            result["invalid"] += 1
        elif data[payload:payload + 4] == b"GBIX":
            result["pvr"] += 1
        else:
            result["indexed8"] += 1
    return dict(result)


def _parse_model_header(data: bytes) -> dict[str, Any]:
    values = struct.unpack_from("<27i", data, 136)
    names = (
        "flags",
        "numbones", "boneindex",
        "numbonecontrollers", "bonecontrollerindex",
        "numhitboxes", "hitboxindex",
        "numseq", "seqindex",
        "numseqgroups", "seqgroupindex",
        "numtextures", "textureindex", "texturedataindex",
        "numskinref", "numskinfamilies", "skinindex",
        "numbodyparts", "bodypartindex",
        "numattachments", "attachmentindex",
        "soundtable", "soundindex", "soundgroups", "soundgroupindex",
        "numtransitions", "transitionindex",
    )
    header: dict[str, Any] = dict(zip(names, values))
    header.update({
        "version": _i32(data, 4),
        "name": _cstring(data[8:72]),
        "declared_length": _i32(data, 72),
    })
    return header


def inspect_file(path: Path) -> dict[str, Any]:
    result: dict[str, Any] = {"path": str(path), "file_size": path.stat().st_size}
    data = path.read_bytes()

    if len(data) < 8:
        result.update(format="invalid", valid=False, errors=["file is shorter than 8 bytes"])
        return result

    magic = data[:4]
    result["magic"] = magic.decode("ascii", errors="backslashreplace")
    result["version"] = _i32(data, 4)

    if magic == b"IDSQ":
        errors = []
        if len(data) < SEQUENCE_HEADER_SIZE:
            errors.append("truncated sequence-group header")
            name = ""
            declared_length = 0
        else:
            name = _cstring(data[8:72])
            declared_length = _i32(data, 72)
            if declared_length != len(data):
                errors.append(
                    f"declared length {declared_length} differs from file size {len(data)}"
                )
        if result["version"] != STUDIO_VERSION:
            errors.append(f"expected studio version {STUDIO_VERSION}")
        result.update(
            format="sequence_group",
            renderer=None,
            loader=None,
            name=name,
            declared_length=declared_length,
            valid=not errors,
            errors=errors,
        )
        return result

    dispatch = MODEL_MAGICS.get(magic)
    if dispatch is None:
        result.update(
            format="unknown",
            valid=False,
            errors=["signature is not IDST, idst, or IDSQ"],
        )
        return result

    model_format, renderer, loader = dispatch
    errors = []
    if len(data) < STUDIO_HEADER_SIZE:
        result.update(
            format=model_format,
            renderer=renderer,
            loader=loader,
            valid=False,
            errors=[f"truncated studio header (need {STUDIO_HEADER_SIZE} bytes)"],
        )
        return result

    header = _parse_model_header(data)
    limit = header["declared_length"]
    if header["version"] != STUDIO_VERSION:
        errors.append(f"expected studio version {STUDIO_VERSION}")
    if limit != len(data):
        errors.append(f"declared length {limit} differs from file size {len(data)}")
    if limit < STUDIO_HEADER_SIZE or limit > len(data):
        limit = len(data)

    arrays = (
        ("bones", header["boneindex"], header["numbones"], 112),
        ("bone controllers", header["bonecontrollerindex"], header["numbonecontrollers"], 24),
        ("hitboxes", header["hitboxindex"], header["numhitboxes"], 32),
        ("sequences", header["seqindex"], header["numseq"], 176),
        ("sequence groups", header["seqgroupindex"], header["numseqgroups"], 104),
        ("textures", header["textureindex"], header["numtextures"], 80),
        ("body parts", header["bodypartindex"], header["numbodyparts"], 76),
        ("attachments", header["attachmentindex"], header["numattachments"], 88),
    )
    for label, offset, count, stride in arrays:
        if count and not _range_is_valid(offset, count, stride, limit):
            errors.append(f"{label} range is outside the declared model length")

    skin_count = header["numskinref"] * header["numskinfamilies"]
    if skin_count and not _range_is_valid(header["skinindex"], skin_count, 2, limit):
        errors.append("skin table is outside the declared model length")

    result.update(
        format=model_format,
        renderer=renderer,
        loader=loader,
        valid=not errors,
        errors=errors,
        texture_payloads=_inspect_textures(data, header),
        **header,
    )
    return result


def expand_paths(paths: Iterable[Path]) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        if path.is_dir():
            files.extend(p for p in path.rglob("*") if p.is_file() and p.suffix.lower() == ".mdl")
        else:
            files.append(path)
    return sorted(set(files), key=lambda path: str(path).lower())


def print_result(result: dict[str, Any]) -> None:
    path = Path(result["path"])
    model_format = result["format"]
    if model_format == "sequence_group":
        detail = "sequence group (not a Neo model)"
    elif model_format in ("standard", "neo"):
        detail = f"{model_format} -> {result['renderer']}"
    else:
        detail = model_format

    fields = [
        f"{path.name}: {result.get('magic', '----')} {detail}",
        f"v{result.get('version', '?')}",
        f"size={result['file_size']}",
    ]
    if "numbones" in result:
        fields.extend((
            f"bones={result['numbones']}",
            f"seq={result['numseq']}",
            f"textures={result['numtextures']}",
            f"bodyparts={result['numbodyparts']}",
        ))
        payloads = result.get("texture_payloads", {})
        if payloads:
            fields.append("payloads=" + ",".join(f"{key}:{value}" for key, value in sorted(payloads.items())))
    if not result["valid"]:
        fields.append("INVALID: " + "; ".join(result["errors"]))
    print("  ".join(fields))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="+", type=Path, help=".mdl file or directory to inspect")
    parser.add_argument("--json", action="store_true", help="write full results as JSON")
    args = parser.parse_args()

    missing = [path for path in args.paths if not path.exists()]
    if missing:
        for path in missing:
            print(f"error: path does not exist: {path}", file=sys.stderr)
        return 2

    files = expand_paths(args.paths)
    results = [inspect_file(path) for path in files]
    if args.json:
        print(json.dumps(results, indent=2))
    else:
        for result in results:
            print_result(result)
        counts = Counter(result["format"] for result in results)
        summary = ", ".join(f"{key}={value}" for key, value in sorted(counts.items()))
        print(f"\nScanned {len(results)} file(s): {summary}")

    return 0 if all(result["valid"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
