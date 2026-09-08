#!/usr/bin/env python3
"""Decode the PVR textures carried inside a Dreamcast studio model.

The Dreamcast build stores skins as GBIX/PVRT blobs in place of the 8-bit
pixels + palette a PC studio model uses.  This decodes the two data formats
the port actually ships -- PVR_RECT (linear 16-bit) and PVR_VQ (2x2 vector
quantised, twiddled) -- so the encoder in studiomdl can be checked against
real data.

Usage:
  python utils/re/pvrdump.py model.mdl [outdir]
  python utils/re/pvrdump.py texture.pvr [outdir]
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from PIL import Image

PVR_TWIDDLE = 0x01
PVR_TWIDDLED_MIPMAP = 0x02
PVR_VQ = 0x03
PVR_VQ_MIPMAP = 0x04
PVR_RECT = 0x09

PVR_ARGB1555 = 0x00
PVR_RGB565 = 0x01
PVR_ARGB4444 = 0x02

DATA_FORMAT_NAMES = {
    PVR_TWIDDLE: "twiddled",
    PVR_TWIDDLED_MIPMAP: "twiddled_mipmap",
    PVR_VQ: "vq",
    PVR_VQ_MIPMAP: "vq_mipmap",
    PVR_RECT: "rect",
}

PIXEL_FORMAT_NAMES = {
    PVR_ARGB1555: "argb1555",
    PVR_RGB565: "rgb565",
    PVR_ARGB4444: "argb4444",
}


def untwiddle_index(x: int, y: int) -> int:
    """Morton order used by the PowerVR2, with v in the low bit."""
    value = 0
    for bit in range(16):
        value |= ((y >> bit) & 1) << (2 * bit)
        value |= ((x >> bit) & 1) << (2 * bit + 1)
    return value


def decode_texel(texel: int, pixel_format: int) -> tuple[int, int, int, int]:
    if pixel_format == PVR_RGB565:
        r = (texel >> 11) & 0x1F
        g = (texel >> 5) & 0x3F
        b = texel & 0x1F
        return (r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2), 255
    if pixel_format == PVR_ARGB1555:
        a = 255 if texel & 0x8000 else 0
        r = (texel >> 10) & 0x1F
        g = (texel >> 5) & 0x1F
        b = texel & 0x1F
        return (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), a
    if pixel_format == PVR_ARGB4444:
        a = (texel >> 12) & 0xF
        r = (texel >> 8) & 0xF
        g = (texel >> 4) & 0xF
        b = texel & 0xF
        return r * 17, g * 17, b * 17, a * 17
    raise ValueError("unsupported pixel format 0x%02x" % pixel_format)


def decode_pvr(blob: bytes) -> tuple[Image.Image, dict]:
    offset = 0
    global_index = None
    if blob[:4] == b"GBIX":
        length = struct.unpack_from("<i", blob, 4)[0]
        global_index = struct.unpack_from("<I", blob, 8)[0]
        offset = 8 + length
    if blob[offset:offset + 4] != b"PVRT":
        raise ValueError("no PVRT header")

    data_size = struct.unpack_from("<i", blob, offset + 4)[0]
    pixel_format = blob[offset + 8]
    data_format = blob[offset + 9]
    width, height = struct.unpack_from("<hh", blob, offset + 12)
    payload = blob[offset + 16:offset + 16 + data_size - 8]

    info = {
        "global_index": global_index,
        "pixel_format": PIXEL_FORMAT_NAMES.get(pixel_format, hex(pixel_format)),
        "data_format": DATA_FORMAT_NAMES.get(data_format, hex(data_format)),
        "width": width,
        "height": height,
        "payload": len(payload),
    }

    image = Image.new("RGBA", (width, height))
    pixels = image.load()

    if data_format == PVR_RECT:
        for y in range(height):
            for x in range(width):
                texel = struct.unpack_from("<H", payload, (y * width + x) * 2)[0]
                pixels[x, y] = decode_texel(texel, pixel_format)
    elif data_format in (PVR_TWIDDLE, PVR_TWIDDLED_MIPMAP):
        base = 0 if data_format == PVR_TWIDDLE else 2
        for y in range(height):
            for x in range(width):
                texel = struct.unpack_from("<H", payload, base + untwiddle_index(x, y) * 2)[0]
                pixels[x, y] = decode_texel(texel, pixel_format)
    elif data_format in (PVR_VQ, PVR_VQ_MIPMAP):
        codebook = payload[:2048]
        indices = payload[2048:]
        if data_format == PVR_VQ_MIPMAP:
            # the chain runs smallest first with no padding between levels, so
            # skip past every level below this one to reach the top
            skip = 0
            size = 1
            while size < width:
                skip += max(1, (size // 2) * (size // 2))
                size *= 2
            indices = indices[skip:]
        for y in range(0, height, 2):
            for x in range(0, width, 2):
                entry = indices[untwiddle_index(x >> 1, y >> 1)] * 8
                for sub in range(4):
                    texel = struct.unpack_from("<H", codebook, entry + sub * 2)[0]
                    pixels[x + (sub >> 1), y + (sub & 1)] = decode_texel(texel, pixel_format)
    else:
        raise ValueError("unsupported data format 0x%02x" % data_format)

    return image, info


def textures_in_model(data: bytes):
    if data[:4] not in (b"IDST", b"idst"):
        return
    count = struct.unpack_from("<i", data, 180)[0]
    index = struct.unpack_from("<i", data, 184)[0]
    for i in range(max(0, count)):
        base = index + i * 80
        name = data[base:base + 64].split(b"\0")[0].decode("ascii", "replace")
        offset = struct.unpack_from("<i", data, base + 76)[0]
        if data[offset:offset + 4] == b"GBIX":
            yield name, data[offset:]


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    path = Path(sys.argv[1])
    outdir = Path(sys.argv[2]) if len(sys.argv) > 2 else path.parent
    outdir.mkdir(parents=True, exist_ok=True)
    data = path.read_bytes()

    if data[:4] in (b"IDST", b"idst"):
        blobs = list(textures_in_model(data))
        if not blobs:
            print("%s: no PVR textures" % path.name)
            return 0
    else:
        blobs = [(path.stem, data)]

    for name, blob in blobs:
        image, info = decode_pvr(blob)
        out = outdir / ("%s_%s.png" % (path.stem, Path(name).stem))
        image.save(out)
        print("%-28s %4dx%-4d %-8s %-16s %7d bytes -> %s"
              % (name, info["width"], info["height"], info["pixel_format"],
                 info["data_format"], info["payload"], out.name))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
