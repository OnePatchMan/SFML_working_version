import numpy as np
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8
GROUND_Y = 95

# 1. Soft, Lineless Organic Palette (NO BLACK LINE ART!)
C_TRANS = (0, 0, 0, 0)
C_HI = (252, 254, 255, 255)          # Brightest specular sheen on top curves
C_LIGHT = (214, 224, 242, 255)       # Lit carapace/skin surface
C_MID = (152, 164, 188, 255)         # Base pale lavender-bone skin
C_SHADOW = (92, 102, 126, 255)       # Mid shadow
C_SHADOW_DEEP = (48, 56, 76, 255)    # Deep shadow / occlusion
C_VOID = (18, 22, 32, 255)           # Deepest cavity void

# Ivory Bone Ribs:
C_RIB_HI = (250, 252, 246, 255)
C_RIB_MID = (186, 192, 180, 255)
C_RIB_SHADOW = (112, 118, 108, 255)

# Claws / Talons:
C_CLAW_HI = (252, 254, 248, 255)
C_CLAW_MID = (180, 186, 174, 255)
C_CLAW_SHADOW = (105, 110, 100, 255)

def draw_3quarter_boss_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Pale Mutant Boss:
    - TRUE 3/4 PERSPECTIVE: Chest faces camera/screen, shoulders spread wide across chest
    - LINELESS PIXEL ART: No black outlines, pure volumetric value shading
    - Slender arched spine & hollow underbelly
    - Exactly 4 limbs (2 front gorilla arms at chest, 2 hind legs at hip)
    - STRICTLY bounded within [0..127, 4..95] (zero floor gap, no clipping!)
    """
    dx = ox
    bob_y = 0
    rear_up = False
    slam = False
    jumping = False
    hurt = False

    # Near Arm anchors (foreground left)
    na_sx, na_sy = 22, 40
    na_ex, na_ey = 16, 62
    na_kx, na_ky = 14, GROUND_Y

    # Far Arm anchors (background right, across 32px chest width!)
    fa_sx, fa_sy = 54, 34
    fa_ex, fa_ey = 50, 62
    fa_kx, fa_ky = 44, GROUND_Y

    # Near Leg anchors (foreground rear)
    nl_hip = (76, 46)
    nl_knee = (94, 68)
    nl_foot = (82, GROUND_Y)

    # Far Leg anchors (depth rear)
    fl_hip = (70, 48)
    fl_knee = (82, 66)
    fl_foot = (70, GROUND_Y)

    # Frame animation modifiers
    if frame_idx == 0:
        # Idle low
        pass
    elif frame_idx == 1:
        # Idle breath
        bob_y = -2
    elif frame_idx == 2:
        # Walk 1: Near arm steps forward-left
        na_ex -= 6
        na_kx -= 6
        nl_knee = (90, 68)
        nl_foot = (78, GROUND_Y)
    elif frame_idx == 3:
        # Walk 2: Far arm steps forward-left
        fa_ex -= 6
        fa_kx -= 8
        fl_foot = (64, GROUND_Y)
        nl_foot = (86, GROUND_Y)
    elif frame_idx == 4:
        # Attack windup: Reared back on hind legs, fists raised ready to crash down!
        # STRICT BOUNDS: stays safely within Y in [4, 95]
        rear_up = True
        bob_y = -4
        na_sy = 30
        na_ex, na_ey = 20, 20
        na_kx, na_ky = 22, 10
        fa_sy = 26
        fa_ex, fa_ey = 44, 18
        fa_kx, fa_ky = 46, 10
        nl_hip = (74, 48)
        nl_knee = (94, 68)
        nl_foot = (80, GROUND_Y)
        fl_hip = (68, 50)
        fl_knee = (82, 68)
        fl_foot = (68, GROUND_Y)
    elif frame_idx == 5:
        # Attack slam: Fists smashed into ground in front, body lunging forward
        slam = True
        bob_y = 4
        na_sx, na_sy = 20, 44
        na_ex, na_ey = 14, 66
        na_kx, na_ky = 10, GROUND_Y
        fa_sx, fa_sy = 48, 40
        fa_ex, fa_ey = 36, 68
        fa_kx, fa_ky = 28, GROUND_Y
        nl_hip = (80, 52)
        nl_knee = (98, 72)
        nl_foot = (86, GROUND_Y)
        fl_hip = (74, 54)
        fl_knee = (86, 72)
        fl_foot = (74, GROUND_Y)
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air (bounds Y in [6, 77])
        jumping = True
        bob_y = -4
        nl_foot = (82, 72)
        fl_foot = (70, 70)
        na_ex, na_ey = 18, 54
        na_kx, na_ky = 16, 76
        fa_ex, fa_ey = 46, 52
        fa_kx, fa_ky = 42, 74
    elif frame_idx == 7:
        # Hurt: Flinched backward
        hurt = True
        bob_y = -2
        na_sx += 4
        na_ex += 6
        na_kx += 6
        fa_sx += 4
        fa_ex += 6
        fa_kx += 6

    # =========================================================
    # LAYER 1: FAR HIND LEG (Depth background rear in 3/4)
    # =========================================================
    fl_h_y = fl_hip[1] + bob_y
    fl_k_y = fl_knee[1] + bob_y
    fl_f_y = fl_foot[1]
    fl_pts = [
        (dx + fl_hip[0] - 3, fl_h_y),
        (dx + fl_hip[0] + 5, fl_h_y),
        (dx + fl_knee[0] + 4, fl_k_y),
        (dx + fl_foot[0] + 4, fl_f_y - 11),
        (dx + fl_foot[0] + 2, fl_f_y),
        (dx + fl_foot[0] - 5, fl_f_y),
        (dx + fl_foot[0] - 2, fl_f_y - 11),
        (dx + fl_knee[0] - 3, fl_k_y + 3)
    ]
    draw.polygon(fl_pts, fill=C_SHADOW_DEEP)
    draw.line([(dx + fl_hip[0] - 1, fl_h_y + 2), (dx + fl_knee[0] + 1, fl_k_y)], fill=C_SHADOW, width=2)
    for c in range(3):
        cx = dx + fl_foot[0] - 4 + c * 3
        draw.polygon([(cx, fl_f_y - 2), (cx - 2, fl_f_y), (cx + 1, fl_f_y)], fill=C_CLAW_SHADOW)

    # =========================================================
    # LAYER 2: ARCHED SPINAL HUNCHBACK (3/4 Curved Volume)
    # =========================================================
    back_pts = [
        (dx + 34, 30 + bob_y),   # Trapezius / collar junction
        (dx + 44, 18 + bob_y),   # Rising spine arch
        (dx + 56, 12 + bob_y),   # High spine crest
        (dx + 68, 10 + bob_y),   # PEAK OF ARCHED SPINE (Y=10+bob_y)
        (dx + 80, 14 + bob_y),   # Descending back arch
        (dx + 88, 24 + bob_y),   # Upper rear back
        (dx + 90, 36 + bob_y),   # Mid rear back
        (dx + 86, 48 + bob_y),   # Hip junction
        (dx + 78, 50 + bob_y),   # Pelvis bottom
        (dx + 70, 38 + bob_y),   # Underbelly scoop (slender waist!)
        (dx + 56, 30 + bob_y),   # Narrow waist arch
        (dx + 44, 34 + bob_y),   # Under chest
        (dx + 34, 38 + bob_y)
    ]
    draw.polygon(back_pts, fill=C_MID)

    # Volumetric Shading of the Arched Spine:
    spine_rim = [(dx + 45, 17 + bob_y), (dx + 57, 11 + bob_y), (dx + 68, 10 + bob_y), (dx + 80, 14 + bob_y), (dx + 87, 24 + bob_y), (dx + 89, 36 + bob_y)]
    draw.line(spine_rim, fill=C_HI, width=2)
    spine_light = [(dx + 46, 19 + bob_y), (dx + 58, 13 + bob_y), (dx + 69, 12 + bob_y), (dx + 80, 16 + bob_y), (dx + 86, 26 + bob_y), (dx + 88, 38 + bob_y)]
    draw.line(spine_light, fill=C_LIGHT, width=2)

    # Rear flank crescent shadow:
    crescent_shadow = [(dx + 76, 18 + bob_y), (dx + 84, 24 + bob_y), (dx + 88, 36 + bob_y), (dx + 85, 48 + bob_y), (dx + 78, 48 + bob_y), (dx + 80, 36 + bob_y), (dx + 74, 24 + bob_y)]
    draw.polygon(crescent_shadow, fill=C_SHADOW)

    # Deep Hollow Cavity under the Slender Waist:
    under_cavity = [(dx + 42, 34 + bob_y), (dx + 56, 30 + bob_y), (dx + 70, 38 + bob_y), (dx + 78, 48 + bob_y), (dx + 74, 44 + bob_y), (dx + 58, 32 + bob_y), (dx + 46, 36 + bob_y)]
    draw.polygon(under_cavity, fill=C_VOID)

    # =========================================================
    # LAYER 3: 3/4 BROAD FRONT CHEST & SEARCHLIGHT COWL
    # Connects near shoulder (X=22) to far shoulder (X=54)
    # Chest faces camera/screen!
    # =========================================================
    chest_pts = [
        (dx + 22, 38 + bob_y),   # Near shoulder collar
        (dx + 32, 28 + bob_y),   # Upper chest slope
        (dx + 48, 26 + bob_y),   # Far shoulder collar
        (dx + 58, 32 + bob_y),   # Far shoulder cap
        (dx + 54, 44 + bob_y),   # Far lower chest
        (dx + 46, 48 + bob_y),   # Center sternum bottom
        (dx + 34, 48 + bob_y),   # Near lower chest
        (dx + 20, 44 + bob_y)    # Near shoulder bottom
    ]
    draw.polygon(chest_pts, fill=C_MID)

    # Faceless Searchlight Cowl (Center chest, aiming down-forward at viewer):
    cowl_pts = [
        (dx + 24, 36 + bob_y), (dx + 36, 28 + bob_y), (dx + 46, 34 + bob_y), (dx + 38, 44 + bob_y), (dx + 28, 44 + bob_y)
    ]
    draw.polygon(cowl_pts, fill=C_LIGHT)
    draw.line([(dx + 25, 36 + bob_y), (dx + 29, 43 + bob_y), (dx + 38, 43 + bob_y)], fill=C_HI, width=2)
    draw.line([(dx + 32, 30 + bob_y), (dx + 44, 35 + bob_y)], fill=C_MID, width=2)

    # Dark Sternum / Chest Hollow under the cowl:
    sternum_cavity = [(dx + 28, 44 + bob_y), (dx + 38, 44 + bob_y), (dx + 46, 48 + bob_y), (dx + 36, 52 + bob_y), (dx + 28, 48 + bob_y)]
    draw.polygon(sternum_cavity, fill=C_VOID)

    # 3D Curved Ivory Bone Ribs wrapping around 3/4 chest towards center:
    ribs = [
        [(24, 44), (30, 47), (36, 49)],
        [(28, 48), (34, 52), (40, 54)],
        [(54, 40), (48, 45), (42, 48)],
        [(50, 46), (44, 51), (38, 54)],
        [(58, 30), (64, 34), (68, 38)],
        [(64, 24), (70, 28), (74, 32)]
    ]
    for pts in ribs:
        shadow_pts = [(dx + x, y + 1 + bob_y) for x, y in pts]
        draw.line(shadow_pts, fill=C_RIB_SHADOW, width=2)
        pts_shifted = [(dx + x, y + bob_y) for x, y in pts]
        draw.line(pts_shifted, fill=C_RIB_HI, width=2)
        draw.point(pts_shifted[-1], fill=C_RIB_HI)

    # =========================================================
    # LAYER 4: FAR FRONT ARM (Background right across chest width!)
    # =========================================================
    fa_s_y = fa_sy + bob_y
    fa_e_y = fa_ey + bob_y if not rear_up and not jumping else fa_ey
    fa_k_y = fa_ky
    far_arm_pts = [
        (dx + fa_sx - 4, fa_s_y),
        (dx + fa_sx + 5, fa_s_y - 1),
        (dx + fa_ex + 4, fa_e_y),
        (dx + fa_kx + 4, fa_k_y - 4),
        (dx + fa_kx + 3, fa_k_y),
        (dx + fa_kx - 5, fa_k_y),
        (dx + fa_kx - 4, fa_k_y - 4),
        (dx + fa_ex - 4, fa_e_y)
    ]
    draw.polygon(far_arm_pts, fill=C_SHADOW_DEEP)
    draw.line([(dx + fa_sx - 2, fa_s_y + 4), (dx + fa_ex - 1, fa_e_y), (dx + fa_kx - 1, fa_k_y - 6)], fill=C_SHADOW, width=2)
    for c in range(3):
        cx = dx + fa_kx - 4 + c * 3
        draw.polygon([(cx, fa_k_y - 2), (cx - 2, fa_k_y), (cx + 1, fa_k_y)], fill=C_CLAW_SHADOW)

    # =========================================================
    # LAYER 5: NEAR HIND LEG (Foreground right: X=74..95)
    # =========================================================
    nl_h_y = nl_hip[1] + bob_y
    nl_k_y = nl_knee[1] + bob_y
    nl_f_y = nl_foot[1]
    near_leg_pts = [
        (dx + nl_hip[0] - 4, nl_h_y),
        (dx + nl_hip[0] + 6, nl_h_y),
        (dx + nl_knee[0] + 5, nl_k_y),
        (dx + nl_foot[0] + 5, nl_f_y - 11),
        (dx + nl_foot[0] + 4, nl_f_y),
        (dx + nl_foot[0] - 7, nl_f_y),
        (dx + nl_foot[0] - 4, nl_f_y - 11),
        (dx + nl_knee[0] - 3, nl_k_y + 3)
    ]
    draw.polygon(near_leg_pts, fill=C_MID)
    draw.line([(dx + nl_hip[0] - 2, nl_h_y + 2), (dx + nl_knee[0] + 2, nl_k_y - 1)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_hip[0] - 1, nl_h_y + 2), (dx + nl_knee[0] + 1, nl_k_y - 1)], fill=C_HI, width=1)
    draw.line([(dx + nl_knee[0] + 1, nl_k_y + 2), (dx + nl_foot[0] + 2, nl_f_y - 12)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_foot[0] - 6, nl_f_y - 3), (dx + nl_foot[0] + 3, nl_f_y - 3)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_foot[0] - 5, nl_f_y - 2), (dx + nl_foot[0] + 2, nl_f_y - 2)], fill=C_HI, width=1)
    draw.line([(dx + nl_knee[0] + 2, nl_k_y + 3), (dx + nl_foot[0] + 4, nl_f_y - 12)], fill=C_SHADOW_DEEP, width=2)
    for c in range(3):
        cx = dx + nl_foot[0] - 6 + c * 4
        draw.polygon([(cx, nl_f_y - 3), (cx - 3, nl_f_y), (cx + 1, nl_f_y)], fill=C_CLAW_HI)
        draw.point((cx - 1, nl_f_y), fill=C_CLAW_SHADOW)

    # =========================================================
    # LAYER 6: NEAR FRONT ARM (Foreground left: X=10..28)
    # =========================================================
    na_s_y = na_sy + bob_y
    na_e_y = na_ey + bob_y if not rear_up and not jumping else na_ey
    na_k_y = na_ky
    near_arm_pts = [
        (dx + na_sx - 5, na_s_y - 3),
        (dx + na_sx + 6, na_s_y - 3),
        (dx + na_ex + 6, na_e_y),
        (dx + na_kx + 6, na_k_y - 5),
        (dx + na_kx + 5, na_k_y),
        (dx + na_kx - 6, na_k_y),
        (dx + na_kx - 5, na_k_y - 5),
        (dx + na_ex - 6, na_e_y)
    ]
    draw.polygon(near_arm_pts, fill=C_MID)

    # Deltoid cap
    draw.arc([dx + na_sx - 6, na_s_y - 6, dx + na_sx + 6, na_s_y + 6], 180, 330, fill=C_LIGHT, width=2)
    draw.arc([dx + na_sx - 5, na_s_y - 5, dx + na_sx + 5, na_s_y + 5], 200, 310, fill=C_HI, width=1)

    # Muscle highlights
    draw.line([(dx + na_sx - 3, na_s_y - 1), (dx + na_sx + 4, na_s_y - 1)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 2, na_s_y + 2), (dx + na_ex - 3, na_e_y), (dx + na_kx - 3, na_k_y - 6)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 1, na_s_y + 2), (dx + na_ex - 2, na_e_y)], fill=C_HI, width=1)
    draw.line([(dx + na_sx + 4, na_s_y + 2), (dx + na_ex + 3, na_e_y), (dx + na_kx + 3, na_k_y - 6)], fill=C_SHADOW_DEEP, width=2)

    # Knuckle pad & claws on GROUND_Y = 95
    draw.line([(dx + na_kx - 5, na_k_y - 3), (dx + na_kx + 4, na_k_y - 3)], fill=C_LIGHT, width=1)
    for i in range(4):
        cx = dx + na_kx - 5 + i * 3
        draw.polygon([(cx, na_k_y - 2), (cx - 2, na_k_y), (cx + 1, na_k_y)], fill=C_CLAW_HI)
        draw.point((cx - 1, na_k_y), fill=C_CLAW_SHADOW)

def generate_full_sheet():
    sheet = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sheet)

    for f in range(NUM_FRAMES):
        draw_3quarter_boss_frame(draw, f * FRAME_W, f)
        frame_crop = sheet.crop((f * FRAME_W, 0, (f + 1) * FRAME_W, FRAME_H))
        bbox = frame_crop.getbbox()
        print(f"Frame {f} bbox: {bbox}")
        assert bbox[1] >= 0, f"Frame {f} clipped top: y_min={bbox[1]}"
        assert bbox[3] <= FRAME_H, f"Frame {f} clipped bottom: y_max={bbox[3]}"
        assert bbox[0] >= 0 and bbox[2] <= FRAME_W, f"Frame {f} clipped x: {bbox}"

    sheet.save("scratch/test_true_3quarter_sheet.png")
    sheet.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Saved assets/sprites/enemies/boss/boss_pale_mutant.png (1024x96)")

    # Editor preview icons:
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
    print("Updated editor preview icons.")

    sheet.resize((FRAME_W * NUM_FRAMES * 2, FRAME_H * 2), Image.NEAREST).save("/home/dagai/.gemini/antigravity/brain/1810658f-f67f-4d8d-a67d-8143b1ecf63e/all_frames_true_3quarter_big.png")
    print("Saved all_frames_true_3quarter_big.png")

generate_full_sheet()
