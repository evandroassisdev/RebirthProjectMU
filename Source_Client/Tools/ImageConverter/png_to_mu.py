"""
Converts a normal image (PNG/JPG/anything Pillow can open) into this
client's .OZT/.OZJ formats, ready to drop into Data\\<folder>\\ and load
via Lua's LoadImage("<folder>\\<name>") (ClientLuaFunction.cpp) - or any
native ::LoadBitmap() call.

Format specs below were reverse-engineered by reading this project's own
loader, GlobalBitmap.cpp (CGlobalBitmap::OpenTga/OpenJpeg), byte-for-byte -
not guessed, and cross-checked by decoding a real shipped asset
(Data\\Interface\\op1_back1.OZT) and comparing the result against an actual
in-game screenshot of that same texture rendered on screen. Both matched
with no row/channel flip needed beyond what's described here.

OZT (use for anything needing transparency - rounded corners, cutouts,
soft edges, etc.):
    bytes  0-15  : unused/ignored by the loader - written as zero here
    bytes 16-17  : width  (int16, little-endian)
    bytes 18-19  : height (int16, little-endian)
    byte   20    : bit depth - must be exactly 32 (OpenTga rejects anything
                   else) - always write 32
    byte   21    : unused/ignored - written as zero here
    bytes 22+    : pixel data, height rows x width pixels, 4 bytes/pixel,
                   BGRA byte order (NOT RGBA - confirmed: OpenTga does
                   dst.rgba = src[2],src[1],src[0],src[3], i.e. it reads
                   BGRA from the file and converts to RGBA in memory), rows
                   in the same top-to-bottom order as a normal image (no
                   manual vertical flip needed - OpenTga's own y-flip when
                   building the GL texture buffer is exactly cancelled out
                   by the v-coordinate flip in RenderBitmap/ZzzOpenglUtil.cpp,
                   confirmed by decoding+comparing a real asset, see above)

OZJ (use for fully opaque art - no transparency support at all, this
format has no alpha channel):
    bytes 0-23   : unused/ignored by the loader (OpenJpeg does
                   fseek(infile, 24, SEEK_SET) and never reads them) -
                   written as zero here
    bytes 24+    : a completely standard JPEG file, byte for byte - no
                   transformation needed, confirmed by extracting and
                   viewing Data\\Interface\\op1_stone.OZJ's real content
                   this same session

Usage:
    python png_to_mu.py <input.png> <output.OZT>
    python png_to_mu.py <input.png> <output.OZJ> [--quality 90]

The output extension (case-insensitive .OZT or .OZJ) picks the format -
matches this client's own on-disk naming convention (real assets are
ALWAYS uppercase .OZT/.OZJ, e.g. op1_back1.OZT).
"""
import io
import os
import struct
import sys

from PIL import Image


def to_ozt(src_path: str, dst_path: str) -> None:
    img = Image.open(src_path).convert("RGBA")
    width, height = img.size

    header = bytearray(22)
    struct.pack_into("<hh", header, 16, width, height)
    header[20] = 32  # bit depth, must be exactly this or OpenTga rejects the file
    header[21] = 0

    pixels = bytearray(width * height * 4)
    src = img.tobytes("raw", "RGBA")
    for i in range(0, len(src), 4):
        r, g, b, a = src[i], src[i + 1], src[i + 2], src[i + 3]
        pixels[i] = b
        pixels[i + 1] = g
        pixels[i + 2] = r
        pixels[i + 3] = a

    with open(dst_path, "wb") as f:
        f.write(header)
        f.write(pixels)

    print(f"{src_path} -> {dst_path} ({width}x{height}, OZT/RGBA)")


def to_ozj(src_path: str, dst_path: str, quality: int) -> None:
    img = Image.open(src_path).convert("RGB")

    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=quality)
    jpeg_bytes = buf.getvalue()

    with open(dst_path, "wb") as f:
        f.write(bytes(24))  # unused header, never read by OpenJpeg
        f.write(jpeg_bytes)

    print(f"{src_path} -> {dst_path} ({img.width}x{img.height}, OZJ/RGB, quality={quality})")
    if img.mode != "RGB" or Image.open(src_path).mode == "RGBA":
        print("  NOTE: source had an alpha channel - OZJ has none, it was dropped. Use .OZT instead if transparency matters here.")


def main() -> None:
    argv = sys.argv[1:]
    quality = 90
    if "--quality" in argv:
        i = argv.index("--quality")
        quality = int(argv[i + 1])
        argv = argv[:i] + argv[i + 2:]  # drop both the flag and its value

    args = argv

    if len(args) != 2:
        print(__doc__)
        sys.exit(1)

    src_path, dst_path = args
    ext = os.path.splitext(dst_path)[1].lower()

    if ext == ".ozt":
        to_ozt(src_path, dst_path)
    elif ext == ".ozj":
        to_ozj(src_path, dst_path, quality)
    else:
        print(f"Unrecognized output extension '{ext}' - use .OZT or .OZJ")
        sys.exit(1)


if __name__ == "__main__":
    main()
