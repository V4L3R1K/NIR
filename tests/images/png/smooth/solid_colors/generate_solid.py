#!/usr/bin/env python3
from PIL import Image
import os

COLORS = {
    "red":     (255, 0, 0),
    "green":   (0, 255, 0),
    "blue":    (0, 0, 255),
    "white":   (255, 255, 255),
    "black":   (0, 0, 0),
    "yellow":  (255, 255, 0),
    "magenta": (255, 0, 255),
    "cyan":    (0, 255, 255),
}

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

for name, rgb in COLORS.items():
    img = Image.new("RGB", (512, 512), rgb)
    path = os.path.join(OUT_DIR, f"{name}.png")
    img.save(path)
    print(f"Generated {path}")

print("Done.")