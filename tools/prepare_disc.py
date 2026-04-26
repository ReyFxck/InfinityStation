#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from pathlib import Path

SUPPORTED_DISC = {".SMC", ".SFC", ".SWC", ".FIG"}
SKIP_ON_DISC = {".ZIP"}

def normalize_rel_for_iso(rel: Path) -> str:
    return "/".join(part.upper() for part in rel.parts)

def main() -> int:
    if len(sys.argv) != 3:
        print("uso: prepare_disc.py <roms_dir> <disc_root>", file=sys.stderr)
        return 1

    roms_dir = Path(sys.argv[1]).expanduser().resolve()
    disc_root = Path(sys.argv[2]).expanduser().resolve()
    out_roms = disc_root / "ROMS"

    if not roms_dir.exists() or not roms_dir.is_dir():
        print(f"diretorio de ROMs invalido: {roms_dir}", file=sys.stderr)
        return 1

    if disc_root.exists():
        shutil.rmtree(disc_root)

    out_roms.mkdir(parents=True, exist_ok=True)

    copied_rel: list[str] = []
    seen_upper: set[str] = set()
    skipped_zip = 0
    skipped_other = 0

    for src in sorted(roms_dir.rglob("*")):
        if src.is_dir():
            continue

        rel = src.relative_to(roms_dir)
        ext = src.suffix.upper()

        if ext in SKIP_ON_DISC:
            skipped_zip += 1
            print(f"[SKIP ZIP] {rel.as_posix()}")
            continue

        if ext not in SUPPORTED_DISC:
            skipped_other += 1
            continue

        iso_rel = normalize_rel_for_iso(rel)
        if iso_rel in seen_upper:
            print(f"conflito apos upper-case: {iso_rel}", file=sys.stderr)
            return 1
        seen_upper.add(iso_rel)

        dst = out_roms / Path(*iso_rel.split("/"))
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)

        copied_rel.append(iso_rel)
        print(f"[COPY] {rel.as_posix()} -> {iso_rel}")

    readme = disc_root / "README.TXT"
    readme.write_text(
        "InfinityStation DVD layout\n"
        "--------------------------\n"
        "Supported disc ROMs live under /ROMS.\n"
        "This build uses native ISO9660 listing for /ROMS.\n"
        "Disc builds intentionally skip ZIP to avoid slow optical load paths.\n"
        "Supported on disc: .SMC .SFC .SWC .FIG\n",
        encoding="ascii",
    )

    print()
    print(f"Pronto: {disc_root}")
    print(f"Copiados: {len(copied_rel)}")
    print(f"ZIP ignorados: {skipped_zip}")
    print(f"Outros ignorados: {skipped_other}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
