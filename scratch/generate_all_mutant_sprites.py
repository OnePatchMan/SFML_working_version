import os
import math
from PIL import Image, ImageDraw

os.makedirs("assets/sprites/enemies/duelist_aggressive", exist_ok=True)
os.makedirs("assets/sprites/enemies/duelist_passive", exist_ok=True)
os.makedirs("assets/sprites/enemies/normal", exist_ok=True)
os.makedirs("assets/sprites/enemies/tank", exist_ok=True)
os.makedirs("assets/sprites/enemies/dummy", exist_ok=True)
os.makedirs("assets/sprites/enemies/lurker", exist_ok=True)

# =========================================================================
# 1. DUELIST AGGRESSIVE (Sharp conical beak head, bent mantis scythes, rotting grey flesh)
# Frame size: 48 x 64, 6 frames (Idle, Walk1, Walk2, Windup, Slash, Hurt)
# =========================================================================
def generate_duelist_aggressive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Sickly rotting grey corpse flesh with bruised necrotic violet-grey undertones
    C_OUT = (18, 20, 26, 255)
    C_DEEP = (42, 44, 54, 255)
    C_SHADOW = (78, 82, 94, 255)
    C_MID = (136, 140, 152, 255)
    C_LIGHT = (194, 200, 212, 255)
    C_HI = (242, 246, 252, 255)
    C_VEIN = (98, 62, 88, 255)
    C_SCYTHE_BASE = (175, 182, 192, 255)
    C_SCYTHE_EDGE = (248, 252, 255, 255)
    C_EYE = (255, 30, 60, 255)

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
        # Claws
        draw.polygon([(fl_foot_x - 3, ground_y - 2), (fl_foot_x - 5, ground_y), (fl_foot_x - 1, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)
        draw.polygon([(fl_foot_x + 1, ground_y - 2), (fl_foot_x, ground_y), (fl_foot_x + 3, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)

        # Far Mantis Scythe Arm (Background)
        if rearing:
            fa_el = (cx - 3, cy - 16)
            fa_tip = (cx + 10, cy - 26)
            draw.line([(cx + 2, cy - 4), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)
        elif slashing:
            fa_el = (cx - 15, cy + 2)
            fa_tip = (cx - 24, cy + 10)
            draw.line([(cx + 2, cy - 2), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)
        else:
            fa_el = (cx - 1, cy + 4)
            fa_tip = (cx - 12, cy + 16)
            draw.line([(cx + 2, cy - 4), fa_el], fill=C_SHADOW, width=3)
            draw.line([fa_el, fa_tip], fill=C_SCYTHE_EDGE, width=2)

        # Torso (Skeletal, pinched, rotting flesh with ribs)
        torso_pts = [
            (cx - 5, cy - 9), (cx + 6, cy - 9),
            (cx + 7, cy + 2), (cx + 4, cy + 11),
            (cx - 3, cy + 11), (cx - 6, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        # Shading & rotting ribs
        draw.line([(cx - 4, cy - 8), (cx - 4, cy + 9)], fill=C_LIGHT, width=2)
        draw.line([(cx - 3, cy - 8), (cx - 3, cy + 3)], fill=C_HI, width=1)
        draw.line([(cx + 4, cy - 7), (cx + 4, cy + 9)], fill=C_SHADOW, width=2)
        # Rotting ribs & bruised veins
        draw.line([(cx - 4, cy - 2), (cx + 4, cy - 2)], fill=C_DEEP, width=1)
        draw.line([(cx - 3, cy + 2), (cx + 3, cy + 2)], fill=C_DEEP, width=1)
        draw.line([(cx - 3, cy + 6), (cx + 2, cy + 6)], fill=C_VEIN, width=1)
        draw.line([(cx - 4, cy - 3), (cx + 1, cy - 3)], fill=C_LIGHT, width=1)

        # Head (Sharp conical beak-like skull from nape to chin strictly per user sketch!)
        head_pts = [
            (cx - 4, cy - 9),
            (cx - 17, cy - 7),   # Sharp beak tip pointing forward-down
            (cx - 15, cy - 13),  # Beak upper curve
            (cx - 6, cy - 21),   # Sloping forehead
            (cx + 4, cy - 22),   # High curving nape
            (cx + 7, cy - 14),   # Back of skull
            (cx + 5, cy - 9)     # Nape connection
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        # Glossy bone crown along skull and beak ridge
        draw.line([(cx + 3, cy - 21), (cx - 5, cy - 20), (cx - 14, cy - 12), (cx - 16, cy - 8)], fill=C_LIGHT, width=2)
        draw.line([(cx + 2, cy - 20), (cx - 4, cy - 19), (cx - 13, cy - 12)], fill=C_HI, width=1)
        draw.line([(cx + 5, cy - 13), (cx + 4, cy - 9)], fill=C_SHADOW, width=2)
        # Eerie glowing blood-red eye slit in the rotting skull
        draw.polygon([(cx - 7, cy - 14), (cx - 4, cy - 15), (cx - 5, cy - 13)], fill=C_EYE)
        draw.point((cx - 6, cy - 14), fill=(255, 180, 200, 255))

        # Near leg (Foreground)
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
        # Talons firmly planted at ground_y
        draw.polygon([(nl_foot_x - 3, ground_y - 2), (nl_foot_x - 6, ground_y), (nl_foot_x - 1, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)
        draw.polygon([(nl_foot_x + 1, ground_y - 2), (nl_foot_x, ground_y), (nl_foot_x + 4, ground_y)], fill=C_SCYTHE_BASE, outline=C_OUT)

        # Near Mantis Arm (Foreground - sharp bent scythe, NO hands!)
        sh_x, sh_y = cx - 2, cy - 4
        if rearing:
            el_x, el_y = cx - 8, cy - 18
            scythe_tip = (cx + 5, cy - 28)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            # Razor blade
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
        elif slashing:
            el_x, el_y = cx - 14, cy - 2
            scythe_tip = (cx - 25, cy + 6)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 2, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            # Extended strike blade
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
        else:
            # Default folded predatory mantis stance ('Р' in sketch)
            el_x, el_y = cx - 1, cy + 4
            scythe_mid = (cx - 8, cy + 9)
            scythe_tip = (cx - 16, cy + 18)
            # Upper arm
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 3, el_y), (el_x - 2, el_y)], fill=C_MID, outline=C_OUT)
            draw.line([(sh_x, sh_y + 1), (el_x, el_y - 1)], fill=C_LIGHT, width=2)
            # Scythe blade tapering to a needle point
            scythe_poly = [
                (el_x - 2, el_y), (el_x + 2, el_y),
                (scythe_mid[0] + 2, scythe_mid[1]),
                (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1]),
                (scythe_mid[0] - 3, scythe_mid[1])
            ]
            draw.polygon(scythe_poly, fill=C_SCYTHE_EDGE, outline=C_OUT)
            draw.line([(el_x, el_y + 1), (scythe_mid[0], scythe_mid[1]), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
            draw.line([(el_x + 1, el_y + 1), (scythe_mid[0] + 1, scythe_mid[1])], fill=C_SCYTHE_BASE, width=1)

    img.save("assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png")
    print("Saved refined duelist_aggressive.png (288x64)")

# =========================================================================
# 2. DUELIST PASSIVE (NO BEAK HEAD! Eerie ghostly cadaver-grey duelist with needle blades)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_duelist_passive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Cold, frostbitten rotting grey flesh with pale cyan undertones
    C_OUT = (16, 22, 28, 255)
    C_DEEP = (38, 48, 56, 255)
    C_SHADOW = (74, 90, 100, 255)
    C_MID = (126, 144, 154, 255)
    C_LIGHT = (186, 206, 216, 255)
    C_HI = (238, 248, 255, 255)
    C_BLADE_EDGE = (248, 252, 255, 255)
    C_BLADE_MID = (175, 195, 205, 255)
    C_EYE = (60, 230, 255, 255)

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

        nl_x = cx - 2 - (step if not slashing else 8)
        draw.polygon([
            (cx - 3, cy + 9), (cx + 3, cy + 9),
            (cx + 4, cy + 18),
            (nl_x + 3, ground_y), (nl_x - 3, ground_y),
            (cx - 2, cy + 18)
        ], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 1, cy + 10), (cx + 2, cy + 18), (nl_x + 1, ground_y - 2)], fill=C_LIGHT, width=2)
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
        # Protective chitin plates
        draw.line([(cx - 4, cy - 2), (cx + 4, cy - 2)], fill=C_SHADOW, width=1)
        draw.line([(cx - 3, cy + 3), (cx + 3, cy + 3)], fill=C_SHADOW, width=1)

        # Head (NO BEAK! Smooth, terrifying alien humanoid skull cowl)
        # Sloping cranial dome extending back, eyeless smooth face plate with narrow cyan ocular slit
        head_pts = [
            (cx - 5, cy - 8),   # Chin / lower jaw
            (cx - 10, cy - 10), # Forward jaw
            (cx - 9, cy - 16),  # Face plate
            (cx - 4, cy - 22),  # Forehead dome
            (cx + 3, cy - 24),  # Cranial crest peaking back
            (cx + 8, cy - 18),  # Elongated alien skull lobe
            (cx + 6, cy - 10),  # Back of neck
            (cx + 3, cy - 8)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        # Smooth polished chitin skull sheen
        draw.line([(cx + 2, cy - 23), (cx - 3, cy - 21), (cx - 8, cy - 15)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy - 22), (cx - 2, cy - 20)], fill=C_HI, width=1)
        draw.line([(cx + 6, cy - 17), (cx + 5, cy - 10)], fill=C_SHADOW, width=2)
        # Eerie glowing cyan slit
        draw.line([(cx - 8, cy - 14), (cx - 5, cy - 14)], fill=C_EYE, width=1)
        draw.point((cx - 7, cy - 14), fill=(210, 255, 255, 255))

        # Arms & Bone Parrying Needle-Daggers
        if blocking:
            # Crossed defensive bone guard in front of chest
            draw.line([(cx - 3, cy - 4), (cx - 8, cy + 2)], fill=C_MID, width=3)
            draw.line([(cx - 8, cy + 2), (cx - 15, cy - 12)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx + 2, cy - 4), (cx - 5, cy + 4)], fill=C_SHADOW, width=3)
            draw.line([(cx - 5, cy + 4), (cx - 11, cy - 8)], fill=C_BLADE_EDGE, width=2)
        elif slashing:
            # Lunging thrust / slash with elongated needle claw
            draw.line([(cx - 3, cy - 4), (cx - 15, cy)], fill=C_MID, width=3)
            draw.line([(cx - 15, cy), (cx - 25, cy + 6)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx - 15, cy - 1), (cx - 23, cy + 4)], fill=C_HI, width=1)
        else:
            # Elegant, eerie spacing guard: hands held close with curved parrying bone talons
            draw.line([(cx - 3, cy - 4), (cx - 4, cy + 4)], fill=C_MID, width=3)
            draw.line([(cx - 4, cy + 4), (cx - 13, cy + 14)], fill=C_BLADE_EDGE, width=2)
            draw.line([(cx - 3, cy + 4), (cx - 11, cy + 12)], fill=C_HI, width=1)

    img.save("assets/sprites/enemies/duelist_passive/duelist_passive.png")
    print("Saved refined duelist_passive.png (288x64)")

# =========================================================================
# 3. NORMAL MUTANT (Sickly greenish-grey rotting corpse ghoul, jagged teeth, feral claws)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_normal_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Decaying, rotting grey corpse flesh with sickly green undertones
    C_OUT = (18, 22, 18, 255)
    C_DEEP = (40, 48, 40, 255)
    C_SHADOW = (76, 88, 78, 255)
    C_MID = (130, 142, 132, 255)
    C_LIGHT = (188, 202, 190, 255)
    C_HI = (235, 245, 235, 255)
    C_TEETH = (248, 242, 215, 255)
    C_CLAW = (225, 220, 195, 255)
    C_EYE = (255, 215, 45, 255)
    C_SINEW = (92, 60, 50, 255)

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
        # Heavy rotting talons
        draw.polygon([(nl_x - 3, ground_y - 2), (nl_x - 6, ground_y), (nl_x - 1, ground_y)], fill=C_CLAW, outline=C_OUT)
        draw.polygon([(nl_x + 1, ground_y - 2), (nl_x, ground_y), (nl_x + 4, ground_y)], fill=C_CLAW, outline=C_OUT)

        # Feral hunched torso (Rotting, sinewy, exposed spine knobs)
        torso_pts = [
            (cx - 6, cy - 8), (cx + 4, cy - 8),
            (cx + 6, cy + 2), (cx + 2, cy + 10),
            (cx - 4, cy + 10), (cx - 7, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx - 5, cy - 7), (cx - 5, cy + 8)], fill=C_LIGHT, width=2)
        draw.line([(cx - 4, cy - 6), (cx - 4, cy + 2)], fill=C_HI, width=1)
        # Exposed spine vertebrae along back
        for s in range(3):
            sx = cx + 2 + s * 2
            sy = cy - 6 + s * 4
            draw.point((sx, sy), fill=C_HI)
            draw.point((sx + 1, sy), fill=C_OUT)

        # Head: Feral rotting mutant skull, sunken eye socket, exposed jagged toothy jaw
        head_pts = [
            (cx - 4, cy - 8),
            (cx - 15, cy - 5),  # Lower toothy jaw
            (cx - 13, cy - 12), # Sunken snout
            (cx - 5, cy - 18),  # Brow
            (cx + 4, cy - 18),  # Cranium
            (cx + 5, cy - 8)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 2, cy - 17), (cx - 4, cy - 17), (cx - 12, cy - 11)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy - 16), (cx - 3, cy - 16)], fill=C_HI, width=1)
        # Feverish yellow eye in sunken dark cavity
        draw.rectangle([cx - 8, cy - 13, cx - 6, cy - 11], fill=C_DEEP)
        draw.point((cx - 7, cy - 12), fill=C_EYE)
        # Jagged teeth exposed along rotted jaw
        for t in range(4):
            tx = cx - 14 + t * 3
            draw.point((tx, cy - 6), fill=C_TEETH)
            draw.point((tx, cy - 7), fill=C_OUT)

        # Clawed Arms (Long, corded, tearing mutated claws)
        sh_x, sh_y = cx - 4, cy - 4
        if windup:
            # Reared high to strike
            el_x, el_y = cx - 2, cy - 16
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_MID, width=4)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            # Wicked claw cluster
            draw.polygon([(el_x - 3, el_y - 2), (el_x - 6, el_y - 8), (el_x + 2, el_y - 6)], fill=C_CLAW, outline=C_OUT)
        elif attacking:
            # Ripping claw swipe forward
            el_x, el_y = cx - 18, cy + 2
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_MID, width=4)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=2)
            # Claws extended forward
            draw.polygon([(el_x, el_y - 3), (el_x - 8, el_y), (el_x, el_y + 3)], fill=C_CLAW, outline=C_OUT)
            draw.line([(el_x - 7, el_y), (el_x - 2, el_y)], fill=C_HI, width=1)
        else:
            # Idle predatory hunch
            el_x, el_y = cx - 10, cy + 10
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_MID, width=3)
            draw.line([(sh_x, sh_y), (el_x, el_y)], fill=C_LIGHT, width=1)
            # 3 grasping claws
            draw.polygon([(el_x - 2, el_y), (el_x - 5, el_y + 5), (el_x + 1, el_y + 3)], fill=C_CLAW, outline=C_OUT)

    img.save("assets/sprites/enemies/normal/normal_mutant.png")
    print("Saved refined normal_mutant.png (288x64)")

# =========================================================================
# 4. TANK MUTANT (72x96, Bloated necrotic corpse titan, bruised veins, calcified armor)
# Frame size: 72 x 96, 6 frames
# =========================================================================
def generate_tank_mutant():
    W, H = 72, 96
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Bloated, pale cadaver grey flesh with bruised purplish-crimson creases and calcified bone crests
    C_OUT = (22, 20, 24, 255)
    C_DEEP = (44, 40, 48, 255)
    C_SHADOW = (84, 80, 90, 255)
    C_MID = (142, 138, 146, 255)
    C_LIGHT = (198, 196, 204, 255)
    C_HI = (246, 246, 252, 255)
    C_VEIN = (108, 48, 64, 255)
    C_BONE = (232, 228, 216, 255)
    C_BONE_SHADOW = (148, 142, 132, 255)
    C_EYE = (255, 110, 40, 255)

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

        draw.polygon([
            (cx - 8, cy + 16), (cx + 6, cy + 16),
            (cx + 6, cy + 32),
            (cx + 4, ground_y), (cx - 10, ground_y),
            (cx - 6, cy + 32)
        ], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 4, cy + 18), (cx + 2, cy + 32), (cx, ground_y - 3)], fill=C_LIGHT, width=3)
        draw.line([(cx - 2, cy + 18), (cx + 3, cy + 32)], fill=C_HI, width=1)
        # Massive calcified toenails
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
        # Bruised necrotic veins pulsing across the swollen gut
        draw.line([(cx - 10, cy), (cx + 10, cy)], fill=C_VEIN, width=2)
        draw.line([(cx - 8, cy + 8), (cx + 8, cy + 8)], fill=C_VEIN, width=2)
        draw.line([(cx - 4, cy - 6), (cx + 6, cy + 4)], fill=C_VEIN, width=1)
        # Calcified bone ribs bursting through the dead flesh
        draw.line([(cx - 12, cy - 4), (cx - 4, cy - 4)], fill=C_BONE, width=2)
        draw.line([(cx - 10, cy + 4), (cx - 2, cy + 4)], fill=C_BONE, width=2)

        # Armored Juggernaut Head / Bone Crest with Spikes
        head_pts = [
            (cx - 8, cy - 14), (cx - 20, cy - 10),
            (cx - 16, cy - 24), (cx - 4, cy - 30),
            (cx + 8, cy - 30), (cx + 12, cy - 14)
        ]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 6, cy - 28), (cx - 4, cy - 28), (cx - 15, cy - 22)], fill=C_LIGHT, width=2)
        draw.line([(cx + 4, cy - 27), (cx - 3, cy - 27)], fill=C_HI, width=1)
        # Bloodshot glowing eye slits
        draw.rectangle([cx - 12, cy - 19, cx - 9, cy - 17], fill=C_DEEP)
        draw.point((cx - 11, cy - 18), fill=C_EYE)
        # Massive calcified bone horns/crests
        for h in range(3):
            hx = cx - 14 + h * 8
            draw.polygon([(hx, cy - 28), (hx + 4, cy - 28), (hx + 2, cy - 36)], fill=C_BONE, outline=C_OUT)
            draw.line([(hx + 1, cy - 29), (hx + 2, cy - 35)], fill=C_HI, width=1)

        # Colossal Mutated Knuckle Fists
        if windup:
            draw.polygon([(cx - 12, cy - 8), (cx - 6, cy - 32), (cx + 6, cy - 32), (cx + 4, cy - 8)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 4, cy - 12), (cx, cy - 30)], fill=C_LIGHT, width=3)
            draw.polygon([(cx - 8, cy - 32), (cx + 8, cy - 32), (cx + 6, cy - 40), (cx - 6, cy - 40)], fill=C_BONE, outline=C_OUT)
        elif smash:
            draw.polygon([(cx - 12, cy), (cx - 28, cy + 26), (cx - 16, cy + 28), (cx - 2, cy + 4)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 10, cy + 4), (cx - 22, cy + 24)], fill=C_LIGHT, width=3)
            # Fist crashing on ground
            draw.rectangle([cx - 32, ground_y - 12, cx - 16, ground_y], fill=C_BONE, outline=C_OUT)
        else:
            # Heavy resting fists
            draw.polygon([(cx - 14, cy - 4), (cx - 24, cy + 18), (cx - 14, cy + 22), (cx - 4, cy)], fill=C_MID, outline=C_OUT)
            draw.line([(cx - 12, cy), (cx - 20, cy + 16)], fill=C_LIGHT, width=3)
            draw.line([(cx - 10, cy), (cx - 18, cy + 16)], fill=C_HI, width=1)
            # Bone-plated heavy knuckle
            draw.rectangle([cx - 25, cy + 16, cx - 13, cy + 24], fill=C_BONE, outline=C_OUT)

    img.save("assets/sprites/enemies/tank/tank_mutant.png")
    print("Saved refined tank_mutant.png (432x96)")

# =========================================================================
# 5. DUMMY MUTANT (Necrotic biological training totem, stitched corpse sack, bruised core)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_dummy_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (18, 20, 24, 255)
    C_DEEP = (42, 44, 52, 255)
    C_SHADOW = (78, 82, 92, 255)
    C_MID = (136, 140, 150, 255)
    C_LIGHT = (194, 200, 210, 255)
    C_HI = (242, 246, 252, 255)
    C_CORE = (210, 35, 45, 255)
    C_CORE_HI = (255, 120, 110, 255)
    C_BONE = (235, 230, 218, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        wobble_x = 0
        if f in (4, 5):
            wobble_x = -3 if f == 4 else 2

        cx = ox + 24 + wobble_x
        cy = 32

        # Rooted rotting spinal column / base on floor
        draw.polygon([(cx - 10, ground_y), (cx + 10, ground_y), (cx + 4, cy + 16), (cx - 4, cy + 16)], fill=C_SHADOW, outline=C_OUT)
        draw.line([(cx - 8, ground_y - 2), (cx + 8, ground_y - 2)], fill=C_LIGHT, width=1)
        # Vertebrae knobs on stalk
        for v in range(3):
            vy = ground_y - 4 - v * 4
            draw.point((cx, vy), fill=C_BONE)

        # Torso pod (Cadaver-grey stitched flesh pod)
        draw.polygon([(cx - 8, cy - 10), (cx + 8, cy - 10), (cx + 9, cy + 6), (cx + 5, cy + 18), (cx - 5, cy + 18), (cx - 9, cy + 6)], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 6, cy - 8), (cx - 6, cy + 14)], fill=C_LIGHT, width=2)
        draw.line([(cx - 5, cy - 8), (cx - 5, cy + 4)], fill=C_HI, width=1)
        # Stitches / sutures
        draw.line([(cx + 3, cy - 8), (cx + 3, cy + 14)], fill=C_DEEP, width=1)
        for st in range(4):
            sty = cy - 6 + st * 5
            draw.line([(cx + 1, sty), (cx + 5, sty)], fill=C_BONE, width=1)

        # Pulsating Bruised Target Core in chest
        draw.ellipse([cx - 5, cy - 4, cx + 5, cy + 6], fill=C_CORE, outline=C_OUT)
        draw.point((cx - 2, cy - 1), fill=C_CORE_HI)

        # Head node (Smooth cadaver dome)
        draw.ellipse([cx - 6, cy - 22, cx + 6, cy - 10], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 3, cy - 20), (cx + 3, cy - 20)], fill=C_HI, width=2)

        # Defensive Bone Plates / Guards
        if f == 2:  # High block
            draw.rectangle([cx - 10, cy - 18, cx - 5, cy - 6], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 9, cy - 17), (cx - 6, cy - 7)], fill=C_HI, width=1)
        elif f == 3: # Low block
            draw.rectangle([cx - 10, cy - 2, cx - 5, cy + 10], fill=C_BONE, outline=C_OUT)
            draw.line([(cx - 9, cy - 1), (cx - 6, cy + 9)], fill=C_HI, width=1)
        else:
            draw.line([(cx - 6, cy - 6), (cx - 8, cy + 8)], fill=C_SHADOW, width=2)

    img.save("assets/sprites/enemies/dummy/dummy_mutant.png")
    print("Saved refined dummy_mutant.png (288x64)")

# =========================================================================
# 6. LURKER MUTANT (Pale necrotic subterranean creeping crawler with mandibles)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_lurker_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (18, 20, 24, 255)
    C_DEEP = (40, 44, 50, 255)
    C_SHADOW = (78, 84, 92, 255)
    C_MID = (132, 138, 148, 255)
    C_LIGHT = (190, 198, 208, 255)
    C_HI = (240, 246, 252, 255)
    C_CLAW = (235, 230, 215, 255)
    C_EYE = (255, 60, 40, 255)

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

        # Legs (Low, spider-like / mantis crawling legs firmly on ground_y)
        draw.line([(cx + 2, cy + 8), (cx + 8, cy + 16), (cx + 6, ground_y)], fill=C_SHADOW, width=3)
        draw.line([(cx - 2, cy + 8), (cx - 6, cy + 16), (cx - 4, ground_y)], fill=C_MID, width=3)
        draw.polygon([(cx - 6, ground_y - 2), (cx - 8, ground_y), (cx - 3, ground_y)], fill=C_CLAW, outline=C_OUT)
        draw.polygon([(cx + 4, ground_y - 2), (cx + 7, ground_y), (cx + 2, ground_y)], fill=C_CLAW, outline=C_OUT)

        # Low creeping necrotic chitin torso
        draw.polygon([(cx - 8, cy - 6), (cx + 6, cy - 6), (cx + 8, cy + 6), (cx + 4, cy + 14), (cx - 4, cy + 14), (cx - 10, cy + 4)], fill=C_MID, outline=C_OUT)
        draw.line([(cx - 7, cy - 4), (cx - 7, cy + 8)], fill=C_LIGHT, width=2)
        draw.line([(cx - 6, cy - 4), (cx - 6, cy + 2)], fill=C_HI, width=1)

        # Predatory Head with large grasping bone mandibles
        head_pts = [(cx - 4, cy - 6), (cx - 14, cy - 2), (cx - 12, cy - 10), (cx - 4, cy - 14), (cx + 4, cy - 14), (cx + 4, cy - 6)]
        draw.polygon(head_pts, fill=C_MID, outline=C_OUT)
        draw.line([(cx + 2, cy - 13), (cx - 3, cy - 13), (cx - 11, cy - 9)], fill=C_LIGHT, width=2)
        draw.point((cx - 7, cy - 8), fill=C_EYE)
        # Sharp bone mandibles
        draw.line([(cx - 13, cy - 4), (cx - 18, cy)], fill=C_CLAW, width=2)
        draw.line([(cx - 13, cy - 8), (cx - 18, cy - 12)], fill=C_CLAW, width=2)

    img.save("assets/sprites/enemies/lurker/lurker_mutant.png")
    print("Saved refined lurker_mutant.png (288x64)")

# Generate all refined mutants
generate_duelist_aggressive()
generate_duelist_passive()
generate_normal_mutant()
generate_tank_mutant()
generate_dummy_mutant()
generate_lurker_mutant()

# Generate tile icons
enemies = {
    'enemy_normal': ('assets/sprites/enemies/normal/normal_mutant.png', 48, 64),
    'enemy_tank': ('assets/sprites/enemies/tank/tank_mutant.png', 72, 96),
    'enemy_duelist': ('assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png', 48, 64),
    'enemy_duelist_passive': ('assets/sprites/enemies/duelist_passive/duelist_passive.png', 48, 64),
    'enemy_dummy': ('assets/sprites/enemies/dummy/dummy_mutant.png', 48, 64),
    'enemy_lurker': ('assets/sprites/enemies/lurker/lurker_mutant.png', 48, 64),
}

for name, (path, fw, fh) in enemies.items():
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

print("Generated all mutant sprites and tile icons successfully!")
