#!/usr/bin/env python3
import os
from PIL import Image

maps_dir = "assets/maps"
cfg_dir = "assets/config"
os.makedirs(maps_dir, exist_ok=True)
os.makedirs(cfg_dir, exist_ok=True)

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

def create_base_room(w, h):
    grid = [[' ' for _ in range(w)] for _ in range(h)]
    floor_y = h - 5

    # 1. Ceiling
    for x in range(w):
        grid[0][x] = '#'

    # 2. Floor & Foundation
    for x in range(w):
        grid[floor_y][x] = '='
        for y in range(floor_y + 1, h):
            grid[y][x] = '#'

    # 3. Left and right border walls
    for y in range(1, floor_y):
        if y >= floor_y - 4 and y < floor_y:
            # Open edge passages for seamless transition
            grid[y][0] = ' '
            grid[y][1] = ' '
            grid[y][w - 2] = ' '
            grid[y][w - 1] = ' '
        else:
            grid[y][0] = '#'
            grid[y][1] = '#'
            grid[y][w - 2] = '#'
            grid[y][w - 1] = '#'

    return grid, floor_y

# Definitions for 10 rooms
rooms = []

# Room 1: Main Training Arena (80x20) - 2 enemies
r1, f1 = create_base_room(80, 20)
r1[f1 - 1][6] = 'P'
r1[f1 - 1][35] = 'E'
r1[f1 - 1][55] = 'E'
# Platforms matching exact unit test specs:
for x in range(16, 24): r1[10][x] = '='
for x in range(56, 64): r1[10][x] = '='
for x in range(35, 45): r1[12][x] = '='
rooms.append((1, 80, 20, r1, "Main Training Ground"))

# Room 2: Vertical Tower (80x25) - 3 enemies
r2, f2 = create_base_room(80, 25)
r2[f2 - 1][6] = 'P'
r2[f2 - 1][38] = 'E'
r2[16][20] = 'E'
r2[10][50] = 'E'
for x in range(14, 26): r2[17][x] = '='
for x in range(30, 44): r2[14][x] = '='
for x in range(46, 58): r2[11][x] = '='
for x in range(25, 45): r2[7][x] = '='
rooms.append((2, 80, 25, r2, "Vertical Tower"))

# Room 3: Pillar Hall (80x20) - 2 enemies
r3, f3 = create_base_room(80, 20)
r3[f3 - 1][6] = 'P'
r3[f3 - 1][45] = 'E'
r3[f3 - 1][65] = 'E'
# Pillars
for y in range(6, f3):
    r3[y][22] = '#'; r3[y][23] = '#'
    r3[y][40] = '#'; r3[y][41] = '#'
    r3[y][58] = '#'; r3[y][59] = '#'
for x in range(24, 38): r3[10][x] = '='
for x in range(42, 56): r3[10][x] = '='
rooms.append((3, 80, 20, r3, "Pillar Hall"))

# Room 4: Dungeon Catacombs (90x22) - 3 enemies
r4, f4 = create_base_room(90, 22)
r4[f4 - 1][6] = 'P'
r4[f4 - 1][30] = 'E'
r4[f4 - 1][60] = 'E'
r4[11][45] = 'E'
for x in range(18, 36): r4[13][x] = '='
for x in range(40, 58): r4[11][x] = '='
for x in range(62, 80): r4[13][x] = '='
rooms.append((4, 90, 22, r4, "Dungeon Catacombs"))

# Room 5: The Great Bridge (120x20) - 4 enemies
r5, f5 = create_base_room(120, 20)
r5[f5 - 1][6] = 'P'
r5[f5 - 1][35] = 'E'
r5[f5 - 1][65] = 'E'
r5[f5 - 1][95] = 'E'
r5[10][60] = 'E'
for x in range(25, 45): r5[10][x] = '='
for x in range(50, 70): r5[10][x] = '='
for x in range(75, 95): r5[10][x] = '='
rooms.append((5, 120, 20, r5, "The Great Bridge"))

# Room 6: Duel Coliseum (80x20) - 2 enemies
r6, f6 = create_base_room(80, 20)
r6[f6 - 1][6] = 'P'
r6[f6 - 1][45] = 'E'
r6[f6 - 1][65] = 'E'
for x in range(16, 30): r6[11][x] = '='
for x in range(50, 64): r6[11][x] = '='
for x in range(32, 48): r6[8][x] = '='
rooms.append((6, 80, 20, r6, "Duel Coliseum"))

# Room 7: Platform Gauntlet (90x24) - 3 enemies
r7, f7 = create_base_room(90, 24)
r7[f7 - 1][6] = 'P'
r7[f7 - 1][75] = 'E'
r7[15][30] = 'E'
r7[11][55] = 'E'
for x in range(15, 27): r7[16][x] = '='
for x in range(25, 37): r7[15][x] = '='
for x in range(37, 49): r7[13][x] = '='
for x in range(50, 62): r7[11][x] = '='
for x in range(65, 77): r7[14][x] = '='
rooms.append((7, 90, 24, r7, "Platform Gauntlet"))

# Room 8: Sky Sanctuary (100x24) - 3 enemies
r8, f8 = create_base_room(100, 24)
r8[f8 - 1][6] = 'P'
r8[f8 - 1][40] = 'E'
r8[f8 - 1][80] = 'E'
r8[12][60] = 'E'
for x in range(18, 34): r8[15][x] = '='
for x in range(40, 56): r8[12][x] = '='
for x in range(62, 78): r8[15][x] = '='
for x in range(30, 70): r8[8][x] = '='
rooms.append((8, 100, 24, r8, "Sky Sanctuary"))

# Room 9: Throne Room (85x22) - 2 enemies
r9, f9 = create_base_room(85, 22)
r9[f9 - 1][6] = 'P'
r9[f9 - 1][45] = 'E'
r9[f9 - 1][70] = 'E'
# Throne dais
for x in range(55, 78):
    r9[f9 - 2][x] = '='
    r9[f9 - 1][x] = '#'
for x in range(65, 78):
    r9[f9 - 3][x] = '='
for x in range(18, 35): r9[12][x] = '='
for x in range(38, 52): r9[10][x] = '='
rooms.append((9, 85, 22, r9, "Throne Room"))

# Room 10: Boss Chamber (110x26) - 3 enemies
r10, f10 = create_base_room(110, 26)
r10[f10 - 1][6] = 'P'
r10[f10 - 1][55] = 'E'
r10[f10 - 1][85] = 'E'
r10[14][55] = 'E'
for x in range(20, 40): r10[16][x] = '='
for x in range(70, 90): r10[16][x] = '='
for x in range(42, 68): r10[12][x] = '='
rooms.append((10, 110, 26, r10, "Boss Chamber"))

for (idx, w, h, grid, title) in rooms:
    txt_path = os.path.join(maps_dir, f"room_{idx}.txt")
    png_path = os.path.join(maps_dir, f"room_{idx}.png")
    kv_path = os.path.join(maps_dir, f"room_{idx}.kv")

    # 1. Save TXT
    lines = [''.join(row) for row in grid]
    with open(txt_path, "w") as f:
        f.write('\n'.join(lines) + '\n')

    # 2. Save PNG
    img = Image.new("RGB", (w, h), (255, 255, 255))
    for y in range(h):
        for x in range(w):
            char = grid[y][x]
            color = char_to_color.get(char, (255, 255, 255))
            img.putpixel((x, y), color)
    img.save(png_path)

    # 3. Save KV
    p_spawn = None
    e_spawns = []
    platforms = []

    for y in range(h):
        cur_start = None
        for x in range(w):
            c = grid[y][x]
            if c == 'P': p_spawn = (x, y)
            elif c == 'E': e_spawns.append((x, y))

            if c in ('=', '1', '-'):
                if cur_start is None: cur_start = x
            else:
                if cur_start is not None:
                    platforms.append((cur_start, y, x - cur_start))
                    cur_start = None
        if cur_start is not None:
            platforms.append((cur_start, y, w - cur_start))

    with open(kv_path, "w") as f:
        f.write(f'// КООРДИНАТНЫЙ КОНФИГ КОМНАТЫ {idx} ({title})\n')
        f.write(f'"Room"\n{{\n')
        f.write(f'    "width"   "{w}"\n')
        f.write(f'    "height"  "{h}"\n\n')
        if p_spawn:
            f.write(f'    "player_spawn"  "{p_spawn[0]} {p_spawn[1]}"\n')
        if e_spawns:
            f.write(f'    "enemy_spawn"   "{e_spawns[0][0]} {e_spawns[0][1]}"\n')
            f.write(f'\n    "Enemies"\n    {{\n')
            for i, es in enumerate(e_spawns):
                f.write(f'        "enemy_{i}"  "{es[0]} {es[1]}"\n')
            f.write(f'    }}\n')
        f.write(f'\n    "Platforms"\n    {{\n')
        for i, (px, py, plen) in enumerate(platforms):
            name = "floor" if py >= h - 5 else f"plat_{i}"
            f.write(f'        "{name}"  "{px} {py} {plen}"\n')
        f.write(f'    }}\n')
        f.write(f'}}\n')

    # If Room 1: also synchronize assets/config/
    if idx == 1:
        with open(os.path.join(cfg_dir, "map.txt"), "w") as f:
            f.write('\n'.join(lines) + '\n')
        img.save(os.path.join(cfg_dir, "map.png"))
        with open(os.path.join(cfg_dir, "map.kv"), "w") as f:
            f.write(f'"ArenaConfig"\n{{\n    "Settings"\n    {{\n        "layout_file" "assets/maps/room_1.txt"\n        "tileset" "assets/sprites/BlocksTest.png"\n        "tile_size" "64.0"\n    }}\n    "Spawns"\n    {{\n        "player_x" "384.0"\n        "player_y" "930.0"\n        "enemy_x" "2240.0"\n        "enemy_y" "932.0"\n    }}\n}}\n')

print("All 10 multi-room maps with multiple enemies and seamless passages generated successfully!")
