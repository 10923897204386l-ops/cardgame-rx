#!/usr/bin/env python3
from pathlib import Path
from math import sin, cos, pi, hypot
import random
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "assets" / "source"
RUN = ROOT / "assets" / "runtime"
OUT = SRC / "m01f"
W, H = 640, 480
CARD_W, CARD_H = 72, 100


def clamp(v):
    return max(0, min(255, int(v)))


def save_bmp(img, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGB").save(path)


def petal(draw, cx, cy, r, angle, fill, outline=None):
    pts = []
    for i in range(18):
        t = 2 * pi * i / 18
        rr = r * (0.58 + 0.42 * abs(sin(t)))
        x = cx + cos(t + angle) * rr * 0.72
        y = cy + sin(t + angle) * rr * 1.28
        pts.append((x, y))
    draw.polygon(pts, fill=fill, outline=outline)


def flower_layer(size, seed):
    random.seed(seed)
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer, "RGBA")
    clusters = [(-18, 418, 1.0), (604, 426, 0.78), (25, -18, 0.50), (612, -12, 0.45)]
    for cx, cy, scale in clusters:
        for j in range(4):
            ox = random.randint(-18, 34)
            oy = random.randint(-20, 30)
            rr = int((18 + random.randint(0, 8)) * scale)
            c = (cx + ox, cy + oy)
            shadow = (c[0] + 2, c[1] + 3)
            for p in range(5):
                a = 2 * pi * p / 5 + random.random() * 0.2
                petal(d, shadow[0] + cos(a) * rr * 0.65, shadow[1] + sin(a) * rr * 0.65,
                      rr, a, (0, 0, 0, 48))
            for p in range(5):
                a = 2 * pi * p / 5 + random.random() * 0.2
                color = random.choice([(245, 80, 126, 230), (255, 122, 154, 230), (255, 172, 196, 220)])
                petal(d, c[0] + cos(a) * rr * 0.65, c[1] + sin(a) * rr * 0.65, rr, a, color, (150, 42, 70, 155))
            d.ellipse((c[0] - rr * .32, c[1] - rr * .32, c[0] + rr * .32, c[1] + rr * .32),
                      fill=(255, 208, 71, 240), outline=(112, 72, 24, 170))
    for cx, cy, a, s in [(514, 405, .4, .8), (548, 73, -0.8, .65), (70, 388, 1.2, .55), (604, 248, .1, .5)]:
        d.ellipse((cx + 2, cy + 3, cx + 17 * s, cy + 10 * s), fill=(0, 0, 0, 34))
        petal(d, cx, cy, 10 * s, a, (255, 115, 151, 190), (154, 49, 76, 110))
    return layer.filter(ImageFilter.GaussianBlur(0.25))


def make_felt():
    random.seed(20261120)
    img = Image.new("RGB", (W, H))
    pix = img.load()
    for y in range(H):
        for x in range(W):
            dx = (x - W * 0.50) / W
            dy = (y - H * 0.45) / H
            r = hypot(dx * 1.20, dy * 1.65)
            light = 1.05 - 0.95 * min(r, 0.85)
            grain = random.randint(-10, 10) + int(6 * sin(x * 0.23) + 5 * sin((x + y) * 0.075))
            base = (9, 102, 58)
            emerald = (23, 151, 84)
            t = max(0, min(1, light))
            red = base[0] * (1 - t) + emerald[0] * t + grain * 0.28
            green = base[1] * (1 - t) + emerald[1] * t + grain
            blue = base[2] * (1 - t) + emerald[2] * t + grain * 0.45
            pix[x, y] = (clamp(red), clamp(green), clamp(blue))
    img = img.filter(ImageFilter.GaussianBlur(0.35))
    overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay, "RGBA")
    for y in range(0, H, 6):
        od.line((0, y, W, y + random.randint(-2, 2)), fill=(255, 255, 255, 8))
    for x in range(0, W, 7):
        od.line((x, 0, x + random.randint(-2, 2), H), fill=(0, 0, 0, 7))
    img = Image.alpha_composite(img.convert("RGBA"), overlay)
    img = Image.alpha_composite(img, flower_layer((W, H), 771))
    out = img.convert("RGB")
    OUT.mkdir(parents=True, exist_ok=True)
    out.save(OUT / "felt_640x480.png")
    save_bmp(out, RUN / "felt_640x480.bmp")


def make_back(path_png, path_bmp, wine=False):
    bg = Image.new("RGBA", (CARD_W, CARD_H), (14, 43, 108, 255) if not wine else (80, 27, 61, 255))
    d = ImageDraw.Draw(bg, "RGBA")
    for y in range(CARD_H):
        shade = int(20 * (1 - abs((y - CARD_H / 2) / (CARD_H / 2))))
        d.line((0, y, CARD_W, y), fill=(16 + shade, 52 + shade, 125 + shade, 255) if not wine else (90 + shade, 30, 68 + shade, 255))
    d.rounded_rectangle((2, 2, CARD_W - 3, CARD_H - 3), radius=8, outline=(31, 25, 19, 255), width=2)
    d.rounded_rectangle((6, 6, CARD_W - 7, CARD_H - 7), radius=5, outline=(238, 186, 72, 255), width=2)
    d.rounded_rectangle((10, 10, CARD_W - 11, CARD_H - 11), radius=4, outline=(255, 225, 130, 180), width=1)
    for cx, cy, s in [(22, 27, 1), (50, 27, -1), (22, 73, -1), (50, 73, 1), (36, 50, 1)]:
        for p in range(5):
            a = p * 2 * pi / 5
            col = (230, 166, 62, 230) if p % 2 else (251, 213, 104, 240)
            petal(d, cx + cos(a) * 7 * s, cy + sin(a) * 7, 7, a, col, (92, 56, 20, 150))
        d.ellipse((cx - 3, cy - 3, cx + 3, cy + 3), fill=(255, 108, 142, 230))
    for i in range(9, CARD_H - 9, 11):
        d.arc((16, i - 5, 56, i + 13), 190, 350, fill=(246, 196, 85, 150), width=1)
    path_png.parent.mkdir(parents=True, exist_ok=True)
    bg.convert("RGB").save(path_png)
    save_bmp(bg, path_bmp)


def warm_faces():
    src_dir = SRC / "kennard-source"
    out_src = OUT / "faces_72x100"
    out_run = RUN / "faces_72x100"
    out_src.mkdir(parents=True, exist_ok=True)
    out_run.mkdir(parents=True, exist_ok=True)
    for png in src_dir.glob("*.png"):
        if len(png.stem) not in (2,):
            continue
        im = Image.open(png).convert("RGBA").resize((CARD_W, CARD_H), Image.Resampling.LANCZOS)
        pix = im.load()
        for y in range(CARD_H):
            for x in range(CARD_W):
                r, g, b, a = pix[x, y]
                if a and r > 232 and g > 232 and b > 225:
                    pix[x, y] = (255, 247, 224, a)
                elif a and r < 45 and g < 45 and b < 45:
                    pix[x, y] = (18, 18, 20, a)
                elif a and r > 135 and g < 75 and b < 75:
                    pix[x, y] = (196, 18, 33, a)
        canvas = Image.new("RGBA", (CARD_W, CARD_H), (0, 0, 0, 0))
        d = ImageDraw.Draw(canvas, "RGBA")
        d.rounded_rectangle((0, 0, CARD_W - 1, CARD_H - 1), radius=8, fill=(255, 247, 224, 255), outline=(30, 27, 24, 255), width=2)
        d.rounded_rectangle((3, 3, CARD_W - 4, CARD_H - 4), radius=6, outline=(125, 104, 79, 120), width=1)
        canvas.alpha_composite(im)
        mask = Image.new("L", (CARD_W, CARD_H), 0)
        ImageDraw.Draw(mask).rounded_rectangle((0, 0, CARD_W - 1, CARD_H - 1), radius=8, fill=255)
        canvas.putalpha(mask)
        canvas.convert("RGB").save(out_src / png.name)
        save_bmp(canvas, out_run / f"{png.stem}.bmp")


def main():
    make_felt()
    make_back(OUT / "back_navy_72x100.png", RUN / "backs" / "back_navy_72x100.bmp", False)
    make_back(OUT / "back_wine_72x100.png", RUN / "backs" / "back_wine_72x100.bmp", True)
    warm_faces()


if __name__ == "__main__":
    main()
