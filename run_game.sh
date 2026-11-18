#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

if [ ! -f "build/FightingComboArena" ]; then
    echo "[INFO] Сборка игры FightingComboArena..."
    cmake -B build -S .
    cmake --build build --target FightingComboArena -j$(nproc)
fi

echo "[START] Запуск игры FightingComboArena..."
./build/FightingComboArena "$@"

