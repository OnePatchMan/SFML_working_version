import math
import os
from PIL import Image, ImageDraw

FRAME_W = 128
FRAME_H = 96
NUM_FRAMES = 8

# Palette based on Image 2 (Sachiel/Angel aesthetic: sleek pale lavender carapace, dark slate core, bone ribs):
CLR_TRANS = (0, 0, 0, 0)
CLR_OUTLINE = (22, 25, 34, 255)         # Crisp dark silhouette

# Sleek Pale Carapace / Shoulder Hood (Upper body, smooth & faceless):
CLR_CARAPACE_HI = (248, 250, 255, 255)  # Glossy rim light along sleek upper hood
CLR_CARAPACE_LIGHT = (222, 226, 238, 255) # Light pale lavender-tinted bone
CLR_CARAPACE_MID = (176, 182, 202, 255)   # Mid carapace
CLR_CARAPACE_SHADOW = (120, 128, 150, 255)# Deep carapace shadow

# Dark Sinewy Inner Body / Under-chest & flanks:
CLR_CORE_DARK = (28, 36, 46, 255)       # Deepest shadow
CLR_CORE_MID = (46, 58, 72, 255)        # Dark teal-slate muscle
CLR_CORE_LIGHT = (72, 88, 106, 255)     # Highlight on core

# Bone Ribs / Spines (Emerging from sides of chest):
CLR_RIB_LIGHT = (245, 246, 240, 255)    # Ivory bone
CLR_RIB_MID = (195, 198, 190, 255)      # Mid bone
CLR_RIB_DARK = (130, 135, 128, 255)     # Bone shadow

# Hands / Knuckles / Claws:
CLR_HAND_BASE = (176, 182, 202, 255)
CLR_CLAW = (245, 246, 240, 255)

def draw_angel_mutant_frame(draw, ox, frame_idx):
    """
    Renders 1 frame of the Angel-like Pale Mutant Boss:
    - Inspired directly by Image 2 (Sachiel aesthetic)
    - Side profile / 3/4 combat stance facing LEFT towards player
    - Sleek arched shoulder hood curving forward
    - WHERE NECK WOULD BE: Completely smooth, seamless organic surface
    - NO EYE, NO ANGEL CORE
    - EXTREMELY PINCHED, skinny hourglass waist with visible bone ribs
    - Long gorilla arms resting firmly on the floor
    - BOTH hands and feet firmly planted at ground_y = 95 with ZERO gap!
    """
    ground_y = 95  # Baseline floor contact level: exactly at bottom of hitbox

    # Frame-specific offsets
    cx = ox + 68
    cy = 51

    lean_x = 0
    bob_y = 0
    l_arm_fx = -34
    l_arm_fy = ground_y
    r_arm_fx = 24
    r_arm_fy = ground_y
    rear_up = False
    slam = False
    jumping = False
    hurt = False

    if frame_idx == 0:
        # Idle low: firmly grounded
        bob_y = 0
        l_arm_fx = -34
        l_arm_fy = ground_y
        r_arm_fx = 24
        r_arm_fy = ground_y
    elif frame_idx == 1:
        # Idle breathing: slight chest rise
        bob_y = -2
        l_arm_fx = -33
        l_arm_fy = ground_y
        r_arm_fx = 25
        r_arm_fy = ground_y
    elif frame_idx == 2:
        # Knuckle walk 1: Front (left) arm reaches forward-left
        bob_y = -1
        lean_x = -4
        l_arm_fx = -46
        l_arm_fy = ground_y - 12  # Reaching forward in stride
        r_arm_fx = 20
        r_arm_fy = ground_y       # Planted firmly on floor
    elif frame_idx == 3:
        # Knuckle walk 2: Rear (right) arm swings forward
        bob_y = -1
        lean_x = -1
        l_arm_fx = -36
        l_arm_fy = ground_y       # Planted firmly on floor
        r_arm_fx = 12
        r_arm_fy = ground_y - 10  # Swinging forward
    elif frame_idx == 4:
        # Attack windup: Rearing up on hind legs, arms raised high!
        rear_up = True
        bob_y = -14
        lean_x = 8
        l_arm_fx = -24
        l_arm_fy = cy - 24
        r_arm_fx = 28
        r_arm_fy = cy - 28
    elif frame_idx == 5:
        # Attack slam: Fists smashed into the floor in front!
        slam = True
        bob_y = 4
        lean_x = -8
        l_arm_fx = -50
        l_arm_fy = ground_y
        r_arm_fx = 14
        r_arm_fy = ground_y
    elif frame_idx == 6:
        # Jump slam: Coiled mid-air
        jumping = True
        bob_y = -24
        lean_x = -4
        l_arm_fx = -34
        l_arm_fy = cy + 18
        r_arm_fx = 18
        r_arm_fy = cy + 16
    elif frame_idx == 7:
        # Hurt / stunned: Flinched back
        hurt = True
        bob_y = -4
        lean_x = 10
        l_arm_fx = -20
        l_arm_fy = cy + 14
        r_arm_fx = 34
        r_arm_fy = cy + 6

    tx = cx + lean_x
    ty = cy + bob_y

    # ========================================================
    # 1. FAR (RIGHT) HIND LEG (Background leg)
    # ========================================================
    draw_hind_leg(draw, tx + 14, ty + 12, ground_y, is_near=False, rear_up=rear_up, jumping=jumping, step_fwd=frame_idx==3)

    # ========================================================
    # 2. FAR (RIGHT) GORILLA ARM (Background arm)
    # ========================================================
    draw_gorilla_arm(draw, tx + 18, ty - 8, tx + r_arm_fx, r_arm_fy, is_near=False, rear_up=rear_up, slam=slam, jumping=jumping)

    # ========================================================
    # 3. EXTREMELY PINCHED HOURGLASS WAIST & CORE (Image 2 style)
    # ========================================================
    draw_hourglass_waist(draw, tx, ty, rear_up=rear_up, slam=slam, hurt=hurt)

    # ========================================================
    # 4. NEAR (LEFT) HIND LEG (Foreground leg - firmly on ground_y!)
    # ========================================================
    draw_hind_leg(draw, tx - 2, ty + 14, ground_y, is_near=True, rear_up=rear_up, jumping=jumping, step_fwd=frame_idx==2)

    # ========================================================
    # 5. SLEEK PALE CARAPACE HOOD & CHEST (Headless, smooth surface)
    # ========================================================
    draw_sleek_hood_and_chest(draw, tx, ty, rear_up=rear_up, slam=slam, hurt=hurt)

    # ========================================================
    # 6. NEAR (LEFT) GORILLA ARM (Foreground arm - firmly on ground_y!)
    # ========================================================
    draw_gorilla_arm(draw, tx - 22, ty - 4, tx + l_arm_fx, l_arm_fy, is_near=True, rear_up=rear_up, slam=slam, jumping=jumping)

def draw_sleek_hood_and_chest(draw, tx, ty, rear_up, slam, hurt):
    """
    Draws the iconic sweeping aerodynamic carapace hood of Image 2:
    - Arches over the entire upper body from rear-right to front-left
    - Front slopes down smoothly into the chest
    - WHERE NECK WOULD BE: Completely smooth, seamless organic surface!
    - NO HEAD, NO NECK, NO HOLE, NO EYE.
    - Bone ribs emerging along the side of the chest.
    """
    # 1. Dark muscular under-chest (visible under the pale hood)
    under_chest = [
        (tx - 26, ty - 4),
        (tx + 12, ty - 4),
        (tx + 6, ty + 6),
        (tx - 8, ty + 6)
    ]
    draw.polygon(under_chest, fill=CLR_CORE_MID, outline=CLR_OUTLINE)
    draw.line([(tx - 24, ty - 3), (tx + 10, ty - 3)], fill=CLR_CORE_LIGHT, width=1)

    # 2. Sleek Sweeping Carapace Hood (Upper back & shoulders)
    # Sweeps from high rear-right peak (tx + 22, ty - 26) forward to front snout (tx - 34, ty - 12)
    hood_pts = [
        (tx - 34, ty - 12),  # Front-most prow of the sleek hood (forward-sloping beak)
        (tx - 26, ty - 22),  # Front curve
        (tx - 10, ty - 28),  # Mid top curve
        (tx + 12, ty - 28),  # Upper ridge
        (tx + 26, ty - 22),  # Rear high peak (angel dorsal fin)
        (tx + 22, ty - 8),   # Rear latissimus
        (tx + 8, ty - 2),    # Lower rear connect
        (tx - 12, ty - 2),   # Under chest connect
        (tx - 28, ty - 6)    # Front prow underside
    ]
    draw.polygon(hood_pts, fill=CLR_CARAPACE_MID, outline=CLR_OUTLINE)

    # Highlight facet along the top aerodynamic curve (smooth, glossy rim)
    draw.polygon([
        (tx - 32, ty - 13),
        (tx - 24, ty - 21),
        (tx - 9, ty - 27),
        (tx + 11, ty - 27),
        (tx + 24, ty - 21),
        (tx + 20, ty - 17),
        (tx + 8, ty - 24),
        (tx - 8, ty - 24),
        (tx - 22, ty - 18)
    ], fill=CLR_CARAPACE_LIGHT)
    draw.line([(tx - 30, ty - 14), (tx - 22, ty - 22), (tx - 8, ty - 27), (tx + 10, ty - 27), (tx + 22, ty - 22)], fill=CLR_CARAPACE_HI, width=2)

    # Underside shadow of the sleek hood
    draw.polygon([
        (tx - 33, ty - 11),
        (tx - 26, ty - 6),
        (tx - 10, ty - 3),
        (tx + 7, ty - 3),
        (tx + 20, ty - 7),
        (tx + 12, ty - 6),
        (tx - 10, ty - 5),
        (tx - 24, ty - 8)
    ], fill=CLR_CARAPACE_SHADOW)

    # WHERE NECK WOULD BE: A completely smooth, seamless surface!
    # A soft, elegant gradient curve blending into the chest with ZERO aperture, zero eye, zero core.
    draw.line([(tx - 22, ty - 10), (tx - 12, ty - 4)], fill=CLR_CARAPACE_LIGHT, width=2)
    draw.line([(tx - 20, ty - 9), (tx - 10, ty - 3)], fill=CLR_CARAPACE_MID, width=1)

    # 3. Bone Ribs / Spines (3 sleek white claws/ribs curving along the chest from Image 2!)
    rib_curves = [
        [(tx - 16, ty - 2), (tx - 8, ty + 1), (tx - 2, ty - 1)],   # Rib 1 (top)
        [(tx - 14, ty + 2), (tx - 6, ty + 5), (tx + 1, ty + 3)],   # Rib 2 (mid)
        [(tx - 12, ty + 6), (tx - 4, ty + 9), (tx + 3, ty + 7)]    # Rib 3 (lower)
    ]
    for pts in rib_curves:
        draw.line(pts, fill=CLR_OUTLINE, width=3)
        draw.line(pts, fill=CLR_RIB_LIGHT, width=2)
        # Tip of rib is sharp ivory
        draw.point(pts[-1], fill=CLR_CARAPACE_HI)

def draw_hourglass_waist(draw, tx, ty, rear_up, slam, hurt):
    """
    Extremely pinched, narrow hourglass waist from Image 2:
    - Red arrows in Image 2 explicitly emphasize the radical inward taper!
    - Waist narrows down to only ~8-10 pixels wide at the center
    - Flares smoothly into the pelvis and groin arch below
    - NO ANGEL CORE (user said: "рисовать ядро ангела ему не надо")
    """
    # Upper ribcage width: ~22px (tx - 11 to tx + 11 at ty + 6)
    # Narrow waist width: ~8px (tx - 4 to tx + 4 at ty + 12)
    # Lower hips/pelvis width: ~20px (tx - 10 to tx + 10 at ty + 20)

    waist_poly = [
        (tx - 11, ty + 6),   # Upper front
        (tx + 11, ty + 6),   # Upper rear
        (tx + 4, ty + 12),   # Pinched rear waist
        (tx + 10, ty + 20),  # Flared pelvis rear
        (tx - 10, ty + 20),  # Flared pelvis front
        (tx - 4, ty + 12)    # Pinched front waist
    ]
    draw.polygon(waist_poly, fill=CLR_CORE_DARK, outline=CLR_OUTLINE)

    # Pinched abdominal tendon highlight down the center (taut sinewy core)
    draw.polygon([
        (tx - 3, ty + 7),
        (tx + 3, ty + 7),
        (tx + 2, ty + 14),
        (tx - 2, ty + 14)
    ], fill=CLR_CORE_MID)
    draw.line([(tx, ty + 7), (tx, ty + 14)], fill=CLR_CORE_LIGHT, width=1)

    # Pelvic groin cowl / arch (matching the curved pale pelvic arch from Image 2!)
    pelvis_arch = [
        (tx - 9, ty + 18),
        (tx + 9, ty + 18),
        (tx + 6, ty + 23),
        (tx, ty + 24),
        (tx - 6, ty + 23)
    ]
    draw.polygon(pelvis_arch, fill=CLR_CARAPACE_MID, outline=CLR_OUTLINE)
    draw.line([(tx - 8, ty + 19), (tx, ty + 20), (tx + 8, ty + 19)], fill=CLR_CARAPACE_LIGHT, width=2)
    draw.line([(tx - 6, ty + 19), (tx, ty + 20)], fill=CLR_CARAPACE_HI, width=1)

def draw_gorilla_arm(draw, sx, sy, hx, hy, is_near, rear_up, slam, jumping):
    """
    Long, sleek gorilla arm:
    - Extends from wide shoulder hood down to the floor
    - Both hands rest firmly on ground_y = 95 (no gap!)
    - Articulated claws/knuckles touching the floor
    """
    c_out = CLR_OUTLINE
    c_hi = CLR_CARAPACE_HI if is_near else CLR_CARAPACE_LIGHT
    c_light = CLR_CARAPACE_LIGHT if is_near else CLR_CARAPACE_MID
    c_mid = CLR_CARAPACE_MID if is_near else CLR_CARAPACE_SHADOW
    c_shadow = CLR_CARAPACE_SHADOW if is_near else CLR_CORE_DARK

    # Elbow joint
    if rear_up:
        ex = (sx + hx) // 2 - (10 if is_near else -6)
        ey = min(sy, hy) - 8
    elif slam:
        ex = (sx + hx) // 2 - 12
        ey = (sy + hy) // 2 - 2
    elif jumping:
        ex = (sx + hx) // 2 - 12
        ey = (sy + hy) // 2
    else:
        # Natural gorilla knuckle walk forward bend
        ex = (sx + hx) // 2 - 8
        ey = (sy + hy) // 2 - 4

    # 1. Upper Arm (Sleek, muscular bicep)
    u_thick = 14 if is_near else 10
    draw.polygon([
        (sx - u_thick // 2, sy),
        (sx + u_thick // 2, sy),
        (ex + u_thick // 2 - 1, ey + 2),
        (ex - u_thick // 2 + 1, ey - 2)
    ], fill=c_mid, outline=c_out)
    draw.line([(sx - u_thick // 2 + 1, sy + 1), (ex - u_thick // 2 + 2, ey - 1)], fill=c_light, width=2)
    draw.line([(sx - u_thick // 2 + 2, sy + 1), (ex - u_thick // 2 + 3, ey - 1)], fill=c_hi, width=1)

    # 2. Forearm (Tapers into heavy knuckle wrist)
    f_thick = 16 if is_near else 12
    f_wrist = 18 if is_near else 13
    draw.polygon([
        (ex - f_thick // 2, ey - 2),
        (ex + f_thick // 2, ey + 2),
        (hx + f_wrist // 2, hy - 6),
        (hx - f_wrist // 2, hy - 10)
    ], fill=c_mid, outline=c_out)
    draw.line([(ex - f_thick // 2 + 1, ey - 1), (hx - f_wrist // 2 + 1, hy - 9)], fill=c_light, width=2)
    draw.line([(ex - f_thick // 2 + 2, ey - 1), (hx - f_wrist // 2 + 2, hy - 9)], fill=c_hi, width=1)
    draw.line([(ex + f_thick // 2 - 1, ey + 2), (hx + f_wrist // 2 - 1, hy - 5)], fill=c_shadow, width=2)

    # 3. Clawed Hand / Knuckles firmly resting on ground_y!
    hand_w = 20 if is_near else 15
    hand_h = 12 if is_near else 9

    # Hand base
    draw.rounded_rectangle([hx - hand_w // 2, hy - hand_h, hx + hand_w // 2, hy - 2], radius=2, fill=c_mid, outline=c_out)
    draw.line([(hx - hand_w // 2 + 1, hy - hand_h + 1), (hx + hand_w // 2 - 1, hy - hand_h + 1)], fill=c_light, width=1)

    # Articulated claws/knuckles touching floor exactly at hy!
    num_claws = 4 if is_near else 3
    spacing = hand_w // num_claws
    for k in range(num_claws):
        kx = hx - hand_w // 2 + 2 + k * spacing
        ky_base = hy - 4
        # Claw tip reaches ground level hy!
        draw.polygon([
            (kx - 1, ky_base),
            (kx + 2, ky_base),
            (kx + 1, hy),
            (kx - 1, hy)
        ], fill=c_light, outline=c_out)
        draw.point((kx, hy), fill=CLR_CLAW)

def draw_hind_leg(draw, hx, hy, ground_y, is_near, rear_up, jumping, step_fwd):
    """
    Digitigrade leg:
    - Feet rest FIRMLY on ground_y = 95 (same baseline as knuckles, ZERO gap!)
    """
    c_out = CLR_OUTLINE
    c_hi = CLR_CARAPACE_HI if is_near else CLR_CARAPACE_LIGHT
    c_light = CLR_CARAPACE_LIGHT if is_near else CLR_CARAPACE_MID
    c_mid = CLR_CARAPACE_MID if is_near else CLR_CARAPACE_SHADOW
    c_shadow = CLR_CARAPACE_SHADOW if is_near else CLR_CORE_DARK

    # Knee position
    kx = hx - (10 if not step_fwd else 16)
    ky = hy + 12

    # Foot position: fx, fy
    fx = hx + (2 if not step_fwd else -8)
    # ZERO GAP TO FLOOR: fy = ground_y (exactly 95!)
    fy = ground_y if not jumping else (hy + 26)

    # 1. Thigh
    draw.polygon([
        (hx - 5, hy - 2),
        (hx + 7, hy - 1),
        (kx + 6, ky + 2),
        (kx - 4, ky + 1)
    ], fill=c_mid, outline=c_out)
    draw.line([(hx - 3, hy), (kx - 2, ky)], fill=c_light, width=2)
    draw.line([(hx - 2, hy), (kx - 1, ky)], fill=c_hi, width=1)
    draw.line([(hx + 5, hy), (kx + 4, ky)], fill=c_shadow, width=2)

    # 2. Calf / Shin
    draw.polygon([
        (kx - 4, ky),
        (kx + 5, ky + 1),
        (fx + 5, fy - 4),
        (fx - 4, fy - 4)
    ], fill=c_mid, outline=c_out)
    draw.line([(kx - 2, ky + 1), (fx - 2, fy - 4)], fill=c_light, width=2)

    # 3. Clawed Foot resting solidly on ground_y (fy)!
    foot_w = 14 if is_near else 10
    draw.rectangle([fx - foot_w // 2, fy - 4, fx + foot_w // 2, fy], fill=c_mid, outline=c_out)
    draw.line([(fx - foot_w // 2 + 1, fy - 3), (fx + foot_w // 2 - 1, fy - 3)], fill=c_light, width=1)
    # Claws planted right on the ground line (fy)!
    draw.polygon([(fx - foot_w // 2, fy - 2), (fx - foot_w // 2 - 3, fy), (fx - foot_w // 2, fy)], fill=CLR_CLAW, outline=c_out)
    draw.polygon([(fx - 1, fy - 2), (fx - 4, fy), (fx - 1, fy)], fill=CLR_CLAW, outline=c_out)

def create_sprite_sheet():
    img = Image.new("RGBA", (FRAME_W * NUM_FRAMES, FRAME_H), CLR_TRANS)
    draw = ImageDraw.Draw(img)

    for f in range(NUM_FRAMES):
        ox = f * FRAME_W
        draw_angel_mutant_frame(draw, ox, f)

    img.save("assets/sprites/enemies/boss/boss_pale_mutant.png")
    print("Generated 8-frame angel-mutant boss_pale_mutant.png (1024x96)!")

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
