import numpy as np
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8
GROUND_Y = 95

# 1. Palette: Sickly pale organic mutant (Sachiel / Alien / Bio-horror)
C_TRANS = (0, 0, 0, 0)
C_OUT = (18, 20, 28, 255)            # Deep organic dark outline
C_SHADOW_DEEP = (44, 50, 68, 255)    # Deep shadow cavity
C_SHADOW = (82, 92, 116, 255)        # Flesh/carapace shadow
C_MID = (146, 158, 184, 255)         # Base pale skin/bone
C_LIGHT = (206, 216, 236, 255)       # Lit skin surface
C_HI = (252, 254, 255, 255)          # Soft rim highlight

C_RIB_DARK = (100, 105, 96, 255)
C_RIB_LIGHT = (244, 246, 238, 255)

C_CLAW_DARK = (108, 112, 104, 255)
C_CLAW_HI = (250, 252, 246, 255)
C_VEIN = (92, 46, 74, 255)
C_SINEW = (136, 42, 58, 255)

def draw_organic_boss_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Pale Mutant Boss (Organic, Slender, 3/4 perspective):
    - NO unneeded middle limb (exactly 4 limbs: 2 front arms at chest, 2 hind legs at hip)
    - Slender arched spine & deeply scooped underbelly (matches user's red sketch)
    - Organic anatomy with smooth muscle contours (NO robot ball joints, boxes or cylinder cuts)
    - Front cowl points down-forward at acute angle (faceless smooth organic carapace)
    - STRICTLY bounded within [0..127, 2..95] (zero gap on ground, never clips outside frame!)
    """
    dx = ox
    bob_y = 0
    rear_up = False
    slam = False
    jumping = False
    hurt = False

    # Near Arm anchors
    na_sx, na_sy = 30, 36
    na_ex, na_ey = 22, 58
    na_kx, na_ky = 18, GROUND_Y

    # Far Arm anchors
    fa_sx, fa_sy = 42, 34
    fa_ex, fa_ey = 38, 58
    fa_kx, fa_ky = 34, GROUND_Y

    # Near Leg anchors
    nl_hip = (74, 48)
    nl_knee = (92, 68)
    nl_foot = (80, GROUND_Y)

    # Far Leg anchors
    fl_hip = (76, 50)
    fl_knee = (90, 70)
    fl_foot = (84, GROUND_Y)

    # Frame animation modifiers
    if frame_idx == 0:
        # Idle low
        pass
    elif frame_idx == 1:
        # Idle breath: subtle chest/spine rise
        bob_y = -2
    elif frame_idx == 2:
        # Knuckle walk 1: Near arm steps forward-left
        na_ex -= 6
        na_kx -= 8
        nl_knee = (88, 68)
        nl_foot = (76, GROUND_Y)
    elif frame_idx == 3:
        # Knuckle walk 2: Far arm steps forward-left
        fa_ex -= 6
        fa_kx -= 8
        fl_foot = (80, GROUND_Y)
        nl_foot = (84, GROUND_Y)
    elif frame_idx == 4:
        # Attack windup: Reared back on hind legs, fists raised ready to crash down!
        # STRICT BOUNDS: stays safely within Y in [4, 95]
        rear_up = True
        bob_y = -4
        na_sy = 28
        na_ex, na_ey = 24, 20
        na_kx, na_ky = 26, 10
        fa_sy = 26
        fa_ex, fa_ey = 44, 18
        fa_kx, fa_ky = 46, 10
        nl_hip = (72, 50)
        nl_knee = (92, 68)
        nl_foot = (78, GROUND_Y)
    elif frame_idx == 5:
        # Attack slam: Fists smashed into ground in front, body lunging forward
        slam = True
        bob_y = 4
        na_sx, na_sy = 28, 42
        na_ex, na_ey = 18, 64
        na_kx, na_ky = 12, GROUND_Y
        fa_sx, fa_sy = 40, 40
        fa_ex, fa_ey = 30, 66
        fa_kx, fa_ky = 24, GROUND_Y
        nl_hip = (78, 54)
        nl_knee = (96, 72)
        nl_foot = (84, GROUND_Y)
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air (bounds Y in [6, 77])
        jumping = True
        bob_y = -4
        nl_foot = (80, 72)
        fl_foot = (86, 70)
        na_ex, na_ey = 22, 54
        na_kx, na_ky = 18, 76
        fa_ex, fa_ey = 38, 52
        fa_kx, fa_ky = 34, 74
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

    # -------------------------------------------------------------
    # 1. FAR HIND LEG (Background)
    # -------------------------------------------------------------
    fl_h_y = fl_hip[1] + bob_y
    fl_k_y = fl_knee[1] + bob_y
    fl_f_y = fl_foot[1]
    far_leg_pts = [
        (dx + fl_hip[0] - 4, fl_h_y),
        (dx + fl_hip[0] + 6, fl_h_y),
        (dx + fl_knee[0] + 5, fl_k_y),
        (dx + fl_foot[0] + 4, fl_f_y - 11),
        (dx + fl_foot[0] + 2, fl_f_y),
        (dx + fl_foot[0] - 6, fl_f_y),
        (dx + fl_foot[0] - 3, fl_f_y - 11),
        (dx + fl_knee[0] - 3, fl_k_y + 3)
    ]
    draw.polygon(far_leg_pts, fill=C_SHADOW, outline=C_OUT)
    for c in range(3):
        cx = dx + fl_foot[0] - 5 + c * 3
        draw.polygon([(cx, fl_f_y - 2), (cx - 2, fl_f_y), (cx + 1, fl_f_y)], fill=C_CLAW_DARK, outline=C_OUT)

    # -------------------------------------------------------------
    # 2. FAR FRONT ARM (Background, at FRONT chest!)
    # -------------------------------------------------------------
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
    draw.polygon(far_arm_pts, fill=C_SHADOW, outline=C_OUT)
    draw.line([(dx + fa_sx - 2, fa_s_y + 4), (dx + fa_ex - 1, fa_e_y), (dx + fa_kx - 1, fa_k_y - 6)], fill=C_MID, width=2)
    draw.line([(dx + fa_sx, fa_s_y + 5), (dx + fa_ex, fa_e_y - 2)], fill=C_VEIN, width=1)
    for c in range(3):
        cx = dx + fa_kx - 4 + c * 3
        draw.polygon([(cx, fa_k_y - 2), (cx - 2, fa_k_y), (cx + 1, fa_k_y)], fill=C_CLAW_DARK, outline=C_OUT)

    # -------------------------------------------------------------
    # 3. SLENDER ARCHED TORSO (User's red curves!)
    # -------------------------------------------------------------
    torso_pts = [
        (dx + 18, 42 + bob_y),   # Front searchlight nose (angled down-forward towards floor)
        (dx + 16, 34 + bob_y),   # Upper front prow
        (dx + 22, 24 + bob_y),   # Front chest slope
        (dx + 34, 16 + bob_y),   # Upper chest curve
        (dx + 50, 11 + bob_y),   # Rising spine arch
        (dx + 66, 10 + bob_y),   # PEAK OF ARCHED SPINE (Y=10+bob_y)
        (dx + 78, 14 + bob_y),   # Descending back arch
        (dx + 86, 22 + bob_y),   # Upper rear back
        (dx + 88, 34 + bob_y),   # Mid rear back
        (dx + 84, 48 + bob_y),   # Hip junction
        (dx + 76, 50 + bob_y),   # Pelvis bottom
        (dx + 68, 38 + bob_y),   # Underbelly scoop: rises UP high (slender body!)
        (dx + 54, 28 + bob_y),   # Narrow waist arch (body thickness only ~14-16px!)
        (dx + 40, 34 + bob_y),   # Under chest
        (dx + 24, 44 + bob_y)    # Nose underside
    ]
    draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)

    # Top rim light along arched spine
    spine_rim = [(dx + 17, 33 + bob_y), (dx + 23, 23 + bob_y), (dx + 35, 15 + bob_y), (dx + 51, 10 + bob_y), (dx + 66, 10 + bob_y), (dx + 78, 14 + bob_y), (dx + 85, 23 + bob_y), (dx + 87, 34 + bob_y)]
    draw.line(spine_rim, fill=C_HI, width=2)
    spine_light = [(dx + 19, 35 + bob_y), (dx + 25, 25 + bob_y), (dx + 37, 17 + bob_y), (dx + 52, 12 + bob_y), (dx + 67, 12 + bob_y), (dx + 78, 16 + bob_y), (dx + 84, 25 + bob_y), (dx + 86, 36 + bob_y)]
    draw.line(spine_light, fill=C_LIGHT, width=2)

    # Jagged bone vertebrae crest along arched spine
    for s in range(5):
        vx = dx + 38 + s * 10
        vy = (16 if s in (0, 4) else 12) + bob_y
        draw.polygon([(vx, vy), (vx + 2, vy - 3), (vx + 4, vy)], fill=C_RIB_LIGHT, outline=C_OUT)
        draw.point((vx + 2, vy - 2), fill=C_HI)

    # Front Cowl
    prow_face = [(dx + 17, 34 + bob_y), (dx + 26, 26 + bob_y), (dx + 32, 38 + bob_y), (dx + 22, 43 + bob_y)]
    draw.polygon(prow_face, fill=C_LIGHT)
    draw.line([(dx + 18, 34 + bob_y), (dx + 22, 42 + bob_y)], fill=C_HI, width=2)

    # Deep underbelly cavity with visceral sinew strands
    under_cavity = [(dx + 38, 34 + bob_y), (dx + 54, 28 + bob_y), (dx + 68, 38 + bob_y), (dx + 76, 48 + bob_y), (dx + 72, 42 + bob_y), (dx + 56, 32 + bob_y), (dx + 42, 36 + bob_y)]
    draw.polygon(under_cavity, fill=C_SHADOW_DEEP)
    draw.line([(dx + 48, 32 + bob_y), (dx + 62, 34 + bob_y)], fill=C_SINEW, width=1)
    draw.line([(dx + 52, 30 + bob_y), (dx + 66, 37 + bob_y)], fill=C_VEIN, width=1)

    # Ivory Bone Ribs (Curving along the flank)
    ribs = [
        [(38, 28), (42, 34), (46, 38)],
        [(46, 24), (50, 30), (54, 34)],
        [(54, 22), (58, 28), (62, 32)],
        [(62, 22), (66, 28), (70, 34)]
    ]
    for pts in ribs:
        pts_shifted = [(dx + x, y + bob_y) for x, y in pts]
        draw.line(pts_shifted, fill=C_RIB_LIGHT, width=2)
        shadow_shifted = [(dx + x, y + 1 + bob_y) for x, y in pts]
        draw.line(shadow_shifted, fill=C_RIB_DARK, width=1)
        draw.point(pts_shifted[-1], fill=C_RIB_LIGHT)

    # Crescent shadow on rear dome
    crescent_shadow = [
        (dx + 76, 18 + bob_y), (dx + 84, 24 + bob_y), (dx + 87, 34 + bob_y), (dx + 84, 46 + bob_y), (dx + 78, 48 + bob_y), (dx + 80, 36 + bob_y), (dx + 74, 24 + bob_y)
    ]
    draw.polygon(crescent_shadow, fill=C_SHADOW)

    # -------------------------------------------------------------
    # 4. NEAR HIND LEG (Foreground leg)
    # -------------------------------------------------------------
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
    draw.polygon(near_leg_pts, fill=C_MID, outline=C_OUT)
    draw.line([(dx + nl_hip[0] - 2, nl_h_y + 2), (dx + nl_knee[0] + 2, nl_k_y - 1)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_knee[0] + 1, nl_k_y + 2), (dx + nl_foot[0] + 2, nl_f_y - 12)], fill=C_LIGHT, width=2)
    draw.line([(dx + nl_foot[0] - 6, nl_f_y - 3), (dx + nl_foot[0] + 3, nl_f_y - 3)], fill=C_LIGHT, width=2)
    for c in range(3):
        cx = dx + nl_foot[0] - 6 + c * 4
        draw.polygon([(cx, nl_f_y - 3), (cx - 3, nl_f_y), (cx + 1, nl_f_y)], fill=C_CLAW_HI, outline=C_OUT)

    # -------------------------------------------------------------
    # 5. NEAR FRONT ARM (Foreground, at FRONT of chest!)
    # -------------------------------------------------------------
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
    draw.polygon(near_arm_pts, fill=C_MID, outline=C_OUT)

    draw.line([(dx + na_sx - 3, na_s_y - 1), (dx + na_sx + 4, na_s_y - 1)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 2, na_s_y + 2), (dx + na_ex - 3, na_e_y), (dx + na_kx - 3, na_k_y - 6)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 1, na_s_y + 2), (dx + na_ex - 2, na_e_y)], fill=C_HI, width=1)
    draw.line([(dx + na_sx + 4, na_s_y + 2), (dx + na_ex + 3, na_e_y), (dx + na_kx + 3, na_k_y - 6)], fill=C_SHADOW, width=2)
    # Bulging dark necrotic veins and sinews running down the front attack arm
    draw.line([(dx + na_sx, na_s_y + 3), (dx + na_ex - 1, na_e_y - 2), (dx + na_kx - 1, na_k_y - 7)], fill=C_VEIN, width=2)
    draw.line([(dx + na_sx + 2, na_s_y + 5), (dx + na_ex + 1, na_e_y)], fill=C_SINEW, width=1)

    # Calcified bone knuckle shield on the fist
    draw.rectangle([dx + na_kx - 6, na_k_y - 6, dx + na_kx + 5, na_k_y - 3], fill=C_RIB_LIGHT, outline=C_OUT)
    draw.line([(dx + na_kx - 5, na_k_y - 5), (dx + na_kx + 4, na_k_y - 5)], fill=C_HI, width=1)

    # Heavy curved bone claws digging into the floor
    for i in range(4):
        cx = dx + na_kx - 5 + i * 3
        draw.polygon([(cx, na_k_y - 3), (cx - 3, na_k_y), (cx + 1, na_k_y)], fill=C_CLAW_HI, outline=C_OUT)
        draw.point((cx - 1, na_k_y), fill=C_CLAW_DARK)

def generate_full_sheet():
    sheet = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sheet)

    for f in range(NUM_FRAMES):
        draw_organic_boss_frame(draw, f * FRAME_W, f)
        # Verify strict bounds
        frame_crop = sheet.crop((f * FRAME_W, 0, (f + 1) * FRAME_W, FRAME_H))
        bbox = frame_crop.getbbox()
        assert bbox[1] >= 0, f"Frame {f} clipped top: y_min={bbox[1]}"
        assert bbox[3] <= FRAME_H, f"Frame {f} clipped bottom: y_max={bbox[3]}"
        assert bbox[0] >= 0 and bbox[2] <= FRAME_W, f"Frame {f} clipped x: {bbox}"

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
