#!/usr/bin/env bash
set -e

DISC_ROOT="${1:-./disc_root}"
ISO_NAME="${2:-InfinityStation-DVD.iso}"

if [ ! -d "$DISC_ROOT" ]; then
  echo "disc root nao encontrado: $DISC_ROOT" >&2
  exit 1
fi

if command -v xorriso >/dev/null 2>&1; then
  xorriso -as mkisofs -V INFSTATION_DVD -o "$ISO_NAME" "$DISC_ROOT"
elif command -v genisoimage >/dev/null 2>&1; then
  genisoimage -V INFSTATION_DVD -o "$ISO_NAME" "$DISC_ROOT"
elif command -v mkisofs >/dev/null 2>&1; then
  mkisofs -V INFSTATION_DVD -o "$ISO_NAME" "$DISC_ROOT"
else
  echo "nenhum gerador de ISO encontrado (xorriso, genisoimage ou mkisofs)" >&2
  exit 1
fi

echo "ISO gerada: $ISO_NAME"
