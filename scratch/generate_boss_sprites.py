import os
from PIL import Image
from pixel_sculpt import PixelLayer, composite_layers

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8
GROUND_Y = 94

C_OUT = (16, 18, 26, 255)
C_DEEP = (40, 46, 64, 255)
C_SHADOW = (80, 90, 114, 255)
C_MID = (146, 158, 184, 255)
C_LIGHT = (204, 214, 234, 255)
C_HI = (248, 252, 255, 255)

C_BONE = (238, 240, 230, 255)
C_BONE_SH = (120, 126, 120, 255)
C_BONE_HI = (255, 255, 250, 255)
C_VEIN = (96, 46, 76, 255)
C_SINEW = (142, 42, 58, 255)
C_IMPACT = (245, 225, 200, 200)

def draw_boss_frame(frame_idx):
    far = PixelLayer(FRAME_W, FRAME_H)
    body = PixelLayer(FRAME_W, FRAME_H)
    near = PixelLayer(FRAME_W, FRAME_H)
    fx = PixelLayer(FRAME_W, FRAME_H)

    bob_y = 0
    lean_x = 0
    rear_up = (frame_idx == 4)
    slam = (frame_idx == 5)
    jumping = (frame_idx == 6)
    hurt = (frame_idx == 7)

    # Default limb anchors
    na_sh = (31, 35)
    na_el = (21, 58)
    na_kn = (17, GROUND_Y - 3)

    fa_sh = (43, 33)
    fa_el = (36, 58)
    fa_kn = (32, GROUND_Y - 3)

    nl_hip = (76, 46)
    nl_knee = (90, 66)
    nl_hock = (86, 78)
    nl_foot = (78, GROUND_Y - 2)

    fl_hip = (79, 48)
    fl_knee = (93, 67)
    fl_hock = (89, 79)
    fl_foot = (82, GROUND_Y - 2)

    if frame_idx == 1:
        bob_y = -2
    elif frame_idx == 2:
        na_el = (16, 56)
        na_kn = (11, GROUND_Y - 3)
        nl_knee = (86, 66)
        nl_foot = (73, GROUND_Y - 2)
    elif frame_idx == 3:
        fa_el = (30, 56)
        fa_kn = (25, GROUND_Y - 3)
        fl_knee = (88, 67)
        fl_foot = (77, GROUND_Y - 2)
        nl_foot = (82, GROUND_Y - 2)
    elif rear_up:
        bob_y = -6
        lean_x = 8
        na_sh = (32, 30)
        na_el = (18, 22)
        na_kn = (14, 9)
        fa_sh = (44, 28)
        fa_el = (34, 18)
        fa_kn = (32, 7)
        nl_hip = (78, 49)
        nl_knee = (93, 67)
        nl_foot = (78, GROUND_Y - 2)
    elif slam:
        bob_y = 5
        lean_x = -5
        na_sh = (27, 41)
        na_el = (17, 63)
        na_kn = (11, GROUND_Y - 3)
        fa_sh = (39, 39)
        fa_el = (29, 64)
        fa_kn = (23, GROUND_Y - 3)
        nl_hip = (77, 52)
        nl_knee = (93, 70)
        nl_foot = (81, GROUND_Y - 2)
    elif jumping:
        bob_y = -6
        na_el = (21, 52)
        na_kn = (17, 74)
        fa_el = (37, 50)
        fa_kn = (33, 72)
        nl_knee = (91, 61)
        nl_hock = (86, 70)
        nl_foot = (77, 74)
        fl_knee = (94, 62)
        fl_hock = (89, 71)
        fl_foot = (82, 75)
    elif hurt:
        bob_y = -2
        lean_x = 5

    dx = lean_x

    # 1. FAR HIND LEG (Muscular thigh -> knee -> tapered shin -> clawed foot)
    far.tapered_limb([
        (dx + fl_hip[0], fl_hip[1] + bob_y, 6.5),
        (dx + fl_knee[0], fl_knee[1] + bob_y, 4.5),
        (dx + fl_hock[0], fl_hock[1] + bob_y, 3.2),
        (dx + fl_foot[0], fl_foot[1], 3.8),
    ], C_SHADOW)
    far.tapered_limb([(dx + fl_foot[0], fl_foot[1], 2.5), (dx + fl_foot[0] - 6, fl_foot[1] + 1, 0.8)], C_BONE_SH)

    # 2. FAR FRONT ARM (Organic deltoid -> forearm -> knuckle fist)
    fa_e_y = fa_el[1] + (0 if (rear_up or jumping) else bob_y)
    far.tapered_limb([
        (dx + fa_sh[0], fa_sh[1] + bob_y, 6.2),
        (dx + fa_el[0], fa_e_y, 4.8),
        (dx + fa_kn[0], fa_kn[1], 5.4),
    ], C_SHADOW)
    far.tapered_limb([(dx + fa_kn[0], fa_kn[1] + 1, 2.5), (dx + fa_kn[0] - 5, fa_kn[1] + 2, 0.6)], C_BONE_SH)
    far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
    far.add_outline(C_OUT, GROUND_Y + 1)

    # 3. SLENDER ARCHED SPINE & ACUTE FRONT COWL (Approved silhouette!)
    # Sculpted with smooth overlapping Bezier curves
    body.bezier_limb((dx + 19, 38 + bob_y), (dx + 34, 18 + bob_y), (dx + 64, 15 + bob_y), 4.5, 9.5, 10.5, C_MID)
    body.bezier_limb((dx + 64, 15 + bob_y), (dx + 84, 20 + bob_y), (dx + 79, 46 + bob_y), 10.5, 10.0, 7.5, C_MID)
    # Acute downward-pointing faceless prow/cowl
    body.polygon([
        (dx + 15, 34 + bob_y),
        (dx + 22, 23 + bob_y),
        (dx + 34, 24 + bob_y),
        (dx + 25, 43 + bob_y),
        (dx + 18, 42 + bob_y),
    ], C_MID)

    body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)

    # Deep scooped underbelly cavity & sinews
    body.bezier_limb((dx + 36, 36 + bob_y), (dx + 54, 29 + bob_y), (dx + 72, 43 + bob_y), 2.5, 2.0, 2.5, C_DEEP)
    body.line([(dx + 46, 31 + bob_y), (dx + 62, 34 + bob_y)], C_SINEW)
    body.line([(dx + 50, 29 + bob_y), (dx + 66, 37 + bob_y)], C_VEIN)

    # Ivory ribcage curving down the slender flank
    for rx, ry in ((40, 26), (48, 22), (56, 20), (64, 21)):
        body.bezier_limb((dx + rx, ry + bob_y), (dx + rx + 4, ry + 6 + bob_y), (dx + rx + 7, ry + 11 + bob_y), 1.3, 1.1, 0.7, C_BONE)
    # Dorsal spine vertebrae along the arch
    for vx, vy in ((38, 16), (48, 12), (58, 10), (68, 11), (77, 15)):
        body.tapered_limb([(dx + vx, vy + bob_y, 1.5), (dx + vx + 1, vy - 3 + bob_y, 0.4)], C_BONE)

    body.auto_shade(C_BONE, C_BONE_HI, C_BONE_HI, C_BONE_SH, C_SHADOW)
    body.add_outline(C_OUT, GROUND_Y + 1)

    # 4. NEAR HIND LEG
    near.tapered_limb([
        (dx + nl_hip[0], nl_hip[1] + bob_y, 7.0),
        (dx + nl_knee[0], nl_knee[1] + bob_y, 4.8),
        (dx + nl_hock[0], nl_hock[1] + bob_y, 3.4),
        (dx + nl_foot[0], nl_foot[1], 4.2),
    ], C_MID)
    near.tapered_limb([(dx + nl_foot[0], nl_foot[1], 2.5), (dx + nl_foot[0] - 7, nl_foot[1] + 1, 0.7)], C_BONE)

    # 5. NEAR FRONT ATTACK ARM (Organic sculpted gorilla-mutant arm + heavy clawed knuckles!)
    na_e_y = na_el[1] + (0 if (rear_up or jumping) else bob_y)
    near.tapered_limb([
        (dx + na_sh[0], na_sh[1] + bob_y, 7.2),
        (dx + na_el[0], na_e_y, 5.4),
        (dx + na_kn[0], na_kn[1], 6.0),
    ], C_MID)
    # Bone knuckle shield & sharp claws
    near.tapered_limb([(dx + na_kn[0] - 4, na_kn[1] - 2, 2.2), (dx + na_kn[0] + 4, na_kn[1] - 2, 2.2)], C_BONE)
    for c_off in (-5, -2, 1):
        near.tapered_limb([(dx + na_kn[0] + c_off, na_kn[1], 1.4), (dx + na_kn[0] + c_off - 3, na_kn[1] + 3, 0.4)], C_BONE)

    near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
    near.auto_shade(C_BONE, C_BONE_HI, C_BONE_HI, C_BONE_SH, C_SHADOW)
    # Necrotic veins running along the sculpted forearm
    near.line([(dx + na_sh[0], na_sh[1] + bob_y + 3), (dx + na_el[0] - 1, na_e_y), (dx + na_kn[0] - 1, na_kn[1] - 5)], C_VEIN, width=2)
    near.add_outline(C_OUT, GROUND_Y + 1)

    if slam:
        fx.line([(dx + na_kn[0] - 10, GROUND_Y), (dx + na_kn[0] - 3, GROUND_Y - 5)], C_IMPACT, width=2)
        fx.line([(dx + na_kn[0] + 7, GROUND_Y), (dx + na_kn[0] + 13, GROUND_Y - 5)], C_IMPACT, width=2)

    return composite_layers(FRAME_W, FRAME_H, [far, body, near, fx])

def generate_full_sheet():
    sheet = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), (0, 0, 0, 0))
    for f in range(NUM_FRAMES):
        frame = draw_boss_frame(f)
        bbox = frame.getbbox()
        assert bbox[1] >= 0 and bbox[3] <= FRAME_H, f"Frame {f} Y out of bounds: {bbox}"
        assert bbox[0] >= 0 and bbox[2] <= FRAME_W, f"Frame {f} X out of bounds: {bbox}"
        sheet.paste(frame, (f * FRAME_W, 0))

    sheet.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Saved redesigned boss_pale_mutant.png (1024x96)")

    f0 = sheet.crop((0, 0, FRAME_W, FRAME_H))
    icon64 = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    icon_sub = f0.resize((64, 48), Image.Resampling.LANCZOS)
    icon64.paste(icon_sub, (0, 8), icon_sub)
    icon64.save("assets/tiles/enemy_boss.png")
    icon64.save("assets/tiles/tools/enemy_boss.png")

    icon32 = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    icon_sub32 = f0.resize((32, 24), Image.Resampling.LANCZOS)
    icon32.paste(icon_sub32, (0, 4), icon_sub32)
    icon32.save("assets/tiles/enemies/enemy_boss.png")
    icon32.save("assets/sprites/enemies/boss/preview.png")
    print("Updated boss preview icons.")

if __name__ == "__main__":
    generate_full_sheet()
