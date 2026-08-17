"""
Removes a solid/near-black photographic background from a Midjourney-style
"isolated on pure black background" asset, WITHOUT erasing black pixels
that are actually part of the design - e.g. a button plate's empty black
interior, or a circular icon frame's black center hole. Those are enclosed
by a non-black border and never touch the image edge, so a flood fill
seeded only from the border can't reach them - it stops at the border of
whatever bright (gold/silver) ring surrounds them.

Uses PIL.ImageDraw.floodfill (a real flood fill, connectivity-aware - not
a flat "every dark pixel becomes transparent" threshold, which WOULD wipe
out those enclosed design areas too) seeded from many points along all 4
edges, so a background with a soft vignette/gradient (not perfectly
uniform black - e.g. the close-button asset) still gets fully caught: each
seed only has to match the threshold against ITS OWN local background
shade, not the whole image's darkest/lightest corner.

Usage:
    python remove_bg.py <input.png> <output.png> [--threshold 40]

Bump --threshold if leftover background fringe remains (the seed's color
similarity tolerance was too tight); lower it if part of the actual design
started disappearing (the tolerance ate into real dark artwork, e.g. a
black gem or an ink shadow).
"""
import sys
from PIL import Image, ImageDraw

MARKER = (255, 0, 255)  # arbitrary, unlikely to occur in real art - swapped for real transparency at the end


def remove_background(src_path: str, dst_path: str, threshold: int) -> None:
    img = Image.open(src_path).convert("RGBA")
    w, h = img.size

    # Seed points spread along every edge (not just the 4 corners) so a
    # background gradient or a design element that happens to touch one
    # edge doesn't block the fill from catching the rest of the border.
    step_x = max(1, w // 60)
    step_y = max(1, h // 60)
    seeds = set()
    for x in range(0, w, step_x):
        seeds.add((x, 0))
        seeds.add((x, h - 1))
    for y in range(0, h, step_y):
        seeds.add((0, y))
        seeds.add((w - 1, y))

    filled = 0
    for seed in seeds:
        px = img.getpixel(seed)
        if px[:3] == MARKER:
            continue  # already filled by an earlier seed
        ImageDraw.floodfill(img, seed, MARKER + (0,), thresh=threshold)
        filled += 1

    pixels = img.load()
    transparent_count = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if (r, g, b) == MARKER:
                pixels[x, y] = (0, 0, 0, 0)
                transparent_count += 1

    img.save(dst_path)
    total = w * h
    print(f"{src_path} -> {dst_path} ({filled} seed fills, {transparent_count}/{total} px "
          f"transparent = {100*transparent_count/total:.1f}%)")


def main() -> None:
    argv = sys.argv[1:]
    threshold = 40
    if "--threshold" in argv:
        i = argv.index("--threshold")
        threshold = int(argv[i + 1])
        argv = argv[:i] + argv[i + 2:]  # drop both the flag and its value

    args = argv

    if len(args) != 2:
        print(__doc__)
        sys.exit(1)

    remove_background(args[0], args[1], threshold)


if __name__ == "__main__":
    main()
