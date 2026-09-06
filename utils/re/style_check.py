#!/usr/bin/env python3
"""Check the legacy Valve block layout used by reconstructed source files."""

import argparse
import pathlib
import re
import sys


SOURCE_SUFFIXES = {".c", ".cpp", ".h"}
CONTROL_BRACE = re.compile(
    r"^(?P<indent>[\t ]*)(?P<control>(?:if|for|while|switch)\b.*\))\s*\{[\t ]*$"
)
ELSE_BRACE = re.compile(r"^(?P<indent>[\t ]*)else\s*\{[\t ]*$")
DO_BRACE = re.compile(r"^(?P<indent>[\t ]*)do\s*\{[\t ]*$")
CLOSE_ELSE = re.compile(r"}\s*(?:else|catch)\b")
TRAILING_WHITESPACE = re.compile(r"[\t ]+$")


def source_files(paths):
    for path in paths:
        if path.is_dir():
            for child in path.rglob("*"):
                if child.suffix.lower() in SOURCE_SUFFIXES:
                    yield child
        elif path.suffix.lower() in SOURCE_SUFFIXES:
            yield path


def split_line(line):
    if line.endswith("\r\n"):
        return line[:-2], "\r\n"
    if line.endswith("\n"):
        return line[:-1], "\n"
    if line.endswith("\r"):
        return line[:-1], "\r"
    return line, ""


def split_control_brace(body):
    match = CONTROL_BRACE.match(body)
    if match:
        return [match.group("indent") + match.group("control"), match.group("indent") + "{"]

    match = ELSE_BRACE.match(body)
    if match:
        return [match.group("indent") + "else", match.group("indent") + "{"]

    match = DO_BRACE.match(body)
    if match:
        return [match.group("indent") + "do", match.group("indent") + "{"]

    return None


def check_file(path, fix):
    original = path.read_bytes().decode("utf-8", errors="surrogateescape")
    output = []
    issues = []

    for number, line in enumerate(original.splitlines(keepends=True), 1):
        body, ending = split_line(line)
        trimmed = TRAILING_WHITESPACE.sub("", body)
        if trimmed != body:
            issues.append((number, "trailing whitespace"))
            body = trimmed if fix else body

        if CLOSE_ELSE.search(body):
            issues.append((number, "joined close and else"))

        replacement = split_control_brace(body)
        if replacement:
            issues.append((number, "control brace on the condition line"))
            if fix:
                output.extend(part + ending for part in replacement)
                continue

        output.append(body + ending)

    updated = "".join(output)
    if fix and updated != original:
        path.write_bytes(updated.encode("utf-8", errors="surrogateescape"))

    return issues, updated != original


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="*", type=pathlib.Path, default=[pathlib.Path("src")])
    parser.add_argument("--fix", action="store_true")
    args = parser.parse_args()

    total = 0
    changed = 0
    for path in sorted(set(source_files(args.paths))):
        issues, modified = check_file(path, args.fix)
        for line, issue in issues:
            print(f"{path}:{line}: {issue}")
        total += len(issues)
        changed += int(modified)

    if args.fix:
        print(f"style_check: fixed {total} issue(s) in {changed} file(s)")
    else:
        print(f"style_check: found {total} issue(s)")

    return 1 if total and not args.fix else 0


if __name__ == "__main__":
    sys.exit(main())
