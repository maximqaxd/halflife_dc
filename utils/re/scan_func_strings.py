#!/usr/bin/env python3
"""
Scan the fork source and map each string literal to the function that contains
it. Emits utils/re/func_strings.tsv (string<TAB>funcname) for strings that are
UNIQUE to a single function -- these let the Ghidra applier name a binary
FUN_ that references the same string.

Function-name convention matches the Ghidra sweeps: C -> bare name,
C++ method -> Class::Method.
"""
import os, re, sys, glob

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(REPO, "src")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "func_strings.tsv")

# a function header just before the opening brace, at depth 0:
#   ret Name( args )   |   ret Class::Method( args )   |   Class::Method( args )
HDR = re.compile(
    r"([A-Za-z_][\w]*(?:\s*::\s*~?[A-Za-z_][\w]*)?)\s*\([^;{}]*\)\s*(?:const)?\s*$")

STR = re.compile(r'"((?:[^"\\]|\\.)*)"')

def strip_comments_strings_aware(text):
    """Remove // and /* */ comments but keep string contents (for both scanning
    passes we only need brace/string tracking, so do a light char scanner)."""
    out = []
    i, n = 0, len(text)
    in_s = in_c = in_lc = in_bc = False
    while i < n:
        c = text[i]
        two = text[i:i+2]
        if in_lc:
            if c == "\n": in_lc = False; out.append(c)
            i += 1; continue
        if in_bc:
            if two == "*/": in_bc = False; i += 2; continue
            i += 1; continue
        if in_s:
            out.append(c)
            if c == "\\": out.append(text[i+1] if i+1<n else ""); i += 2; continue
            if c == '"': in_s = False
            i += 1; continue
        if in_c:
            if c == "\\": i += 2; continue
            if c == "'": in_c = False
            i += 1; continue
        if two == "//": in_lc = True; i += 2; continue
        if two == "/*": in_bc = True; i += 2; continue
        if c == '"': in_s = True; out.append(c); i += 1; continue
        if c == "'": in_c = True; i += 1; continue
        out.append(c); i += 1
    return "".join(out)

def funcname_from_header(hdr):
    m = HDR.search(hdr.strip())
    if not m:
        return None
    name = re.sub(r"\s+", "", m.group(1))
    # skip control keywords that look like calls
    if name.split("::")[0] in ("if","for","while","switch","return","sizeof","else","do"):
        return None
    return name

def scan_file(path, mapping):
    raw = open(path, encoding="latin-1").read()
    text = strip_comments_strings_aware(raw)
    depth = 0
    i, n = 0, len(text)
    cur = None
    line_start = 0
    # track header = text of current logical line-ish before a '{'
    while i < n:
        c = text[i]
        if c == '"':
            # string literal
            m = STR.match(text, i)
            if m:
                s = m.group(1)
                if cur and len(s) >= 5 and not re.fullmatch(r"[%\-+ 0-9.lsdxfgcp#\\n\t]*", s):
                    mapping.setdefault(s, set()).add(cur)
                i = m.end(); continue
            i += 1; continue
        if c == "{":
            if depth == 0:
                # find header: text from last ';' '}' '{' up to here
                j = i - 1
                while j > 0 and text[j] not in ";}{":
                    j -= 1
                hdr = text[j+1:i]
                nm = funcname_from_header(hdr)
                cur = nm
            depth += 1; i += 1; continue
        if c == "}":
            depth -= 1
            if depth <= 0:
                depth = 0; cur = None
            i += 1; continue
        i += 1

def main():
    mapping = {}
    files = []
    for ext in ("*.c","*.C","*.cpp"):
        files += glob.glob(os.path.join(SRC, "**", ext), recursive=True)
    for f in files:
        try: scan_file(f, mapping)
        except Exception as e: print("skip", f, e, file=sys.stderr)
    uniq = {s: next(iter(fs)) for s, fs in mapping.items() if len(fs) == 1}
    with open(OUT, "w", encoding="utf-8") as w:
        for s, fn in sorted(uniq.items()):
            if "\t" in s or "\n" in s: continue
            w.write(s + "\t" + fn + "\n")
    print("files=%d  total_strings=%d  unique=%d  ->%s"
          % (len(files), len(mapping), len(uniq), OUT))

if __name__ == "__main__":
    main()
