"""
Encrypts .lua source files into the .enc format ClientScriptLoader.cpp (and,
if ported there too, ScriptLoader.cpp) load directly, and deploys the result
straight to Global Debug\\Lua - no separate copy step.

Format: 5-byte magic "RPMU" + 0x01, then the source XOR'd against a fixed
16-byte repeating key. Must match ScriptCryptKey/ScriptCryptMagic exactly.

Usage: python encrypt_lua.py <file1.lua> [file2.lua ...]
Subfolders (e.g. System\\ScriptCore.lua) are preserved under the deploy
folder. Pass --local too to also drop a copy next to the source (rarely
needed - only the deployed one is what the client actually loads).
"""
import os
import sys

KEY = bytes([
    0x52, 0x65, 0x62, 0x69, 0x72, 0x74, 0x68, 0x4D,
    0x55, 0x32, 0x30, 0x32, 0x36, 0x21, 0x21, 0x21
])
MAGIC = bytes([0x52, 0x50, 0x4D, 0x55, 0x01])  # "RPMU" + 0x01

# Lua-Source and "Global Debug" are always siblings under Source_Client.
DEPLOY_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Global Debug", "Lua")


def encrypt(data: bytes) -> bytes:
    out = bytearray(data)
    for i in range(len(out)):
        out[i] ^= KEY[i % len(KEY)]
    return bytes(out)


def main():
    args = sys.argv[1:]
    write_local = "--local" in args
    files = [a for a in args if a != "--local"]

    if not files:
        print("Usage: python encrypt_lua.py <file1.lua> [file2.lua ...] [--local]")
        sys.exit(1)

    for src in files:
        with open(src, "rb") as f:
            data = f.read()

        encrypted = MAGIC + encrypt(data)
        enc_name = src[:-4] + ".enc" if src.endswith(".lua") else src + ".enc"

        deployed = os.path.join(DEPLOY_DIR, enc_name)
        os.makedirs(os.path.dirname(deployed), exist_ok=True)
        with open(deployed, "wb") as f:
            f.write(encrypted)

        print(f"{src} -> {deployed} ({len(data)} bytes source, {len(encrypted)} bytes encrypted)")

        if write_local:
            with open(enc_name, "wb") as f:
                f.write(encrypted)
            print(f"{src} -> {enc_name} (local copy)")


if __name__ == "__main__":
    main()
