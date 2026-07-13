#!/usr/bin/env python3
"""Build dc_draw_checklist.xlsx: every dc_draw.c function present in the binary,
with its match %% and whether we've reverse-engineered/rewritten it (V) or not (X)."""
import os
from openpyxl import Workbook
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

RE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(RE))

# --- load addresses ---
addr = {}
for ln in open(os.path.join(RE, "symbols.tsv"), encoding="latin-1"):
    p = ln.rstrip("\n").split("\t")
    if len(p) == 3:
        nm = p[0][6:] if p[0].startswith("maybe_") else p[0]
        addr.setdefault(nm, p[1])

# --- load scores (name, obj, bin, exact%, struct%) ---
rows = []
for ln in open(os.path.join(RE, "build", "scores.tsv"), encoding="latin-1"):
    p = ln.rstrip("\n").split("\t")
    if len(p) == 5:
        nm, obj, binn, m, st = p
        rows.append([nm, addr.get(nm, ""), int(binn), int(obj), float(m), float(st)])

# --- what we touched this session (function -> note) ---
TOUCHED = {
    "DC_LoadTexture":            "Ported to binary structure (cache-hit, GetSlot inline, switch+2-phase dispatch)",
    "DC_SetupTextureSlot":       "Rewritten to binary 9-arg form (surface/tex/name/palIdx/dims + LRU link)",
    "DCV_ClearCachedFlag":       "Rewritten: pPrev walk; made __inline",
    "DCV_TwiddleBlit8":          "Made __inline (binary carries it inline)",
    "DCV_PrepSurface16":         "Converted to binary ABI + DCV_CREATE_SURFACE inline",
    "DCV_PrepSurfaceIndexed":    "Converted to binary ABI + inline create",
    "DCV_PrepSurfaceTrueColor":  "Converted; 3-way RGBA->565/4444 branch",
    "DCV_PrepSurfacePaletted":   "Converted to P8 path; inline TwiddleBlit8",
    "DCV_PrepSurfaceTwiddled":   "Converted to binary ABI + inline create",
    "DCV_PrepSurfaceResampled":  "Converted; passes fmt straight to ResampleBlit",
    "DCV_PrepSurfacePVR":        "Rewritten: color/image if-else chains + payload copy",
    "DC_FreeTextureSlot":        "Rewritten from Ghidra: GL_BindStage(0,0), inline LRU unlink, tex-before-surface release, g_bTextureLog",
    "DC_InitTextureList":        "Reduced to binary's 5-stmt sentinel wiring + g_bTextureLog log (was 93-insn slot/palette clear)",
    "GL_BindStage":              "Rewritten from Ghidra: s_nD3dCurrentTexnum gate, inline LRU touch, spawn-count refresh, global g_pD3DDevice",
    "DC_DecacheTextureSlot":     "Rewritten from Ghidra: pbCacheData guard, g_pDD4 CreateSurface retry, SetPalette, short copy, QueryInterface GetTexture",
    "DC_CacheTextureToRam":      "Rewritten from Ghidra: early bCached=1, Mnemo_LastChanceActive guard, GetSurfaceDesc+Lock, VRAM->RAM copy, release tex+surface",
    "DC_ReclaimTextureSlot":     "Tail-walk from LRU, single GL_UnloadTextures on miss, Sys_FPrintf(g_bTextureLog); cache-or-free decision",
    "DC_SetupTextureSlot":       "operator new/delete name alloc (1-arg via DCV_AllocName), inline LRU link-front, Sys_FPrintf",
    "DC_TouchTexture":           "Bound + servercount guard, inline LRU touch (DCV_LinkTextureSlotFront now __inline), spawn-count refresh",
    "DCV_SetHudDepth":           "Rebuilt from stub: SetViewport, identity transforms, Ortho at depth layer, SetViewportDepthRange",
    "DCV_TwiddleBlit8Recurse":   "size/=2 signed rounding + packed 16-bit stores for the 2x2 Morton block",
    "EnableScissorTest":         "Clamp to screen + Y-flip for D3D coords (fixed truncated symbol size)",
    "Draw_AlphaSubPic":          "Rebuilt on the qgl immediate-mode API (qglBegin(GL_QUADS)/TexCoord2f/Vertex2f), normalized float color",
    "Draw_SpriteFrame":          "Reduced to a pass-through to Draw_Frame (was over-built with additive/color/opaque)",
    "DCV_UpdateTextureSubRect":  "Minimal guard structure, DDError-wrapped Lock/Unlock, per-pixel copy loop",
    "DC_TexDump":                "Split out body: count loaded slots, Con_Printf max = numgltextures+1, close g_bTextureLog with 'Texture log end.'",
    "DC_TexDump_f":              "Reduced to a pass-through wrapper to DC_TexDump (was counting inline)",
    "GL_PaletteTag":             "GoldSrc hash form tag=(tag+pPal[i])^pPal[i-1] over i=1..767 (100%)",
    "DCV_BuildPalette565":       "Post-increment pointer reads + unsigned locals (logical shifts) + unsigned loop counter",
    "DCV_BuildPalette1555":      "Post-increment pointer reads + unsigned locals, 0x8000 alpha bit, out[255]=0 transparent entry",
    "DCV_BuildMaxChannelAlpha":  "Unsigned channel compares (cmp/hi) + logical >>4, alpha from brightest channel, white RGB",
    "DC_CreatePalette":          "Rewritten from Ghidra: inline old-palette Release, g_pDD4 direct, DDError-wrapped CreatePalette(0x44), no referenceCount write",
    "dcpalette_t::dcpalette_t":  "dcpalette_t is a C++ class; ctor tag=-1/refcount=0/lpPalette=NULL (100%)",
    "dcpalette_t::~dcpalette_t": "dtor releases lpPalette if set (100%)",
    "DC_ReleasePalettes":        "Compiler-generated atexit array-dtor thunk for static gGLPalette[4] (reverse order)",
    "DC_InitPalettes":           "Compiler-generated static-init thunk for gGLPalette[4]: 4 ctors + atexit(dtor thunk)",
    "Draw_Init":                 "Full rewrite from binary: first-time-guarded registration, menu_wad = MnemoAllocDbg ptr, custom_wad handler, memset(decal_names), DCV_GammaRefresh_f, 'nada' 16x16 fallback, lambda LoadTransPic, creditsfont-only charset (TEX_TYPE_FONT), conback via menu_wad cache",
    "DCV_ResampleBlit":          "RECT* dest descriptor, floors() coverage bounds (floatmathlib.h), do-while sampling (>=1 texel), g_nLastUploadBytes, DDError-wrapped Lock",
    "DCV_PrepSurfaceResampled":  "Builds RECT{0,0,w,h} and passes it to DCV_ResampleBlit",
    "dc_texture_s::dc_texture_s":  "dctexture_t is a C++ class; ctor clears slot (iPalette=-1, nServerCount=-1, rest 0) (100%)",
    "dc_texture_s::~dc_texture_s": "dtor releases pd3dtTexture + pddsSurface, deletes pszName, nServerCount=-1 (100%)",
    "DCV_ComputeTextureHash":    "Renamed from bogus DCV_ComputeTextureKey: rolling-XOR hash over texel bytes (cap 8000), switch on tex-type for bytes-per-texel; ceiling is MSVC-vs-SHCL switch layout",
    "DCV_PaletteIsMonochromeQuantized": "Post-increment reads + unsigned counter, break-to-shared-return-0 to match the binary loop shape",
    "DCV_TwiddleBlit16Recurse":  "size/=2 (round-toward-zero, not >>1); ceiling is leaf-store block ordering (SHCL puts it before the loop)",
    "Draw_ConsoleBackground":    "Fixed symbols.tsv mislabel (0x139370 was tagged Draw_SpriteFrameGeneric); now scores 88% (float-arg scheduling in the SetHudDepth calls is the gap)",
    "DCV_PrepSurfacePVR":        "Reads the .PVR header via the new pvrheader_t struct (GBIX+PVRT, 0x20 bytes) instead of raw 0x14/0x18/0x20 offsets; codegen unchanged",
    "Draw_FadeScreen":           "Rewritten to the qgl immediate path (Enable BLEND / Disable TEXTURE_2D, Color4f black 0.8, QUAD, restore) (100%)",
    "DCV_ComputeScaledSize":     "GBIX branch reads pvrheader_t.width/height (signed short); POT-size walk + round_down/round_up axis pick",
    "Draw_Pic":                  "DCV_2D_SetupStates + SetPackedColor(white) + GL_BindStage; inline DCV_AddVertex with depth=dc_depthhud.value-0.1, inline gl UVs",
    "Draw_Pic2":                 "DCV_2D_SetupStates + GL_BindStage (no color); inline DCV_AddVertex depth-0.1, inline gl UVs (89%)",
    "Draw_Frame":                "GL_BindStage + inline DCV_AddVertex depth-0.1 + inline UVs (removed the DCV_2D_AddVertex wrapper the binary never had)",
    "DCV_TwiddleBlit16":         "size==1 degenerate case reads the twiddle_pal16 global (not the pal16 param) for the repeated writes (100%)",
    "AdjustSubRect":             "Inlined ValidateWRect; half-texel epsilon on the UVs + one reciprocal per axis (95%)",
    "DC_FreeTextureByIndex":     "Dropped the out-of-range bounds check the binary never had; early-return form",
    "Draw_MessageFontInfo":      "256-entry charwidth copy from draw_chars->fontinfo, then Font_CharHeight",
    "GL_LoadTexture":            "Reduced to a pure passthrough to DC_LoadTexture (dropped the invented pal-index<<16 encoding the binary never had)",
    "DC_ReleaseTexture":         "Dropped the out-of-range bounds check; decrement nUsageCount, DC_FreeTextureSlot at zero",
    "LoadTransPic":              "MnemoAllocDbg (not MnemoAlloc), first-time flag starts at 1, texnum stored raw (no clamp/mask)",
    "DCV_BuildAlphaGradient4444":"Const RGB from palette entry 255, 16-step alpha ramp (rgb hoisted out of the loop)",
    "DC_TexCache":               "Corrected: takes a name param, caches only slots whose (nameless-fallback) name matches via strcmp",
    "DCV_BuildLumaRemap":        "Post-increment pointer reads + (r+g+b)/3 divlu, unsigned counter (85%)",
    "DC_UncacheOneTexture":      "Fixed field: LRU-walk for a slot with pbCacheData (sysmem copy), not bCached",
    "GL_UnloadTextures":         "Corrected conditions: slot->nServerCount (not the parallel array), <gHostSpawnCount, nUsageCount==0; returns freed count",
    "DC_FreeStaleTextureSlots":  "Corrected: iTextureType==5 (RGB565) gate, not nUsageCount==0",
    "DC_FindTextureSlot":        "Reconstructed the name[3]++ retry loop for same-name/different-size collisions",
    "GL_LoadPicTexture":         "Thin wrapper to GL_LoadTexture with the trailing palette pointer",
    "Draw_SpriteFrameHoles":     "DCV_2D_SetupStates + conditional Blend + Draw_Frame + Opaque (no SetColor) (100%)",
    "Draw_SpriteFrameAdditive":  "DCV_TexState_Additive + Draw_Frame + Opaque (no SetColor) (100%)",
    "Draw_String":               "do-while over the string with the char cached, DCV_TexState_Blend per glyph",
    "DC_FreeTextureByName":      "Nameless-fallback + double-ternary + nServerCount>=0/nUsageCount==0 gate, returns last servercount",
    "DC_ForceFreeTextureByName": "Nameless-fallback + double-ternary strcmp, returns last servercount",
    "DC_GetPaletteIndex":        "Pool scan, CreatePalette on free slot / refcount on match, (short) return",
    "Draw_ConsoleBackground":    "Corrected to call DCV_TexState_Opaque (0x12bcbc), matching the binary (was Additive)",
    # verified 100% (reconstructed in earlier sessions, re-confirmed exact obj==bin)
    "DCV_GetSlot":               "verified 100%: linear slot scan for nServerCount==-1, else Sys_Error",
    "DisableScissorTest":        "verified 100%",
    "Draw_BeginDisc":            "verified 100%: Draw_CenterPic(draw_disc) if present",
    "Draw_Character":            "verified 100%: text render states + Font_DrawCharI",
    "Draw_FillRGBA":             "verified 100%: 93-insn vert-color quad fill",
    "Draw_MessageCharacterAdd":  "verified 100%",
    "Draw_StringLen":            "verified 100%: Font_SetScale + Font_StringWidth",
    "Draw_TextureMode_f":        "verified 100% (2-insn stub)",
    "Draw_TileClear":            "verified 100%",
    "GL_FindTexture":            "verified 100%",
    "GL_PaletteEqual":           "verified 100%: 73-insn GetEntries + per-channel compare",
    "GL_Texels_f":               "verified 100%",
    "GL_UnloadTexture":          "verified 100%: DC_FreeTextureByName wrapper",
    "IntersectWRect":            "verified 100%: 42-insn rect clamp/intersect",
}

# source-only helpers we REMOVED (not present in the binary)
REMOVED = [
    ("DC_UploadIndexed4444",             "Source-only wrapper; binary uses PrepSurfaceIndexed"),
    ("DC_UploadResampledIndexed4444",    "Source-only wrapper"),
    ("DC_UploadResampledIndexed16",      "Source-only wrapper"),
    ("DCV_BuildPalette4444Alpha",        "Source-only; unused after port"),
    ("DCV_CreateTextureSurface16",       "Binary inlines surface creation per-fn"),
    ("DCV_CreateTextureSurfaceCompressedVQ", "Binary inlines create into PrepSurfacePVR"),
    ("DCV_FmtToColor",                   "Source-only adapter; removed"),
]

# functions we converged that are now INLINED (no standalone obj symbol to score)
INLINED = {
    "DCV_ClearCachedFlag": "Rewritten (pPrev walk) + made __inline; now inlined into every PrepSurface*",
    "DCV_TwiddleBlit8":    "Made __inline; now inlined into DCV_PrepSurfacePaletted",
}
scored_names = {r[0] for r in rows}
for nm, note in INLINED.items():
    if nm not in scored_names:
        rows.append([nm, addr.get(nm, ""), None, None, None, None])
        TOUCHED[nm] = note

# sort: touched first (by match desc), then untouched (by match asc = worst first)
def key(r):
    t = r[0] in TOUCHED
    m = r[4] if r[4] is not None else 100.0   # inlined -> treat as done/top
    return (0 if t else 1, -m if t else m)
rows.sort(key=key)

# --- styles ---
ARIAL   = "Arial"
hdr_fill  = PatternFill("solid", fgColor="1F3864")
hdr_font  = Font(name=ARIAL, bold=True, color="FFFFFF", size=11)
title_font= Font(name=ARIAL, bold=True, size=16, color="1F3864")
sub_font  = Font(name=ARIAL, italic=True, size=10, color="555555")
v_fill    = PatternFill("solid", fgColor="C6EFCE")   # green
x_fill    = PatternFill("solid", fgColor="F2F2F2")   # grey
v_font    = Font(name=ARIAL, bold=True, color="0B6E1E", size=11)
x_font    = Font(name=ARIAL, bold=True, color="9C5700", size=11)
cell_font = Font(name=ARIAL, size=10)
mono_font = Font(name=ARIAL, size=10)
thin = Side(style="thin", color="D9D9D9")
border = Border(left=thin, right=thin, top=thin, bottom=thin)

def match_fill(m):
    if m >= 99.9: return PatternFill("solid", fgColor="A9D08E")
    if m >= 60:   return PatternFill("solid", fgColor="C6EFCE")
    if m >= 40:   return PatternFill("solid", fgColor="FFEB9C")
    if m >= 20:   return PatternFill("solid", fgColor="FCE4D6")
    return PatternFill("solid", fgColor="FFC7CE")

wb = Workbook()
ws = wb.active
ws.title = "dc_draw checklist"

ws["A1"] = "Half-Life Dreamcast - dc_draw.c reconstruction checklist"
ws["A1"].font = title_font
ws["A2"] = ("V = reverse-engineered & rewritten this pass   |   X = not started   |   "
            "Exact%% = objdiff obj-vs-binary   |   Struct%% = same instruction set "
            "ignoring register/spill choices (regalloc-invariant authenticity)")
ws["A2"].font = sub_font
ws["A3"] = "Binary: RE/HALFLIFE_DC.EXE  (SH-4)   |   Source: src/render/dc_draw.c"
ws["A3"].font = sub_font

HDR_ROW = 5
headers = ["#", "Function", "Binary addr", "Bin insns", "Obj insns", "Exact %",
           "Struct %", "Done", "Notes"]
for c, h in enumerate(headers, 1):
    cell = ws.cell(HDR_ROW, c, h)
    cell.fill = hdr_fill; cell.font = hdr_font
    cell.alignment = Alignment(horizontal="center", vertical="center")
    cell.border = border

r = HDR_ROW + 1
for i, (nm, ad, binn, obj, m, st) in enumerate(rows, 1):
    touched = nm in TOUCHED
    inlined = m is None
    vals = [i, nm, ("0x"+ad) if ad else "",
            binn if binn is not None else "",
            obj if obj is not None else "",
            (m/100.0) if m is not None else "inlined",
            (st/100.0) if st is not None else "inlined",
            "V" if touched else "X", TOUCHED.get(nm, "")]
    for c, v in enumerate(vals, 1):
        cell = ws.cell(r, c, v)
        cell.font = cell_font
        cell.border = border
        if c == 1:  cell.alignment = Alignment(horizontal="center")
        if c == 2:  cell.font = Font(name=ARIAL, size=10, bold=touched)
        if c == 3:  cell.font = mono_font; cell.alignment = Alignment(horizontal="left")
        if c in (4, 5): cell.alignment = Alignment(horizontal="center")
        if c in (6, 7):
            cell.alignment = Alignment(horizontal="center")
            v2 = m if c == 6 else st
            if inlined:
                cell.font = Font(name=ARIAL, size=9, italic=True, color="0B6E1E")
                cell.fill = v_fill
            else:
                cell.number_format = "0.0%"
                cell.fill = match_fill(v2)
        if c == 8:
            cell.alignment = Alignment(horizontal="center")
            cell.fill = v_fill if touched else x_fill
            cell.font = v_font if touched else x_font
    r += 1

last = r - 1
# summary block (computed values - static snapshot; LibreOffice recalc unavailable on Windows)
import datetime
n_total = len(rows)
n_done  = sum(1 for x in rows if x[0] in TOUCHED)
n_rem   = n_total - n_done
scored  = [x[4] for x in rows if x[4] is not None]
scoredst= [x[5] for x in rows if x[5] is not None]
n_exact = sum(1 for m in scored if m >= 99.9)
mean_m  = (sum(scored) / len(scored) / 100.0) if scored else 0
mean_st = (sum(scoredst) / len(scoredst) / 100.0) if scoredst else 0
sr = r + 1
summ = [
    ("Total functions (in binary)", n_total, None),
    ("Done (V)",                    n_done,  "0B6E1E"),
    ("Remaining (X)",               n_rem,   "9C5700"),
    ("Exact match (100%)",          n_exact, None),
    ("Mean Exact % (scored fns)",   mean_m,  None),
    ("Mean Struct % (regalloc-invariant)", mean_st, "0B6E1E"),
]
for k, (lbl, val, col) in enumerate(summ):
    lc = ws.cell(sr+k, 2, lbl)
    lc.font = Font(name=ARIAL, bold=True, size=10, color=col) if col else Font(name=ARIAL, bold=True, size=10)
    vc = ws.cell(sr+k, 6, val)
    vc.font = Font(name=ARIAL, bold=True, size=10)
    vc.alignment = Alignment(horizontal="center")
    if lbl.startswith("Mean"):
        vc.number_format = "0.0%"
        ws.cell(sr+k, 6).fill = match_fill(val*100)
ws.cell(sr+len(summ)+1, 2,
        "Snapshot: " + datetime.date.today().isoformat() +
        "   |   inlined = converged but compiled inline (no standalone symbol to score)"
        ).font = sub_font

widths = [4, 34, 13, 9, 9, 9, 9, 7, 60]
for c, w in enumerate(widths, 1):
    ws.column_dimensions[get_column_letter(c)].width = w
ws.freeze_panes = "A%d" % (HDR_ROW + 1)
ws.auto_filter.ref = "A%d:I%d" % (HDR_ROW, last)

# --- sheet 2: removed source-only functions ---
ws2 = wb.create_sheet("Removed (not in binary)")
ws2["A1"] = "Source-only functions removed (binary has no equivalent)"
ws2["A1"].font = title_font
for c, h in enumerate(["Function", "Reason removed"], 1):
    cell = ws2.cell(3, c, h); cell.fill = hdr_fill; cell.font = hdr_font
    cell.alignment = Alignment(horizontal="center"); cell.border = border
for i, (nm, why) in enumerate(REMOVED, 4):
    a = ws2.cell(i, 1, nm); a.font = Font(name=ARIAL, size=10, bold=True); a.border = border
    b = ws2.cell(i, 2, why); b.font = cell_font; b.border = border
ws2.column_dimensions["A"].width = 36
ws2.column_dimensions["B"].width = 52

out = os.path.join(REPO, "utils", "re", "dc_draw_checklist.xlsx")
wb.save(out)
print("wrote", out, "with", len(rows), "functions;", sum(1 for x in rows if x[0] in TOUCHED), "done")
