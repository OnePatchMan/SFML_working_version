import random
from collections import deque

GRID_COLS = 4
GRID_ROWS = 6
ROOM_W = 14
ROOM_H = 10
MAP_W = 60
MAP_H = 70

DOOR_LEFT  = 1
DOOR_RIGHT = 2
DOOR_UP    = 4
DOOR_DOWN  = 8

TEMPLATES = [
    # 0: StaircaseRight
    { "name": "StaircaseRight", "rows": [
        "##############",
        "#............#",
        "#..........==#",
        "#............#",
        "#......===...#",
        "#............#",
        "#..===.......#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 1: StaircaseLeft
    { "name": "StaircaseLeft", "rows": [
        "##############",
        "#............#",
        "#==..........#",
        "#............#",
        "#...===......#",
        "#............#",
        "#.......===..#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 2: Balconies
    { "name": "Balconies", "rows": [
        "##############",
        "#............#",
        "#....====....#",
        "#............#",
        "#.===....===.#",
        "#............#",
        "#....====....#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 3: ZigZag
    { "name": "ZigZag", "rows": [
        "##############",
        "#............#",
        "#...====.....#",
        "#............#",
        "#.......====.#",
        "#............#",
        "#..====......#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 4: Pyramid
    { "name": "Pyramid", "rows": [
        "##############",
        "#............#",
        "#....====....#",
        "#............#",
        "#...======...#",
        "#............#",
        "#.==========.#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 5: TwoPillars
    { "name": "TwoPillars", "rows": [
        "##############",
        "#............#",
        "#....====....#",
        "#............#",
        "#.==......==.#",
        "#............#",
        "#...==..==...#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 6: FloatingIslands
    { "name": "FloatingIslands", "rows": [
        "##############",
        "#............#",
        "#.......===..#",
        "#............#",
        "#..===.......#",
        "#............#",
        "#......===...#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 7: DoubleBridge
    { "name": "DoubleBridge", "rows": [
        "##############",
        "#............#",
        "#..===..===..#",
        "#............#",
        "#....====....#",
        "#............#",
        "#.==......==.#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 8: Courtyard
    { "name": "Courtyard", "rows": [
        "##############",
        "#............#",
        "#.==......==.#",
        "#............#",
        "#...======...#",
        "#............#",
        "#.==......==.#",
        "#............#",
        "#============#",
        "##############"
    ]},
    # 9: Shaft (Dedicated vertical connector)
    { "name": "Shaft", "rows": [
        "##############",
        "#............#",
        "#...====.....#",
        "#............#",
        "#.......====.#",
        "#............#",
        "#...====.....#",
        "#............#",
        "#============#",
        "##############"
    ]}
]

def generate_dungeon(seed=None):
    if seed is not None:
        random.seed(seed)

    grid = [[{'active': False, 'doors': 0, 'tmpl': 0} for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]

    # Pick start room in row 2 or 3
    start_r = random.randint(2, 3)
    start_c = random.randint(1, 2)
    grid[start_r][start_c]['active'] = True

    target_rooms = random.randint(13, 16)
    active_count = 1

    cur_r, cur_c = start_r, start_c
    dc = [-1, 1, 0, 0]
    dr = [0, 0, -1, 1]
    dmask = [DOOR_LEFT, DOOR_RIGHT, DOOR_UP, DOOR_DOWN]
    oppmask = [DOOR_RIGHT, DOOR_LEFT, DOOR_DOWN, DOOR_UP]

    steps = 0
    while active_count < target_rooms and steps < 400:
        steps += 1
        # 70% horizontal, 30% vertical
        if random.random() < 0.70:
            d_idx = 0 if random.random() < 0.5 else 1
        else:
            d_idx = 2 if random.random() < 0.5 else 3

        nr = cur_r + dr[d_idx]
        nc = cur_c + dc[d_idx]

        if 0 <= nr < GRID_ROWS and 0 <= nc < GRID_COLS:
            grid[cur_r][cur_c]['doors'] |= dmask[d_idx]
            grid[nr][nc]['doors'] |= oppmask[d_idx]
            if not grid[nr][nc]['active']:
                grid[nr][nc]['active'] = True
                active_count += 1
            cur_r, cur_c = nr, nc

    # Extra horizontal doors between active neighbors
    for r in range(GRID_ROWS):
        for c in range(GRID_COLS - 1):
            if grid[r][c]['active'] and grid[r][c+1]['active'] and random.random() < 0.45:
                grid[r][c]['doors'] |= DOOR_RIGHT
                grid[r][c+1]['doors'] |= DOOR_LEFT

    # Assign templates
    for r in range(GRID_ROWS):
        for c in range(GRID_COLS):
            if not grid[r][c]['active']:
                continue
            doors = grid[r][c]['doors']
            if (doors & DOOR_UP) or (doors & DOOR_DOWN):
                grid[r][c]['tmpl'] = 9 # Shaft template
            else:
                grid[r][c]['tmpl'] = random.randint(0, 8) # Any horizontal prefab

    # Verify BFS connectivity from start room
    visited = set()
    queue = deque([(start_r, start_c)])
    visited.add((start_r, start_c))

    while queue:
        r, c = queue.popleft()
        doors = grid[r][c]['doors']
        if (doors & DOOR_LEFT) and c > 0 and (r, c-1) not in visited:
            visited.add((r, c-1))
            queue.append((r, c-1))
        if (doors & DOOR_RIGHT) and c < GRID_COLS - 1 and (r, c+1) not in visited:
            visited.add((r, c+1))
            queue.append((r, c+1))
        if (doors & DOOR_UP) and r > 0 and (r-1, c) not in visited:
            visited.add((r-1, c))
            queue.append((r-1, c))
        if (doors & DOOR_DOWN) and r < GRID_ROWS - 1 and (r+1, c) not in visited:
            visited.add((r+1, c))
            queue.append((r+1, c))

    assert len(visited) == active_count, f"Disconnected rooms! {len(visited)} visited out of {active_count}"
    return grid, active_count

# Test 500 seeds
for s in range(500):
    generate_dungeon(s)

print("500 random dungeon seeds generated and verified for 100% graph connectivity!")
