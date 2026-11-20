from __future__ import annotations
from collections import deque
from pathlib import Path
from PIL import Image, ImageDraw
import hashlib, json

ROOT = Path(__file__).resolve().parent
ORIG = ROOT / 'original'
OUT72 = ROOT / 'faces_72x100'
OUT144 = ROOT / 'faces_144x200'
RUNTIME72 = ROOT.parent.parent / 'runtime' / 'faces_72x100'
CONTACT = ROOT / 'contact_sheet_52.png'
RUNTIME_CONTACT = ROOT / 'croupier_card_fronts_v1_25pct_contact_sheet.png'
HASHES = ROOT / 'SHA256.json'
PKG = ROOT / 'PACKAGE_SHA256.txt'
BASE_PAPER = (250, 236, 183)
NAMES = [r+s for s in 'SHDC' for r in 'A23456789TJQK']


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def close_enough(rgb, seed, tol=36):
    return sum(abs(int(a)-int(b)) for a, b in zip(rgb, seed)) <= tol


def paper_mask(img: Image.Image):
    rgba = img.convert('RGBA')
    w, h = rgba.size
    px = rgba.load()
    seeds = [(12, 12), (w-13, 12), (12, h-13), (w-13, h-13)]
    seen = [[False] * h for _ in range(w)]
    mask = Image.new('L', (w, h), 0)
    mp = mask.load()
    for sx, sy in seeds:
        r, g, b, a = px[sx, sy]
        if a == 0:
            continue
        seed = (r, g, b)
        q = deque([(sx, sy)])
        while q:
            x, y = q.popleft()
            if x < 0 or y < 0 or x >= w or y >= h or seen[x][y]:
                continue
            seen[x][y] = True
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            rgb = (r, g, b)
            if r < 180 or g < 170 or b < 130:
                continue
            if not close_enough(rgb, seed):
                continue
            mp[x, y] = 255
            q.append((x + 1, y))
            q.append((x - 1, y))
            q.append((x, y + 1))
            q.append((x, y - 1))
    return mask


def apply_lighting(src: Path, dst: Path):
    img = Image.open(src).convert('RGBA')
    w, h = img.size
    mask = paper_mask(img)
    px = img.load()
    mp = mask.load()
    for y in range(h):
        for x in range(w):
            if mp[x, y] == 0:
                continue
            r, g, b, a = px[x, y]
            nx = x / max(1, w - 1)
            ny = y / max(1, h - 1)
            hi = max(0.0, 1.0 - (0.78 * nx + 0.88 * ny))
            sh = max(0.0, (0.82 * nx + 0.92 * ny) - 0.60)
            rr = r * 0.58 + BASE_PAPER[0] * 0.42
            gg = g * 0.58 + BASE_PAPER[1] * 0.42
            bb = b * 0.58 + BASE_PAPER[2] * 0.42
            rr += 12.0 * hi - 22.0 * sh
            gg += 10.0 * hi - 20.0 * sh
            bb += 3.0 * hi - 10.0 * sh
            px[x, y] = (max(0, min(255, int(round(rr)))),
                        max(0, min(255, int(round(gg)))),
                        max(0, min(255, int(round(bb)))), a)
    dst.parent.mkdir(parents=True, exist_ok=True)
    img.save(dst, optimize=True)


def png_to_runtime_bmp(src: Path, dst: Path):
    img = Image.open(src).convert('RGBA')
    bg = Image.new('RGB', img.size, (0, 0, 0))
    bg.paste(img, mask=img.getchannel('A'))
    dst.parent.mkdir(parents=True, exist_ok=True)
    bg.save(dst)


def build_contact_sheet():
    cards = []
    for name in NAMES:
        cards.append((name, Image.open(OUT72 / f'{name}.png').convert('RGBA')))
    sheet = Image.new('RGB', (13 * 85 + 12, 4 * 120 + 20), (238, 236, 226))
    draw = ImageDraw.Draw(sheet)
    for i, (name, im) in enumerate(cards):
        x = 12 + (i % 13) * 85
        y = 8 + (i // 13) * 120
        sheet.paste(im, (x, y), im)
        draw.text((x + 17, y + 101), name, fill='black')
    sheet.save(CONTACT, optimize=True)
    sheet.save(RUNTIME_CONTACT, optimize=True)


def main():
    for name in NAMES:
        for folder, outfolder in [('faces_72x100', OUT72), ('faces_144x200', OUT144)]:
            src = ORIG / folder / f'{name}.png'
            dst = outfolder / f'{name}.png'
            apply_lighting(src, dst)
        png_to_runtime_bmp(OUT72 / f'{name}.png', RUNTIME72 / f'{name}.bmp')
    build_contact_sheet()
    manifest = {}
    for path in sorted(list(OUT72.glob('*.png')) + list(OUT144.glob('*.png')) + [CONTACT, RUNTIME_CONTACT] + list(RUNTIME72.glob('*.bmp'))):
        manifest[str(path.relative_to(ROOT.parent.parent.parent))] = sha256(path)
    HASHES.write_text(json.dumps(manifest, indent=2) + '\n')
    PKG.write_text('croupier_card_fronts_v1_25pct warm-lighting rebuild\n')
    PKG.write_text(PKG.read_text() + '\n'.join(f'{k}  {v}' for k, v in sorted(manifest.items())) + '\n')
    print('rebuilt', len(manifest), 'artifacts')


if __name__ == '__main__':
    main()
