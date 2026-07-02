#!/usr/bin/env python3
"""
Generate/refresh utils/re/re_progress.csv: the master 1:1 reconstruction tracker.
One row per source file (plus binary-only files that must be decompiled fresh),
ordered by attack priority, merged with the latest scoreboard.csv measurements.

Columns:
  order, group, file, subsystem, n_funcs, exact, mean_pct, completion_pct, status, note

completion_pct = exact / n_funcs * 100  (fraction of the file's functions that are
byte-1:1 with the binary). status: missing | blocked | todo | started | done.

Re-run after each scoreboard.py pass to update numbers.
"""
import os, csv, glob
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(REPO, "src")
SCORE = os.path.join(HERE, "scoreboard.csv")
OUT = os.path.join(HERE, "re_progress.csv")

# files present in the binary (from assert-path strings) but absent in the fork:
# these have no diff target -> decompile fresh.
MISSING = [
    ("audio/afile.cpp","audio"), ("audio/audio.cpp","audio"),
    ("audio/audio_cd.cpp","audio"), ("audio/audio_mgr.cpp","audio"),
    ("audio/audio_static.cpp","audio"), ("audio/audio_stream.cpp","audio"),
    ("render/r_studio_neo.c","render"), ("engine/ui.c","engine"),
    ("engine/eng_cdll_int.c","engine"), ("util/Zap.cpp","util"),
]
BLOCKED = {  # compile blockers (reason)
    "engine/cl_input.c": "SHCL ICE CBE6051",
    "engine/sv_phys.c": "SHCL ICE CBE6051",
    "engine/conproc.c": "Win32-only, exclude from DC",
    "engine/glHud.c": "GL stub undefined",
    "render/glHud.c": "GL stub undefined",
}

def subsystem(rel):
    return rel.split("/")[1] if rel.startswith("src/") else rel.split("/")[0]

def group_and_order(rel, sub):
    """Return (order, group-label). Lower order = attack first."""
    base = os.path.basename(rel).lower()
    if sub == "engine":
        if base.startswith("sys_") or base in ("in_dc.c","dc_framebuffer.c"): return (10,"1-platform")
        if base.startswith("host"): return (20,"2-host")
        if base.startswith(("cmd","cvar","common","zone","mathlib","crc","wad","hashpak","tmessage","buildnum")): return (30,"3-engine-core")
        if base.startswith("pr_") or base.startswith("progs") or base.startswith("entity"): return (35,"4-progs")
        if base.startswith(("cl_","console","keys","view","chase","cam","hud","cdll")): return (40,"5-engine-client")
        if base.startswith(("sv_","world","physics","pmove")): return (50,"6-engine-server")
        if base.startswith("l_studio") or base.startswith("cmodel"): return (55,"7-engine-model")
        return (39,"3-engine-core")
    if sub == "network": return (60,"8-network")
    if sub == "util": return (32,"3-engine-core")
    if sub == "render": return (70,"9-render")
    if sub == "audio":  return (80,"10-audio")
    if sub == "client": return (95,"12-client-dll")
    if sub == "halflife": return (90,"11-game-last")
    if sub == "common": return (31,"3-engine-core")
    return (99,"13-other")

def load_scores():
    per = defaultdict(list)
    if not os.path.exists(SCORE): return per
    with open(SCORE) as f:
        for row in csv.DictReader(f):
            relf = os.path.relpath(row["file"], REPO).replace("\\","/")
            per[relf].append(float(row["match"]))
    return per

def main():
    scores = load_scores()
    files = []
    for ext in ("*.c","*.C","*.cpp"):
        files += glob.glob(os.path.join(SRC,"**",ext), recursive=True)
    # de-dup: Windows glob is case-insensitive so *.c and *.C collide
    files = sorted({os.path.normcase(os.path.abspath(p)): p for p in files}.values())
    rows = []
    seen = set()
    for path in files:
        rel = os.path.relpath(path, REPO).replace("\\","/")
        sub = subsystem(rel); seen.add(rel)
        order, group = group_and_order(rel, sub)
        key = rel[4:] if rel.startswith("src/") else rel
        ms = scores.get(rel, [])
        n = len(ms); exact = sum(1 for x in ms if x>=99.9)
        mean = round(sum(ms)/n,1) if n else ""
        comp = round(exact/n*100,1) if n else 0.0
        if key in BLOCKED: status, note = "blocked", BLOCKED[key]
        elif n==0: status, note = "todo", ""
        elif exact==n: status, note = "done", ""
        elif exact>0 or (mean and mean>0): status, note = "started", ""
        else: status, note = "todo", ""
        rows.append([order, group, rel, sub, n, exact, mean, comp, status, note])
    for relf, sub in MISSING:
        rel = "src/"+relf
        if rel in seen: continue
        order, group = group_and_order(rel, sub)
        rows.append([order, group, rel, sub, "", "", "", 0.0, "missing", "decompile fresh from binary"])
    rows.sort(key=lambda r:(r[0], r[2]))
    with open(OUT,"w",newline="") as f:
        w=csv.writer(f)
        w.writerow(["order","group","file","subsystem","n_funcs","exact","mean_pct","completion_pct","status","note"])
        w.writerows(rows)
    # summary
    done=sum(1 for r in rows if r[8]=="done"); started=sum(1 for r in rows if r[8]=="started")
    todo=sum(1 for r in rows if r[8]=="todo"); miss=sum(1 for r in rows if r[8]=="missing")
    blk=sum(1 for r in rows if r[8]=="blocked")
    print("wrote %d rows -> %s" % (len(rows), OUT))
    print("done=%d started=%d todo=%d missing=%d blocked=%d" % (done,started,todo,miss,blk))

if __name__=="__main__":
    main()
