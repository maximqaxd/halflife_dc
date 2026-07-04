# Export all function symbols (name, entry VA, byte size) to a TSV so the
# standalone objdiff harness can slice the reference binary's disassembly by
# Ghidra address. Re-run after any renaming sweep.
# Python port of ExportSymbols.java (MCP can't compile .java: OSGi source-bundle quirk).
#@category HalfLifeDC
OUT = r"C:\dev\Dreamcast\halflife_dc\utils\re\symbols.tsv"

fm = currentProgram.getFunctionManager()
w = open(OUT, "w")
n = 0
for f in fm.getFunctions(True):
    if f.isThunk() or f.isExternal():
        continue
    ep = f.getEntryPoint().getOffset() & 0xFFFFFFFF
    size = f.getBody().getNumAddresses()
    w.write("%s\t%x\t%d\n" % (f.getName(), ep, size))
    n += 1
w.close()
print("wrote %d symbols to %s" % (n, OUT))
