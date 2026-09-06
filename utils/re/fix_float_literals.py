#!/usr/bin/env python3
"""Apply safe single-precision literal fixes from SHCL CBE4717 warnings.

The tool intentionally changes only decimal literals on warning lines.  Integer
literals may take part in float arithmetic too, but inferring that safely needs
human review; they are reported instead of rewritten.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


WARNING = re.compile(
    r"^(?P<path>.*?src[\\/]+(?P<tree>client|halflife)[\\/].+?\.(?:c|cpp))"
    r"\((?P<line>\d+)\)\s*:\s*warning CBE4717:",
    re.IGNORECASE,
)
DECIMAL = re.compile(
    r"(?<![A-Za-z0-9_.])(?P<value>(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?)(?![A-Za-z0-9_.])"
)
INTEGER = re.compile(r"(?<![A-Za-z0-9_.])\d+(?![A-Za-z0-9_.])")


def warning_locations(log: Path, root: Path, trees: set[str]) -> dict[Path, set[int]]:
    locations: dict[Path, set[int]] = {}
    for raw in log.read_text(encoding="utf-8", errors="replace").splitlines():
        match = WARNING.match(raw)
        if not match or match.group("tree").lower() not in trees:
            continue
        source = Path(match.group("path"))
        if not source.is_absolute():
            source = root / source
        try:
            source = source.resolve()
        except OSError:
            continue
        locations.setdefault(source, set()).add(int(match.group("line")))
    return locations


def suffix_decimals(line: str) -> tuple[str, int]:
    def replace(match: re.Match[str]) -> str:
        nonlocal changes
        changes += 1
        return match.group("value") + "f"

    changes = 0
    code, marker, comment = line.partition("//")
    return DECIMAL.sub(replace, code) + marker + comment, changes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="full client or game-DLL build log")
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root")
    parser.add_argument(
        "--tree",
        action="append",
        choices=("client", "halflife"),
        help="source tree to change; repeat as needed (default: both)",
    )
    parser.add_argument("--apply", action="store_true", help="write changed source files")
    args = parser.parse_args()

    root = args.root.resolve()
    changed = 0
    review: list[tuple[Path, int, str]] = []
    trees = set(args.tree or ("client", "halflife"))
    for source, line_numbers in warning_locations(args.log, root, trees).items():
        if not source.is_file():
            print("missing:", source)
            continue
        lines = source.read_text(encoding="utf-8", errors="surrogateescape").splitlines(keepends=True)
        dirty = False
        for line_number in sorted(line_numbers):
            if not 1 <= line_number <= len(lines):
                print("out of range:", source, line_number)
                continue
            before = lines[line_number - 1]
            after, count = suffix_decimals(before)
            if count:
                print(f"{source.relative_to(root)}:{line_number}: {before.rstrip()} -> {after.rstrip()}")
                lines[line_number - 1] = after
                dirty = True
                changed += count
            elif INTEGER.search(before):
                review.append((source, line_number, before.rstrip()))
        if dirty and args.apply:
            source.write_text("".join(lines), encoding="utf-8", errors="surrogateescape", newline="")

    if review:
        print("\ninteger-only warning lines (review manually):")
        for source, line_number, line in review:
            print(f"{source.relative_to(root)}:{line_number}: {line}")
    print(f"{'applied' if args.apply else 'proposed'} {changed} decimal literal suffixes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
