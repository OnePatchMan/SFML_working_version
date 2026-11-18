#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

if [ -f "./MapEditor" ]; then
    echo "[START] Запуск графического редактора карт MapEditor..."
    ./MapEditor "$@"
elif [ -f "build/MapEditor" ]; then
    echo "[START] Запуск графического редактора карт MapEditor..."
    ./build/MapEditor "$@"
else
    echo "[INFO] Сборка автономного MapEditor..."
    cmake -B build -S .
    cmake --build build --target MapEditor -j$(nproc)
    ./build/MapEditor "$@"
fi

