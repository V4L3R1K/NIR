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
SIZE = 512

color_names = list(COLORS.keys())

def lerp(a, b, t):
    """Linear interpolation between two tuples."""
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

for i in range(len(color_names)):
    for j in range(i + 1, len(color_names)):
        name_a = color_names[i]
        name_b = color_names[j]
        rgb_a = COLORS[name_a]
        rgb_b = COLORS[name_b]

        img = Image.new("RGB", (SIZE, SIZE))
        pixels = img.load()

        for y in range(SIZE):
            for x in range(SIZE):
                # diagonal position: t = (x + y) / (2 * (SIZE - 1))
                t = (x + y) / (2.0 * (SIZE - 1))
                t = min(t, 1.0)
                pixels[x, y] = lerp(rgb_a, rgb_b, t)

        filename = f"{name_a}_{name_b}.png"
        path = os.path.join(OUT_DIR, filename)
        img.save(path)
        print(f"Generated {path}")

print("Done.")