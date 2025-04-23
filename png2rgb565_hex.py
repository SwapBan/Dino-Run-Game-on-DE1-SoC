#!/usr/bin/env python3
"""
Convert a 32×32 sprite to a 1024‑word RGB565 HEX file
compatible with SystemVerilog $readmemh().
Usage:   python png2rgb565_hex.py dino.png dino_sprite.hex
"""

import sys
from pathlib import Path
from PIL import Image

# ---------- Helpers ----------------------------------------------------------
def rgb888_to_rgb565(r, g, b):
    """Return 16‑bit int in 5‑6‑5 format."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# ---------- Main -------------------------------------------------------------
if len(sys.argv) != 3:
    sys.exit("Usage: png2rgb565_hex.py <input.png> <output.hex>")

in_file  = Path(sys.argv[1])
out_file = Path(sys.argv[2])

im = Image.open(in_file).convert("RGBA")            # keep alpha if present
im = im.resize((32, 32), Image.NEAREST)             # force 32×32

# Substitute fully‑transparent pixels with magenta (or any BG colour you like)
BG = (255,   0, 255)
pixels = [
    rgb888_to_rgb565(*(im.getpixel((x, y))[:3] if im.getpixel((x, y))[3] else BG))
    for y in range(32)
    for x in range(32)
]

# Write as plain hex, one 4‑digit word per line
with out_file.open("w") as f:
    for word in pixels:
        f.write(f"{word:04X}\n")

print(f"Wrote {len(pixels)} words to {out_file}")
