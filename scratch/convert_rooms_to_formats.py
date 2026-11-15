#!/usr/bin/env python3
import os
from PIL import Image

rooms_dir = "assets/maps"

# Color palette for PNG maps:
# Wall: Black (0, 0, 0)
# Platform: Green (76, 175, 80)
# Door: Cyan (0, 229, 255)
# Player: Blue (0, 128, 255)
# Enemy: Red (255, 0, 0)
# Air: White (255, 255, 255)

char_to_color = {
    '#': (0, 0, 0),
    '2': (0, 0, 0),
    '=': (76, 175, 80),
    '1': (76, 175, 80),
    '-': (76, 175, 80),
    'D': (0, 229, 255),
    '3': (0, 229, 255),
    'P': (0, 128, 255),
    'E': (255, 0, 0),
    ' ': (255, 255, 255),
    '.': (255, 255, 255),
}

for r in range(1, 11):
    txt_path = os.path.join(rooms_dir, f"room_{r}.txt")
    if not os.path.exists(txt_path):
        continue

    with open(txt_path, "r") as f:
        lines = [line.rstrip('\r\n') for line in f if not line.startswith('//')]
    while lines and len(lines[-1].strip()) == 0:
        lines.pop()

    h = len(lines)
    w = max(len(line) for line in lines)

    # 1. Create PNG map
    img = Image.new("RGB", (w, h), (255, 255, 255))
    for y, line in enumerate(lines):
        for x, char in enumerate(line):
            color = char_to_color.get(char, (255, 255, 255))
            img.putpixel((x, y), color)

    png_path = os.path.join(rooms_dir, f"room_{r}.png")
    img.save(png_path)

    # 2. Create Coordinate KV file
    kv_path = os.path.join(rooms_dir, f"room_{r}.kv")
    
    # Extract platforms runs
    platforms = []
    for y, line in enumerate(lines):
        cur_start = None
        for x, char in enumerate(line):
            if char in ('=', '1', '-'):
                if cur_start is None:
                    cur_start = x
            else:
                if cur_start is not None:
                    platforms.append((cur_start, y, x - cur_start))
                    cur_start = None
        if cur_start is not None:
            platforms.append((cur_start, y, len(line) - cur_start))

    # Extract spawns and doors
    p_spawn = None
    e_spawn = None
    doors = []
    for y, line in enumerate(lines):
        for x, char in enumerate(line):
            if char == 'P': p_spawn = (x, y)
            elif char == 'E': e_spawn = (x, y)
            elif char == 'D': doors.append((x, y))

    with open(kv_path, "w") as f:
        f.write(f'// =========================================================================\n')
        f.write(f'// КООРДИНАТНЫЙ КОНФИГ КОМНАТЫ {r} (Удобное блочное редактирование)\n')
        f.write(f'// =========================================================================\n\n')
        f.write(f'"Room"\n{{\n')
        f.write(f'    "width"   "{w}"\n')
        f.write(f'    "height"  "{h}"\n\n')
        if p_spawn:
            f.write(f'    "player_spawn"  "{p_spawn[0]} {p_spawn[1]}"\n')
        if e_spawn:
            f.write(f'    "enemy_spawn"   "{e_spawn[0]} {e_spawn[1]}"\n')
        if doors:
            f.write(f'    "door"          "{doors[0][0]} {doors[0][1]}"\n')
        f.write(f'\n    // Список платформ: [x  y  длина]\n')
        f.write(f'    "Platforms"\n    {{\n')
        for i, (px, py, plen) in enumerate(platforms):
            name = "floor" if py >= h - 5 else f"plat_{i}"
            f.write(f'        "{name}"  "{px} {py} {plen}"\n')
        f.write(f'    }}\n')
        f.write(f'}}\n')

print("Created 10 PNG image maps and 10 coordinate KV files successfully!")

