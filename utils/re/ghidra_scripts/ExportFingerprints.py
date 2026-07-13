# Export per-function fingerprints for the current program, for cross-binary
# symbol porting (hlds_l x86 -> HALFLIFE_DC.EXE SH-4). Run in the Script Manager
# on BOTH programs (switch the active program, Run, repeat).
#
# Output: utils/re/portdata/<progname>_fp.tsv, one row per function:
#   entry_hex \t name \t is_default \t strings \t callee_hexes
#     strings      = referenced string literals, escaped, joined by \x01
#     callee_hexes = called-function entry addresses, comma-joined hex
#
# SH-4 references a string through the literal pool (instruction -> pool slot ->
# string), so we walk refs-to-string one hop further out when the direct source
# isn't inside a function -- same trick as SweepFuncStrings.java.
#@category HalfLifeDC
import os, codecs
from ghidra.util.task import TaskMonitor

OUTDIR = r"C:\dev\Dreamcast\halflife_dc\utils\re\portdata"

prog = currentProgram
fm = prog.getFunctionManager()
listing = prog.getListing()
rm = prog.getReferenceManager()


def esc(s):
    return (s.replace(u"\\", u"\\\\").replace(u"\t", u"\\t")
             .replace(u"\r", u"\\r").replace(u"\n", u"\\n").replace(u"\x01", u" "))


# 1) address -> string value, for every defined string in the program
str_at = {}
di = listing.getDefinedData(True)
while di.hasNext():
    d = di.next()
    try:
        if d is not None and d.hasStringValue():
            v = d.getValue()
            if v is not None:
                str_at[d.getAddress()] = unicode(v)
    except:
        pass
print("defined strings: %d" % len(str_at))

# 2) function -> set of referenced string values (with SH-4 pool second hop)
func_strs = {}


def add_str(f, val):
    func_strs.setdefault(f, set()).add(val)


for sa, val in str_at.items():
    for ref in rm.getReferencesTo(sa):
        frm = ref.getFromAddress()
        f = fm.getFunctionContaining(frm)
        if f is not None:
            add_str(f, val)
        else:
            # 'frm' is a literal-pool slot; the real users are instructions that load it
            for r2 in rm.getReferencesTo(frm):
                f2 = fm.getFunctionContaining(r2.getFromAddress())
                if f2 is not None:
                    add_str(f2, val)

# 3) function -> callee entry addresses
mon = TaskMonitor.DUMMY
func_callees = {}
for f in fm.getFunctions(True):
    cs = set()
    try:
        for c in f.getCalledFunctions(mon):
            cs.add(c.getEntryPoint().getOffset() & 0xFFFFFFFF)
    except:
        pass
    func_callees[f] = cs

# 4) write the table
if not os.path.isdir(OUTDIR):
    os.makedirs(OUTDIR)
outp = os.path.join(OUTDIR, prog.getName() + "_fp.tsv")
w = codecs.open(outp, "w", "utf-8")
n = 0
for f in fm.getFunctions(True):
    if f.isThunk() or f.isExternal():
        continue
    ea = f.getEntryPoint().getOffset() & 0xFFFFFFFF
    nm = f.getName()
    isdef = 1 if (nm.startswith("FUN_") or nm.startswith("SUB_") or nm.startswith("LAB_")) else 0
    strs = u"\x01".join([esc(x) for x in func_strs.get(f, set())])
    cals = ",".join(["%x" % a for a in sorted(func_callees.get(f, set()))])
    w.write(u"%x\t%s\t%d\t%s\t%s\n" % (ea, nm, isdef, strs, cals))
    n += 1
w.close()
print("wrote %d functions to %s" % (n, outp))
