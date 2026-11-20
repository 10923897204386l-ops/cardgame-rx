#!/usr/bin/env python3
"""Install approved M0.1F package assets into runtime BMP paths.

Does not regenerate or reinterpret artwork. Converts pinned PNGs under
assets/source/m01f to SDL-loadable BMPs only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "assets" / "source" / "m01f"
RUNTIME = ROOT / "assets" / "runtime"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png_to_bmp(src: Path, dst: Path) -> None:
    image = Image.open(src)
    if image.mode in ("RGBA", "LA"):
        # Preserve historical opaque BMP corner treatment used by prior faces.
        background = Image.new("RGB", image.size, (0, 0, 0))
        background.paste(image, mask=image.split()[-1])
        image = background
    elif image.mode != "RGB":
        image = image.convert("RGB")
    dst.parent.mkdir(parents=True, exist_ok=True)
    image.save(dst, format="BMP")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=SOURCE,
        help="Approved M0.1F source directory (default: assets/source/m01f)",
    )
    parser.add_argument(
        "--runtime",
        type=Path,
        default=RUNTIME,
        help="Runtime asset directory (default: assets/runtime)",
    )
    args = parser.parse_args()
    source = args.source
    runtime = args.runtime

    felt = source / "backgrounds" / "felt_approved_640x480.png"
    back = source / "backs" / "back_tropical_blue_gold_72x100.png"
    faces = sorted((source / "faces_72x100").glob("*.png"))
    if not felt.is_file():
        raise SystemExit(f"missing approved felt: {felt}")
    if not back.is_file():
        raise SystemExit(f"missing approved back: {back}")
    if len(faces) != 52:
        raise SystemExit(f"expected 52 face PNGs, found {len(faces)}")

    manifest_path = source / "manifest_sha256.json"
    if manifest_path.is_file():
        expected = json.loads(manifest_path.read_text())
        checks = {
            "backgrounds/felt_approved_640x480.png": felt,
            "backs/back_tropical_blue_gold_72x100.png": back,
        }
        for face in faces:
            checks[f"faces_72x100/{face.name}"] = face
        for rel, path in checks.items():
            got = sha256(path)
            want = expected.get(rel)
            if want and got != want:
                raise SystemExit(f"hash mismatch for {rel}: got {got}, want {want}")

    png_to_bmp(felt, runtime / "felt_640x480.bmp")
    png_to_bmp(back, runtime / "backs" / "back_navy_72x100.bmp")
    for face in faces:
        png_to_bmp(face, runtime / "faces_72x100" / f"{face.stem}.bmp")

    print("Installed approved M0.1F assets:")
    print("  felt", sha256(runtime / "felt_640x480.bmp"))
    print("  back_navy", sha256(runtime / "backs" / "back_navy_72x100.bmp"))
    print("  faces", len(list((runtime / "faces_72x100").glob("*.bmp"))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
