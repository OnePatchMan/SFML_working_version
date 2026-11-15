#!/usr/bin/env python3
import os

rooms_dir = "assets/maps"
os.makedirs(rooms_dir, exist_ok=True)

def make_room(w, h, fill=' '):
    return [[fill for _ in range(w)] for _ in range(h)]

def add_borders(grid):
    h = len(grid)
    w = len(grid[0])
    for x in range(w):
        grid[0][x] = '#'
        grid[h-1][x] = '#'
        grid[h-2][x] = '#'
        grid[h-3][x] = '#'
        grid[h-4][x] = '#'
        grid[h-5][x] = '='
    for y in range(h):
        grid[y][0] = '#'
        grid[y][w-1] = '#'

def to_str(grid):
    return '\n'.join(''.join(row) for row in grid) + '\n'

# Room 1: Main Training Ground (80x20)
r1 = make_room(80, 20)
add_borders(r1)
# Platforms
for x in range(16, 23): r1[10][x] = '='
for x in range(56, 63): r1[10][x] = '='
for x in range(35, 43): r1[12][x] = '=' # Center platform on row 12!
# Spawns & Door
r1[14][6] = 'P'
r1[14][51] = 'E'
r1[14][76] = 'D' # Door to Room 2
with open(os.path.join(rooms_dir, "room_1.txt"), "w") as f:
    f.write(to_str(r1))

# Room 2: Vertical Tower / High Platforms (80x25)
r2 = make_room(80, 25)
add_borders(r2)
r2[20][3] = 'D' # Door from Room 1
r2[20][7] = 'P'
r2[20][50] = 'E'
r2[20][76] = 'D' # Door to Room 3
# Spiral / climbing platforms
for x in range(12, 24): r2[17][x] = '='
for x in range(28, 40): r2[14][x] = '='
for x in range(44, 56): r2[11][x] = '='
for x in range(60, 72): r2[8][x] = '='
for x in range(30, 48): r2[5][x] = '='
with open(os.path.join(rooms_dir, "room_2.txt"), "w") as f:
    f.write(to_str(r2))

# Room 3: Pillar Hall (80x20)
r3 = make_room(80, 20)
add_borders(r3)
r3[14][3] = 'D'
r3[14][7] = 'P'
r3[14][55] = 'E'
r3[14][76] = 'D'
# Pillars
for y in range(8, 15):
    r3[y][20] = '#'
    r3[y][21] = '#'
    r3[y][40] = '#'
    r3[y][41] = '#'
    r3[y][60] = '#'
    r3[y][61] = '#'
for x in range(18, 24): r3[7][x] = '='
for x in range(38, 44): r3[7][x] = '='
for x in range(58, 64): r3[7][x] = '='
for x in range(28, 34): r3[11][x] = '='
for x in range(48, 54): r3[11][x] = '='
with open(os.path.join(rooms_dir, "room_3.txt"), "w") as f:
    f.write(to_str(r3))

# Room 4: Dungeon Catacombs (90x22)
r4 = make_room(90, 22)
add_borders(r4)
r4[16][3] = 'D'
r4[16][7] = 'P'
r4[16][60] = 'E'
r4[16][86] = 'D'
# Internal floors
for x in range(15, 75): r4[11][x] = '='
for x in range(25, 40): r4[6][x] = '='
for x in range(50, 65): r4[6][x] = '='
with open(os.path.join(rooms_dir, "room_4.txt"), "w") as f:
    f.write(to_str(r4))

# Room 5: The Great Bridge - Massive Horizontal Arena (120x20)
r5 = make_room(120, 20)
add_borders(r5)
r5[14][3] = 'D'
r5[14][10] = 'P'
r5[14][80] = 'E'
r5[14][116] = 'D'
# Bridge arches and elevated watch posts
for x in range(20, 35): r5[10][x] = '='
for x in range(45, 75): r5[8][x] = '='
for x in range(85, 100): r5[10][x] = '='
with open(os.path.join(rooms_dir, "room_5.txt"), "w") as f:
    f.write(to_str(r5))

# Room 6: Duel Arena - Symmetrical Competitive Hall (80x20)
r6 = make_room(80, 20)
add_borders(r6)
r6[14][3] = 'D'
r6[14][20] = 'P'
r6[14][60] = 'E'
r6[14][76] = 'D'
# 3 battlefield platforms (tournament style)
for x in range(22, 32): r6[11][x] = '='
for x in range(48, 58): r6[11][x] = '='
for x in range(35, 45): r6[8][x] = '='
with open(os.path.join(rooms_dir, "room_6.txt"), "w") as f:
    f.write(to_str(r6))

# Room 7: Platformer Gauntlet (90x24)
r7 = make_room(90, 24)
add_borders(r7)
r7[18][3] = 'D'
r7[18][7] = 'P'
r7[18][70] = 'E'
r7[18][86] = 'D'
# Staggered stepping stones
for i, px in enumerate(range(14, 76, 8)):
    py = 15 - (i % 4) * 3
    for x in range(px, px + 5):
        r7[py][x] = '='
with open(os.path.join(rooms_dir, "room_7.txt"), "w") as f:
    f.write(to_str(r7))

# Room 8: Sky Ruins (100x24)
r8 = make_room(100, 24)
add_borders(r8)
r8[18][3] = 'D'
r8[18][8] = 'P'
r8[18][75] = 'E'
r8[18][96] = 'D'
# Floating sky isles
for x in range(15, 30): r8[14][x] = '='
for x in range(38, 58): r8[10][x] = '='
for x in range(65, 85): r8[13][x] = '='
for x in range(45, 55): r8[6][x] = '='
with open(os.path.join(rooms_dir, "room_8.txt"), "w") as f:
    f.write(to_str(r8))

# Room 9: Throne Room (85x22)
r9 = make_room(85, 22)
add_borders(r9)
r9[16][3] = 'D'
r9[16][8] = 'P'
r9[16][65] = 'E'
r9[16][81] = 'D'
# Throne pedestal steps
for x in range(50, 75): r9[14][x] = '#'
for x in range(55, 75): r9[12][x] = '#'
for x in range(60, 75): r9[10][x] = '#'
r9[14][50] = '='
r9[12][55] = '='
r9[10][60] = '='
for x in range(15, 35): r9[12][x] = '='
with open(os.path.join(rooms_dir, "room_9.txt"), "w") as f:
    f.write(to_str(r9))

# Room 10: Boss Chamber (110x26)
r10 = make_room(110, 26)
add_borders(r10)
r10[20][3] = 'D'
r10[20][10] = 'P'
r10[20][80] = 'E'
r10[20][106] = 'D'
# Grand arena with multi-level battle balconies
for x in range(15, 35): r10[16][x] = '='
for x in range(75, 95): r10[16][x] = '='
for x in range(25, 45): r10[11][x] = '='
for x in range(65, 85): r10[11][x] = '='
for x in range(45, 65): r10[7][x] = '='
with open(os.path.join(rooms_dir, "room_10.txt"), "w") as f:
    f.write(to_str(r10))

print("Created 10 rooms in assets/maps successfully!")
