import math
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8

# Colors
CLR_TRANS = (0, 0, 0, 0)
CLR_OUTLINE = (22, 27, 34, 255)         # Dark contour
CLR_SKIN_BASE = (218, 224, 233, 255)     # Pale ashen flesh
CLR_SKIN_MID = (185, 195, 208, 255)      # Midtone pale gray-blue
CLR_SKIN_SHADOW = (138, 150, 168, 255)   # Shadowed pale muscle
CLR_SKIN_DARK = (95, 106, 122, 255)      # Deep shadow under torso
CLR_SKIN_HILIGHT = (245, 248, 252, 255)  # Pure pale highlight
CLR_VEIN = (99, 102, 241, 190)           # Subcutaneous purple/indigo vein
CLR_VEIN_DARK = (67, 56, 202, 210)       # Dark bruised vein
CLR_BONE = (241, 245, 249, 255)          # Bone spurs / spinal crest
CLR_BONE_SHADOW = (148, 163, 184, 255)   # Bone shadow
CLR_EYE_CORE = (255, 255, 255, 255)      # Blinding white eye center
CLR_EYE_RED = (239, 68, 68, 255)         # Crimson sensory fissure
CLR_EYE_DARK = (153, 27, 27, 255)        # Dark red fissure rim
CLR_KNUCKLE = (175, 185, 198, 255)       # Calloused knuckles

def create_sprite_sheet():
    img = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), CLR_TRANS)
    draw = ImageDraw.Draw(img)

    for f in range(NUM_FRAMES):
        ox = f * FRAME_W
        draw_frame(draw, ox, f)

    img.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Saved boss_pale_mutant.png!")

    # Also generate a 64x64 and 32x32 editor icon
    # Extract Frame 0 and make an icon
    frame0 = img.crop((0, 0, FRAME_W, FRAME_H))
    
    # Square thumbnail with transparent background
    icon64 = Image.new("RGBA", (64, 64), CLR_TRANS)
    # Fit frame0 into 64x64 maintaining aspect ratio
    resized_preview = frame0.resize((64, 48), Image.Resampling.LANCZOS)
    icon64.paste(resized_preview, (0, 8))
    icon64.save("assets/tiles/enemy_boss.png")
    
    # 32x32 for tools/palette
    icon32 = Image.new("RGBA", (32, 32), CLR_TRANS)
    resized32 = frame0.resize((32, 24), Image.Resampling.LANCZOS)
    icon32.paste(resized32, (0, 4))
    icon32.save("assets/tiles/tools/enemy_boss.png")
    icon32.save("assets/tiles/enemies/enemy_boss.png")
    print("Saved editor preview icons!")

def draw_frame(draw, ox, frame_idx):
    # Base ground level is around Y = 90
    ground_y = 90

    # Animation offsets based on frame
    # 0: Idle low
    # 1: Idle breath high
    # 2: Crawl step left
    # 3: Crawl step right
    # 4: Attack windup (rearing back)
    # 5: Attack slam (knuckle crash)
    # 6: Jump slam (coiled mid-air)
    # 7: Hurt / stunned
    
    torso_y_off = 0
    l_arm_y_off = 0
    r_arm_y_off = 0
    l_arm_x_off = 0
    r_arm_x_off = 0
    eye_flare = False
    rearing = False
    slamming = False
    jumping = False
    hurt = False

    if frame_idx == 0:
        torso_y_off = 2
    elif frame_idx == 1:
        torso_y_off = 0
    elif frame_idx == 2:
        torso_y_off = 1
        l_arm_y_off = -10
        l_arm_x_off = 8
        r_arm_x_off = -4
    elif frame_idx == 3:
        torso_y_off = 1
        r_arm_y_off = -10
        r_arm_x_off = 8
        l_arm_x_off = -4
    elif frame_idx == 4:
        rearing = True
        torso_y_off = -14
        l_arm_y_off = -30
        r_arm_y_off = -32
        eye_flare = True
    elif frame_idx == 5:
        slamming = True
        torso_y_off = 6
        l_arm_y_off = 4
        r_arm_y_off = 4
        eye_flare = True
    elif frame_idx == 6:
        jumping = True
        torso_y_off = -22
        l_arm_y_off = -18
        r_arm_y_off = -18
    elif frame_idx == 7:
        hurt = True
        torso_y_off = -6
        l_arm_y_off = -8
        r_arm_y_off = -12

    # Center coordinates for this frame
    cx = ox + FRAME_W // 2
    cy = 48 + torso_y_off

    # 1. Hind legs (shorter, powerful digitigrade legs behind torso)
    draw_hind_legs(draw, cx, cy, ground_y, rearing, jumping)

    # 2. Narrow waist & Pelvis (thin lower torso)
    draw_narrow_waist(draw, cx, cy, rearing, hurt)

    # 3. Massive Broad Shoulders & Upper Torso (NO neck, NO head!)
    draw_hulking_upper_torso(draw, cx, cy, eye_flare, rearing, hurt)

    # 4. Long Gorilla Arms & Knuckles (resting on the ground)
    draw_gorilla_arms(draw, cx, cy, ground_y, l_arm_x_off, l_arm_y_off, r_arm_x_off, r_arm_y_off, rearing, slamming, jumping)

def draw_hind_legs(draw, cx, cy, ground_y, rearing, jumping):
    # Pelvis center is around (cx, cy + 18)
    pelvis_y = cy + 18
    
    # Left and right bent legs
    for side in (-1, 1):
        hip_x = cx + side * 14
        knee_x = cx + side * (24 if not rearing else 18)
        knee_y = pelvis_y + 12
        foot_x = cx + side * (26 if not rearing else 22)
        foot_y = ground_y - 2 if not jumping else (pelvis_y + 24)

        # Thigh contour
        draw.polygon([
            (hip_x - side * 4, pelvis_y - 2),
            (knee_x + side * 6, knee_y),
            (knee_x - side * 4, knee_y + 4),
            (hip_x + side * 2, pelvis_y + 6)
        ], fill=CLR_SKIN_SHADOW, outline=CLR_OUTLINE)

        # Calf & Foot
        draw.polygon([
            (knee_x - side * 2, knee_y),
            (knee_x + side * 5, knee_y + 2),
            (foot_x + side * 6, foot_y),
            (foot_x - side * 6, foot_y),
            (knee_x - side * 4, knee_y + 6)
        ], fill=CLR_SKIN_DARK, outline=CLR_OUTLINE)

        # Muscle highlight
        draw.line([(hip_x, pelvis_y + 2), (knee_x, knee_y + 2)], fill=CLR_SKIN_MID, width=2)
        # Claws / foot pads
        draw.rectangle([foot_x - 5, foot_y - 3, foot_x + 5, foot_y], fill=CLR_KNUCKLE, outline=CLR_OUTLINE)

def draw_narrow_waist(draw, cx, cy, rearing, hurt):
    # Narrow waist tapers down from broad chest to pelvis
    # Upper ribcage: width 48, lower waist: width 24
    w_top = 26
    w_bot = 12
    y_top = cy + 4
    y_bot = cy + 22

    waist_poly = [
        (cx - w_top, y_top),
        (cx + w_top, y_top),
        (cx + w_bot, y_bot),
        (cx - w_bot, y_bot)
    ]
    draw.polygon(waist_poly, fill=CLR_SKIN_SHADOW, outline=CLR_OUTLINE)

    # Core abdominal muscles (pale with shadow grooves)
    # Tightly packed mutated abs
    for i in range(3):
        ay = y_top + 4 + i * 5
        aw = int(w_top - i * 4.5)
        draw.rectangle([cx - aw + 3, ay, cx - 2, ay + 3], fill=CLR_SKIN_MID)
        draw.rectangle([cx + 2, ay, cx + aw - 3, ay + 3], fill=CLR_SKIN_MID)
        draw.rectangle([cx - aw + 4, ay + 1, cx - 3, ay + 2], fill=CLR_SKIN_BASE)
        draw.rectangle([cx + 3, ay + 1, cx + aw - 4, ay + 2], fill=CLR_SKIN_BASE)
    
    # Veins along narrow waist
    draw.line([(cx - 7, y_top + 3), (cx - 5, y_bot - 2)], fill=CLR_VEIN, width=1)
    draw.line([(cx + 6, y_top + 4), (cx + 8, y_bot - 1)], fill=CLR_VEIN_DARK, width=1)

def draw_hulking_upper_torso(draw, cx, cy, eye_flare, rearing, hurt):
    # HEADLESS & NECKLESS:
    # Torso ends with a monstrous muscular hunchback/shoulder bridge!
    # Extreme broad shoulders: width reaches ~96px (cx - 48 to cx + 48)
    
    hump_top_y = cy - 24
    shoulder_y = cy - 12
    chest_bottom_y = cy + 6

    # Main torso trapezoid & hunch
    torso_pts = [
        (cx - 20, hump_top_y),         # Top of hunchback
        (cx + 20, hump_top_y),
        (cx + 46, shoulder_y - 2),     # Right shoulder joint
        (cx + 52, shoulder_y + 8),     # Right deltoid bulge
        (cx + 34, chest_bottom_y),     # Right ribcage taper
        (cx - 34, chest_bottom_y),     # Left ribcage taper
        (cx - 52, shoulder_y + 8),     # Left deltoid bulge
        (cx - 46, shoulder_y - 2)      # Left shoulder joint
    ]
    draw.polygon(torso_pts, fill=CLR_SKIN_BASE, outline=CLR_OUTLINE)

    # Shading on shoulders and lower chest
    # Deltoid shadows
    draw.polygon([(cx - 51, shoulder_y + 8), (cx - 45, shoulder_y - 1), (cx - 32, shoulder_y + 12), (cx - 34, chest_bottom_y)], fill=CLR_SKIN_SHADOW)
    draw.polygon([(cx + 51, shoulder_y + 8), (cx + 45, shoulder_y - 1), (cx + 32, shoulder_y + 12), (cx + 34, chest_bottom_y)], fill=CLR_SKIN_SHADOW)
    
    # Pectoral plates (broad, pale, scarred)
    draw.polygon([(cx - 30, shoulder_y + 2), (cx - 4, shoulder_y + 2), (cx - 3, chest_bottom_y - 2), (cx - 28, chest_bottom_y - 2)], fill=CLR_SKIN_HILIGHT, outline=CLR_OUTLINE)
    draw.polygon([(cx + 4, shoulder_y + 2), (cx + 30, shoulder_y + 2), (cx + 28, chest_bottom_y - 2), (cx + 3, chest_bottom_y - 2)], fill=CLR_SKIN_HILIGHT, outline=CLR_OUTLINE)
    
    # Pectoral shadow under-crease
    draw.line([(cx - 27, chest_bottom_y - 1), (cx - 4, chest_bottom_y - 1)], fill=CLR_SKIN_DARK, width=2)
    draw.line([(cx + 4, chest_bottom_y - 1), (cx + 27, chest_bottom_y - 1)], fill=CLR_SKIN_DARK, width=2)

    # SPINAL CREST / BONE PROTRUSIONS ON HUNCH (Replacing head/neck)
    # 5 protruding mutated vertebrae spikes along the top hunch
    for i, sx in enumerate([-16, -8, 0, 8, 16]):
        spike_h = 7 if sx == 0 else (6 if abs(sx) == 8 else 4)
        sy = hump_top_y
        draw.polygon([
            (cx + sx - 3, sy + 2),
            (cx + sx, sy - spike_h),
            (cx + sx + 3, sy + 2)
        ], fill=CLR_BONE, outline=CLR_OUTLINE)
        draw.line([(cx + sx - 1, sy + 1), (cx + sx, sy - spike_h + 1)], fill=CLR_SKIN_HILIGHT)

    # Severe subcutaneous mutated veins crawling across chest and shoulders
    vein_coords_left = [(cx - 38, shoulder_y + 2), (cx - 24, shoulder_y - 4), (cx - 16, shoulder_y + 4), (cx - 12, shoulder_y + 10)]
    vein_coords_right = [(cx + 36, shoulder_y + 4), (cx + 22, shoulder_y - 3), (cx + 15, shoulder_y + 6)]
    draw.line(vein_coords_left, fill=CLR_VEIN, width=1)
    draw.line(vein_coords_right, fill=CLR_VEIN_DARK, width=1)

    # CENTRAL SENSORY FISSIRE / GLOWING CYCLOPEAN EYE IN UPPER CHEST
    # An ominous red mutated fissure where a neck would normally be
    eye_y = shoulder_y - 2
    fissure_w = 8 if not eye_flare else 12
    fissure_h = 10 if not eye_flare else 14
    
    draw.ellipse([cx - fissure_w, eye_y - fissure_h // 2, cx + fissure_w, eye_y + fissure_h // 2], fill=CLR_EYE_DARK, outline=CLR_OUTLINE)
    draw.ellipse([cx - fissure_w + 2, eye_y - fissure_h // 2 + 2, cx + fissure_w - 2, eye_y + fissure_h // 2 - 2], fill=CLR_EYE_RED)
    draw.ellipse([cx - 2, eye_y - 2, cx + 2, eye_y + 2], fill=CLR_EYE_CORE)

    # Pulsing horizontal sensory slits beside the eye
    for s in (-1, 1):
        draw.line([(cx + s * 9, eye_y), (cx + s * 18, eye_y - 1)], fill=CLR_EYE_RED, width=1)
        draw.line([(cx + s * 10, eye_y + 3), (cx + s * 16, eye_y + 3)], fill=CLR_EYE_DARK, width=1)

def draw_gorilla_arms(draw, cx, cy, ground_y, l_off_x, l_off_y, r_off_x, r_off_y, rearing, slamming, jumping):
    # GORILLA KNUCKLE-WALKING ARMS:
    # Huge, thick, muscular arms reaching all the way to the ground
    
    # Left Arm
    draw_single_arm(draw, cx, cy, ground_y, side=-1, off_x=l_off_x, off_y=l_off_y, rearing=rearing, slamming=slamming, jumping=jumping)
    
    # Right Arm
    draw_single_arm(draw, cx, cy, ground_y, side=1, off_x=r_off_x, off_y=r_off_y, rearing=rearing, slamming=slamming, jumping=jumping)

def draw_single_arm(draw, cx, cy, ground_y, side, off_x, off_y, rearing, slamming, jumping):
    shoulder_x = cx + side * 44
    shoulder_y = cy - 6

    # Normal knuckle stance: hands touch the ground at ground_y
    if rearing:
        # Fists raised high overhead in ferocious combat pose
        elbow_x = shoulder_x + side * 14
        elbow_y = shoulder_y - 18
        hand_x = elbow_x - side * 6 + off_x
        hand_y = elbow_y - 20 + off_y
    elif jumping:
        # Tucked or raised
        elbow_x = shoulder_x + side * 16
        elbow_y = shoulder_y + 14
        hand_x = elbow_x - side * 8 + off_x
        hand_y = elbow_y + 12 + off_y
    elif slamming:
        # Smashing down with full force into floor
        elbow_x = shoulder_x + side * 12
        elbow_y = shoulder_y + 20
        hand_x = shoulder_x + side * 18 + off_x
        hand_y = ground_y + 2 + off_y
    else:
        # Standard knuckle-walking stance (hands resting firmly on ground)
        elbow_x = shoulder_x + side * 12
        elbow_y = cy + 18
        hand_x = shoulder_x + side * 6 + off_x
        hand_y = ground_y - 2 + off_y

    # Upper Arm / Bicep (very thick, pale muscular cylinder)
    arm_thickness = 14
    draw.polygon([
        (shoulder_x - arm_thickness // 2, shoulder_y),
        (shoulder_x + arm_thickness // 2, shoulder_y),
        (elbow_x + arm_thickness // 2, elbow_y),
        (elbow_x - arm_thickness // 2, elbow_y)
    ], fill=CLR_SKIN_BASE, outline=CLR_OUTLINE)

    # Bicep muscle shading
    draw.line([(shoulder_x + side * 2, shoulder_y + 4), (elbow_x + side * 2, elbow_y - 2)], fill=CLR_SKIN_HILIGHT, width=3)
    draw.line([(shoulder_x - side * 4, shoulder_y + 4), (elbow_x - side * 4, elbow_y - 2)], fill=CLR_SKIN_SHADOW, width=3)

    # Forearm (even thicker, monstrous gorilla forearm)
    forearm_thick_top = 16
    forearm_thick_bot = 18
    draw.polygon([
        (elbow_x - forearm_thick_top // 2, elbow_y),
        (elbow_x + forearm_thick_top // 2, elbow_y),
        (hand_x + forearm_thick_bot // 2, hand_y - 8),
        (hand_x - forearm_thick_bot // 2, hand_y - 8)
    ], fill=CLR_SKIN_MID, outline=CLR_OUTLINE)

    # Forearm muscle ridges
    draw.line([(elbow_x, elbow_y + 2), (hand_x, hand_y - 8)], fill=CLR_SKIN_HILIGHT, width=3)
    # Forearm dark veins
    draw.line([(elbow_x - side * 3, elbow_y + 4), (hand_x - side * 2, hand_y - 10)], fill=CLR_VEIN, width=1)

    # Massive Knuckle / Clenched Fist
    fist_w = 20
    fist_h = 14
    fist_rect = [hand_x - fist_w // 2, hand_y - fist_h, hand_x + fist_w // 2, hand_y]
    draw.rounded_rectangle(fist_rect, radius=4, fill=CLR_KNUCKLE, outline=CLR_OUTLINE)
    
    # 4 hardened pale bone knuckles
    for k in range(4):
        kx = (hand_x - fist_w // 2 + 3) + k * 4
        ky = hand_y - 4
        draw.rectangle([kx, ky, kx + 2, ky + 2], fill=CLR_BONE)
        draw.point((kx + 1, ky + 1), fill=CLR_SKIN_HILIGHT)

if __name__ == "__main__":
    import os
    os.makedirs("assets/sprites/enemies/boss", exist_ok=True)
    os.makedirs("assets/tiles", exist_ok=True)
    os.makedirs("assets/tiles/tools", exist_ok=True)
    os.makedirs("assets/tiles/enemies", exist_ok=True)
    create_sprite_sheet()
