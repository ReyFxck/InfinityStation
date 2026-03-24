#!/usr/bin/env bash
LOG=build.log

: > "$LOG"

if make -s >"$LOG" 2>&1; then
    if grep -q "warning:" "$LOG"; then
        echo "[OK] build concluido com warnings"
        grep -n "warning:" "$LOG"
    else
        echo "[OK] build concluido"
    fi

    cp -f ps2snes2005_boot.elf /sdcard/PS2/ && echo "[OK] ELF copiado para /sdcard/PS2/"
else
    echo "[ERROR] build falhou"
    grep -nE "warning:|error:" "$LOG" | tail -n 80
    exit 1
fi
