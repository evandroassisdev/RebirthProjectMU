"""
Encrypts .lua source files into the .enc format ClientScriptLoader.cpp (and,
if ported there too, ScriptLoader.cpp) load directly.

Format: 5-byte magic "RPMU" + 0x01, then the source XOR'd against a fixed
16-byte repeating key. Must match ScriptCryptKey/ScriptCryptMagic exactly.

Usage: python encrypt_lua.py <file1.lua> [file2.lua ...]
Writes <fileN>.enc next to each input file.
"""
import sys

KEY = bytes([
    0x52, 0x65, 0x62, 0x69, 0x72, 0x74, 0x68, 0x4D,
    0x55, 0x32, 0x30, 0x32, 0x36, 0x21, 0x21, 0x21
])
MAGIC = bytes([0x52, 0x50, 0x4D, 0x55, 0x01])  # "RPMU" + 0x01


def encrypt(data: bytes) -> bytes:
    out = bytearray(data)
    for i in range(len(out)):
        out[i] ^= KEY[i % len(KEY)]
    return bytes(out)


def main():
    if len(sys.argv) < 2:
        print("Usage: python encrypt_lua.py <file1.lua> [file2.lua ...]")
        sys.exit(1)

    for src in sys.argv[1:]:
        with open(src, "rb") as f:
            data = f.read()

        dst = src[:-4] + ".enc" if src.endswith(".lua") else src + ".enc"

        with open(dst, "wb") as f:
            f.write(MAGIC)
            f.write(encrypt(data))

        print(f"{src} -> {dst} ({len(data)} bytes source, {len(MAGIC) + len(data)} bytes encrypted)")


if __name__ == "__main__":
    main()
