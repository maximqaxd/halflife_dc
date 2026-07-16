#!/usr/bin/env python3
"""Generate the save/restore function-pointer registrars for the statically
linked game DLL.  Scans every game class body for EXPORT member functions and
emits, per source file, a registrar that hands each function pointer to
Sys_RegisterExport under a unique code.  GameDLL_RegisterSaveExports() (in the
generated saveexports.cpp) calls them all; each registered class is granted
friendship so the registrar can reach private handlers.

Codes are generated (AA, AB, ...) for now; the authentic retail 2-char codes
live in utils/re/saverestore/raw_pairs.tsv for a later exact pass.
"""
import os, re, glob

HL = "src/halflife"
BEGIN = "// BEGIN GENERATED SAVE-RESTORE EXPORTS"
END = "// END GENERATED SAVE-RESTORE EXPORTS"
FRIEND_TAG = "//SR_FRIEND"

def strip_code(text):
    # single pass that removes // and /* */ comments and blanks string/char
    # literal contents, respecting which construct starts first (so "//****"
    # line comments aren't mistaken for /* block-comment starts).  Newlines are
    # preserved so line numbers and per-line class parsing stay intact.
    out = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "//":
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text[i:j].count("\n"))
            i = j
        elif text[i] == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == '\\' else 1
            out.append('""')
            i = j + 1
        elif text[i] == "'":
            j = i + 1
            while j < n and text[j] != "'":
                j += 2 if text[j] == '\\' else 1
            out.append("''")
            i = j + 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)

# macros that are undefined / zero in the shipped (NDEBUG) game DLL build
FALSE_CONDS = ("0", "_DEBUG", "DEBUG", "PATH_SPARKLE_DEBUG")

def eval_cond(s):
    """approximate truth of an #if/#ifdef/#ifndef directive for the release build"""
    m = re.match(r"#\s*if\s+(.+?)\s*$", s)
    if m:
        return m.group(1).strip() not in FALSE_CONDS
    m = re.match(r"#\s*ifdef\s+(\w+)", s)
    if m:
        return m.group(1) not in FALSE_CONDS          # debug macros aren't defined
    m = re.match(r"#\s*ifndef\s+(\w+)", s)
    if m:
        return m.group(1) != "NDEBUG"                 # only #ifndef NDEBUG is false (guards stay)
    return True

def strip_disabled(text):
    """blank out lines inside disabled #if/#ifdef/#ifndef blocks (with #else)"""
    out, active, cond = [], [True], [True]
    for line in text.split("\n"):
        s = line.lstrip()
        if re.match(r"#\s*if(?:n?def)?\b", s):
            true = eval_cond(s)
            cond.append(true)
            active.append(active[-1] and true)
            out.append("")
        elif re.match(r"#\s*elif\b", s):
            was = cond.pop() if len(cond) > 1 else True
            if len(active) > 1: active.pop()
            cond.append(True)
            active.append(active[-1])
            out.append("")
        elif re.match(r"#\s*else\b", s):
            was = cond.pop() if len(cond) > 1 else True
            if len(active) > 1: active.pop()
            cond.append(not was)
            active.append(active[-1] and (not was))
            out.append("")
        elif re.match(r"#\s*endif\b", s):
            if len(cond) > 1: cond.pop()
            if len(active) > 1: active.pop()
            out.append("")
        else:
            out.append(line if active[-1] else "")
    return "\n".join(out)

files = sorted(glob.glob(HL + "/*.cpp") + glob.glob(HL + "/*.h"))

class_body_file = {}     # class -> file where its body is declared
exports = []             # ordered (class, func)
seen = set()

for path in files:
    base = os.path.basename(path)
    code = strip_disabled(strip_code(open(path, encoding="latin-1").read()))
    depth = 0
    stack = []
    pending = None
    for line in code.split("\n"):
        m = re.match(r"\s*class\s+(C\w+)", line)
        if m:
            pending = m.group(1)
        if pending and ";" in line and "{" not in line:
            pending = None
        for ch in line:
            if ch == "{":
                if pending:
                    stack.append((pending, depth))
                    class_body_file.setdefault(pending, base)
                    pending = None
                depth += 1
            elif ch == "}":
                depth -= 1
                if stack and stack[-1][1] == depth:
                    stack.pop()
        cur = stack[-1][0] if stack else None
        em = re.search(r"EXPORT\s+(\w+)\s*\(", line)
        if em and cur:
            key = (cur, em.group(1))
            if key not in seen:
                seen.add(key)
                inline = "{" in line          # inline body -> defined here
                exports.append((key[0], key[1], inline))

# build the set of member functions that actually have a definition, so we
# never take the address of a declared-but-undefined handler (would not link)
defined = set()
defre = re.compile(r"(?m)^[A-Za-z_][\w\s\*&:<>]*?\b(C\w+)\s*::\s*(~?\w+)\s*\(")
for c in glob.glob(HL + "/*.cpp"):
    txt = strip_disabled(strip_code(open(c, encoding="latin-1").read()))
    for mm in defre.finditer(txt):
        defined.add((mm.group(1), mm.group(2)))

exports = [(cls, fn) for cls, fn, inline in exports if inline or (cls, fn) in defined]

def cpp_defining(cls):
    # a method definition starts at column 0 (return type), not an indented call
    pat = re.compile(r"(?m)^[^\s/].*\b" + re.escape(cls) + r"\s*::")
    for c in sorted(glob.glob(HL + "/*.cpp")):
        if pat.search(strip_disabled(strip_code(open(c, encoding="latin-1").read()))):
            return os.path.basename(c)
    return None

class_home = {}
for cls in class_body_file:
    body = class_body_file[cls]
    class_home[cls] = body if body.endswith(".cpp") else (cpp_defining(cls) or "cbase.cpp")

by_home = {}
for cls, fn in exports:
    by_home.setdefault(class_home.get(cls, "cbase.cpp"), []).append((cls, fn))

def code_gen():
    for a in range(26):
        for b in range(26):
            yield chr(65 + a) + chr(65 + b)
codes = code_gen()
code_of = {}
for home in sorted(by_home):
    for cls, fn in sorted(by_home[home]):
        code_of[(cls, fn)] = next(codes)

reg_name = lambda home: "SR_Register_" + os.path.splitext(home)[0]

# ---- inject a friend declaration into every registered class body ----
classes_in_home = {}   # class -> home registrar it needs friendship with
for cls, fn in exports:
    classes_in_home[cls] = class_home.get(cls, "cbase.cpp")

by_declfile = {}
for cls, home in classes_in_home.items():
    by_declfile.setdefault(class_body_file[cls], []).append(cls)

for declfile, classes in by_declfile.items():
    path = HL + "/" + declfile
    text = open(path, encoding="latin-1").read()
    # drop any previous injected friends first (idempotent)
    text = re.sub(r"[ \t]*friend void SR_Register_\w+\( void \);[ \t]*" + re.escape(FRIEND_TAG) + r"\n", "", text)
    for cls in classes:
        friend = "\tfriend void %s( void ); %s\n" % (reg_name(classes_in_home[cls]), FRIEND_TAG)
        # insert right after the opening brace of "class Cls ... {"
        pat = re.compile(r"(class\s+" + re.escape(cls) + r"\b[^{;]*\{)")
        text, n = pat.subn(lambda mm: mm.group(1) + "\n" + friend, text, count=1)
    open(path, "w", encoding="latin-1", newline="\n").write(text)

# ---- emit per-file registrar blocks ----
block_re = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END) + r"\n?", re.S)
# drop stale blocks from files that no longer have any registrations
for c in glob.glob(HL + "/*.cpp"):
    if os.path.basename(c) not in by_home:
        txt = open(c, encoding="latin-1").read()
        new = block_re.sub("", txt).rstrip() + "\n"
        if new != txt:
            open(c, "w", encoding="latin-1", newline="\n").write(new)
for home in sorted(by_home):
    path = HL + "/" + home
    body = "\n".join(
        '\tSR_REGISTER( "%s", %s, %s );' % (code_of[(cls, fn)], cls, fn)
        for cls, fn in by_home[home]
    )
    block = (BEGIN + "\n" + "void " + reg_name(home) + "( void )\n{\n"
             + body + "\n}\n" + END + "\n")
    text = open(path, encoding="latin-1").read()
    text = block_re.sub("", text).rstrip() + "\n\n" + block
    open(path, "w", encoding="latin-1", newline="\n").write(text)

# ---- dispatcher ----
externs = "\n".join("void %s( void );" % reg_name(h) for h in sorted(by_home))
calls = "\n".join("\t%s();" % reg_name(h) for h in sorted(by_home))
open(HL + "/saveexports.cpp", "w", encoding="latin-1", newline="\n").write(
    "// Generated by utils/re/gen_saveexports.py -- do not edit by hand.\n"
    '#include "extdll.h"\n#include "util.h"\n#include "cbase.h"\n\n'
    + externs + "\n\n"
    'extern "C" void GameDLL_RegisterSaveExports( void )\n{\n' + calls + "\n}\n")

print("functions=%d  home_files=%d  classes=%d" % (len(exports), len(by_home), len(class_body_file)))
