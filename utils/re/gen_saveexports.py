#!/usr/bin/env python3
"""Generate the save/restore function-pointer registrars for the statically
linked game DLL.

Saved function pointers are stored by a two-character code, so the registry has
to be built exactly the way the shipped game builds it: the same codes naming
the same functions, registered in the same order (NameForFunction returns the
first code whose function matches, and several classes share one handler).

utils/re/saverestore/code_map.tsv holds that table, recovered from the binary by
extract_saveexports.py + map_saveexports.py.  This script turns it into one
registrar per class, emitted into the source file that defines the class, plus
the dispatcher in saveexports.cpp that calls them in registration order.  Each
registered class is granted friendship so the registrar can reach private
handlers.
"""
import os
import re
import glob

HL = "src/halflife"
MAP = "utils/re/saverestore/code_map.tsv"
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
            if len(cond) > 1:
                cond.pop()
            if len(active) > 1:
                active.pop()
            cond.append(True)
            active.append(active[-1])
            out.append("")
        elif re.match(r"#\s*else\b", s):
            was = cond.pop() if len(cond) > 1 else True
            if len(active) > 1:
                active.pop()
            cond.append(not was)
            active.append(active[-1] and (not was))
            out.append("")
        elif re.match(r"#\s*endif\b", s):
            if len(cond) > 1:
                cond.pop()
            if len(active) > 1:
                active.pop()
            out.append("")
        else:
            out.append(line if active[-1] else "")
    return "\n".join(out)


def clean(path):
    return strip_disabled(strip_code(open(path, encoding="latin-1").read()))


# ---- where every class body is declared, and which .cpp defines it ----
class_body_file = {}
for path in sorted(glob.glob(HL + "/*.cpp") + glob.glob(HL + "/*.h")):
    base = os.path.basename(path)
    depth, stack, pending = 0, [], None
    for line in clean(path).split("\n"):
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


def cpp_defining(cls):
    # a method definition starts at column 0 (return type), not an indented call
    pat = re.compile(r"(?m)^[^\s/].*\b" + re.escape(cls) + r"\s*::")
    for c in sorted(glob.glob(HL + "/*.cpp")):
        if pat.search(clean(c)):
            return os.path.basename(c)
    return None


class_home = {}
for cls, body in class_body_file.items():
    class_home[cls] = body if body.endswith(".cpp") else (cpp_defining(cls) or "cbase.cpp")

# ---- the shipped registry ----
rows = []
for line in open(MAP):
    if line.startswith(("seq", "#")):
        continue
    p = line.rstrip("\n").split("\t")
    rows.append(dict(seq=int(p[0]), code=p[1], cls=p[2], method=p[3],
                     registrar=p[7], how=p[5],
                     home=(p[8] + ".cpp") if len(p) > 8 and p[8] != "-" else None))

skipped = [r for r in rows if r["cls"] == "?"]
rows = [r for r in rows if r["cls"] != "?"]

# ---- one registrar per shipped registrar, in registration order ----
blocks = []
for r in rows:
    if blocks and blocks[-1][0] == r["registrar"]:
        blocks[-1][1].append(r)
    else:
        blocks.append((r["registrar"], [r]))

groups = []
used_names = {}
for registrar, block in blocks:
    home = block[0]["home"]
    counts = {}
    for r in block:
        counts[r["cls"]] = counts.get(r["cls"], 0) + 1
    # name the registrar after the class it belongs to; a block that only holds
    # inherited helpers names no class of its own, so use its file instead
    lead = max(counts, key=lambda c: counts[c])
    own = [c for c in counts if class_home.get(c) == home]
    if own:
        lead = max(own, key=lambda c: counts[c])
        name = "SR_Register_" + lead
    elif all(class_body_file.get(c) == "cbase.h" for c in counts):
        # nothing but the common handlers every entity inherits: the class it
        # was registered for left no trace, so the file has to name it
        name = "SR_Register_" + os.path.splitext(home or "cbase.cpp")[0]
    else:
        home = class_home.get(lead, "cbase.cpp")
        name = "SR_Register_" + lead
    n = used_names.get(name, 0) + 1
    used_names[name] = n
    if n > 1:
        name += "_%d" % n
    groups.append((name, lead, home, block))

# ---- friendship: every class a registrar reaches into ----
by_declfile = {}
for name, lead, home, block in groups:
    for r in block:
        declfile = class_body_file.get(r["cls"])
        if declfile:
            by_declfile.setdefault(declfile, set()).add((r["cls"], name))

for path in sorted(glob.glob(HL + "/*.cpp") + glob.glob(HL + "/*.h")):
    declfile = os.path.basename(path)
    text = open(path, encoding="latin-1").read()
    original = text
    text = re.sub(r"[ \t]*friend void SR_Register_\w+\( void \);[ \t]*"
                  + re.escape(FRIEND_TAG) + r"\n", "", text)
    for cls, name in sorted(by_declfile.get(declfile, ())):
        friend = "\tfriend void %s( void ); %s" % (name, FRIEND_TAG)
        # take the newline after the brace with it, or the class body grows a
        # blank line every time this runs
        pat = re.compile(r"(class\s+" + re.escape(cls) + r"\b[^{;]*\{)\n")
        text, _ = pat.subn(lambda mm: mm.group(1) + "\n" + friend + "\n", text, count=1)
    if text != original:
        open(path, "w", encoding="latin-1", newline="\n").write(text)

# ---- emit the registrar blocks into each class's own file ----
by_home = {}
for name, lead, home, block in groups:
    where = home if home and os.path.exists(HL + "/" + home) else class_home.get(lead, "cbase.cpp")
    by_home.setdefault(where, []).append((name, block))

block_re = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END) + r"\n?", re.S)
for c in sorted(glob.glob(HL + "/*.cpp")):
    home = os.path.basename(c)
    text = open(c, encoding="latin-1").read()
    stripped = block_re.sub("", text).rstrip() + "\n"
    if home not in by_home:
        if stripped != text:
            open(c, "w", encoding="latin-1", newline="\n").write(stripped)
        continue
    parts = []
    for name, block in by_home[home]:
        body = "\n".join('\tSR_REGISTER( "%s", %s, %s );' % (r["code"], r["cls"], r["method"])
                         for r in block)
        parts.append("void %s( void )\n{\n%s\n}\n" % (name, body))
    open(c, "w", encoding="latin-1", newline="\n").write(
        stripped + "\n" + BEGIN + "\n" + "\n".join(parts) + END + "\n")

# ---- dispatcher, in registration order ----
externs = "\n".join("void %s( void );" % g[0] for g in groups)
calls = "\n".join("\t%s();" % g[0] for g in groups)
open(HL + "/saveexports.cpp", "w", encoding="latin-1", newline="\n").write(
    "// Generated by utils/re/gen_saveexports.py -- do not edit by hand.\n"
    '#include "extdll.h"\n#include "util.h"\n#include "cbase.h"\n\n'
    + externs + "\n\n"
    'extern "C" void GameDLL_RegisterSaveExports( void )\n{\n' + calls + "\n}\n")

print("registrations=%d  registrars=%d  home_files=%d  unmapped=%d"
      % (len(rows), len(groups), len(by_home), len(skipped)))
for r in skipped:
    print("   no source function for code %s" % r["code"])
