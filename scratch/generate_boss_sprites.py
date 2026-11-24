import numpy as np
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8
GROUND_Y = 95

# 1. Soft, Lineless Organic Palette (NO BLACK LINE ART!)
# Pure volumetric shading and value contrast (from bright specular to deep ambient tone):
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

def draw_lineless_boss_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Pale Mutant Boss:
    - True 3/4 perspective (near limbs in foreground, far limbs set back in depth)
    - Lineless pixel art (NO black outlines, pure volumetric shading)
    - Slender arched spine & deeply scooped underbelly (Sachiel-like gaunt silhouette)
    - Downward-angled faceless searchlight cowl
    - EXACTLY 4 limbs (2 front gorilla arms at chest, 2 digitigrade hind legs at hip)
    - STRICTLY bounded within [0..127, 4..95] (zero gap with floor, never clips outside frame!)
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

    # Far Arm anchors (depth front-right)
    fa_sx, fa_sy = 42, 34
    fa_ex, fa_ey = 38, 58
    fa_kx, fa_ky = 34, GROUND_Y

    # Near Leg anchors (foreground rear)
    nl_hip = (74, 48)
    nl_knee = (92, 68)
    nl_foot = (80, GROUND_Y)

    # Far Leg anchors (depth rear)
    fl_hip = (70, 50)
    fl_knee = (80, 68)
    fl_foot = (68, GROUND_Y)

    # Frame animation modifiers
    if frame_idx == 0:
        # Idle low
        pass
    elif frame_idx == 1:
        # Idle breath: subtle spine/chest rise
        bob_y = -2
    elif frame_idx == 2:
        # Knuckle walk 1: Near arm steps forward-left, Far arm & feet planted
        na_ex -= 6
        na_kx -= 8
        nl_knee = (88, 68)
        nl_foot = (76, GROUND_Y)
    elif frame_idx == 3:
        # Knuckle walk 2: Far arm steps forward-left, Near arm & feet planted
        fa_ex -= 6
        fa_kx -= 8
        fl_foot = (64, GROUND_Y)
        nl_foot = (84, GROUND_Y)
    elif frame_idx == 4:
        # Attack windup: Reared back on hind legs, fists raised!
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
        fl_hip = (68, 52)
        fl_knee = (80, 70)
        fl_foot = (66, GROUND_Y)
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
        fl_hip = (74, 56)
        fl_knee = (84, 74)
        fl_foot = (72, GROUND_Y)
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air (bounds Y in [6, 77])
        jumping = True
        bob_y = -4
        nl_foot = (80, 72)
        fl_foot = (68, 70)
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

    # =========================================================
    # LAYER 1: FAR HIND LEG (Background in 3/4 view)
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
    # LAYER 2: FAR FRONT ARM (Background at front chest in 3/4)
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
    # LAYER 3: 3/4 ARCHED TORSO & HOOD (Lineless, Organic 3D Form)
    # =========================================================
    torso_pts = [
        (dx + 18, 42 + bob_y),   # Front searchlight cowl (aimed down-forward)
        (dx + 16, 34 + bob_y),   # Upper prow
        (dx + 22, 24 + bob_y),   # Front slope
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
        (dx + 24, 44 + bob_y)    # Prow underside
    ]
    draw.polygon(torso_pts, fill=C_MID)

    # A. 3D Volumetric Shading of the Arched Spine (Top highlight, smooth falloff)
    spine_rim = [(dx + 17, 33 + bob_y), (dx + 23, 23 + bob_y), (dx + 35, 15 + bob_y), (dx + 51, 10 + bob_y), (dx + 66, 10 + bob_y), (dx + 78, 14 + bob_y), (dx + 85, 23 + bob_y), (dx + 87, 34 + bob_y)]
    draw.line(spine_rim, fill=C_HI, width=2)
    spine_light = [(dx + 19, 35 + bob_y), (dx + 25, 25 + bob_y), (dx + 37, 17 + bob_y), (dx + 52, 12 + bob_y), (dx + 67, 12 + bob_y), (dx + 78, 16 + bob_y), (dx + 84, 25 + bob_y), (dx + 86, 36 + bob_y)]
    draw.line(spine_light, fill=C_LIGHT, width=2)

    # B. Front "Searchlight" Cowl in 3/4 Perspective:
    prow_face = [(dx + 17, 34 + bob_y), (dx + 26, 26 + bob_y), (dx + 32, 38 + bob_y), (dx + 22, 43 + bob_y)]
    draw.polygon(prow_face, fill=C_LIGHT)
    draw.line([(dx + 18, 34 + bob_y), (dx + 22, 42 + bob_y)], fill=C_HI, width=2)
    draw.line([(dx + 24, 30 + bob_y), (dx + 30, 39 + bob_y)], fill=C_MID, width=2)

    # C. Deep Hollow Cavity Under the Slender Waist
    under_cavity = [(dx + 38, 34 + bob_y), (dx + 54, 28 + bob_y), (dx + 68, 38 + bob_y), (dx + 76, 48 + bob_y), (dx + 72, 42 + bob_y), (dx + 56, 32 + bob_y), (dx + 42, 36 + bob_y)]
    draw.polygon(under_cavity, fill=C_VOID)

    # D. Curved Organic Ivory Ribs
    ribs = [
        [(38, 28), (42, 34), (46, 38)],
        [(46, 24), (50, 30), (54, 34)],
        [(54, 22), (58, 28), (62, 32)],
        [(62, 22), (66, 28), (70, 34)]
    ]
    for pts in ribs:
        shadow_pts = [(dx + x, y + 1 + bob_y) for x, y in pts]
        draw.line(shadow_pts, fill=C_RIB_SHADOW, width=2)
        pts_shifted = [(dx + x, y + bob_y) for x, y in pts]
        draw.line(pts_shifted, fill=C_RIB_HI, width=2)
        draw.point(pts_shifted[-1], fill=C_RIB_HI)

    # E. Rear organic crescent shadow
    crescent_shadow = [
        (dx + 76, 18 + bob_y), (dx + 84, 24 + bob_y), (dx + 87, 34 + bob_y), (dx + 84, 46 + bob_y), (dx + 78, 48 + bob_y), (dx + 80, 36 + bob_y), (dx + 74, 24 + bob_y)
    ]
    draw.polygon(crescent_shadow, fill=C_SHADOW)

    # =========================================================
    # LAYER 4: NEAR HIND LEG (Foreground in 3/4 view)
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
    # LAYER 5: NEAR FRONT ARM (Foreground, closest to viewer)
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

    # Deltoid highlight
    draw.line([(dx + na_sx - 3, na_s_y - 1), (dx + na_sx + 4, na_s_y - 1)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 2, na_s_y + 2), (dx + na_ex - 3, na_e_y), (dx + na_kx - 3, na_k_y - 6)], fill=C_LIGHT, width=2)
    draw.line([(dx + na_sx - 1, na_s_y + 2), (dx + na_ex - 2, na_e_y)], fill=C_HI, width=1)
    # Inner shadow (defining separation from torso without black outline)
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
        draw_lineless_boss_frame(draw, f * FRAME_W, f)
        # Check strict bounds
        frame_crop = sheet.crop((f * FRAME_W, 0, (f + 1) * FRAME_W, FRAME_H))
        bbox = frame_crop.getbbox()
        print(f"Frame {f} bbox: {bbox}")
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

    # Save 2x full sheet check for visual verification
    sheet.resize((FRAME_W * NUM_FRAMES * 2, FRAME_H * 2), Image.NEAREST).save("/home/dagai/.gemini/antigravity/brain/1810658f-f67f-4d8d-a67d-8143b1ecf63e/all_frames_lineless_big.png")
    print("Saved all_frames_lineless_big.png")

if __name__ == "__main__":
    generate_full_sheet()
