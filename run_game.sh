#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

if [ -f "./FightingComboArena" ]; then
    echo "[START] Запуск игры FightingComboArena..."
    ./FightingComboArena "$@"
elif [ -f "build/FightingComboArena" ]; then
    echo "[START] Запуск игры FightingComboArena..."
    ./build/FightingComboArena "$@"
else
    echo "[INFO] Сборка игры FightingComboArena..."
    cmake -B build -S .
    cmake --build build --target FightingComboArena -j$(nproc)
    ./build/FightingComboArena "$@"
fi

