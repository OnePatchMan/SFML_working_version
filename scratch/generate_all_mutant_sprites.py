import os
import math
from PIL import Image, ImageDraw

os.makedirs("assets/sprites/enemies/duelist_aggressive", exist_ok=True)
os.makedirs("assets/sprites/enemies/duelist_passive", exist_ok=True)
os.makedirs("assets/sprites/enemies/normal", exist_ok=True)
os.makedirs("assets/sprites/enemies/tank", exist_ok=True)
os.makedirs("assets/sprites/enemies/dummy", exist_ok=True)
os.makedirs("assets/sprites/enemies/lurker", exist_ok=True)
os.makedirs("assets/sprites/enemies/boss", exist_ok=True)

# =========================================================================
# 1. DUELIST AGGRESSIVE (Sharp conical beak head, bent mantis scythes, rotting grey flesh)
# Frame size: 48 x 64, 6 frames (Idle, Walk1, Walk2, Windup, Slash, Hurt)
# =========================================================================
def generate_duelist_aggressive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (14, 16, 22, 255)
    C_DEEP = (34, 36, 46, 255)
    C_SHADOW = (64, 68, 82, 255)
    C_MID_DARK = (92, 96, 110, 255)
    C_MID = (134, 138, 152, 255)
    C_LIGHT = (186, 192, 206, 255)
    C_HI = (238, 244, 254, 255)
    C_VEIN = (98, 48, 76, 255)
    C_SINEW = (148, 38, 56, 255)
    C_SCYTHE_BASE = (168, 175, 186, 255)
    C_SCYTHE_EDGE = (250, 254, 255, 255)
    C_EYE = (255, 25, 65, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        bob_y = 0
        step = 0
        rearing = (f == 3)
        slashing = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -1
            step = -3
        elif f == 2:
            bob_y = -1
            step = 3
        elif f == 3:
            bob_y = -2
        elif f == 4:
            bob_y = 2
        elif f == 5:
            bob_y = -1

        cx = ox + 24
        cy = 32 + bob_y

        # Far leg (Background)
        fl_foot_x = cx + 8 + (step if not slashing else 12)
        fl_knee_x = cx + 11 + (step // 2)
        far_leg = [
            (cx + 3, cy + 9), (cx + 8, cy + 9),
            (fl_knee_x + 3, cy + 18),
            (fl_foot_x + 3, ground_y), (fl_foot_x - 3, ground_y),
            (fl_knee_x - 3, cy + 19)
        ]
        draw.polygon(far_leg, fill=C_SHADOW, outline=C_OUT)
        draw.line([(fl_knee_x, cy + 19), (fl_foot_x, ground_y - 2)], fill=C_MID_DARK, width=1)
        # Claws
        draw.polygon([(fl_foot_x - 3, ground_y - 2), (fl_foot_x - 5, ground_y), (fl_foot_x - 1, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)
        draw.polygon([(fl_foot_x + 1, ground_y - 2), (fl_foot_x, ground_y), (fl_foot_x + 3, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)

        # Far Mantis Scythe Arm (Background)
        if rearing:
            fa_el = (cx - 4, cy - 18)
            fa_tip = (cx + 12, cy - 28)
            draw.line([(cx + 2, cy - 4), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)
            draw.polygon([(fa_el[0] + 4, fa_el[1] - 3), (fa_el[0] + 7, fa_el[1] - 4), (fa_el[0] + 5, fa_el[1] - 1)], fill=C_SCYTHE_EDGE)
        elif slashing:
            fa_el = (cx - 16, cy + 1)
            fa_tip = (cx - 26, cy + 8)
            draw.line([(cx + 2, cy - 2), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)
            draw.polygon([(cx - 20, cy + 5), (cx - 22, cy + 2), (cx - 22, cy + 7)], fill=C_SCYTHE_EDGE)
        else:
            fa_el = (cx - 2, cy + 4)
            fa_tip = (cx - 14, cy + 16)
            draw.line([(cx + 2, cy - 4), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)
            draw.polygon([(cx - 7, cy + 11), (cx - 9, cy + 9), (cx - 8, cy + 13)], fill=C_SCYTHE_EDGE)

        # Torso (Skeletal, pinched, rotting flesh with exposed ribs and bruised sinews)
        torso_pts = [
            (cx - 5, cy - 9), (cx + 6, cy - 9),
            (cx + 7, cy + 2), (cx + 4, cy + 11),
            (cx - 3, cy + 11), (cx - 6, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx - 4, cy - 8), (cx - 4, cy + 9)], fill=C_LIGHT, width=2)
        draw.line([(cx - 3, cy - 8), (cx - 3, cy + 3)], fill=C_HI, width=1)
        draw.line([(cx + 4, cy - 7), (cx + 4, cy + 9)], fill=C_SHADOW, width=2)
        draw.line([(cx + 5, cy - 6), (cx + 5, cy + 8)], fill=C_DEEP, width=1)
        # Rotting ribs & dark necrotic cavities
        for ry in range(cy - 4, cy + 7, 3):
            draw.line([(cx - 4, ry), (cx + 4, ry)], fill=C_DEEP, width=1)
            draw.line([(cx - 4, ry - 1), (cx + 2, ry - 1)], fill=C_HI, width=1)
        # Bruised necrotic veins pulsing across flank
        draw.line([(cx - 2, cy + 1), (cx + 3, cy + 5)], fill=C_VEIN, width=1)
        draw.line([(cx - 3, cy + 7), (cx + 2, cy + 8)], fill=C_SINEW, width=1)

        # Head (Sharp conical beak-like skull from nape to chin strictly per user sketch!)
        head_pts = [
            (cx - 4, cy - 9),
            (cx - 18, cy - 7),   # Sharp beak tip pointing forward-down
            (cx - 16, cy - 13),  # Beak upper curve
            (cx - 7, cy - 22),   # Sloping forehead
            (cx + 4, cy - 23),   # High curving nape
            (cx + 8, cy - 14),   # Back of skull
            (cx + 5, cy - 9)     # Nape connection
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        # Shading & calcified bone crest along the skull ridge
        draw.line([(cx + 3, cy - 22), (cx - 6, cy - 21), (cx - 15, cy - 13), (cx - 17, cy - 8)], fill=C_LIGHT, width=2)
        draw.line([(cx + 2, cy - 21), (cx - 5, cy - 20), (cx - 14, cy - 12)], fill=C_HI, width=1)
        draw.line([(cx + 6, cy - 14), (cx + 5, cy - 9)], fill=C_SHADOW, width=2)
        draw.line([(cx + 7, cy - 13), (cx + 6, cy - 10)], fill=C_DEEP, width=1)
        # Eerie glowing blood-red eye slit in the rotting skull
        draw.polygon([(cx - 8, cy - 14), (cx - 4, cy - 15), (cx - 5, cy - 13)], fill=C_DEEP)
        draw.line([(cx - 7, cy - 14), (cx - 5, cy - 14)], fill=C_EYE, width=1)
        draw.point((cx - 6, cy - 14), fill=(255, 180, 200, 255))
        draw.point((cx - 1, cy - 17), fill=C_HI)
        draw.point((cx + 2, cy - 18), fill=C_HI)

        # Near leg (Foreground digitigrade clawed leg)
        nl_foot_x = cx - 2 - (step if not slashing else 8)
        nl_knee_x = cx + 4 - (step // 2)
        near_leg = [
            (cx - 2, cy + 9), (cx + 4, cy + 9),
            (cx + 6, cy + 18),
            (nl_foot_x + 4, ground_y), (nl_foot_x - 4, ground_y),
            (nl_knee_x - 3, cy + 19)
        ]
        draw.polygon(near_leg, fill=C_MID, outline=C_OUT)
        draw.line([(cx, cy + 10), (nl_knee_x + 1, cy + 18), (nl_foot_x + 2, ground_y - 2)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy + 10), (nl_knee_x + 2, cy + 18)], fill=C_HI, width=1)
        draw.line([(cx + 3, cy + 10), (nl_knee_x + 4, cy + 18), (nl_foot_x + 3, ground_y - 2)], fill=C_SHADOW, width=1)
        draw.line([(nl_knee_x - 1, cy + 16), (nl_knee_x + 1, cy + 20)], fill=C_VEIN, width=1)
        # Talons firmly planted at ground_y
        draw.polygon([(nl_foot_x - 3, ground_y - 2), (nl_foot_x - 6, ground_y), (nl_foot_x - 1, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)
        draw.polygon([(nl_foot_x + 1, ground_y - 2), (nl_foot_x, ground_y), (nl_foot_x + 4, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)

        # -----------------------------------------------------------------
        # NEAR MANTIS ATTACK ARM (Hyper-detailed, barbed predator scythe, NO hands!)
        # -----------------------------------------------------------------
        sh_x, sh_y = cx - 2, cy - 4
        if rearing:
            el_x, el_y = cx - 8, cy - 19
            scythe_tip = (cx + 8, cy - 29)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            draw.line([(sh_x + 1, sh_y), (el_x + 1, el_y)], fill=C_SINEW, width=1)
            draw.polygon([(el_x - 3, el_y), (el_x - 6, el_y + 2), (el_x - 1, el_y + 3)], fill=C_SCYTHE_BASE, outline=C_OUT)
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
            # Barbed serrations on inner edge!
            draw.polygon([(el_x + 1, el_y - 3), (el_x + 5, el_y - 4), (el_x + 2, el_y - 1)], fill=C_SCYTHE_EDGE)
            draw.polygon([(el_x + 5, el_y - 7), (el_x + 9, el_y - 8), (el_x + 6, el_y - 5)], fill=C_SCYTHE_EDGE)
        elif slashing:
            el_x, el_y = cx - 15, cy - 2
            scythe_tip = (cx - 27, cy + 6)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 2, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            draw.line([(sh_x + 1, sh_y + 1), (el_x + 1, el_y)], fill=C_VEIN, width=1)
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
            # Inner barbed cutting teeth
            draw.polygon([(cx - 19, cy + 1), (cx - 22, cy - 2), (cx - 20, cy + 3)], fill=C_SCYTHE_EDGE)
            draw.polygon([(cx - 23, cy + 4), (cx - 26, cy + 1), (cx - 24, cy + 6)], fill=C_SCYTHE_EDGE)
        else:
            # Default folded predatory mantis stance ('Р' in sketch)
            el_x, el_y = cx - 1, cy + 4
            scythe_mid = (cx - 9, cy + 10)
            scythe_tip = (cx - 18, cy + 19)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 3, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y + 1), (el_x, el_y - 1)], fill=C_LIGHT, width=2)
            draw.line([(sh_x + 1, sh_y + 1), (el_x + 1, el_y - 1)], fill=C_SINEW, width=1)
            draw.polygon([(el_x + 2, el_y - 1), (el_x + 5, el_y), (el_x + 2, el_y + 2)], fill=C_SCYTHE_BASE, outline=C_OUT)
            scythe_poly = [
                (el_x - 2, el_y), (el_x + 2, el_y),
                (scythe_mid[0] + 2, scythe_mid[1]),
                (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1]),
                (scythe_mid[0] - 3, scythe_mid[1])
            ]
            draw.polygon(scythe_poly, fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y + 1), (scythe_mid[0], scythe_mid[1]), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
            draw.line([(el_x + 1, el_y + 1), (scythe_mid[0] + 1, scythe_mid[1])], fill=C_SCYTHE_BASE, width=1)
            # 2 deadly barbed hooks on inner blade edge!
            draw.polygon([(scythe_mid[0] - 1, scythe_mid[1] + 1), (scythe_mid[0] - 4, scythe_mid[1] - 1), (scythe_mid[0] - 2, scythe_mid[1] + 3)], fill=C_SCYTHE_EDGE)
            draw.polygon([(scythe_mid[0] + 3, scythe_mid[1] - 4), (scythe_mid[0], scythe_mid[1] - 6), (scythe_mid[0] + 2, scythe_mid[1] - 2)], fill=C_SCYTHE_EDGE)

    img.save("assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png")
    print("Saved hyper-detailed duelist_aggressive.png (288x64)")

# =========================================================================
# 2. DUELIST PASSIVE (NO BEAK! Ghostly pale alien cadaver, multi-jointed needle claws)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_duelist_passive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (14, 18, 24, 255)
    C_DEEP = (32, 42, 50, 255)
    C_SHADOW = (66, 82, 94, 255)
    C_MID_DARK = (94, 114, 126, 255)
    C_MID = (128, 146, 158, 255)
    C_LIGHT = (188, 208, 218, 255)
    C_HI = (240, 250, 255, 255)
    C_VEIN = (60, 80, 115, 255)
    C_BLADE_EDGE = (250, 254, 255, 255)
    C_BLADE_MID = (175, 198, 208, 255)
    C_EYE = (40, 240, 255, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        bob_y = 0
        step = 0
        blocking = (f == 3)
        slashing = (f == 4)
        hurt = (f == 5)

        if f == 1:
            step = 3
            bob_y = -1
        elif f == 2:
            step = -3
            bob_y = -1
        elif f == 3:
            bob_y = 1
        elif f == 4:
            bob_y = 2
        elif f == 5:
            bob_y = -1

        cx = ox + 24
        cy = 32 + bob_y

        # Legs (Tall, agile digitigrade posture)
        fl_x = cx + 8 + (step if not slashing else 10)
        draw.polygon([
            (cx + 3, cy + 9), (cx + 8, cy + 9),
            (cx + 12, cy + 18),
            (fl_x + 3, ground_y), (fl_x - 3, ground_y),
            (cx + 7, cy + 18)
        ], fill=C_SHADOW, outline=C_OUT)
        draw.line([(cx + 10, cy + 18), (fl_x + 1, ground_y - 2)], fill=C_MID_DARK, width=1)

        nl_x = cx - 2 - (step if not slashing else 8)
        draw.polygon([
            (cx - 3, cy + 9), (cx + 3, cy + 9),
            (cx + 4, cy + 18),
            (nl_x + 3, ground_y), (nl_x - 3, ground_y),
            (cx - 2, cy + 18)
        ], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 1, cy + 10), (cx + 2, cy + 18), (nl_x + 1, ground_y - 2)], fill=C_LIGHT, width=2)
        draw.line([(cx, cy + 10), (cx + 3, cy + 18)], fill=C_HI, width=1)
        # Talons on ground
        draw.polygon([(nl_x - 3, ground_y - 2), (nl_x - 5, ground_y), (nl_x, ground_y)], fill=C_BLADE_MID, outline=C_OUT)
        draw.polygon([(nl_x, ground_y - 2), (nl_x + 3, ground_y), (nl_x - 1, ground_y)], fill=C_BLADE_MID, outline=C_OUT)

        # Torso (Lean, evasive, armored with pale calcified chest carapace)
        torso_pts = [
            (cx - 5, cy - 8), (cx + 5, cy - 8),
            (cx + 6, cy + 2), (cx + 3, cy + 11),
            (cx - 3, cy + 11), (cx - 6, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx - 4, cy - 7), (cx - 4, cy + 9)], fill=C_LIGHT, width=2)
        draw.line([(cx - 3, cy - 7), (cx - 3, cy + 4)], fill=C_HI, width=1)
        draw.line([(cx - 4, cy - 2), (cx + 4, cy - 2)], fill=C_SHADOW, width=1)
        draw.line([(cx - 3, cy + 3), (cx + 3, cy + 3)], fill=C_SHADOW, width=1)
        draw.line([(cx - 2, cy + 1), (cx + 2, cy + 5)], fill=C_VEIN, width=1)

        # Head (NO BEAK! Smooth, terrifying alien humanoid skull cowl)
        head_pts = [
            (cx - 5, cy - 8),   # Chin
            (cx - 10, cy - 10), # Forward jaw
            (cx - 9, cy - 16),  # Face plate
            (cx - 4, cy - 22),  # Forehead dome
            (cx + 3, cy - 24),  # Cranial crest peaking back
            (cx + 8, cy - 18),  # Elongated alien skull lobe
            (cx + 6, cy - 10),  # Back of neck
            (cx + 3, cy - 8)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 2, cy - 23), (cx - 3, cy - 21), (cx - 8, cy - 15)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy - 22), (cx - 2, cy - 20)], fill=C_HI, width=1)
        draw.line([(cx + 6, cy - 17), (cx + 5, cy - 10)], fill=C_SHADOW, width=2)
        # Eerie glowing cyan horizontal ocular slit
        draw.line([(cx - 8, cy - 14), (cx - 4, cy - 14)], fill=C_EYE, width=1)
        draw.point((cx - 6, cy - 14), fill=(220, 255, 255, 255))

        # -----------------------------------------------------------------
        # ARMS & PARALYZING BONE NEEDLE CLAWS (The attack hands!)
        # -----------------------------------------------------------------
        if blocking:
            draw.line([(cx - 3, cy - 4), (cx - 8, cy + 2)], fill=C_MID, width=3)
            draw.line([(cx - 8, cy + 2), (cx - 16, cy - 13)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx - 10, cy - 2), (cx - 14, cy - 10)], fill=C_HI, width=1)
            draw.line([(cx + 2, cy - 4), (cx - 5, cy + 4)], fill=C_SHADOW, width=3)
            draw.line([(cx - 5, cy + 4), (cx - 12, cy - 8)], fill=C_BLADE_EDGE, width=2)
            draw.point((cx - 16, cy - 13), fill=C_HI)
            draw.point((cx - 12, cy - 8), fill=C_HI)
        elif slashing:
            draw.line([(cx - 3, cy - 4), (cx - 14, cy)], fill=C_MID, width=3)
            draw.line([(cx - 13, cy), (cx - 13, cy + 2)], fill=C_VEIN, width=1)
            draw.line([(cx - 14, cy), (cx - 27, cy + 5)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx - 14, cy - 1), (cx - 25, cy + 3)], fill=C_HI, width=1)
            draw.line([(cx - 18, cy + 1), (cx - 25, cy + 10)], fill=C_BLADE_EDGE, width=1)
            draw.line([(cx - 18, cy - 1), (cx - 26, cy)], fill=C_BLADE_EDGE, width=1)
        else:
            draw.line([(cx - 3, cy - 4), (cx - 5, cy + 4)], fill=C_MID, width=3)
            draw.line([(cx - 2, cy - 3), (cx - 4, cy + 3)], fill=C_LIGHT, width=1)
            draw.line([(cx - 5, cy + 4), (cx - 14, cy + 14)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx - 4, cy + 4), (cx - 12, cy + 12)], fill=C_HI, width=1)
            draw.line([(cx - 6, cy + 6), (cx - 12, cy + 18)], fill=C_BLADE_MID, width=1)
            draw.line([(cx - 7, cy + 4), (cx - 15, cy + 10)], fill=C_BLADE_MID, width=1)

    img.save("assets/sprites/enemies/duelist_passive/duelist_passive.png")
    print("Saved hyper-detailed duelist_passive.png (288x64)")

# =========================================================================
# 3. NORMAL MUTANT (Feral ghoul, gaping needle-toothed jaw, MASSIVE clawed attack arms)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_normal_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (14, 18, 14, 255)
    C_DEEP = (32, 40, 32, 255)
    C_SHADOW = (64, 76, 66, 255)
    C_MID_DARK = (94, 108, 96, 255)
    C_MID = (128, 144, 130, 255)
    C_LIGHT = (182, 198, 184, 255)
    C_HI = (232, 244, 234, 255)
    C_TEETH = (248, 242, 212, 255)
    C_CLAW = (242, 238, 214, 255)
    C_CLAW_DARK = (112, 98, 76, 255)
    C_EYE = (255, 215, 35, 255)
    C_VEIN = (88, 48, 70, 255)
    C_SINEW = (142, 42, 52, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        bob_y = 0
        step = 0
        windup = (f == 3)
        attacking = (f == 4)
        hurt = (f == 5)

        if f == 1:
            step = 3
            bob_y = -1
        elif f == 2:
            step = -3
            bob_y = -1
        elif f == 3:
            bob_y = -2
        elif f == 4:
            bob_y = 2
        elif f == 5:
            bob_y = -2

        cx = ox + 24
        cy = 32 + bob_y

        # Hind legs (Feral hunch, thick rotting digitigrade legs)
        fl_x = cx + 8 + (step if not attacking else 10)
        draw.polygon([
            (cx + 3, cy + 8), (cx + 8, cy + 8),
            (cx + 10, cy + 18),
            (fl_x + 3, ground_y), (fl_x - 3, ground_y),
            (cx + 5, cy + 18)
        ], fill=C_SHADOW, outline=C_OUT)

        nl_x = cx - 3 - (step if not attacking else 8)
        draw.polygon([
            (cx - 3, cy + 8), (cx + 3, cy + 8),
            (cx + 4, cy + 18),
            (nl_x + 3, ground_y), (nl_x - 3, ground_y),
            (cx - 2, cy + 18)
        ], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 1, cy + 9), (cx + 2, cy + 18), (nl_x + 1, ground_y - 2)], fill=C_LIGHT, width=2)
        # Rotting talons on ground
        draw.polygon([(nl_x - 3, ground_y - 2), (nl_x - 6, ground_y), (nl_x - 1, ground_y)], fill=C_CLAW, outline=C_OUT)
        draw.polygon([(nl_x + 1, ground_y - 2), (nl_x, ground_y), (nl_x + 4, ground_y)], fill=C_CLAW, outline=C_OUT)

        # Feral hunched torso (Rotting, sinewy, exposed spine vertebrae)
        torso_pts = [
            (cx - 6, cy - 8), (cx + 4, cy - 8),
            (cx + 6, cy + 2), (cx + 2, cy + 10),
            (cx - 4, cy + 10), (cx - 7, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx - 5, cy - 7), (cx - 5, cy + 8)], fill=C_LIGHT, width=2)
        draw.line([(cx - 4, cy - 6), (cx - 4, cy + 2)], fill=C_HI, width=1)
        draw.line([(cx + 4, cy - 6), (cx + 4, cy + 8)], fill=C_SHADOW, width=2)
        # Exposed spine knobs along back
        for s in range(4):
            sx = cx + 1 + s * 2
            sy = cy - 7 + s * 4
            draw.polygon([(sx, sy - 1), (sx + 2, sy), (sx, sy + 1)], fill=C_HI)
            draw.point((sx + 2, sy), fill=C_OUT)
        # Bruised necrotic veins on flank
        draw.line([(cx - 4, cy), (cx + 2, cy + 4)], fill=C_VEIN, width=1)
        draw.line([(cx - 3, cy + 5), (cx + 1, cy + 7)], fill=C_SINEW, width=1)

        # Head: Feral rotting mutant skull, unhinged jaw, crooked needle fangs, feverish yellow eye
        head_pts = [
            (cx - 4, cy - 8),
            (cx - 16, cy - 5),  # Lower toothy jaw (gaping)
            (cx - 14, cy - 12), # Upper snout
            (cx - 6, cy - 19),  # Brow
            (cx + 4, cy - 19),  # Cranium
            (cx + 5, cy - 8)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 2, cy - 18), (cx - 5, cy - 18), (cx - 13, cy - 11)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy - 17), (cx - 4, cy - 17)], fill=C_HI, width=1)
        draw.line([(cx + 4, cy - 12), (cx + 4, cy - 8)], fill=C_SHADOW, width=2)
        # Sunken dark cavity with feverish yellow/amber eye
        draw.rectangle([cx - 8, cy - 14, cx - 5, cy - 11], fill=C_DEEP)
        draw.point((cx - 7, cy - 13), fill=C_EYE)
        draw.point((cx - 6, cy - 13), fill=(255, 255, 180, 255))
        # Gaping mouth cavity with sharp needle fangs
        draw.polygon([(cx - 15, cy - 7), (cx - 10, cy - 8), (cx - 9, cy - 5), (cx - 14, cy - 4)], fill=C_DEEP)
        for t in range(4):
            tx = cx - 15 + t * 2
            draw.point((tx, cy - 8), fill=C_TEETH)
            draw.point((tx, cy - 5), fill=C_TEETH)

        # -----------------------------------------------------------------
        # MASSIVE MUTATED CLAWED ATTACK ARMS (The primary weapons!)
        # Overgrown, sinewy, heavily veined, huge jagged claws!
        # -----------------------------------------------------------------
        sh_x, sh_y = cx - 4, cy - 4
        if windup:
            el_x, el_y = cx - 3, cy - 18
            draw.polygon([(sh_x - 3, sh_y), (sh_x + 3, sh_y), (el_x + 3, el_y), (el_x - 3, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            draw.line([(sh_x + 1, sh_y), (el_x + 1, el_y)], fill=C_VEIN, width=1)
            # Massive claw cluster rearing back
            draw.polygon([(el_x - 4, el_y - 2), (el_x - 9, el_y - 10), (el_x + 3, el_y - 8)], fill=C_CLAW, outline=C_OUT)
            draw.line([(el_x - 8, el_y - 9), (el_x - 2, el_y - 4)], fill=C_HI, width=1)
            draw.polygon([(el_x - 9, el_y - 10), (el_x - 12, el_y - 14), (el_x - 7, el_y - 11)], fill=C_CLAW_DARK)
            draw.polygon([(el_x - 5, el_y - 10), (el_x - 7, el_y - 15), (el_x - 3, el_y - 9)], fill=C_CLAW_DARK)
        elif attacking:
            el_x, el_y = cx - 18, cy + 1
            draw.polygon([(sh_x - 3, sh_y), (sh_x + 2, sh_y), (el_x + 2, el_y + 3), (el_x - 2, el_y - 2)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            draw.line([(sh_x, sh_y + 1), (el_x, el_y + 1)], fill=C_VEIN, width=1)
            hand_x, hand_y = el_x - 4, el_y
            draw.polygon([(el_x - 2, el_y - 3), (hand_x, hand_y - 4), (hand_x, hand_y + 4), (el_x + 1, el_y + 4)], fill=C_MID, outline=C_OUT)
            # 4 wicked talons raking the air
            draw.line([(hand_x, hand_y - 3), (hand_x - 9, hand_y - 5)], fill=C_CLAW, width=2)
            draw.line([(hand_x, hand_y - 1), (hand_x - 11, hand_y - 1)], fill=C_CLAW, width=2)
            draw.line([(hand_x, hand_y + 1), (hand_x - 10, hand_y + 3)], fill=C_CLAW, width=2)
            draw.line([(hand_x, hand_y + 3), (hand_x - 8, hand_y + 6)], fill=C_CLAW, width=2)
            draw.point((hand_x - 9, hand_y - 5), fill=C_HI)
            draw.point((hand_x - 11, hand_y - 1), fill=C_HI)
            draw.point((hand_x - 10, hand_y + 3), fill=C_HI)
            draw.point((hand_x - 8, hand_y + 6), fill=C_HI)
        else:
            el_x, el_y = cx - 11, cy + 9
            draw.polygon([(sh_x - 3, sh_y), (sh_x + 2, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            draw.line([(sh_x + 1, sh_y + 1), (el_x + 1, el_y)], fill=C_VEIN, width=1)
            # Massive hand hanging down
            draw.polygon([(el_x - 2, el_y), (el_x - 6, el_y + 7), (el_x + 2, el_y + 5)], fill=C_CLAW, outline=C_OUT)
            draw.line([(el_x - 1, el_y + 1), (el_x - 4, el_y + 6)], fill=C_HI, width=1)
            draw.line([(el_x - 4, el_y + 6), (el_x - 7, el_y + 11)], fill=C_CLAW_DARK, width=1)
            draw.line([(el_x - 2, el_y + 6), (el_x - 4, el_y + 12)], fill=C_CLAW_DARK, width=1)
            draw.line([(el_x, el_y + 5), (el_x - 1, el_y + 11)], fill=C_CLAW_DARK, width=1)

    img.save("assets/sprites/enemies/normal/normal_mutant.png")
    print("Saved hyper-detailed normal_mutant.png (288x64)")

# =========================================================================
# 4. TANK MUTANT (72x96, Bloated necrotic titan, massive spiked bone knuckle club-arms)
# Frame size: 72 x 96, 6 frames
# =========================================================================
def generate_tank_mutant():
    W, H = 72, 96
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (18, 16, 20, 255)
    C_DEEP = (38, 34, 42, 255)
    C_SHADOW = (76, 72, 82, 255)
    C_MID_DARK = (106, 102, 112, 255)
    C_MID = (140, 136, 146, 255)
    C_LIGHT = (194, 192, 202, 255)
    C_HI = (246, 246, 254, 255)
    C_VEIN = (104, 42, 60, 255)
    C_VEIN_DARK = (60, 22, 34, 255)
    C_BONE = (236, 232, 218, 255)
    C_BONE_DARK = (136, 130, 120, 255)
    C_EYE = (255, 95, 30, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 93
        bob_y = 0
        smash = (f == 4)
        windup = (f == 3)
        hurt = (f == 5)

        if f in (1, 2):
            bob_y = -2
        elif f == 5:
            bob_y = -2

        cx = ox + 36
        cy = 48 + bob_y

        # Massive Pillar Legs
        draw.polygon([
            (cx + 6, cy + 16), (cx + 18, cy + 16),
            (cx + 22, cy + 32),
            (cx + 20, ground_y), (cx + 8, ground_y),
            (cx + 12, cy + 32)
        ], fill=C_SHADOW, outline=C_OUT)
        draw.line([(cx + 15, cy + 20), (cx + 14, ground_y - 2)], fill=C_DEEP, width=2)

        draw.polygon([
            (cx - 8, cy + 16), (cx + 6, cy + 16),
            (cx + 6, cy + 32),
            (cx + 4, ground_y), (cx - 10, ground_y),
            (cx - 6, cy + 32)
        ], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 4, cy + 18), (cx + 2, cy + 32), (cx, ground_y - 3)], fill=C_LIGHT, width=3)
        draw.line([(cx - 2, cy + 18), (cx + 3, cy + 32)], fill=C_HI, width=1)
        # Heavy bone-capped toes
        draw.polygon([(cx - 8, ground_y - 3), (cx - 10, ground_y), (cx - 4, ground_y)], fill=C_BONE, outline=C_OUT)
        draw.polygon([(cx - 2, ground_y - 3), (cx + 2, ground_y), (cx - 4, ground_y)], fill=C_BONE, outline=C_OUT)
        draw.polygon([(cx + 10, ground_y - 3), (cx + 15, ground_y), (cx + 8, ground_y)], fill=C_BONE, outline=C_OUT)

        # Colossal Bloated Torso (Swollen pale flesh, bruised necrotic veins, calcified ribs)
        torso_pts = [
            (cx - 14, cy - 14), (cx + 14, cy - 14),
            (cx + 18, cy + 4), (cx + 10, cy + 20),
            (cx - 10, cy + 20), (cx - 18, cy + 4)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx - 13, cy - 10), (cx - 13, cy + 14)], fill=C_LIGHT, width=3)
        draw.line([(cx - 11, cy - 10), (cx - 11, cy + 14)], fill=C_HI, width=1)
        draw.line([(cx + 13, cy - 8), (cx + 13, cy + 14)], fill=C_SHADOW, width=3)
        draw.line([(cx + 16, cy - 6), (cx + 16, cy + 12)], fill=C_DEEP, width=2)
        # Pulsating bruised veins across swollen gut
        draw.line([(cx - 10, cy), (cx + 10, cy)], fill=C_VEIN, width=2)
        draw.line([(cx - 8, cy + 8), (cx + 8, cy + 8)], fill=C_VEIN, width=2)
        draw.line([(cx - 4, cy - 6), (cx + 6, cy + 4)], fill=C_VEIN_DARK, width=1)
        draw.line([(cx - 6, cy + 4), (cx - 1, cy + 12)], fill=C_VEIN_DARK, width=1)
        # Calcified bone ribs bursting through the rotting flesh
        for r in range(3):
            ry = cy - 6 + r * 6
            draw.line([(cx - 13, ry), (cx - 5, ry)], fill=C_BONE, width=2)
            draw.line([(cx - 12, ry - 1), (cx - 6, ry - 1)], fill=C_HI, width=1)
            draw.point((cx - 4, ry), fill=C_BONE_DARK)

        # Armored Juggernaut Head / Bone Horn Crest
        head_pts = [
            (cx - 8, cy - 14), (cx - 20, cy - 10),
            (cx - 16, cy - 24), (cx - 4, cy - 30),
            (cx + 8, cy - 30), (cx + 12, cy - 14)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 6, cy - 28), (cx - 4, cy - 28), (cx - 15, cy - 22)], fill=C_LIGHT, width=2)
        draw.line([(cx + 4, cy - 27), (cx - 3, cy - 27)], fill=C_HI, width=1)
        # Sunken dark pit sockets with feverish red eye slits
        draw.rectangle([cx - 13, cy - 20, cx - 9, cy - 17], fill=C_DEEP)
        draw.point((cx - 11, cy - 19), fill=C_EYE)
        draw.point((cx - 10, cy - 19), fill=(255, 180, 80, 255))
        # 3 Massive jagged bone horns jutting from the crest
        for h in range(3):
            hx = cx - 14 + h * 9
            draw.polygon([(hx, cy - 28), (hx + 4, cy - 28), (hx + 2, cy - 38)], fill=C_BONE, outline=C_OUT)
            draw.line([(hx + 1, cy - 29), (hx + 2, cy - 37)], fill=C_HI, width=1)

        # -----------------------------------------------------------------
        # TITANIC CLUB-ARMS & SPIKED BONE KNUCKLE FISTS (Attack weapons!)
        # -----------------------------------------------------------------
        if windup:
            draw.polygon([(cx - 14, cy - 8), (cx - 7, cy - 34), (cx + 7, cy - 34), (cx + 5, cy - 8)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 5, cy - 12), (cx, cy - 32)], fill=C_LIGHT, width=4)
            draw.line([(cx - 2, cy - 12), (cx + 2, cy - 32)], fill=C_VEIN, width=2)
            draw.polygon([(cx - 12, cy - 24), (cx - 16, cy - 28), (cx - 10, cy - 28)], fill=C_BONE, outline=C_OUT)
            draw.polygon([(cx - 10, cy - 34), (cx + 10, cy - 34), (cx + 7, cy - 44), (cx - 7, cy - 44)], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 6, cy - 43), (cx + 6, cy - 43)], fill=C_HI, width=1)
            draw.polygon([(cx - 6, cy - 44), (cx - 9, cy - 48), (cx - 3, cy - 44)], fill=C_BONE, outline=C_OUT)
            draw.polygon([(cx + 3, cy - 44), (cx + 6, cy - 48), (cx, cy - 44)], fill=C_BONE, outline=C_OUT)
        elif smash:
            draw.polygon([(cx - 12, cy), (cx - 28, cy + 24), (cx - 16, cy + 28), (cx - 2, cy + 4)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 10, cy + 4), (cx - 22, cy + 24)], fill=C_LIGHT, width=4)
            draw.line([(cx - 8, cy + 6), (cx - 20, cy + 24)], fill=C_VEIN, width=2)
            draw.rectangle([cx - 34, ground_y - 14, cx - 14, ground_y], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 32, ground_y - 13), (cx - 16, ground_y - 13)], fill=C_HI, width=2)
            for sk in range(3):
                skx = cx - 30 + sk * 6
                draw.polygon([(skx, ground_y - 14), (skx + 2, ground_y - 18), (skx + 4, ground_y - 14)], fill=C_BONE, outline=C_OUT)
        else:
            # Heavy resting club-arms hanging low
            draw.polygon([(cx - 14, cy - 4), (cx - 25, cy + 18), (cx - 14, cy + 24), (cx - 4, cy)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 12, cy), (cx - 20, cy + 18)], fill=C_LIGHT, width=4)
            draw.line([(cx - 10, cy), (cx - 18, cy + 18)], fill=C_HI, width=1)
            draw.line([(cx - 8, cy + 2), (cx - 16, cy + 20)], fill=C_VEIN, width=2)
            draw.polygon([(cx - 23, cy + 6), (cx - 28, cy + 9), (cx - 22, cy + 12)], fill=C_BONE, outline=C_OUT)
            draw.rectangle([cx - 26, cy + 16, cx - 12, cy + 26], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 25, cy + 17), (cx - 13, cy + 17)], fill=C_HI, width=1)
            for sk in range(3):
                skx = cx - 24 + sk * 5
                draw.polygon([(skx, cy + 26), (skx + 2, cy + 30), (skx + 4, cy + 26)], fill=C_BONE, outline=C_OUT)

    img.save("assets/sprites/enemies/tank/tank_mutant.png")
    print("Saved hyper-detailed tank_mutant.png (432x96)")

# =========================================================================
# 5. DUMMY MUTANT (Necrotic biological training totem, stitched cadaver, grafted mutated blocking arms)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_dummy_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (16, 18, 22, 255)
    C_DEEP = (38, 40, 48, 255)
    C_SHADOW = (72, 76, 86, 255)
    C_MID = (132, 136, 146, 255)
    C_LIGHT = (190, 196, 206, 255)
    C_HI = (240, 246, 252, 255)
    C_CORE = (210, 30, 40, 255)
    C_CORE_HI = (255, 120, 110, 255)
    C_VEIN = (98, 46, 68, 255)
    C_BONE = (235, 230, 218, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        wobble_x = 0
        if f in (4, 5):
            wobble_x = -3 if f == 4 else 2

        cx = ox + 24 + wobble_x
        cy = 32

        # Rooted rotting spinal column on floor
        draw.polygon([(cx - 10, ground_y), (cx + 10, ground_y), (cx + 4, cy + 16), (cx - 4, cy + 16)], fill=C_SHADOW, outline=C_OUT)
        draw.line([(cx - 8, ground_y - 2), (cx + 8, ground_y - 2)], fill=C_LIGHT, width=1)
        for v in range(3):
            vy = ground_y - 4 - v * 4
            draw.point((cx, vy), fill=C_BONE)

        # Torso pod (Cadaver-grey stitched flesh pod)
        draw.polygon([(cx - 8, cy - 10), (cx + 8, cy - 10), (cx + 9, cy + 6), (cx + 5, cy + 18), (cx - 5, cy + 18), (cx - 9, cy + 6)], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 6, cy - 8), (cx - 6, cy + 14)], fill=C_LIGHT, width=2)
        draw.line([(cx - 5, cy - 8), (cx - 5, cy + 4)], fill=C_HI, width=1)
        # Crude black autopsy sutures holding flesh slabs together
        draw.line([(cx + 3, cy - 8), (cx + 3, cy + 14)], fill=C_DEEP, width=1)
        for st in range(4):
            sty = cy - 6 + st * 5
            draw.line([(cx + 1, sty), (cx + 5, sty)], fill=C_BONE, width=1)
        # Bruised veins radiating from core
        draw.line([(cx - 4, cy - 6), (cx - 1, cy - 3)], fill=C_VEIN, width=1)
        draw.line([(cx + 2, cy + 8), (cx + 5, cy + 12)], fill=C_VEIN, width=1)

        # Pulsating Bruised Target Core in chest
        draw.ellipse([cx - 5, cy - 4, cx + 5, cy + 6], fill=C_CORE, outline=C_OUT)
        draw.point((cx - 2, cy - 1), fill=C_CORE_HI)
        draw.point((cx - 1, cy - 2), fill=(255, 200, 190, 255))

        # Head node (Smooth cadaver dome)
        draw.ellipse([cx - 6, cy - 22, cx + 6, cy - 10], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 3, cy - 20), (cx + 3, cy - 20)], fill=C_HI, width=2)

        # Grafted Mutated Blocking Claw-Arms on flanks
        if f == 2:  # High block
            draw.line([(cx - 5, cy - 4), (cx - 9, cy - 14)], fill=C_MID, width=3)
            draw.line([(cx - 9, cy - 14), (cx - 4, cy - 20)], fill=C_BONE, width=2)
            draw.polygon([(cx - 11, cy - 18), (cx - 5, cy - 18), (cx - 6, cy - 7), (cx - 12, cy - 7)], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 10, cy - 17), (cx - 7, cy - 8)], fill=C_HI, width=1)
        elif f == 3: # Low block
            draw.line([(cx - 5, cy - 2), (cx - 10, cy + 6)], fill=C_MID, width=3)
            draw.polygon([(cx - 12, cy - 2), (cx - 5, cy - 2), (cx - 6, cy + 10), (cx - 13, cy + 10)], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 11, cy - 1), (cx - 7, cy + 9)], fill=C_HI, width=1)
        else:
            # Resting folded arms
            draw.line([(cx - 6, cy - 6), (cx - 9, cy + 6)], fill=C_SHADOW, width=2)
            draw.line([(cx - 9, cy + 6), (cx - 6, cy + 12)], fill=C_BONE, width=1)

    img.save("assets/sprites/enemies/dummy/dummy_mutant.png")
    print("Saved hyper-detailed dummy_mutant.png (288x64)")

# =========================================================================
# 6. LURKER MUTANT (Subterranean cadaver crawler, grasping predatory claw-arms)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_lurker_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (16, 18, 22, 255)
    C_DEEP = (36, 40, 46, 255)
    C_SHADOW = (72, 78, 86, 255)
    C_MID = (128, 134, 144, 255)
    C_LIGHT = (186, 194, 204, 255)
    C_HI = (238, 244, 252, 255)
    C_CLAW = (240, 236, 218, 255)
    C_EYE = (255, 45, 30, 255)
    C_VEIN = (92, 46, 64, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        cx = ox + 24
        cy = 34

        # Frame 4 is burrowed mound under ground
        if f == 4:
            draw.ellipse([cx - 14, ground_y - 6, cx + 14, ground_y + 4], fill=C_SHADOW, outline=C_OUT)
            draw.line([(cx - 4, ground_y - 8), (cx - 2, ground_y - 14)], fill=C_MID, width=2)
            draw.line([(cx + 4, ground_y - 8), (cx + 2, ground_y - 14)], fill=C_MID, width=2)
            draw.point((cx - 2, ground_y - 14), fill=C_EYE)
            draw.point((cx + 2, ground_y - 14), fill=C_EYE)
            continue

        # Legs (Low, spider-like crawling limbs on ground_y)
        draw.line([(cx + 2, cy + 8), (cx + 8, cy + 16), (cx + 6, ground_y)], fill=C_SHADOW, width=3)
        draw.line([(cx - 2, cy + 8), (cx - 6, cy + 16), (cx - 4, ground_y)], fill=C_MID, width=3)
        draw.polygon([(cx - 6, ground_y - 2), (cx - 8, ground_y), (cx - 3, ground_y)], fill=C_CLAW, outline=C_OUT)
        draw.polygon([(cx + 4, ground_y - 2), (cx + 7, ground_y), (cx + 2, ground_y)], fill=C_CLAW, outline=C_OUT)

        # Low creeping necrotic chitin torso
        draw.polygon([(cx - 8, cy - 6), (cx + 6, cy - 6), (cx + 8, cy + 6), (cx + 4, cy + 14), (cx - 4, cy + 14), (cx - 10, cy + 4)], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 7, cy - 4), (cx - 7, cy + 8)], fill=C_LIGHT, width=2)
        draw.line([(cx - 6, cy - 4), (cx - 6, cy + 2)], fill=C_HI, width=1)
        draw.line([(cx - 4, cy + 2), (cx + 2, cy + 5)], fill=C_VEIN, width=1)

        # Predatory Head with grasping bone mandibles & bead eyes
        head_pts = [(cx - 4, cy - 6), (cx - 14, cy - 2), (cx - 12, cy - 10), (cx - 4, cy - 14), (cx + 4, cy - 14), (cx + 4, cy - 6)]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 2, cy - 13), (cx - 3, cy - 13), (cx - 11, cy - 9)], fill=C_LIGHT, width=2)
        # Cluster of 3 creepy glowing red eyes
        draw.point((cx - 7, cy - 8), fill=C_EYE)
        draw.point((cx - 9, cy - 8), fill=C_EYE)
        draw.point((cx - 8, cy - 6), fill=C_EYE)
        # Sharp bone mandibles
        draw.line([(cx - 13, cy - 4), (cx - 19, cy)], fill=C_CLAW, width=2)
        draw.line([(cx - 13, cy - 8), (cx - 19, cy - 12)], fill=C_CLAW, width=2)

        # Predatory Grasping Front Claw-Arms (Primary attack limbs reaching forward)
        if f in (1, 3):
            draw.line([(cx - 4, cy), (cx - 15, cy + 4), (cx - 22, cy + 12)], fill=C_MID, width=3)
            draw.line([(cx - 15, cy + 4), (cx - 22, cy + 12)], fill=C_LIGHT, width=1)
            draw.polygon([(cx - 21, cy + 11), (cx - 25, cy + 16), (cx - 20, cy + 14)], fill=C_CLAW, outline=C_OUT)
        else:
            draw.line([(cx - 4, cy), (cx - 12, cy + 6), (cx - 18, cy + 14)], fill=C_MID, width=3)
            draw.line([(cx - 12, cy + 6), (cx - 18, cy + 14)], fill=C_LIGHT, width=1)
            draw.polygon([(cx - 17, cy + 13), (cx - 21, cy + 18), (cx - 16, cy + 16)], fill=C_CLAW, outline=C_OUT)

    img.save("assets/sprites/enemies/lurker/lurker_mutant.png")
    print("Saved hyper-detailed lurker_mutant.png (288x64)")

# Generate all 6 standard mutants
generate_duelist_aggressive()
generate_duelist_passive()
generate_normal_mutant()
generate_tank_mutant()
generate_dummy_mutant()
generate_lurker_mutant()

# Copy icons to assets/tiles/enemies and preview folders
enemies = {
    'enemy_normal': ('assets/sprites/enemies/normal/normal_mutant.png', 48, 64, 'assets/sprites/enemies/normal/preview.png'),
    'enemy_tank': ('assets/sprites/enemies/tank/tank_mutant.png', 72, 96, 'assets/sprites/enemies/tank/preview.png'),
    'enemy_duelist': ('assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png', 48, 64, 'assets/sprites/enemies/duelist_aggressive/preview.png'),
    'enemy_duelist_passive': ('assets/sprites/enemies/duelist_passive/duelist_passive.png', 48, 64, 'assets/sprites/enemies/duelist_passive/preview.png'),
    'enemy_dummy': ('assets/sprites/enemies/dummy/dummy_mutant.png', 48, 64, 'assets/sprites/enemies/dummy/preview.png'),
    'enemy_lurker': ('assets/sprites/enemies/lurker/lurker_mutant.png', 48, 64, 'assets/sprites/enemies/lurker/preview.png'),
}

for name, (path, fw, fh, prev_path) in enemies.items():
    img = Image.open(path)
    f0 = img.crop((0, 0, fw, fh))
    aspect = fw / fh
    if aspect <= 1.0:
        new_h = 30
        new_w = max(1, int(new_h * aspect))
    else:
        new_w = 30
        new_h = max(1, int(new_w / aspect))
    scaled = f0.resize((new_w, new_h), Image.Resampling.LANCZOS)
    icon32 = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    paste_x = (32 - new_w) // 2
    paste_y = 32 - new_h
    icon32.paste(scaled, (paste_x, paste_y))
    for dir_path in ['assets/tiles', 'assets/tiles/tools', 'assets/tiles/enemies']:
        os.makedirs(dir_path, exist_ok=True)
        out_p = os.path.join(dir_path, f'{name}.png')
        icon32.save(out_p)
    icon32.save(prev_path)

print("All enemy sprites, icons and canvas previews updated!")
