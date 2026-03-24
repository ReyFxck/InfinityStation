from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
OUT_IMG = ROOT / "assets/libsdpcm_ioprp.img"
OUT_HDR = ROOT / "assets/libsdpcm_ioprp_blob.h"

LIBSD = Path("/root/ps2dev/ps2sdk/iop/irx/libsd.irx")
LIBSDPCM = ROOT / "iop/libsdpcm/irx/libsdpcm.irx"

if not LIBSD.exists():
    raise SystemExit(f"nao achei {LIBSD}")
if not LIBSDPCM.exists():
    raise SystemExit(f"nao achei {LIBSDPCM}")

conf = b"!include IOPBTCONF\nLIBSD\nLIBSDPCM\n"
libsd = LIBSD.read_bytes()
libsdpcm = LIBSDPCM.read_bytes()

def align16(x: bytes) -> bytes:
    pad = (-len(x)) & 15
    return x + (b'\0' * pad)

def romdir_entry(name: str, size: int, extinfo_size: int = 0) -> bytes:
    n = name.encode("ascii")
    if len(n) > 10:
        raise ValueError(f"nome grande demais: {name}")
    return n.ljust(10, b'\0') + struct.pack("<H", extinfo_size) + struct.pack("<I", size)

entries = [
    ("IOPBTCONF", conf),
    ("LIBSD", libsd),
    ("LIBSDPCM", libsdpcm),
]

romdir = b"".join(romdir_entry(name, len(data)) for name, data in entries)
romdir += romdir_entry("", 0)
romdir = align16(romdir)

img = bytearray()
img += romdir
for _, data in entries:
    img += align16(data)

OUT_IMG.write_bytes(img)

with OUT_HDR.open("w", encoding="utf-8") as f:
    f.write("#ifndef LIBSDPCM_IOPRP_BLOB_H\n")
    f.write("#define LIBSDPCM_IOPRP_BLOB_H\n\n")
    f.write("static const unsigned char libsdpcm_ioprp[] = {\n")
    for i in range(0, len(img), 12):
        chunk = img[i:i+12]
        f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk))
        if i + 12 < len(img):
            f.write(",")
        f.write("\n")
    f.write("};\n")
    f.write(f"static const unsigned int libsdpcm_ioprp_len = {len(img)};\n\n")
    f.write("#endif\n")

print(f"ioprp size = {len(img)}")
