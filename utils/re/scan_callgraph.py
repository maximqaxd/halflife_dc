#!/usr/bin/env python3
"""
Extract the fork call graph: for each function, the set of other fork functions
it calls. Emits utils/re/func_calls.tsv  (funcname<TAB>callee1,callee2,...).
Ultra-common callees (utility fns called from very many places) are dropped so
the signature stays distinctive. Used by SweepCallGraph.java to name string-less
binary functions by their callee signature.
"""
import os, re, glob, sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(REPO, "src")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "func_calls.tsv")

HDR = re.compile(
    r"([A-Za-z_][\w]*(?:\s*::\s*~?[A-Za-z_][\w]*)?)\s*\([^;{}]*\)\s*(?:const)?\s*$")
IDENT_CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
KEYWORDS = {"if","for","while","switch","return","sizeof","else","do","catch"}

# reuse the comment/char scanner from scan_func_strings (inlined)
def strip(text):
    out=[]; i=0; n=len(text); s=c=lc=bc=False
    while i<n:
        ch=text[i]; two=text[i:i+2]
        if lc:
            if ch=="\n": lc=False; out.append(ch)
            i+=1; continue
        if bc:
            if two=="*/": bc=False; i+=2; continue
            i+=1; continue
        if s:
            if ch=="\\": i+=2; continue
            if ch=='"': s=False
            i+=1; continue
        if c:
            if ch=="\\": i+=2; continue
            if ch=="'": c=False
            i+=1; continue
        if two=="//": lc=True; i+=2; continue
        if two=="/*": bc=True; i+=2; continue
        if ch=='"': s=True; i+=1; continue
        if ch=="'": c=True; i+=1; continue
        out.append(ch); i+=1
    return "".join(out)

def funcname(hdr):
    m=HDR.search(hdr.strip())
    if not m: return None
    nm=re.sub(r"\s+","",m.group(1))
    if nm.split("::")[0] in KEYWORDS: return None
    return nm

def scan(path, calls):
    text=strip(open(path,encoding="latin-1").read())
    depth=0; i=0; n=len(text); cur=None; body_start=0
    while i<n:
        ch=text[i]
        if ch=="{":
            if depth==0:
                j=i-1
                while j>0 and text[j] not in ";}{": j-=1
                cur=funcname(text[j+1:i]); body_start=i
            depth+=1; i+=1; continue
        if ch=="}":
            depth-=1
            if depth<=0 and cur:
                body=text[body_start:i]
                for m in IDENT_CALL.finditer(body):
                    callee=m.group(1)
                    if callee not in KEYWORDS:
                        calls[cur].add(callee)
                depth=0; cur=None
            elif depth<0: depth=0
            i+=1; continue
        i+=1

def main():
    calls=defaultdict(set)
    files=[]
    for ext in ("*.c","*.C","*.cpp"): files+=glob.glob(os.path.join(SRC,"**",ext),recursive=True)
    for f in files:
        try: scan(f,calls)
        except Exception as e: print("skip",f,e,file=sys.stderr)
    names=set(calls.keys())
    # callee frequency across all functions (only count edges to known funcs)
    freq=defaultdict(int)
    for fn,cs in calls.items():
        for c in cs:
            if c in names: freq[c]+=1
    COMMON=30  # drop utility callees called from >COMMON functions
    n_emit=0
    with open(OUT,"w",encoding="utf-8") as w:
        for fn in sorted(calls):
            sig=sorted(c for c in calls[fn] if c in names and c!=fn and freq[c]<=COMMON)
            if len(sig)>=3:
                w.write(fn+"\t"+",".join(sig)+"\n"); n_emit+=1
    print("files=%d funcs=%d emitted(sig>=3)=%d common_dropped=%d -> %s"
          % (len(files),len(calls),n_emit,sum(1 for c in freq if freq[c]>COMMON),OUT))

if __name__=="__main__":
    main()
