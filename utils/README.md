# Content tools

The Half-Life SDK utilities, with the changes the Dreamcast port needs.

| directory | what it does |
|---|---|
| `common/` | shared library code, plus `pvrtex.c` for PowerVR texture images |
| `studiomdl/` | compiles `.qc` + `.smd` into a studio model |
| `qlumpy/` | builds a wad from a lump script |
| `makels/` | writes the lump scripts qlumpy reads |

`utils/re/` is the reverse-engineering tooling and is unrelated to these.

## Building

VC6 project files are checked in — open `hltools.dsw` in Developer Studio, or from
a command line:

```bash
utils/build_tools.bat
```

It finds a VC98 installation on its own (set `VCROOT` to override), and drops the
executables in `utils/build/`.  Pass `debug` for an unoptimised build.

## PowerVR textures

The hardware wants powers of two from 8 to 1024 and reads two kinds of image:

- **rectangle** — plain 16 bit RGB565 pixels in scanline order.
- **vector quantized** — a 2048 byte codebook of 256 two-by-two texel blocks
  followed by one byte per block, which is a quarter of the memory.  Only square
  textures can be quantized, and only 64 and up are worth quantizing.  A
  mipmapped one shares its codebook across the whole chain and stores the levels
  smallest first, packed together, with the top level last.

Every image carries a `GBIX`/`PVRT` header and is padded out to a multiple of 32
bytes.  `common/pvrtex.c` writes them.

## studiomdl

Skins can be handed to it already converted, or converted on the way in.

```
$pvrtextures                 convert every skin in this model
studiomdl -p model.qc        the same, from the command line
```

A skin named `foo.pvr` in the `.qc` is loaded as a texture image and copied into
the model whole, whatever the `-p` setting is; the model then takes its skin
dimensions from the image.  Skins that are still `.bmp` are resampled to the
nearest power of two, no larger than 256, and the texture coordinates follow.

Each texture in the model now starts on a long boundary, which is what the engine
means by "run it through a newer studiomdl.exe".

## Neo models

```
$neomodel                    build this model in the Neo format
studiomdl -e model.qc        the same, from the command line
```

A Neo model is marked with a lower case `idst` and is laid out the same as a
normal one except in two places.

Attachments drop the name and the type nothing reads back, leaving the bone and
the offsets: 52 bytes instead of 88.

Animation keeps the same run structure — so many values, then so many frames that
repeat the last of them — but the values are written as a bit stream rather than a
word each.  Each run opens with four bits saying how wide its differences are,
then the first value in full, then the rest as differences from the one before.
Positions keep all sixteen bits; rotations drop their bottom four on the way in,
which is far below what a bone can turn by and is what makes the differences small
enough to be worth packing.  Typical animation comes out at about 40% of its
former size.

A run whose values move too far apart to pack is split so each half can carry its
own first value, so nothing has to be thrown away to fit.

## qlumpy and level wads

`pvr` is a grab command alongside `miptex`:

```
$DEST    "1_C0A0.WAD"
$PVRMAXSIZE 128
$loadbmp "textures\OUT_WALL8.BMP"
OUT_WALL8  pvr -1 -1 -1 -1
```

`$PVRMAXSIZE` caps the largest dimension; a lopsided texture is squared up when
the quantized copy would come out smaller than the flat one.  World textures are
written with a full mip chain, model skins without.

Only one level wad is held in memory at a time, so a level's textures are cut
into pieces of about 128K and the engine pulls in whichever piece holds the
texture it wants.  `makels` does the cutting:

```
makels -bsp maps\c0a0.bsp textures\ scripts\ [-maxsize 128]
```

It reads the texture list out of the map, walks it in map order so that textures
used near each other stay together, and writes `1_C0A0.LS`, `2_C0A0.LS` and so on
for qlumpy to build.  Its original form still works:

```
makels <source directory> <wadfile name> <script name>
```
