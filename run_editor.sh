#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

if [ ! -f "build/MapEditor" ]; then
    echo "[INFO] Сборка автономного MapEditor..."
    cmake -B build -S .
    cmake --build build --target MapEditor -j$(nproc)
fi

echo "[START] Запуск графического редактора карт MapEditor..."
./build/MapEditor "$@"
