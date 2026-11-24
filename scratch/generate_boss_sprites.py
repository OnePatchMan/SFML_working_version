import os
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8

# Reference-accurate Palette for Pale Mutant Boss:
CLR_TRANS = (0, 0, 0, 0)
CLR_OUTLINE = (24, 28, 38, 255)         # Crisp pixel-art contour

# Pale Mutant Flesh:
CLR_SKIN_SHADOW_DEEP = (62, 72, 92, 255) # Deep muscle shadow
CLR_SKIN_SHADOW = (112, 126, 148, 255)   # Mid shadow
CLR_SKIN_BASE = (174, 188, 206, 255)     # Pale mutant flesh
CLR_SKIN_LIGHT = (218, 228, 240, 255)    # Light pale skin
CLR_SKIN_HI = (248, 251, 255, 255)       # Rim highlight

# Dark Chitin / Armored Under-Plates (From reference):
CLR_PLATE_DEEP = (26, 32, 42, 255)
CLR_PLATE_DARK = (42, 48, 60, 255)
CLR_PLATE_MID = (68, 76, 92, 255)
CLR_PLATE_HI = (102, 114, 134, 255)

# Bone Crest / Collar / Gauntlet Bands (Warm golden-bone accents like reference):
CLR_BONE_DEEP = (78, 66, 54, 255)
CLR_BONE_DARK = (112, 98, 80, 255)
CLR_BONE_MID = (168, 148, 116, 255)
CLR_BONE_LIGHT = (212, 192, 150, 255)
CLR_BONE_HI = (255, 234, 182, 255)

# Cavity (Headless neck hollow):
CLR_CAVITY = (18, 22, 30, 255)

# Claws / Hardened knuckle points:
CLR_CLAW = (246, 244, 236, 255)
CLR_VEIN = (82, 80, 130, 220)

def draw_boss_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Pale Mutant Boss in the EXACT artistic style of the reference image:
    - Dynamic 3/4 combat stance facing LEFT towards the player
    - NO head, NO neck, NO eye (headless collar mantle framing sunken hollow)
    - VERY skinny, lean waist (12-14px wide) with gaunt ribcage and groin tasset
    - Massive gorilla shoulders with pauldron armor
    - Enormous gorilla knuckle-arms with segmented banded gauntlets & claws
    - 5 tall angled dorsal bone spines rising from the back
    """
    ground_y = 88

    # Core anchors
    cx = ox + 66
    cy = 50

    # Animation offsets
    torso_x = cx
    torso_y = cy
    l_arm_x = -32
    l_arm_y = ground_y - 2
    r_arm_x = 28
    r_arm_y = ground_y - 4
    rear_up = False
    slam = False
    jumping = False
    hurt = False

    if frame_idx == 0:
        # Idle low
        torso_y = 52
        l_arm_x = -32
        l_arm_y = ground_y
        r_arm_x = 28
        r_arm_y = ground_y - 2
    elif frame_idx == 1:
        # Idle breathing high
        torso_y = 49
        l_arm_x = -30
        l_arm_y = ground_y
        r_arm_x = 29
        r_arm_y = ground_y - 2
    elif frame_idx == 2:
        # Knuckle walk 1: Front (left) arm reaches forward-left
        torso_y = 50
        torso_x -= 4
        l_arm_x = -44
        l_arm_y = ground_y - 12
        r_arm_x = 24
        r_arm_y = ground_y
    elif frame_idx == 3:
        # Knuckle walk 2: Rear (right) arm swings forward
        torso_y = 50
        torso_x -= 1
        l_arm_x = -34
        l_arm_y = ground_y
        r_arm_x = 14
        r_arm_y = ground_y - 10
    elif frame_idx == 4:
        # Attack windup: Rearing up onto hind legs! Fists raised high!
        rear_up = True
        torso_y = 40
        torso_x += 8
        l_arm_x = -22
        l_arm_y = torso_y - 24
        r_arm_x = 34
        r_arm_y = torso_y - 28
    elif frame_idx == 5:
        # Attack slam: Smashed into floor in front!
        slam = True
        torso_y = 56
        torso_x -= 8
        l_arm_x = -48
        l_arm_y = ground_y + 2
        r_arm_x = 18
        r_arm_y = ground_y + 2
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air
        jumping = True
        torso_y = 32
        torso_x -= 4
        l_arm_x = -34
        l_arm_y = torso_y + 20
        r_arm_x = 20
        r_arm_y = torso_y + 18
    elif frame_idx == 7:
        # Hurt / stunned: Flinched back
        hurt = True
        torso_y = 46
        torso_x += 10
        l_arm_x = -20
        l_arm_y = torso_y + 14
        r_arm_x = 38
        r_arm_y = torso_y + 6

    tx = torso_x
    ty = torso_y

    # ========================================================
    # 1. DORSAL SPINES / BONE PILLARS (Behind the back)
    # ========================================================
    spines = [
        (4, -16, 24, 6, 2),
        (10, -18, 34, 7, 3),
        (18, -19, 42, 7, 4), # Tallest center spine
        (25, -16, 32, 6, 3),
        (31, -12, 22, 5, 2)
    ]
    tilt = 8 if not rear_up else 3
    if hurt: tilt = 12

    for sx, sy, h, w, rings in spines:
        bx = tx + sx
        by = ty + sy
        tx_tip = bx + tilt
        ty_tip = by - h

        # Spine column
        pts = [(bx - w//2, by), (bx + w//2, by), (tx_tip + w//2 - 1, ty_tip), (tx_tip - w//2 + 1, ty_tip)]
        draw.polygon(pts, fill=CLR_PLATE_DARK, outline=CLR_OUTLINE)
        # Left highlight face
        draw.polygon([(bx - w//2 + 1, by), (bx, by), (tx_tip, ty_tip), (tx_tip - w//2 + 1, ty_tip)], fill=CLR_PLATE_MID)
        draw.line([(bx - w//2 + 1, by), (tx_tip - w//2 + 1, ty_tip)], fill=CLR_PLATE_HI, width=1)

        # Golden bone rings around spine
        for r in range(1, rings + 1):
            t = r / (rings + 1)
            rx = int(bx * (1 - t) + tx_tip * t)
            ry = int(by * (1 - t) + ty_tip * t)
            draw.rectangle([rx - w//2, ry - 2, rx + w//2, ry + 1], fill=CLR_BONE_LIGHT, outline=CLR_OUTLINE)
            draw.line([(rx - w//2 + 1, ry - 1), (rx, ry - 1)], fill=CLR_BONE_HI, width=1)
            draw.line([(rx + 1, ry), (rx + w//2 - 1, ry)], fill=CLR_BONE_DARK, width=1)

        # Rounded bone cap on top
        draw.ellipse([tx_tip - w//2, ty_tip - 3, tx_tip + w//2, ty_tip + 2], fill=CLR_BONE_LIGHT, outline=CLR_OUTLINE)
        draw.point((tx_tip - 1, ty_tip - 1), fill=CLR_BONE_HI)

    # ========================================================
    # 2. FAR (RIGHT) HIND LEG (Background leg)
    # ========================================================
    draw_hind_leg(draw, tx + 18, ty + 12, ground_y, is_near=False, rear_up=rear_up, jumping=jumping, step_fwd=frame_idx==3)

    # ========================================================
    # 3. FAR (RIGHT) GORILLA ARM (Background arm)
    # ========================================================
    draw_gorilla_arm(draw, tx + 24, ty - 8, tx + r_arm_x, r_arm_y, is_near=False, rear_up=rear_up, slam=slam, jumping=jumping)

    # ========================================================
    # 4. SKINNY TORSO & PELVIS (Very thin waist, ribcage, groin tasset)
    # ========================================================
    waist_w = 13
    waist_top_y = ty + 3
    waist_bot_y = ty + 18

    # Narrow abdomen core
    draw.polygon([
        (tx - waist_w // 2, waist_top_y),
        (tx + waist_w // 2, waist_top_y),
        (tx + waist_w // 2 + 2, waist_bot_y),
        (tx - waist_w // 2 - 1, waist_bot_y)
    ], fill=CLR_PLATE_DARK, outline=CLR_OUTLINE)

    # Pale skin ribs along the narrow flank
    for r in range(3):
        ry = waist_top_y + 2 + r * 4
        draw.line([(tx - 5, ry), (tx + 5, ry - 1)], fill=CLR_SKIN_BASE, width=1)
        draw.line([(tx - 4, ry + 1), (tx + 6, ry)], fill=CLR_PLATE_DEEP, width=1)
        draw.point((tx - 5, ry), fill=CLR_SKIN_LIGHT)

    # Groin Tasset Armor Plate (The iconic vertical plate with emblem from reference!)
    tasset_x = tx - 2
    tasset_y = waist_bot_y - 2
    draw.rounded_rectangle([tasset_x - 5, tasset_y, tasset_x + 5, tasset_y + 12], radius=2, fill=CLR_BONE_LIGHT, outline=CLR_OUTLINE)
    draw.rectangle([tasset_x - 3, tasset_y + 2, tasset_x + 3, tasset_y + 10], fill=CLR_BONE_MID)
    # Emblem "1" on tasset
    draw.line([(tasset_x, tasset_y + 3), (tasset_x, tasset_y + 9)], fill=CLR_BONE_HI, width=1)
    draw.point((tasset_x - 1, tasset_y + 4), fill=CLR_BONE_HI)
    draw.line([(tasset_x - 2, tasset_y + 9), (tasset_x + 2, tasset_y + 9)], fill=CLR_BONE_HI, width=1)

    # ========================================================
    # 5. NEAR (LEFT) HIND LEG (Foreground leg)
    # ========================================================
    draw_hind_leg(draw, tx - 2, ty + 14, ground_y, is_near=True, rear_up=rear_up, jumping=jumping, step_fwd=frame_idx==2)

    # ========================================================
    # 6. HULKING SHOULDERS & HEADLESS COLLAR MANTLE
    # ========================================================
    # Upper chest & shoulder span
    # Pale pectoral plate below collar:
    chest_pts = [
        (tx - 22, ty - 6),
        (tx + 16, ty - 6),
        (tx + 10, ty + 5),
        (tx - 10, ty + 5)
    ]
    draw.polygon(chest_pts, fill=CLR_SKIN_BASE, outline=CLR_OUTLINE)
    draw.line([(tx - 20, ty - 5), (tx + 14, ty - 5)], fill=CLR_SKIN_LIGHT, width=1)
    draw.line([(tx - 2, ty - 5), (tx - 1, ty + 4)], fill=CLR_SKIN_SHADOW, width=1) # Center pectoral seam

    # Far Shoulder Pauldron (Right shoulder, behind collar)
    draw.polygon([
        (tx + 12, ty - 14),
        (tx + 26, ty - 16),
        (tx + 30, ty - 4),
        (tx + 16, ty - 2)
    ], fill=CLR_PLATE_DARK, outline=CLR_OUTLINE)
    draw.line([(tx + 14, ty - 14), (tx + 25, ty - 15)], fill=CLR_BONE_LIGHT, width=2) # Gold pauldron rim

    # Headless Collar Arch (Thick golden/bone cowl arching over the neck hollow!)
    collar_pts = [
        (tx - 18, ty - 4),
        (tx - 19, ty - 15),
        (tx - 12, ty - 24),
        (tx + 2, ty - 24),
        (tx + 12, ty - 17),
        (tx + 12, ty - 4),
        (tx + 4, ty - 2),
        (tx - 8, ty - 2)
    ]
    draw.polygon(collar_pts, fill=CLR_BONE_LIGHT, outline=CLR_OUTLINE)
    # Upper rim golden highlight
    draw.polygon([
        (tx - 18, ty - 14),
        (tx - 11, ty - 23),
        (tx + 1, ty - 23),
        (tx + 10, ty - 16),
        (tx + 8, ty - 12),
        (tx - 8, ty - 17)
    ], fill=CLR_BONE_HI)
    draw.line([(tx - 16, ty - 15), (tx - 10, ty - 23), (tx, ty - 23)], fill=(255, 245, 220, 255), width=2)

    # SUNKEN HEADLESS CAVITY (NO HEAD, NO NECK, NO EYE!)
    # Pure dark shadowed hollow inside the collar arch
    hollow_pts = [
        (tx - 14, ty - 8),
        (tx - 12, ty - 17),
        (tx - 2, ty - 17),
        (tx + 2, ty - 10),
        (tx - 4, ty - 5),
        (tx - 10, ty - 5)
    ]
    draw.polygon(hollow_pts, fill=CLR_CAVITY, outline=CLR_OUTLINE)
    # Dark organic folds inside cavity (completely eye-less)
    draw.line([(tx - 11, ty - 13), (tx - 5, ty - 8)], fill=CLR_PLATE_DEEP, width=2)
    draw.line([(tx - 8, ty - 15), (tx - 3, ty - 11)], fill=CLR_PLATE_DEEP, width=2)

    # Near Shoulder Pauldron (Left shoulder, in front of collar)
    draw.polygon([
        (tx - 32, ty - 6),
        (tx - 28, ty - 18),
        (tx - 15, ty - 18),
        (tx - 14, ty - 4),
        (tx - 24, ty + 2)
    ], fill=CLR_PLATE_MID, outline=CLR_OUTLINE)
    # Pauldron golden/bone rim trim
    draw.line([(tx - 30, ty - 17), (tx - 16, ty - 17)], fill=CLR_BONE_HI, width=2)
    draw.line([(tx - 31, ty - 16), (tx - 31, ty - 7)], fill=CLR_BONE_LIGHT, width=2)
    draw.line([(tx - 28, ty - 6), (tx - 18, ty - 6)], fill=CLR_PLATE_HI, width=1)

    # ========================================================
    # 7. NEAR (LEFT) GORILLA ARM (Foreground arm)
    # ========================================================
    draw_gorilla_arm(draw, tx - 24, ty - 3, tx + l_arm_x, l_arm_y, is_near=True, rear_up=rear_up, slam=slam, jumping=jumping)

def draw_gorilla_arm(draw, sx, sy, hx, hy, is_near, rear_up, slam, jumping):
    """
    Massive gorilla arm with segmented banded gauntlets & claws directly matching
    the visual style of the reference character!
    """
    c_out = CLR_OUTLINE
    c_base = CLR_SKIN_LIGHT if is_near else CLR_SKIN_SHADOW
    c_mid = CLR_SKIN_BASE if is_near else CLR_SKIN_SHADOW_DEEP
    c_dark = CLR_PLATE_DARK if is_near else CLR_PLATE_DEEP
    c_hi = CLR_SKIN_HI if is_near else CLR_SKIN_LIGHT

    # Band colors (gold/bone cuffs)
    b_light = CLR_BONE_HI if is_near else CLR_BONE_LIGHT
    b_mid = CLR_BONE_LIGHT if is_near else CLR_BONE_MID
    b_dark = CLR_BONE_DARK if is_near else CLR_PLATE_DEEP

    # Elbow joint
    if rear_up:
        ex = (sx + hx) // 2 - (12 if is_near else -8)
        ey = min(sy, hy) - 10
    elif slam:
        ex = (sx + hx) // 2 - 12
        ey = (sy + hy) // 2 - 2
    elif jumping:
        ex = (sx + hx) // 2 - 14
        ey = (sy + hy) // 2
    else:
        # Natural gorilla stance: elbow angled outward and up
        ex = (sx + hx) // 2 - 10
        ey = (sy + hy) // 2 - 6

    # 1. Bicep / Upper Arm
    u_thick = 16 if is_near else 12
    draw.polygon([
        (sx - u_thick // 2, sy),
        (sx + u_thick // 2, sy),
        (ex + u_thick // 2 - 2, ey + 2),
        (ex - u_thick // 2 + 2, ey - 2)
    ], fill=c_mid, outline=c_out)
    draw.line([(sx - u_thick // 2 + 2, sy + 1), (ex - u_thick // 2 + 3, ey - 1)], fill=c_hi, width=2)
    draw.line([(sx + u_thick // 2 - 2, sy + 1), (ex + u_thick // 2 - 1, ey + 1)], fill=c_dark, width=2)

    # 2. Forearm Muscle Body
    f_thick = 18 if is_near else 14
    f_wrist = 20 if is_near else 15
    draw.polygon([
        (ex - f_thick // 2, ey - 2),
        (ex + f_thick // 2, ey + 2),
        (hx + f_wrist // 2, hy - 8),
        (hx - f_wrist // 2, hy - 12)
    ], fill=c_base, outline=c_out)
    draw.line([(ex - f_thick // 2 + 2, ey - 1), (hx - f_wrist // 2 + 2, hy - 11)], fill=c_hi, width=2)

    # 3. SEGMENTED BANDED GAUNTLETS (The heavy wrist & forearm cuffs from reference!)
    # Band 1: Near elbow
    t1 = 0.35
    bx1 = int(ex * (1 - t1) + hx * t1)
    by1 = int(ey * (1 - t1) + hy * t1)
    bw1 = f_thick + 4
    draw.rectangle([bx1 - bw1 // 2, by1 - 4, bx1 + bw1 // 2, by1 + 4], fill=b_mid, outline=c_out)
    draw.line([(bx1 - bw1 // 2 + 1, by1 - 3), (bx1 + bw1 // 2 - 1, by1 - 3)], fill=b_light, width=2)
    draw.line([(bx1 - bw1 // 2 + 1, by1 + 3), (bx1 + bw1 // 2 - 1, by1 + 3)], fill=b_dark, width=1)

    # Band 2: Near wrist (Flared, massive cuff)
    t2 = 0.72
    bx2 = int(ex * (1 - t2) + hx * t2)
    by2 = int(ey * (1 - t2) + hy * t2)
    bw2 = f_wrist + 6
    draw.rectangle([bx2 - bw2 // 2, by2 - 6, bx2 + bw2 // 2, by2 + 6], fill=b_mid, outline=c_out)
    draw.line([(bx2 - bw2 // 2 + 1, by2 - 5), (bx2 + bw2 // 2 - 1, by2 - 5)], fill=b_light, width=2)
    draw.line([(bx2 - bw2 // 2 + 1, by2 + 5), (bx2 + bw2 // 2 - 1, by2 + 5)], fill=b_dark, width=1)
    # Inner dark seam
    draw.line([(bx2 - bw2 // 2 + 2, by2), (bx2 + bw2 // 2 - 2, by2)], fill=CLR_PLATE_DEEP, width=1)

    # 4. COLOSSAL HAND / KNUCKLES / CLAWED DIGITS
    hand_w = 22 if is_near else 16
    hand_h = 16 if is_near else 12

    # Hand block
    draw.rounded_rectangle([hx - hand_w // 2, hy - hand_h, hx + hand_w // 2, hy - 4], radius=3, fill=CLR_PLATE_DARK, outline=c_out)

    # 4 distinct clawed digits reaching down (like the articulated claws in reference!)
    digit_count = 4 if is_near else 3
    spacing = hand_w // digit_count
    for d in range(digit_count):
        dx = hx - hand_w // 2 + 2 + d * spacing
        dy_base = hy - 6
        dy_tip = hy + (0 if not slam else 2)

        draw.polygon([
            (dx - 1, dy_base),
            (dx + 2, dy_base),
            (dx + 1, dy_tip),
            (dx - 2, dy_tip)
        ], fill=c_mid, outline=c_out)
        # Sharp ivory claw point
        draw.point((dx - 1, dy_tip - 1), fill=CLR_CLAW)
        draw.point((dx, dy_tip), fill=CLR_CLAW)

def draw_hind_leg(draw, hx, hy, ground_y, is_near, rear_up, jumping, step_fwd):
    """
    Digitigrade muscular hind leg with armored greave and claws.
    Matches the leg style and armored greaves in the reference!
    """
    c_out = CLR_OUTLINE
    c_dark = CLR_PLATE_DARK if is_near else CLR_PLATE_DEEP
    c_mid = CLR_SKIN_BASE if is_near else CLR_PLATE_DARK
    c_hi = CLR_SKIN_HI if is_near else CLR_SKIN_LIGHT

    # Knee & Foot positions
    kx = hx - (10 if not step_fwd else 16)
    ky = hy + 14
    fx = hx + (2 if not step_fwd else -8)
    fy = ground_y - 2 if not jumping else (hy + 26)

    # 1. Thigh
    draw.polygon([
        (hx - 6, hy - 2),
        (hx + 8, hy - 1),
        (kx + 7, ky + 2),
        (kx - 4, ky + 1)
    ], fill=c_mid, outline=c_out)
    draw.line([(hx - 4, hy), (kx - 2, ky)], fill=c_hi, width=2)
    draw.line([(hx + 6, hy), (kx + 5, ky)], fill=c_dark, width=2)

    # 2. Shin / Armored Greave (The rectangular boot plate from reference!)
    draw.polygon([
        (kx - 5, ky),
        (kx + 6, ky + 1),
        (fx + 6, fy - 4),
        (fx - 4, fy - 4)
    ], fill=c_dark, outline=c_out)

    # Golden/bone greave plate on front of shin
    draw.polygon([
        (kx - 4, ky + 1),
        (kx + 1, ky + 1),
        (fx + 1, fy - 5),
        (fx - 3, fy - 5)
    ], fill=CLR_BONE_LIGHT, outline=c_out)
    draw.line([(kx - 3, ky + 2), (fx - 2, fy - 6)], fill=CLR_BONE_HI, width=1)

    # 3. Heavy Armored Foot with Claws
    foot_w = 14 if is_near else 10
    draw.rectangle([fx - foot_w // 2, fy - 4, fx + foot_w // 2, fy], fill=CLR_PLATE_DARK, outline=c_out)
    draw.rectangle([fx - foot_w // 2 + 1, fy - 4, fx + foot_w // 2 - 1, fy - 2], fill=CLR_BONE_LIGHT)
    # Forward claws
    draw.polygon([(fx - foot_w // 2, fy - 1), (fx - foot_w // 2 - 3, fy), (fx - foot_w // 2, fy)], fill=CLR_CLAW)
    draw.polygon([(fx - 2, fy - 1), (fx - 5, fy), (fx - 2, fy)], fill=CLR_CLAW)

def create_sprite_sheet():
    img = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), CLR_TRANS)
    draw = ImageDraw.Draw(img)

    for f in range(NUM_FRAMES):
        ox = f * FRAME_W
        draw_boss_frame(draw, ox, f)

    img.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Generated 8-frame side-profile boss_pale_mutant.png (1024x96)!")

    # Generate editor preview icons from Frame 0
    frame0 = img.crop((0, 0, FRAME_W, FRAME_H))

    # 64x64 icon
    icon64 = Image.new("RGBA", (64, 64), CLR_TRANS)
    resized64 = frame0.resize((64, 48), Image.Resampling.LANCZOS)
    icon64.paste(resized64, (0, 8))
    icon64.save("assets/tiles/enemy_boss.png")

    # 32x32 icons
    icon32 = Image.new("RGBA", (32, 32), CLR_TRANS)
    resized32 = frame0.resize((32, 24), Image.Resampling.LANCZOS)
    icon32.paste(resized32, (0, 4))
    icon32.save("assets/tiles/tools/enemy_boss.png")
    icon32.save("assets/tiles/enemies/enemy_boss.png")
    print("Generated editor icons!")

if __name__ == "__main__":
    create_sprite_sheet()
