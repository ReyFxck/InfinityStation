#!/usr/bin/env bash
set -e

cd "$(dirname "$0")"

rm -f main.o ps2_video.o smw_blob.o ps2snes2005_boot.elf
make
cp -f ps2snes2005_boot.elf /sdcard/ps2/
ls -lh ps2snes2005_boot.elf /sdcard/ps2/ps2snes2005_boot.elf
