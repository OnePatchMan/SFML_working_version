import numpy as np
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8
GROUND_Y = 95

# Authentic Pixel Art Palette (Sachiel/Angel aesthetic + Golem shading):
C_TRANS = (0, 0, 0, 0)
C_OUT = (18, 20, 28, 255)            # Deep dark outline
C_SHADOW_DEEP = (50, 58, 76, 255)    # Deepest carapace shadow / cavity
C_SHADOW = (94, 104, 128, 255)       # Mid carapace shadow
C_MID = (156, 168, 192, 255)         # Base pale lavender carapace
C_LIGHT = (212, 222, 240, 255)       # Lit carapace surface
C_HI = (252, 254, 255, 255)          # Bright rim light / specular glint

# Dark abdominal core (slender dark waist):
C_CORE_VOID = (12, 14, 20, 255)
C_CORE_DARK = (24, 30, 42, 255)
C_CORE_MID = (42, 52, 70, 255)
C_CORE_LIGHT = (68, 82, 106, 255)

# Ivory Bone Ribs:
C_RIB_DARK = (110, 116, 108, 255)
C_RIB_MID = (180, 186, 176, 255)
C_RIB_LIGHT = (244, 246, 238, 255)

# Claws / Talons:
C_CLAW_DARK = (116, 120, 112, 255)
C_CLAW_MID = (186, 190, 180, 255)
C_CLAW_HI = (250, 252, 246, 255)

def draw_boss_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Pale Mutant Boss:
    - 3/4 turn towards screen (showing both shoulders, chest angled forward-down)
    - Massive curved hunched back arching high (dome peaking at Y=10..14)
    - Downward-angled 'searchlight' cowl looking down towards floor at an acute angle
    - Headless, neckless, eyeless smooth organic carapace
    - Extremely pinched Sachiel-style waist with ivory ribs
    - Long gorilla arms reaching to floor
    - BOTH hands and feet firmly on ground_y = 95 with ZERO GAP!
    """
    dx = ox
    bob_y = 0
    rear_up = False
    slam = False
    jumping = False
    hurt = False

    # Near Arm anchors
    na_sx, na_sy = 34, 38
    na_ex, na_ey = 20, 62
    na_kx, na_ky = 15, GROUND_Y

    # Far Arm anchors
    fa_sx, fa_sy = 60, 36
    fa_ex, fa_ey = 54, 62
    fa_kx, fa_ky = 46, GROUND_Y

    # Near Leg anchors
    nl_hip = (80, 60)
    nl_knee = (96, 74)
    nl_foot = (84, GROUND_Y)

    # Far Leg anchors
    fl_hip = (86, 60)
    fl_knee = (100, 72)
    fl_foot = (90, GROUND_Y)

    # Frame-specific pose adjustments
    if frame_idx == 0:
        # Idle low
        pass
    elif frame_idx == 1:
        # Idle breath: arched spine rises slightly, chest expands
        bob_y = -2
        na_sy -= 1
        fa_sy -= 1
    elif frame_idx == 2:
        # Knuckle walk 1: Near arm reaches forward-left, Far arm & feet planted
        na_ex -= 6
        na_kx -= 8
        nl_knee = (92, 72)
        nl_foot = (80, GROUND_Y)
        fl_foot = (94, GROUND_Y)
    elif frame_idx == 3:
        # Knuckle walk 2: Far arm reaches forward-left, Near arm & feet planted
        fa_ex -= 6
        fa_kx -= 8
        nl_foot = (88, GROUND_Y)
        fl_foot = (84, GROUND_Y)
    elif frame_idx == 4:
        # Attack windup: Reared back on hind legs, fists raised overhead!
        rear_up = True
        bob_y = -10
        na_sy = 30
        na_ex, na_ey = 24, 18
        na_kx, na_ky = 26, 8
        fa_sy = 28
        fa_ex, fa_ey = 50, 16
        fa_kx, fa_ky = 52, 8
        nl_hip = (76, 56)
        nl_knee = (94, 72)
        nl_foot = (80, GROUND_Y)
        fl_hip = (82, 56)
        fl_knee = (98, 70)
        fl_foot = (88, GROUND_Y)
    elif frame_idx == 5:
        # Attack slam: Fists smashed down into floor in front, body lunging forward-down!
        slam = True
        bob_y = 2
        na_sx, na_sy = 32, 42
        na_ex, na_ey = 18, 64
        na_kx, na_ky = 12, GROUND_Y
        fa_sx, fa_sy = 54, 40
        fa_ex, fa_ey = 36, 66
        fa_kx, fa_ky = 30, GROUND_Y
        nl_hip = (82, 62)
        nl_knee = (100, 76)
        nl_foot = (88, GROUND_Y)
        fl_hip = (88, 62)
        fl_knee = (104, 74)
        fl_foot = (94, GROUND_Y)
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air
        jumping = True
        bob_y = -22
        nl_foot = (80, GROUND_Y - 24)
        fl_foot = (88, GROUND_Y - 26)
        na_ex, na_ey = 22, 54
        na_kx, na_ky = 20, 80
        fa_ex, fa_ey = 50, 52
        fa_kx, fa_ky = 46, 78
    elif frame_idx == 7:
        # Hurt / stunned: Flinched backward, recoiling
        hurt = True
        bob_y = -3
        na_sx += 4
        na_ex += 6
        na_kx += 6
        fa_sx += 4
        fa_ex += 6
        fa_kx += 6
        nl_foot = (88, GROUND_Y)
        fl_foot = (94, GROUND_Y)

    # 1. FAR HIND LEG
    f_hock = (fl_foot[0] + 4, fl_foot[1] - 8)
    draw.polygon([
        (dx + fl_hip[0]-4, fl_hip[1] + bob_y),
        (dx + fl_hip[0]+6, fl_hip[1] + bob_y),
        (dx + fl_knee[0]+4, fl_knee[1] + bob_y),
        (dx + fl_knee[0]-3, fl_knee[1]+3 + bob_y)
    ], fill=C_SHADOW, outline=C_OUT)
    draw.polygon([
        (dx + fl_knee[0]+4, fl_knee[1] + bob_y),
        (dx + fl_knee[0]-3, fl_knee[1]+3 + bob_y),
        (dx + f_hock[0]-3, f_hock[1]),
        (dx + f_hock[0]+3, f_hock[1])
    ], fill=C_SHADOW, outline=C_OUT)
    draw.polygon([
        (dx + f_hock[0]-3, f_hock[1]),
        (dx + f_hock[0]+3, f_hock[1]),
        (dx + fl_foot[0]+5, fl_foot[1]-4),
        (dx + fl_foot[0]+5, fl_foot[1]),
        (dx + fl_foot[0]-6, fl_foot[1])
    ], fill=C_SHADOW, outline=C_OUT)
    draw.line([(dx + fl_foot[0]-5, fl_foot[1]-3), (dx + fl_foot[0]+4, fl_foot[1]-3)], fill=C_MID, width=2)
    for c in range(3):
        cx = dx + fl_foot[0] - 5 + c * 4
        draw.polygon([(cx, fl_foot[1]-2), (cx-2, fl_foot[1]), (cx+1, fl_foot[1])], fill=C_CLAW_MID, outline=C_OUT)

    # 2. PELVIS & SLENDER DARK CORE
    pelvis_pts = [
        (dx + 72, 60 + bob_y),
        (dx + 90, 60 + bob_y),
        (dx + 88, 70 + bob_y),
        (dx + 80, 74 + bob_y),
        (dx + 74, 70 + bob_y)
    ]
    draw.polygon(pelvis_pts, fill=C_MID, outline=C_OUT)
    draw.line([(dx + 74, 62 + bob_y), (dx + 80, 64 + bob_y), (dx + 88, 62 + bob_y)], fill=C_LIGHT, width=2)

    core_pts = [
        (dx + 44, 46 + bob_y),
        (dx + 74, 44 + bob_y),
        (dx + 86, 60 + bob_y),
        (dx + 72, 60 + bob_y),
        (dx + 56, 52 + bob_y)
    ]
    draw.polygon(core_pts, fill=C_CORE_VOID, outline=C_OUT)
    draw.line([(dx + 58, 48 + bob_y), (dx + 76, 46 + bob_y)], fill=C_CORE_MID, width=2)
    draw.line([(dx + 62, 54 + bob_y), (dx + 80, 54 + bob_y)], fill=C_CORE_DARK, width=2)

    # Ivory Bone Ribs
    ribs = [
        (76, 44, 64, 46, 52, 45),
        (78, 48, 66, 51, 55, 50),
        (80, 53, 69, 56, 59, 55),
        (82, 58, 72, 61, 64, 60)
    ]
    for rx0, ry0, rx1, ry1, rx2, ry2 in ribs:
        draw.line([(dx + rx0, ry0 + bob_y), (dx + rx1, ry1 + bob_y), (dx + rx2, ry2 + bob_y)], fill=C_RIB_LIGHT, width=2)
        draw.line([(dx + rx0, ry0 + 1 + bob_y), (dx + rx1, ry1 + 1 + bob_y), (dx + rx2, ry2 + 1 + bob_y)], fill=C_RIB_DARK, width=1)
        draw.point((dx + rx2 - 1, ry2 + bob_y), fill=C_RIB_LIGHT)

    # 3. MASSIVE ARCHED HUNCHBACK & SEARCHLIGHT PROW
    hunch_pts = [
        (dx + 24, 46 + bob_y),   # Front searchlight nose
        (dx + 22, 38 + bob_y),   # Lower front slope
        (dx + 26, 28 + bob_y),   # Upper front slope
        (dx + 38, 18 + bob_y),   # Ascending chest
        (dx + 54, 12 + bob_y),   # Rising spine arch
        (dx + 72, 10 + bob_y),   # PEAK OF THE ARCHED SPINE (Highest point Y=10)
        (dx + 85, 14 + bob_y),   # Descending back arch
        (dx + 95, 23 + bob_y),   # Upper rear back
        (dx + 98, 36 + bob_y),   # Mid rear back
        (dx + 95, 52 + bob_y),   # Lower back into hip
        (dx + 86, 56 + bob_y),   # Hip junction
        (dx + 76, 50 + bob_y),   # Spine junction
        (dx + 58, 46 + bob_y),   # Upper waist
        (dx + 40, 48 + bob_y),   # Under chest
        (dx + 26, 48 + bob_y)
    ]
    draw.polygon(hunch_pts, fill=C_MID, outline=C_OUT)

    # Top rim light along arched spine
    spine_rim = [(dx + 23, 37 + bob_y), (dx + 27, 27 + bob_y), (dx + 39, 17 + bob_y), (dx + 55, 11 + bob_y), (dx + 72, 11 + bob_y), (dx + 85, 15 + bob_y), (dx + 94, 24 + bob_y), (dx + 97, 36 + bob_y)]
    draw.line(spine_rim, fill=C_HI, width=2)
    spine_light = [(dx + 25, 39 + bob_y), (dx + 30, 29 + bob_y), (dx + 41, 19 + bob_y), (dx + 57, 13 + bob_y), (dx + 73, 13 + bob_y), (dx + 86, 17 + bob_y), (dx + 94, 26 + bob_y), (dx + 97, 38 + bob_y)]
    draw.line(spine_light, fill=C_LIGHT, width=3)

    # Curved contour shadow on the rear dome
    for y in range(24, 53):
        cur_y = y + bob_y
        x_r = 98 if y < 40 else (98 - (y - 40) // 3)
        x_s = x_r - (10 if y < 42 else 14)
        draw.line([(dx + x_s, cur_y), (dx + x_r - 2, cur_y)], fill=C_SHADOW, width=1)
        draw.line([(dx + x_r - 5, cur_y), (dx + x_r - 2, cur_y)], fill=C_SHADOW_DEEP, width=1)

    # Searchlight Cowl (Smooth faceless oval looking down at acute angle)
    spotlight_rim = [
        (dx + 23, 40 + bob_y), (dx + 32, 32 + bob_y), (dx + 38, 42 + bob_y), (dx + 28, 47 + bob_y)
    ]
    draw.polygon(spotlight_rim, fill=C_LIGHT, outline=C_OUT)
    draw.line([(dx + 24, 40 + bob_y), (dx + 28, 47 + bob_y)], fill=C_HI, width=2)
    draw.line([(dx + 29, 35 + bob_y), (dx + 35, 43 + bob_y)], fill=C_MID, width=2)

    # 4. NEAR HIND LEG
    n_hock = (nl_foot[0] + 4, nl_foot[1] - 8)
    draw.polygon([
        (dx + nl_hip[0]-5, nl_hip[1] + bob_y),
        (dx + nl_hip[0]+6, nl_hip[1] + bob_y),
        (dx + nl_knee[0]+5, nl_knee[1] + bob_y),
        (dx + nl_knee[0]-3, nl_knee[1]+4 + bob_y)
    ], fill=C_MID, outline=C_OUT)
    draw.line([(dx + nl_hip[0]-2, nl_hip[1]+1 + bob_y), (dx + nl_knee[0]+2, nl_knee[1]-1 + bob_y)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_hip[0]-1, nl_hip[1]+1 + bob_y), (dx + nl_knee[0]+1, nl_knee[1]-1 + bob_y)], fill=C_HI, width=1)

    draw.polygon([
        (dx + nl_knee[0]+5, nl_knee[1] + bob_y),
        (dx + nl_knee[0]-3, nl_knee[1]+4 + bob_y),
        (dx + n_hock[0]-4, n_hock[1]),
        (dx + n_hock[0]+4, n_hock[1])
    ], fill=C_MID, outline=C_OUT)
    draw.line([(dx + nl_knee[0]+1, nl_knee[1]+2 + bob_y), (dx + n_hock[0]-1, n_hock[1]-1)], fill=C_LIGHT, width=2)

    draw.polygon([
        (dx + n_hock[0]-4, n_hock[1]),
        (dx + n_hock[0]+4, n_hock[1]),
        (dx + nl_foot[0]+6, nl_foot[1]-4),
        (dx + nl_foot[0]+6, nl_foot[1]),
        (dx + nl_foot[0]-8, nl_foot[1])
    ], fill=C_MID, outline=C_OUT)
    draw.line([(dx + nl_foot[0]-7, nl_foot[1]-3), (dx + nl_foot[0]+5, nl_foot[1]-3)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_foot[0]-6, nl_foot[1]-2), (dx + nl_foot[0]+4, nl_foot[1]-2)], fill=C_HI, width=1)
    for c in range(3):
        cx = dx + nl_foot[0] - 7 + c * 4
        draw.polygon([(cx, nl_foot[1]-3), (cx-3, nl_foot[1]), (cx+1, nl_foot[1])], fill=C_CLAW_HI, outline=C_OUT)

    # 5. FAR ARM (Descends from Far Shoulder at X=60, Y=36 down to floor)
    # Far shoulder: rounded deltoid in 3/4 perspective
    f_sh_r = 7
    draw.ellipse([dx + fa_sx - f_sh_r, fa_sy - f_sh_r + bob_y, dx + fa_sx + f_sh_r, fa_sy + f_sh_r + bob_y], fill=C_MID, outline=C_OUT)
    draw.arc([dx + fa_sx - f_sh_r + 1, fa_sy - f_sh_r + 1 + bob_y, dx + fa_sx + f_sh_r - 1, fa_sy + f_sh_r - 1 + bob_y], 160, 330, fill=C_LIGHT, width=2)

    # Far upper arm
    draw.polygon([
        (dx + fa_sx - 4, fa_sy + 4 + bob_y),
        (dx + fa_sx + 5, fa_sy + 4 + bob_y),
        (dx + fa_ex + 5, fa_ey + bob_y),
        (dx + fa_ex - 5, fa_ey + bob_y)
    ], fill=C_SHADOW, outline=C_OUT)
    draw.line([(dx + fa_sx - 2, fa_sy + 5 + bob_y), (dx + fa_ex - 3, fa_ey - 2 + bob_y)], fill=C_MID, width=2)

    # Far forearm
    draw.polygon([
        (dx + fa_ex - 5, fa_ey + bob_y),
        (dx + fa_ex + 6, fa_ey + bob_y),
        (dx + fa_kx + 6, fa_ky - 8),
        (dx + fa_kx - 6, fa_ky - 8)
    ], fill=C_SHADOW, outline=C_OUT)
    draw.line([(dx + fa_ex - 3, fa_ey + 2 + bob_y), (dx + fa_kx - 4, fa_ky - 9)], fill=C_MID, width=2)
    draw.line([(dx + fa_ex - 4, fa_ey + 8 + bob_y), (dx + fa_ex + 5, fa_ey + 8 + bob_y)], fill=C_MID, width=1)

    # Far knuckle fist & claws on floor (fa_ky)
    draw.rectangle([dx + fa_kx - 6, fa_ky - 8, dx + fa_kx + 6, fa_ky - 4], fill=C_SHADOW, outline=C_OUT)
    draw.line([(dx + fa_kx - 5, fa_ky - 7), (dx + fa_kx + 5, fa_ky - 7)], fill=C_MID, width=1)
    draw.rectangle([dx + fa_kx - 7, fa_ky - 4, dx + fa_kx + 7, fa_ky - 1], fill=C_SHADOW, outline=C_OUT)
    for c in range(3):
        cx = dx + fa_kx - 5 + c * 4
        draw.polygon([(cx, fa_ky - 2), (cx-2, fa_ky), (cx+1, fa_ky)], fill=C_CLAW_MID, outline=C_OUT)

    # 6. NEAR ARM & SHOULDER (Foreground: deltoid at X=34, Y=38)
    sh_x, sh_y, sh_r = na_sx, na_sy + bob_y, 9
    draw.ellipse([dx + sh_x - sh_r, sh_y - sh_r, dx + sh_x + sh_r, sh_y + sh_r], fill=C_MID, outline=C_OUT)
    draw.arc([dx + sh_x - sh_r + 1, sh_y - sh_r + 1, dx + sh_x + sh_r - 1, sh_y + sh_r - 1], 160, 330, fill=C_LIGHT, width=2)
    draw.arc([dx + sh_x - sh_r + 2, sh_y - sh_r + 2, dx + sh_x + sh_r - 2, sh_y + sh_r - 2], 190, 300, fill=C_HI, width=2)
    draw.arc([dx + sh_x - sh_r + 1, sh_y - sh_r + 1, dx + sh_x + sh_r - 1, sh_y + sh_r - 1], 20, 140, fill=C_SHADOW_DEEP, width=2)

    # Upper Arm:
    draw.polygon([
        (dx + sh_x - 5, sh_y + 6),
        (dx + sh_x + 6, sh_y + 6),
        (dx + na_ex + 5, na_ey + bob_y),
        (dx + na_ex - 6, na_ey + bob_y)
    ], fill=C_MID, outline=C_OUT)
    draw.line([(dx + sh_x - 3, sh_y + 8), (dx + na_ex - 3, na_ey - 2 + bob_y)], fill=C_LIGHT, width=2)
    draw.line([(dx + sh_x - 2, sh_y + 8), (dx + na_ex - 2, na_ey - 2 + bob_y)], fill=C_HI, width=1)

    # Forearm Gauntlet:
    draw.polygon([
        (dx + na_ex - 6, na_ey + bob_y),
        (dx + na_ex + 6, na_ey + bob_y),
        (dx + na_kx + 8, na_ky - 8),
        (dx + na_kx - 8, na_ky - 8)
    ], fill=C_MID, outline=C_OUT)
    draw.line([(dx + na_ex - 4, na_ey + 2 + bob_y), (dx + na_kx - 6, na_ky - 9)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_ex - 3, na_ey + 2 + bob_y), (dx + na_kx - 5, na_ky - 9)], fill=C_HI, width=1)
    # Gauntlet band ridges:
    draw.line([(dx + na_ex - 4, na_ey + 8 + bob_y), (dx + na_ex + 5, na_ey + 8 + bob_y)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_ex - 4, na_ey + 9 + bob_y), (dx + na_ex + 5, na_ey + 9 + bob_y)], fill=C_OUT, width=1)
    draw.line([(dx + na_kx - 7, na_ky - 12), (dx + na_kx + 7, na_ky - 12)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_kx - 7, na_ky - 11), (dx + na_kx + 7, na_ky - 11)], fill=C_OUT, width=1)

    # Heavy Knuckle Fist planted on floor (na_ky)
    draw.rectangle([dx + na_kx - 8, na_ky - 8, dx + na_kx + 8, na_ky - 4], fill=C_MID, outline=C_OUT)
    draw.line([(dx + na_kx - 7, na_ky - 7), (dx + na_kx + 7, na_ky - 7)], fill=C_HI, width=1)
    draw.rectangle([dx + na_kx - 9, na_ky - 4, dx + na_kx + 9, na_ky - 1], fill=C_MID, outline=C_OUT)
    # 4 distinct claws touching na_ky exactly:
    for i in range(4):
        cx = dx + na_kx - 7 + i * 4
        draw.polygon([(cx, na_ky - 2), (cx-3, na_ky), (cx+1, na_ky)], fill=C_CLAW_HI, outline=C_OUT)
        draw.point((cx - 1, na_ky), fill=C_CLAW_DARK)

def generate_full_sheet():
    sheet = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sheet)
    for f in range(NUM_FRAMES):
        draw_boss_frame(draw, f * FRAME_W, f)
    
    sheet.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Saved assets/sprites/enemies/boss/boss_pale_mutant.png (1024x96)")

    # Editor preview icons:
    f0 = sheet.crop((0, 0, FRAME_W, FRAME_H))
    
    # 64x64 preview icon
    icon64 = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    icon_sub = f0.resize((64, 48), Image.Resampling.LANCZOS)
    icon64.paste(icon_sub, (0, 8), icon_sub)
    icon64.save("assets/tiles/enemy_boss.png")
    icon64.save("assets/tiles/tools/enemy_boss.png")

    # 32x32 preview icon
    icon32 = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    icon_sub32 = f0.resize((32, 24), Image.Resampling.LANCZOS)
    icon32.paste(icon_sub32, (0, 4), icon_sub32)
    icon32.save("assets/tiles/enemies/enemy_boss.png")
    print("Updated editor preview icons.")

if __name__ == "__main__":
    generate_full_sheet()
