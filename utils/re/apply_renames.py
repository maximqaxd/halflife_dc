#!/usr/bin/env python3
"""
Apply the reviewed rename_map.tsv (from backport_names.py) across the fork source.
Whole-identifier replacement only, and only in real code -- string/char literals
and //, /* */ comments are never touched (so names like BigLong or memsearch that
appear in messages/comments are safe).

Safety refusals (reported, not applied):
  * the new name already exists as an identifier somewhere in src (would merge two
    distinct symbols)
  * two different src names map to the same new name
  * a new name is itself another rename's old name (rename chain)

Usage:
  python apply_renames.py            # dry run: report what would change
  python apply_renames.py --write    # actually edit the files
"""
import os, re, sys, glob

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MAP = os.path.join(HERE, "portdata", "rename_map.tsv")
SRC_DIRS = ["engine", "render", "network", "common", "util", "client", "audio", "halflife"]

IDENT = re.compile(r"[A-Za-z_]\w*")


def src_files():
    fs = []
    for d in SRC_DIRS:
        for ext in ("*.c", "*.C", "*.cpp", "*.h", "*.hpp"):
            fs += glob.glob(os.path.join(REPO, "src", d, "**", ext), recursive=True)
    return sorted(set(fs))


def transform(text, rename, collect=None):
    """Walk C/C++ text; skip strings/chars/comments. In code, collect identifiers
    (collect set) or replace those in the rename dict. Returns (newtext, nchanges)."""
    out = []
    i, n = 0, len(text)
    changes = 0
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//":
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(text[i:j]); i = j; continue
        if two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(text[i:j]); i = j; continue
        if c == '"' or c == "'":
            q = c; out.append(c); i += 1
            while i < n:
                out.append(text[i])
                if text[i] == "\\" and i + 1 < n:
                    out.append(text[i + 1]); i += 2; continue
                if text[i] == q:
                    i += 1; break
                i += 1
            continue
        if c.isalpha() or c == "_":
            m = IDENT.match(text, i)
            ident = m.group(0)
            if collect is not None:
                collect.add(ident)
            elif ident in rename:
                out.append(rename[ident]); changes += 1; i = m.end(); continue
            out.append(ident); i = m.end(); continue
        out.append(c); i += 1
    return "".join(out), changes


def main():
    write = "--write" in sys.argv
    pairs = []
    for ln in open(MAP, encoding="utf-8"):
        p = ln.rstrip("\n").split("\t")
        if len(p) < 2 or p[0] == "src_name":
            continue
        pairs.append((p[0], p[1]))
    files = src_files()

    # gather every identifier that currently exists in code
    present = set()
    for f in files:
        transform(open(f, encoding="latin-1").read(), {}, collect=present)

    olds = {o for o, _ in pairs}
    new_count = {}
    for _, nw in pairs:
        new_count[nw] = new_count.get(nw, 0) + 1

    rename, skipped = {}, []
    for old, nw in pairs:
        if old == nw:
            continue
        if nw in present:
            skipped.append((old, nw, "new already in src")); continue
        if new_count[nw] > 1:
            skipped.append((old, nw, "ambiguous (multiple src->same new)")); continue
        if nw in olds:
            skipped.append((old, nw, "rename chain")); continue
        if old in rename and rename[old] != nw:
            skipped.append((old, nw, "src name maps to two names")); continue
        rename[old] = nw

    print("[apply] %d renames to apply, %d skipped" % (len(rename), len(skipped)))
    for old, nw, why in skipped:
        print("  SKIP %-28s -> %-28s (%s)" % (old, nw, why))

    total = 0
    for f in files:
        text = open(f, encoding="latin-1").read()
        newtext, ch = transform(text, rename)
        if ch:
            total += ch
            print("  %-44s %d" % (os.path.relpath(f, REPO).replace("\\", "/"), ch))
            if write:
                open(f, "w", encoding="latin-1", newline="").write(newtext)
    print("[apply] %d identifier replacements%s" % (total, "" if write else "  (dry run; pass --write)"))


if __name__ == "__main__":
    main()
