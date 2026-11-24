import os
from PIL import Image, ImageDraw

os.makedirs("assets/sprites/enemies/duelist_aggressive", exist_ok=True)
os.makedirs("assets/sprites/enemies/duelist_passive", exist_ok=True)
os.makedirs("assets/sprites/enemies/normal", exist_ok=True)
os.makedirs("assets/sprites/enemies/tank", exist_ok=True)
os.makedirs("assets/sprites/enemies/dummy", exist_ok=True)
os.makedirs("assets/sprites/enemies/lurker", exist_ok=True)

# Common utility: draw outline & fill
def draw_claw(draw, cx, cy, fill_col, out_col):
    draw.polygon([(cx, cy - 2), (cx - 2, cy), (cx + 1, cy)], fill=fill_col, outline=out_col)

# =========================================================================
# 1. DUELIST AGGRESSIVE (Mantis-scythe arms, sharp beak head from nape to chin)
# Frame size: 48 x 64, 6 frames (Idle, Walk1, Walk2, Windup, Slash, Hurt)
# =========================================================================
def generate_duelist_aggressive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (18, 12, 30, 255)
    C_BASE = (145, 45, 215, 255)
    C_LIGHT = (205, 110, 255, 255)
    C_HI = (245, 210, 255, 255)
    C_DARK = (80, 20, 125, 255)
    C_DEEP = (40, 10, 70, 255)
    C_BLADE = (250, 252, 255, 255)
    C_BLADE_EDGE = (210, 220, 245, 255)
    C_EYE = (255, 30, 80, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61

        # Frame modifiers
        bob_y = 0
        l_arm_raised = False
        r_arm_slash = False
        step = 0
        hurt = False

        if f == 0:
            bob_y = 0
        elif f == 1:
            bob_y = -1
            step = -3
        elif f == 2:
            bob_y = -1
            step = 3
        elif f == 3:
            # Windup: blades reared back
            bob_y = -2
            l_arm_raised = True
        elif f == 4:
            # Slash: lunged forward with scythes
            bob_y = 2
            r_arm_slash = True
        elif f == 5:
            # Hurt: flinched
            bob_y = -1
            hurt = True

        cx = ox + 24
        cy = 32 + bob_y

        # Far leg (Background)
        fl_foot_x = cx + 8 + (step if f != 4 else 12)
        fl_knee_x = cx + 12 + (step // 2)
        draw.polygon([
            (cx + 4, cy + 10), (cx + 10, cy + 10),
            (fl_knee_x + 3, cy + 18),
            (fl_foot_x + 3, ground_y), (fl_foot_x - 4, ground_y),
            (fl_knee_x - 3, cy + 19)
        ], fill=C_DARK, outline=C_OUT)
        draw_claw(draw, fl_foot_x - 1, ground_y, C_BLADE, C_OUT)

        # Far Mantis Arm (Background)
        if l_arm_raised:
            # Reared high
            fa_el = (cx - 2, cy - 14)
            fa_tip = (cx + 8, cy - 24)
            draw.line([(cx + 2, cy - 4), fa_el], fill=C_DARK, width=3)
            draw.line([fa_el, fa_tip], fill=C_BLADE, width=2)
        elif r_arm_slash:
            # Slashed forward
            fa_el = (cx - 14, cy + 2)
            fa_tip = (cx - 22, cy + 12)
            draw.line([(cx + 2, cy - 2), fa_el], fill=C_DARK, width=3)
            draw.line([fa_el, fa_tip], fill=C_BLADE, width=2)
        else:
            # Folded predator stance: upper arm down-fwd, elbow up-back, scythe down-fwd
            fa_el = (cx - 2, cy - 2)
            fa_tip = (cx - 12, cy + 12)
            draw.line([(cx + 2, cy - 4), (fa_el[0] + 4, fa_el[1] + 6)], fill=C_DARK, width=3)
            draw.line([(fa_el[0] + 4, fa_el[1] + 6), fa_tip], fill=C_BLADE, width=2)

        # Torso (Lean, agile, pinched waist)
        torso_pts = [
            (cx - 5, cy - 8),
            (cx + 4, cy - 8),
            (cx + 6, cy + 2),
            (cx + 3, cy + 12),
            (cx - 3, cy + 12),
            (cx - 6, cy + 2)
        ]
        draw.polygon(torso_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 4, cy - 6), (cx - 4, cy + 8)], fill=C_LIGHT, width=2)
        draw.line([(cx - 3, cy - 6), (cx - 3, cy + 8)], fill=C_HI, width=1)
        draw.line([(cx + 3, cy - 6), (cx + 3, cy + 8)], fill=C_DARK, width=2)

        # HEAD: Exact match to user sketch ('Г')!
        # Sharp beak/cone head starting from nape (затылок) down to chin point (подбородок)!
        # Forward pointing predatory wedge
        head_pts = [
            (cx - 3, cy - 8),   # Neck junction
            (cx - 16, cy - 8),  # Sharp beak / snout tip pointing forward-down!
            (cx - 14, cy - 14), # Upper beak slope
            (cx - 6, cy - 20),  # Forehead
            (cx + 3, cy - 22),  # Crown / Nape peak
            (cx + 6, cy - 14),  # Back of head
            (cx + 4, cy - 8)    # Nape to neck
        ]
        draw.polygon(head_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx + 2, cy - 21), (cx - 6, cy - 19), (cx - 14, cy - 13), (cx - 15, cy - 8)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy - 20), (cx - 5, cy - 18), (cx - 13, cy - 12)], fill=C_HI, width=1)
        # Slanted predatory eye (from user's sketch)
        draw.polygon([(cx - 7, cy - 14), (cx - 4, cy - 15), (cx - 5, cy - 13)], fill=C_EYE)

        # Near leg (Foreground)
        nl_foot_x = cx - 2 - (step if f != 4 else 8)
        nl_knee_x = cx + 4 - (step // 2)
        draw.polygon([
            (cx - 2, cy + 10), (cx + 4, cy + 10),
            (nl_knee_x + 3, cy + 18),
            (nl_foot_x + 4, ground_y), (nl_foot_x - 4, ground_y),
            (nl_knee_x - 3, cy + 19)
        ], fill=C_BASE, outline=C_OUT)
        draw.line([(cx, cy + 11), (nl_knee_x + 1, cy + 18), (nl_foot_x + 2, ground_y - 2)], fill=C_LIGHT, width=2)
        draw.line([(cx + 1, cy + 11), (nl_knee_x + 2, cy + 18)], fill=C_HI, width=1)
        draw_claw(draw, nl_foot_x - 1, ground_y, C_BLADE, C_OUT)

        # Near Mantis Arm (Foreground) - EXACT match to user sketch ('Р')!
        # Bent mantis arm with NO hands/palms, tapering to sharp scythe!
        if l_arm_raised:
            # Reared up high ready to strike
            sh_x, sh_y = cx - 3, cy - 5
            el_x, el_y = cx - 8, cy - 18
            scythe_tip = (cx + 4, cy - 28)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_BASE, outline=C_OUT)
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_BLADE, outline=C_OUT)
        elif r_arm_slash:
            # Full extension slash forward
            sh_x, sh_y = cx - 3, cy - 4
            el_x, el_y = cx - 12, cy - 2
            scythe_tip = (cx - 24, cy + 6)
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 2, sh_y), (el_x + 2, el_y), (el_x - 2, el_y)], fill=C_BASE, outline=C_OUT)
            draw.polygon([(el_x - 2, el_y), (el_x + 2, el_y), (scythe_tip[0] + 1, scythe_tip[1]), (scythe_tip[0] - 1, scythe_tip[1])], fill=C_BLADE, outline=C_OUT)
            draw.line([(el_x, el_y), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)
        else:
            # Default folded mantis posture ('Р'):
            # Shoulder -> upper arm goes down-fwd -> elbow bent sharp backwards/upwards -> blade curves down-forward!
            sh_x, sh_y = cx - 3, cy - 4
            el_x, el_y = cx - 1, cy + 4
            scythe_mid = (cx - 7, cy + 8)
            scythe_tip = (cx - 15, cy + 18)
            # Upper arm
            draw.polygon([(sh_x - 2, sh_y), (sh_x + 3, sh_y), (el_x + 3, el_y), (el_x - 2, el_y)], fill=C_BASE, outline=C_OUT)
            draw.line([(sh_x, sh_y + 1), (el_x, el_y - 1)], fill=C_LIGHT, width=2)
            # Mantis Scythe blade (no hands!)
            scythe_poly = [
                (el_x - 2, el_y),
                (el_x + 2, el_y),
                (scythe_mid[0] + 2, scythe_mid[1]),
                (scythe_tip[0] + 1, scythe_tip[1]),
                (scythe_tip[0] - 1, scythe_tip[1]),
                (scythe_mid[0] - 3, scythe_mid[1])
            ]
            draw.polygon(scythe_poly, fill=C_BLADE, outline=C_OUT)
            draw.line([(el_x, el_y + 1), (scythe_mid[0], scythe_mid[1]), (scythe_tip[0], scythe_tip[1])], fill=C_HI, width=1)

    img.save("assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png")
    print("Generated duelist_aggressive.png (288x64)")

# =========================================================================
# 2. DUELIST PASSIVE (Cyan/azure defensive mantis mutant)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_duelist_passive():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (10, 24, 38, 255)
    C_BASE = (35, 170, 235, 255)
    C_LIGHT = (110, 220, 255, 255)
    C_HI = (215, 245, 255, 255)
    C_DARK = (20, 95, 145, 255)
    C_BLADE = (248, 252, 255, 255)
    C_EYE = (80, 245, 255, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        bob_y = 0
        step = 0
        blocking = (f == 3)
        slashing = (f == 4)
        if f in (1, 2):
            step = 3 if f == 1 else -3
            bob_y = -1
        elif f == 5:
            bob_y = -1

        cx = ox + 24
        cy = 32 + bob_y

        # Legs
        draw.polygon([(cx + 4, cy + 10), (cx + 9, cy + 10), (cx + 12, cy + 18), (cx + 9, ground_y), (cx + 4, ground_y), (cx + 7, cy + 18)], fill=C_DARK, outline=C_OUT)
        draw.polygon([(cx - 2, cy + 10), (cx + 4, cy + 10), (cx + 4, cy + 18), (cx - 2, ground_y), (cx - 7, ground_y), (cx - 1, cy + 18)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx, cy + 11), (cx + 2, cy + 18), (cx - 3, ground_y - 2)], fill=C_LIGHT, width=2)
        draw_claw(draw, cx - 4, ground_y, C_BLADE, C_OUT)
        draw_claw(draw, cx + 7, ground_y, C_BLADE, C_OUT)

        # Torso with defensive chitin shield ridge
        draw.polygon([(cx - 5, cy - 8), (cx + 5, cy - 8), (cx + 6, cy + 2), (cx + 3, cy + 12), (cx - 3, cy + 12), (cx - 6, cy + 2)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 4, cy - 6), (cx - 4, cy + 8)], fill=C_LIGHT, width=2)

        # Head (Sleek aerodynamic beak mutant head)
        head_pts = [(cx - 3, cy - 8), (cx - 15, cy - 8), (cx - 13, cy - 14), (cx - 5, cy - 20), (cx + 3, cy - 22), (cx + 6, cy - 14), (cx + 4, cy - 8)]
        draw.polygon(head_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx + 2, cy - 21), (cx - 5, cy - 19), (cx - 13, cy - 13)], fill=C_HI, width=2)
        draw.polygon([(cx - 7, cy - 14), (cx - 4, cy - 15), (cx - 5, cy - 13)], fill=C_EYE)

        # Arms: folded defensive scythe blades in front
        if blocking:
            # Crossed guard
            draw.line([(cx - 3, cy - 4), (cx - 8, cy + 4)], fill=C_BASE, width=3)
            draw.line([(cx - 8, cy + 4), (cx - 14, cy - 10)], fill=C_BLADE, width=3)
            draw.line([(cx + 2, cy - 4), (cx - 6, cy + 6)], fill=C_DARK, width=3)
            draw.line([(cx - 6, cy + 6), (cx - 12, cy - 8)], fill=C_BLADE, width=3)
        elif slashing:
            draw.line([(cx - 3, cy - 4), (cx - 14, cy)], fill=C_BASE, width=3)
            draw.line([(cx - 14, cy), (cx - 24, cy + 8)], fill=C_BLADE, width=2)
        else:
            # Defensive mantis fold
            draw.line([(cx - 3, cy - 4), (cx - 2, cy + 4)], fill=C_BASE, width=3)
            draw.line([(cx - 2, cy + 4), (cx - 12, cy + 16)], fill=C_BLADE, width=2)

    img.save("assets/sprites/enemies/duelist_passive/duelist_passive.png")
    print("Generated duelist_passive.png (288x64)")

# =========================================================================
# 3. NORMAL MUTANT (Grey bipedal feral stalker with clawed arms & hunch)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_normal_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (18, 20, 24, 255)
    C_BASE = (130, 138, 148, 255)
    C_LIGHT = (185, 194, 206, 255)
    C_HI = (235, 240, 248, 255)
    C_DARK = (70, 76, 84, 255)
    C_CLAW = (245, 248, 250, 255)
    C_EYE = (255, 215, 40, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        bob_y = 0
        step = 0
        attacking = (f == 4)
        windup = (f == 3)
        if f in (1, 2):
            step = 3 if f == 1 else -3
            bob_y = -1
        elif f == 5:
            bob_y = -1

        cx = ox + 24
        cy = 32 + bob_y

        # Hind legs (Feral hunch)
        draw.polygon([(cx + 3, cy + 8), (cx + 8, cy + 8), (cx + 10, cy + 18), (cx + 8, ground_y), (cx + 2, ground_y), (cx + 5, cy + 18)], fill=C_DARK, outline=C_OUT)
        draw.polygon([(cx - 3, cy + 8), (cx + 3, cy + 8), (cx + 3, cy + 18), (cx - 3, ground_y), (cx - 8, ground_y), (cx - 2, cy + 18)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 1, cy + 9), (cx + 1, cy + 18), (cx - 4, ground_y - 2)], fill=C_LIGHT, width=2)
        draw_claw(draw, cx - 5, ground_y, C_CLAW, C_OUT)
        draw_claw(draw, cx + 5, ground_y, C_CLAW, C_OUT)

        # Feral hunched torso
        draw.polygon([(cx - 6, cy - 8), (cx + 4, cy - 8), (cx + 6, cy + 2), (cx + 2, cy + 10), (cx - 4, cy + 10), (cx - 7, cy + 2)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 5, cy - 6), (cx - 5, cy + 6)], fill=C_LIGHT, width=2)

        # Head: Feral toothy mutant jaw & yellow eye
        head_pts = [(cx - 4, cy - 8), (cx - 14, cy - 6), (cx - 12, cy - 12), (cx - 4, cy - 18), (cx + 4, cy - 18), (cx + 4, cy - 8)]
        draw.polygon(head_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx + 2, cy - 17), (cx - 4, cy - 17), (cx - 11, cy - 11)], fill=C_HI, width=2)
        draw.point((cx - 7, cy - 12), fill=C_EYE)
        # Jagged teeth
        for t in range(3):
            tx = cx - 13 + t * 3
            draw.point((tx, cy - 7), fill=C_CLAW)

        # Clawed Arms
        if windup:
            draw.line([(cx - 4, cy - 4), (cx - 2, cy - 18)], fill=C_BASE, width=3)
            draw_claw(draw, cx - 2, cy - 18, C_CLAW, C_OUT)
        elif attacking:
            draw.line([(cx - 4, cy - 4), (cx - 18, cy)], fill=C_BASE, width=4)
            draw_claw(draw, cx - 18, cy, C_CLAW, C_OUT)
        else:
            draw.line([(cx - 4, cy - 4), (cx - 10, cy + 10)], fill=C_BASE, width=3)
            draw_claw(draw, cx - 10, cy + 10, C_CLAW, C_OUT)

    img.save("assets/sprites/enemies/normal/normal_mutant.png")
    print("Generated normal_mutant.png (288x64)")

# =========================================================================
# 4. TANK MUTANT (Hulking crimson brute mutant with massive chitin armor)
# Frame size: 72 x 96, 6 frames
# =========================================================================
def generate_tank_mutant():
    W, H = 72, 96
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (24, 10, 14, 255)
    C_BASE = (205, 38, 48, 255)
    C_LIGHT = (245, 95, 105, 255)
    C_HI = (255, 180, 190, 255)
    C_DARK = (115, 18, 24, 255)
    C_DEEP = (60, 8, 12, 255)
    C_HORN = (240, 235, 220, 255)
    C_EYE = (255, 240, 60, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 93
        bob_y = 0
        smash = (f == 4)
        windup = (f == 3)
        if f in (1, 2):
            bob_y = -2
        elif f == 5:
            bob_y = -2

        cx = ox + 36
        cy = 48 + bob_y

        # Massive Pillar Legs
        draw.polygon([(cx + 6, cy + 16), (cx + 18, cy + 16), (cx + 22, cy + 32), (cx + 20, ground_y), (cx + 8, ground_y), (cx + 12, cy + 32)], fill=C_DARK, outline=C_OUT)
        draw.polygon([(cx - 8, cy + 16), (cx + 6, cy + 16), (cx + 6, cy + 32), (cx + 4, ground_y), (cx - 10, ground_y), (cx - 6, cy + 32)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 4, cy + 18), (cx + 2, cy + 32), (cx, ground_y - 3)], fill=C_LIGHT, width=3)
        draw_claw(draw, cx - 5, ground_y, C_HORN, C_OUT)
        draw_claw(draw, cx + 14, ground_y, C_HORN, C_OUT)

        # Colossal Armored Chitin Torso
        torso_pts = [
            (cx - 14, cy - 14), (cx + 14, cy - 14),
            (cx + 18, cy + 4), (cx + 10, cy + 20),
            (cx - 10, cy + 20), (cx - 18, cy + 4)
        ]
        draw.polygon(torso_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 12, cy - 10), (cx - 12, cy + 14)], fill=C_LIGHT, width=3)
        draw.line([(cx - 10, cy - 10), (cx - 10, cy + 14)], fill=C_HI, width=1)
        # Armor plates/bands across chest
        draw.line([(cx - 12, cy - 2), (cx + 12, cy - 2)], fill=C_DARK, width=2)
        draw.line([(cx - 10, cy + 6), (cx + 10, cy + 6)], fill=C_DARK, width=2)

        # Armored Juggernaut Head / Crest with Spikes
        head_pts = [(cx - 8, cy - 14), (cx - 20, cy - 10), (cx - 16, cy - 24), (cx - 4, cy - 30), (cx + 8, cy - 30), (cx + 12, cy - 14)]
        draw.polygon(head_pts, fill=C_BASE, outline=C_OUT)
        draw.line([(cx + 6, cy - 28), (cx - 4, cy - 28), (cx - 15, cy - 22)], fill=C_HI, width=2)
        draw.point((cx - 10, cy - 18), fill=C_EYE)
        # Chitin crest horns
        for h in range(3):
            hx = cx - 14 + h * 8
            draw.polygon([(hx, cy - 28), (hx + 4, cy - 28), (hx + 2, cy - 36)], fill=C_HORN, outline=C_OUT)

        # Huge Mutated Fists / Forearms
        if windup:
            draw.polygon([(cx - 12, cy - 8), (cx - 6, cy - 32), (cx + 6, cy - 32), (cx + 4, cy - 8)], fill=C_BASE, outline=C_OUT)
        elif smash:
            draw.polygon([(cx - 12, cy), (cx - 28, cy + 26), (cx - 16, cy + 28), (cx - 2, cy + 4)], fill=C_BASE, outline=C_OUT)
        else:
            draw.polygon([(cx - 14, cy - 4), (cx - 24, cy + 18), (cx - 14, cy + 22), (cx - 4, cy)], fill=C_BASE, outline=C_OUT)
            draw.line([(cx - 12, cy), (cx - 20, cy + 16)], fill=C_LIGHT, width=3)

    img.save("assets/sprites/enemies/tank/tank_mutant.png")
    print("Generated tank_mutant.png (432x96)")

# =========================================================================
# 5. DUMMY MUTANT (Mutated biological training dummy / fleshy pod)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_dummy_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (24, 18, 10, 255)
    C_BASE = (215, 165, 65, 255)
    C_LIGHT = (250, 210, 110, 255)
    C_HI = (255, 240, 170, 255)
    C_DARK = (130, 95, 30, 255)
    C_CORE = (210, 40, 35, 255)
    C_CORE_HI = (255, 120, 110, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        wobble_x = 0
        if f in (4, 5):
            wobble_x = -3 if f == 4 else 2

        cx = ox + 24 + wobble_x
        cy = 32

        # Rooted biological stalk / base on floor
        draw.polygon([(cx - 10, ground_y), (cx + 10, ground_y), (cx + 4, cy + 16), (cx - 4, cy + 16)], fill=C_DARK, outline=C_OUT)
        draw.line([(cx - 8, ground_y - 2), (cx + 8, ground_y - 2)], fill=C_LIGHT, width=1)

        # Torso pod
        draw.polygon([(cx - 8, cy - 10), (cx + 8, cy - 10), (cx + 9, cy + 6), (cx + 5, cy + 18), (cx - 5, cy + 18), (cx - 9, cy + 6)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 6, cy - 8), (cx - 6, cy + 14)], fill=C_LIGHT, width=2)

        # Pulsating Target Core in chest
        draw.ellipse([cx - 5, cy - 4, cx + 5, cy + 6], fill=C_CORE, outline=C_OUT)
        draw.point((cx - 2, cy - 1), fill=C_CORE_HI)

        # Head node
        draw.ellipse([cx - 6, cy - 22, cx + 6, cy - 10], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 3, cy - 20), (cx + 3, cy - 20)], fill=C_HI, width=2)

        # Arms: wrapped around or defensive
        if f == 2:  # High block
            draw.rectangle([cx - 10, cy - 18, cx - 6, cy - 6], fill=C_LIGHT, outline=C_OUT)
        elif f == 3: # Low block
            draw.rectangle([cx - 10, cy - 2, cx - 6, cy + 10], fill=C_LIGHT, outline=C_OUT)
        else:
            draw.line([(cx - 6, cy - 6), (cx - 8, cy + 8)], fill=C_DARK, width=2)

    img.save("assets/sprites/enemies/dummy/dummy_mutant.png")
    print("Generated dummy_mutant.png (288x64)")

# =========================================================================
# 6. LURKER MUTANT (Orange burrowing subterranean stalker with mandibles)
# Frame size: 48 x 64, 6 frames
# =========================================================================
def generate_lurker_mutant():
    W, H = 48, 64
    NFRAMES = 6
    img = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    C_OUT = (24, 12, 8, 255)
    C_BASE = (240, 130, 20, 255)
    C_LIGHT = (255, 185, 80, 255)
    C_HI = (255, 230, 160, 255)
    C_DARK = (145, 65, 10, 255)
    C_CLAW = (245, 240, 225, 255)
    C_EYE = (255, 40, 40, 255)

    for f in range(NFRAMES):
        ox = f * W
        ground_y = 61
        cx = ox + 24
        cy = 34

        # Frame 4 is burrowed mound under ground
        if f == 4:
            draw.ellipse([cx - 14, ground_y - 6, cx + 14, ground_y + 4], fill=C_DARK, outline=C_OUT)
            draw.line([(cx - 4, ground_y - 8), (cx - 2, ground_y - 14)], fill=C_BASE, width=2)
            draw.line([(cx + 4, ground_y - 8), (cx + 2, ground_y - 14)], fill=C_BASE, width=2)
            continue

        # Legs (Low, spider-like / mantis crawling legs)
        draw.line([(cx + 2, cy + 8), (cx + 8, cy + 16), (cx + 6, ground_y)], fill=C_DARK, width=3)
        draw.line([(cx - 2, cy + 8), (cx - 6, cy + 16), (cx - 4, ground_y)], fill=C_BASE, width=3)
        draw_claw(draw, cx - 4, ground_y, C_CLAW, C_OUT)
        draw_claw(draw, cx + 6, ground_y, C_CLAW, C_OUT)

        # Low creeping chitinous torso
        draw.polygon([(cx - 8, cy - 6), (cx + 6, cy - 6), (cx + 8, cy + 6), (cx + 4, cy + 14), (cx - 4, cy + 14), (cx - 10, cy + 4)], fill=C_BASE, outline=C_OUT)
        draw.line([(cx - 7, cy - 4), (cx - 7, cy + 8)], fill=C_LIGHT, width=2)

        # Predatory Head with large grasping mandibles
        head_pts = [(cx - 4, cy - 6), (cx - 14, cy - 2), (cx - 12, cy - 10), (cx - 4, cy - 14), (cx + 4, cy - 14), (cx + 4, cy - 6)]
        draw.polygon(head_pts, fill=C_BASE, outline=C_OUT)
        draw.point((cx - 7, cy - 8), fill=C_EYE)
        # Mandibles
        draw.line([(cx - 13, cy - 4), (cx - 18, cy)], fill=C_CLAW, width=2)
        draw.line([(cx - 13, cy - 8), (cx - 18, cy - 12)], fill=C_CLAW, width=2)

    img.save("assets/sprites/enemies/lurker/lurker_mutant.png")
    print("Generated lurker_mutant.png (288x64)")

generate_duelist_aggressive()
generate_duelist_passive()
generate_normal_mutant()
generate_tank_mutant()
generate_dummy_mutant()
generate_lurker_mutant()
