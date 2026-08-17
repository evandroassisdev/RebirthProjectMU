"""
Decodes this client's .OZT/.OZJ formats back into a normal Pillow Image -
the reverse of png_to_mu.py. Same format spec (reverse-engineered from
GlobalBitmap.cpp, cross-verified against a real shipped asset + a round
trip through png_to_mu.py - see that file's own docstring for the byte
layout details).

Usage as a library:
    from mu_decode import load_mu_image
    img = load_mu_image("path/to/asset.OZT")   # or .OZJ - picked by extension

Usage as a CLI (handy for a quick look at what's actually in a deployed
asset without opening the game):
    python mu_decode.py <input.OZT_or_OZJ> <output.png>
"""
import io
import os
import struct
import sys

import numpy as np
from PIL import Image


def _load_ozt(path: str) -> Image.Image:
    with open(path, "rb") as f:
        data = f.read()

    index = 16
    nx, ny = struct.unpack_from("<hh", data, index)
    index += 4
    bit = data[index]
    index += 2  # bit-depth byte + 1 padding byte

    if bit != 32:
        raise ValueError(f"{path}: unexpected bit depth {bit} (expected 32)")

    # Vectorized BGRA->RGBA via numpy instead of a per-pixel Python loop -
    # the loop was fine for a one-off asset but far too slow to decode
    # hundreds of files for a browsable list (a 1024x1024 asset is 1M+
    # iterations of pure-Python pixel assignment).
    pixel_bytes = np.frombuffer(data, dtype=np.uint8, count=nx * ny * 4, offset=index)
    bgra = pixel_bytes.reshape((ny, nx, 4))
    rgba = bgra[:, :, [2, 1, 0, 3]]

    return Image.fromarray(rgba, mode="RGBA")


def _load_ozj(path: str) -> Image.Image:
    with open(path, "rb") as f:
        data = f.read()

    return Image.open(io.BytesIO(data[24:])).convert("RGB")


def load_mu_image(path: str) -> Image.Image:
    ext = os.path.splitext(path)[1].lower()

    if ext == ".ozt":
        return _load_ozt(path)
    if ext == ".ozj":
        return _load_ozj(path)

    raise ValueError(f"Unrecognized extension '{ext}' - expected .OZT or .OZJ")


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)

    img = load_mu_image(sys.argv[1])
    img.save(sys.argv[2])
    print(f"{sys.argv[1]} -> {sys.argv[2]} ({img.size[0]}x{img.size[1]}, {img.mode})")


if __name__ == "__main__":
    main()
