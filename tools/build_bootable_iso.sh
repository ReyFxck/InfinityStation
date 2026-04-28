#!/usr/bin/env bash
set -euo pipefail

ELF_SRC="${1:-../ps2snes2005_boot.elf}"
DISC_ROOT="${2:-./disc_root}"
ISO_GAME_ID="${ISO_GAME_ID:-SLUS_999.99}"
ISO_GAME_NAME="${ISO_GAME_NAME:-InfinityStation}"
ISO_NAME="${3:-${ISO_GAME_ID}.${ISO_GAME_NAME}.iso}"
WORK_DIR="./boot_iso_root"
# Open PS2 Loader exige que o ELF dentro da ISO se chame exatamente
# como o GAME_ID (sem extensao) e que o BOOT2 do SYSTEM.CNF aponte
# pra esse mesmo nome — caso contrario pinta a tela branca de debug.
ELF_NAME="${ISO_GAME_ID}"

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
BOOT2 = cdrom0:\\${ELF_NAME};1
VER = 1.00
VMODE = NTSC
CNF

# -J / -joliet-long emits a Joliet SVD with the original UCS-2 filenames
# so the launcher renders pretty names; the ELF/SYSTEM.CNF entries on
# the PVD stay 8.3-clean for the PS2 BIOS boot path, which only reads
# the Primary Volume Descriptor.
#
# NUNCA passar `-iso-level 2` ou `-full-iso9660-filenames` aqui: o
# CDVDMAN do Open PS2 Loader assume disco strict ISO9660 level 1 e
# usa um buffer de 14 caracteres por entrada de TOC, entao' nomes
# longos no PVD estouram o buffer e a tela do OPL fica branca antes
# do ELF rodar. Detalhes em modules/iopcore/cdvdman/searchfile.c do
# upstream do OPL.
ISO_FLAGS=(-V INFSTATION_PS2 -sysid PLAYSTATION -A PLAYSTATION -publisher PLAYSTATION -J -joliet-long)
if command -v xorriso >/dev/null 2>&1; then
  xorriso -as mkisofs "${ISO_FLAGS[@]}" -o "$ISO_NAME" "$WORK_DIR"
elif command -v genisoimage >/dev/null 2>&1; then
  genisoimage "${ISO_FLAGS[@]}" -o "$ISO_NAME" "$WORK_DIR"
elif command -v mkisofs >/dev/null 2>&1; then
  mkisofs "${ISO_FLAGS[@]}" -o "$ISO_NAME" "$WORK_DIR"
else
  echo "nenhum gerador de ISO encontrado (xorriso, genisoimage ou mkisofs)" >&2
  exit 1
fi

echo "ISO bootavel gerada: $ISO_NAME"
echo "Conteudo raiz:"
find "$WORK_DIR" -maxdepth 2 | sort
