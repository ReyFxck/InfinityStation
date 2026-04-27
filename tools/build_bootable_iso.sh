#!/usr/bin/env bash
set -euo pipefail

ELF_SRC="${1:-../ps2snes2005_boot.elf}"
DISC_ROOT="${2:-./disc_root}"
ISO_NAME="${3:-InfinityStation-BOOT.iso}"
WORK_DIR="./boot_iso_root"
ELF_NAME="INFST.ELF"

if [ ! -f "$ELF_SRC" ]; then
  echo "ELF nao encontrado: $ELF_SRC" >&2
  exit 1
fi

if [ ! -d "$DISC_ROOT" ]; then
  echo "disc root nao encontrado: $DISC_ROOT" >&2
  exit 1
fi

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"

cp -a "$DISC_ROOT"/. "$WORK_DIR"/
cp -f "$ELF_SRC" "$WORK_DIR/$ELF_NAME"

cat > "$WORK_DIR/SYSTEM.CNF" <<CNF
BOOT2 = cdrom0:\\$ELF_NAME;1
VER = 1.0
VMODE = NTSC
CNF

# -J / -joliet-long emits a Joliet SVD with the original UCS-2 filenames
# so the launcher renders pretty names; the ELF/SYSTEM.CNF entries on
# the PVD stay 8.3-clean for the PS2 BIOS boot path, which only reads
# the Primary Volume Descriptor.
if command -v xorriso >/dev/null 2>&1; then
  xorriso -as mkisofs -V INFSTATION_PS2 -J -joliet-long -o "$ISO_NAME" "$WORK_DIR"
elif command -v genisoimage >/dev/null 2>&1; then
  genisoimage -V INFSTATION_PS2 -J -joliet-long -o "$ISO_NAME" "$WORK_DIR"
elif command -v mkisofs >/dev/null 2>&1; then
  mkisofs -V INFSTATION_PS2 -J -joliet-long -o "$ISO_NAME" "$WORK_DIR"
else
  echo "nenhum gerador de ISO encontrado (xorriso, genisoimage ou mkisofs)" >&2
  exit 1
fi

echo "ISO bootavel gerada: $ISO_NAME"
echo "Conteudo raiz:"
find "$WORK_DIR" -maxdepth 2 | sort
