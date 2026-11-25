import os
import math
from PIL import Image, ImageDraw
from pixel_sculpt import PixelLayer, composite_layers

os.makedirs("assets/sprites/enemies/duelist_aggressive", exist_ok=True)
os.makedirs("assets/sprites/enemies/duelist_passive", exist_ok=True)
os.makedirs("assets/sprites/enemies/normal", exist_ok=True)
os.makedirs("assets/sprites/enemies/tank", exist_ok=True)
os.makedirs("assets/sprites/enemies/dummy", exist_ok=True)
os.makedirs("assets/sprites/enemies/lurker", exist_ok=True)

def draw_digitigrade_leg(layer, hip, knee, hock, foot, r_hip, r_knee, r_hock, r_foot, col_skin, col_claw):
    """
    Properly articulated digitigrade leg facing LEFT:
    Hip -> forward-down to Knee -> backward-down to Hock -> forward-down to Foot/Toes.
    """
    layer.tapered_limb([
        (hip[0], hip[1], r_hip),
        (knee[0], knee[1], r_knee),
        (hock[0], hock[1], r_hock),
        (foot[0], foot[1], r_foot),
    ], col_skin)
    # Clawed toes pointing forward (left)
    layer.tapered_limb([
        (foot[0], foot[1], r_foot),
        (foot[0] - 3, foot[1] + 0.5, 1.0),
        (foot[0] - 5, foot[1] + 1.0, 0.4),
    ], col_claw)

# =========================================================================
# 1. DUELIST AGGRESSIVE (Mantis Stalker: conical beak skull, bent 'P' scythes)
# Frame size: 48 x 64, 6 frames (0:Idle, 1:Walk1, 2:Walk2, 3:Windup, 4:Slash, 5:Hurt)
# =========================================================================
def generate_duelist_aggressive():
    W, H = 48, 64
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (14, 14, 20, 255)
    C_DEEP = (36, 36, 48, 255)
    C_SHADOW = (66, 68, 84, 255)
    C_MID = (128, 132, 148, 255)
    C_LIGHT = (182, 188, 204, 255)
    C_HI = (232, 238, 248, 255)
    C_VEIN = (110, 44, 72, 255)
    C_SINEW = (158, 42, 58, 255)
    C_SCYTHE = (224, 230, 240, 255)
    C_SCYTHE_SH = (138, 146, 162, 255)
    C_SCYTHE_HI = (252, 254, 255, 255)
    C_EYE = (255, 30, 68, 255)
    C_SLASH = (255, 70, 95, 190)
    C_SLASH_HI = (255, 220, 230, 230)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)
        fx = PixelLayer(W, H)

        ground_y = 60
        bob_y = 0
        lean_x = 0
        step = 0
        windup = (f == 3)
        slash = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -1
            step = 4
        elif f == 2:
            bob_y = -1
            step = -4
        elif windup:
            bob_y = -2
            lean_x = 3
        elif slash:
            bob_y = 3
            lean_x = -5
        elif hurt:
            bob_y = -1
            lean_x = 4

        cx = 25 + lean_x
        cy = 32 + bob_y

        # --- FAR LEG (consistent digitigrade: hip -> forward knee -> back hock -> forward toes) ---
        fl_hip = (cx + 3, cy + 8)
        fl_knee = (cx - 1 - step // 2, cy + 16)
        fl_hock = (cx + 6 - step // 2, cy + 22)
        fl_foot = (cx + 2 - step, ground_y)
        draw_digitigrade_leg(far, fl_hip, fl_knee, fl_hock, fl_foot, 2.8, 2.0, 1.4, 1.6, C_SHADOW, C_SCYTHE_SH)

        # --- FAR MANTIS SCYTHE ARM ---
        if windup:
            far.tapered_limb([(cx + 1, cy - 5), (cx + 5, cy - 16), (cx + 2, cy - 22)], C_SHADOW)
            far.tapered_limb([(cx + 2, cy - 22), (cx - 7, cy - 26), (cx - 14, cy - 22)], C_SCYTHE_SH)
        elif slash:
            far.tapered_limb([(cx, cy - 4), (cx - 11, cy - 1)], C_SHADOW)
            far.tapered_limb([(cx - 11, cy - 1), (cx - 19, cy + 3), (cx - 23, cy + 8)], C_SCYTHE_SH)
        else:
            far.tapered_limb([(cx + 1, cy - 5), (cx + 6, cy + 2)], C_SHADOW)
            far.tapered_limb([(cx + 6, cy + 2), (cx - 2, cy + 9), (cx - 9, cy + 16)], C_SCYTHE_SH)

        far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
        far.add_outline(C_OUT, ground_y + 1)

        # --- TORSO (Skeletal, arched S-curve predator) ---
        body.bezier_limb((cx + 3, cy + 8), (cx + 5, cy), (cx - 1, cy - 8), 3.6, 4.2, 3.8, C_MID)
        # Neck curving forward into skull
        body.tapered_limb([(cx - 1, cy - 8), (cx - 2, cy - 13)], C_MID)

        # --- HEAD (Smooth curved conical plague-beak skull per user sketch) ---
        # Cranium dome + sweeping beak down to sharp tip
        body.bezier_limb((cx + 4, cy - 16), (cx - 2, cy - 21), (cx - 10, cy - 15), 4.8, 4.5, 2.6, C_MID)
        body.tapered_limb([(cx - 8, cy - 15), (cx - 13, cy - 11), (cx - 17, cy - 7)], C_MID)
        body.set_px(cx - 18, cy - 6, C_MID)
        # Underside jaw line from beak tip back to neck
        body.polygon([(cx - 16, cy - 7), (cx - 5, cy - 11), (cx + 2, cy - 12), (cx + 4, cy - 15)], C_MID)

        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)

        # Anatomical details on body (ribs, veins, crimson eye slit)
        for ry in range(cy - 3, cy + 6, 3):
            body.line([(cx - 3, ry), (cx + 3, ry + 1)], C_DEEP)
            body.line([(cx - 3, ry - 1), (cx + 1, ry)], C_HI)
        body.line([(cx + 1, cy + 2), (cx + 3, cy + 7)], C_VEIN)
        # Eye socket & glowing slit
        body.polygon([(cx - 9, cy - 15), (cx - 5, cy - 16), (cx - 6, cy - 13)], C_DEEP)
        body.line([(cx - 8, cy - 15), (cx - 6, cy - 15)], C_EYE)
        body.set_px(cx - 7, cy - 15, (255, 190, 200, 255))
        body.add_outline(C_OUT, ground_y + 1)

        # --- NEAR LEG (Digitigrade, same natural bend!) ---
        nl_hip = (cx + 1, cy + 8)
        nl_knee = (cx - 4 + step // 2, cy + 16)
        nl_hock = (cx + 3 + step // 2, cy + 22)
        nl_foot = (cx - 1 + step, ground_y)
        draw_digitigrade_leg(near, nl_hip, nl_knee, nl_hock, nl_foot, 3.2, 2.2, 1.5, 1.8, C_MID, C_SCYTHE)

        # --- NEAR MANTIS SCYTHE ARM (Primary weapon, NO hands, thick curved scythe!) ---
        sh = (cx - 1, cy - 5)
        if windup:
            el = (cx + 3, cy - 16)
            near.tapered_limb([(sh[0], sh[1], 2.6), (el[0], el[1], 2.0)], C_MID)
            # Raised scythe poised high to cleave down
            near.bezier_limb((el[0], el[1]), (cx - 4, cy - 25), (cx - 15, cy - 22), 2.4, 2.0, 0.5, C_SCYTHE)
            # Elbow spur
            near.tapered_limb([(el[0], el[1], 1.5), (el[0] + 4, el[1] + 2, 0.5)], C_SCYTHE)
            # Inner barbs
            near.set_px(cx - 5, cy - 22, C_SCYTHE_HI)
            near.set_px(cx - 9, cy - 21, C_SCYTHE_HI)
        elif slash:
            el = (cx - 9, cy - 2)
            near.tapered_limb([(sh[0], sh[1], 2.6), (el[0], el[1], 2.0)], C_MID)
            # Huge forward cleaving scythe!
            near.bezier_limb((el[0], el[1]), (cx - 17, cy + 1), (cx - 21, cy + 11), 2.5, 2.2, 0.5, C_SCYTHE)
            near.set_px(cx - 14, cy + 3, C_SCYTHE_HI)
            near.set_px(cx - 17, cy + 6, C_SCYTHE_HI)
            # Slash trail FX
            fx.bezier_limb((cx - 12, cy - 14), (cx - 22, cy - 2), (cx - 18, cy + 13), 0.6, 1.6, 0.4, C_SLASH)
            fx.bezier_limb((cx - 14, cy - 11), (cx - 21, cy - 1), (cx - 19, cy + 10), 0.3, 0.8, 0.2, C_SLASH_HI)
        else:
            # Default folded 'P' mantis stance
            el = (cx + 4, cy + 2)
            near.tapered_limb([(sh[0], sh[1], 2.6), (el[0], el[1], 2.0)], C_MID)
            # Scythe curving forward-down from elbow
            near.bezier_limb((el[0], el[1]), (cx - 3, cy + 7), (cx - 13, cy + 17), 2.5, 2.0, 0.5, C_SCYTHE)
            # Elbow bone spur
            near.tapered_limb([(el[0], el[1], 1.4), (el[0] + 3, el[1] + 2, 0.4)], C_SCYTHE)
            # Serrated inner teeth
            near.set_px(cx - 2, cy + 9, C_SCYTHE_HI)
            near.set_px(cx - 6, cy + 13, C_SCYTHE_HI)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_SCYTHE, C_SCYTHE_HI, C_SCYTHE_HI, C_SCYTHE_SH, C_SHADOW)
        # Exposed sinew at shoulder/elbow
        near.line([(sh[0], sh[1]), (el[0] - 1, el[1])], C_SINEW)
        near.add_outline(C_OUT, ground_y + 1)

        frame_img = composite_layers(W, H, [far, body, near, fx])
        sheet.paste(frame_img, (f * W, 0))

    sheet.save("assets/sprites/enemies/duelist_aggressive/duelist_aggressive.png")
    print("Saved redesigned duelist_aggressive.png (288x64)")

# =========================================================================
# 2. DUELIST PASSIVE (Pale Wraith / Bone Surgeon - NO BEAK!)
# Frame size: 48 x 64, 6 frames (0:Idle, 1:Walk1, 2:Walk2, 3:Block/Windup, 4:LungeSlash, 5:Hurt)
# =========================================================================
def generate_duelist_passive():
    W, H = 48, 64
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (14, 18, 24, 255)
    C_DEEP = (32, 42, 52, 255)
    C_SHADOW = (64, 80, 94, 255)
    C_MID = (128, 146, 160, 255)
    C_LIGHT = (188, 208, 220, 255)
    C_HI = (238, 248, 255, 255)
    C_VEIN = (62, 82, 120, 255)
    C_BLADE = (225, 240, 248, 255)
    C_BLADE_SH = (138, 162, 178, 255)
    C_BLADE_HI = (255, 255, 255, 255)
    C_EYE = (50, 240, 255, 255)
    C_SLASH = (70, 230, 255, 200)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)
        fx = PixelLayer(W, H)

        ground_y = 60
        bob_y = 0
        lean_x = 0
        step = 0
        blocking = (f == 3)
        lunge = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -1
            step = 4
        elif f == 2:
            bob_y = -1
            step = -4
        elif blocking:
            bob_y = 1
            lean_x = 2
        elif lunge:
            bob_y = 3
            lean_x = -5
        elif hurt:
            bob_y = -1
            lean_x = 4

        cx = 25 + lean_x
        cy = 32 + bob_y

        # --- FAR LEG ---
        fl_hip = (cx + 2, cy + 8)
        fl_knee = (cx - 2 - step // 2, cy + 16)
        fl_hock = (cx + 5 - step // 2, cy + 22)
        fl_foot = (cx + 1 - step, ground_y)
        draw_digitigrade_leg(far, fl_hip, fl_knee, fl_hock, fl_foot, 2.5, 1.8, 1.3, 1.5, C_SHADOW, C_BLADE_SH)

        # Far arm
        if blocking:
            far.tapered_limb([(cx + 1, cy - 6), (cx - 5, cy - 2), (cx - 9, cy - 12)], C_SHADOW)
        elif lunge:
            far.tapered_limb([(cx + 1, cy - 6), (cx + 9, cy - 1), (cx + 15, cy + 3)], C_SHADOW)
        else:
            far.tapered_limb([(cx + 1, cy - 6), (cx + 4, cy + 2), (cx + 6, cy + 10)], C_SHADOW)
        far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
        far.add_outline(C_OUT, ground_y + 1)

        # --- TORSO (Tall, gaunt, eerie alien-cadaver posture) ---
        body.bezier_limb((cx + 2, cy + 8), (cx + 4, cy), (cx, cy - 9), 3.2, 4.0, 3.6, C_MID)
        # Slender curved neck
        body.tapered_limb([(cx, cy - 9), (cx - 1, cy - 14)], C_MID)

        # --- HEAD (NO BEAK! Smooth elongated alien skull with recessed jaw & cold cyan slit) ---
        body.bezier_limb((cx + 4, cy - 20), (cx - 1, cy - 21), (cx - 6, cy - 16), 4.2, 4.4, 3.6, C_MID)
        body.tapered_limb([(cx - 5, cy - 15), (cx - 4, cy - 11)], C_MID)
        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)

        # Carapace seam & veins
        body.line([(cx - 3, cy - 2), (cx + 3, cy - 1)], C_SHADOW)
        body.line([(cx - 2, cy + 3), (cx + 2, cy + 5)], C_VEIN)
        # Cold glowing cyan slit
        body.line([(cx - 7, cy - 16), (cx - 4, cy - 16)], C_EYE)
        body.set_px(cx - 6, cy - 16, C_BLADE_HI)
        body.add_outline(C_OUT, ground_y + 1)

        # --- NEAR LEG ---
        nl_hip = (cx, cy + 8)
        nl_knee = (cx - 5 + step // 2, cy + 16)
        nl_hock = (cx + 2 + step // 2, cy + 22)
        nl_foot = (cx - 2 + step, ground_y)
        draw_digitigrade_leg(near, nl_hip, nl_knee, nl_hock, nl_foot, 2.8, 2.0, 1.4, 1.6, C_MID, C_BLADE)

        # --- NEAR ARM & BONE STILETTO BLADES ---
        sh = (cx - 1, cy - 6)
        if blocking:
            # High vertical parry guard with crossed bone-blade!
            el = (cx - 5, cy + 1)
            near.tapered_limb([(sh[0], sh[1], 2.2), (el[0], el[1], 1.8)], C_MID)
            near.tapered_limb([(el[0], el[1], 1.8), (cx - 10, cy - 8, 1.4), (cx - 12, cy - 18, 0.4)], C_BLADE)
            near.tapered_limb([(cx - 8, cy - 5, 1.0), (cx - 15, cy - 12, 0.3)], C_BLADE)
        elif lunge:
            # Explosive horizontal fencing lunge!
            el = (cx - 10, cy - 4)
            near.tapered_limb([(sh[0], sh[1], 2.2), (el[0], el[1], 1.8)], C_MID)
            near.tapered_limb([(el[0], el[1], 1.8), (cx - 24, cy - 2, 0.4)], C_BLADE)
            near.tapered_limb([(el[0] - 4, el[1] + 1, 1.1), (cx - 21, cy + 3, 0.3)], C_BLADE)
            fx.line([(cx - 14, cy - 2), (cx - 26, cy - 2)], C_SLASH, width=2)
            fx.line([(cx - 18, cy - 2), (cx - 26, cy - 2)], C_BLADE_HI, width=1)
        else:
            # Poised low guard, multi-bladed wrist hanging ready
            el = (cx + 2, cy + 2)
            near.tapered_limb([(sh[0], sh[1], 2.2), (el[0], el[1], 1.8)], C_MID)
            near.tapered_limb([(el[0], el[1], 1.6), (cx - 5, cy + 9, 1.2), (cx - 12, cy + 15, 0.4)], C_BLADE)
            near.tapered_limb([(cx - 5, cy + 9, 0.9), (cx - 10, cy + 18, 0.3)], C_BLADE)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_BLADE, C_BLADE_HI, C_BLADE_HI, C_BLADE_SH, C_SHADOW)
        near.add_outline(C_OUT, ground_y + 1)

        sheet.paste(composite_layers(W, H, [far, body, near, fx]), (f * W, 0))

    sheet.save("assets/sprites/enemies/duelist_passive/duelist_passive.png")
    print("Saved redesigned duelist_passive.png (288x64)")

# =========================================================================
# 3. NORMAL MUTANT (Feral Hunched Ghoul - massive clawed attack arms)
# Frame size: 48 x 64, 6 frames (0:Idle, 1:Walk1, 2:Walk2, 3:Windup, 4:ClawSwipe, 5:Hurt)
# =========================================================================
def generate_normal_mutant():
    W, H = 48, 64
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (14, 18, 14, 255)
    C_DEEP = (32, 40, 32, 255)
    C_SHADOW = (64, 76, 66, 255)
    C_MID = (126, 142, 128, 255)
    C_LIGHT = (180, 196, 182, 255)
    C_HI = (232, 244, 234, 255)
    C_VEIN = (96, 46, 68, 255)
    C_SINEW = (148, 42, 52, 255)
    C_CLAW = (236, 230, 204, 255)
    C_CLAW_SH = (132, 122, 98, 255)
    C_CLAW_HI = (255, 252, 240, 255)
    C_EYE = (255, 215, 45, 255)
    C_SLASH = (240, 220, 180, 200)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)
        fx = PixelLayer(W, H)

        ground_y = 60
        bob_y = 0
        lean_x = 0
        step = 0
        windup = (f == 3)
        attack = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -1
            step = 4
        elif f == 2:
            bob_y = -1
            step = -4
        elif windup:
            bob_y = -2
            lean_x = 3
        elif attack:
            bob_y = 3
            lean_x = -5
        elif hurt:
            bob_y = -1
            lean_x = 4

        cx = 25 + lean_x
        cy = 34 + bob_y

        # --- FAR LEG (natural crouched predator leg) ---
        fl_hip = (cx + 4, cy + 7)
        fl_knee = (cx - 1 - step // 2, cy + 15)
        fl_hock = (cx + 6 - step // 2, cy + 21)
        fl_foot = (cx + 2 - step, ground_y)
        draw_digitigrade_leg(far, fl_hip, fl_knee, fl_hock, fl_foot, 3.0, 2.2, 1.5, 1.7, C_SHADOW, C_CLAW_SH)

        # Far Clawed Arm
        if windup:
            far.tapered_limb([(cx - 1, cy - 5), (cx + 4, cy - 15), (cx + 8, cy - 22)], C_SHADOW)
            far.tapered_limb([(cx + 8, cy - 22), (cx + 12, cy - 25)], C_CLAW_SH)
        elif attack:
            far.tapered_limb([(cx - 1, cy - 4), (cx - 12, cy - 2), (cx - 18, cy + 2)], C_SHADOW)
        else:
            far.tapered_limb([(cx - 1, cy - 5), (cx - 7, cy + 3), (cx - 11, cy + 12)], C_SHADOW)
        far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
        far.add_outline(C_OUT, ground_y + 1)

        # --- HUNCHED FERAL TORSO ---
        body.bezier_limb((cx + 5, cy + 7), (cx + 6, cy - 4), (cx - 3, cy - 9), 4.0, 5.0, 4.4, C_MID)
        # Neck thrusting forward
        body.tapered_limb([(cx - 3, cy - 9), (cx - 7, cy - 11)], C_MID)

        # --- GAUNT GHOUL HEAD (sloping skull + hanging unhinged jaw) ---
        body.bezier_limb((cx - 2, cy - 14), (cx - 8, cy - 16), (cx - 13, cy - 11), 4.2, 3.6, 2.2, C_MID)
        # Lower jaw (opens wide on windup/attack)
        jaw_drop = 3 if (windup or attack) else 1
        body.tapered_limb([(cx - 5, cy - 9), (cx - 10, cy - 7 + jaw_drop), (cx - 14, cy - 7 + jaw_drop)], C_MID)

        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)

        # Spine knobs & ribs & veins
        for s in range(4):
            sx = cx + 4 - s * 2
            sy = cy - 6 + s * 3
            body.set_px(sx + 2, sy, C_CLAW)
        body.line([(cx - 2, cy - 2), (cx + 3, cy)], C_DEEP)
        body.line([(cx - 1, cy + 2), (cx + 3, cy + 4)], C_VEIN)
        # Mouth cavity + jagged fangs
        body.polygon([(cx - 13, cy - 10), (cx - 8, cy - 10), (cx - 8, cy - 7 + jaw_drop), (cx - 13, cy - 7 + jaw_drop)], C_DEEP)
        for tx in (cx - 13, cx - 11, cx - 9):
            body.set_px(tx, cy - 10, C_CLAW_HI)
            body.set_px(tx, cy - 7 + jaw_drop, C_CLAW_HI)
        # Feverish eye in sunken socket
        body.disc(cx - 8, cy - 13, 1.3, C_DEEP)
        body.set_px(cx - 9, cy - 13, C_EYE)
        body.add_outline(C_OUT, ground_y + 1)

        # --- NEAR LEG ---
        nl_hip = (cx + 2, cy + 7)
        nl_knee = (cx - 4 + step // 2, cy + 15)
        nl_hock = (cx + 3 + step // 2, cy + 21)
        nl_foot = (cx - 1 + step, ground_y)
        draw_digitigrade_leg(near, nl_hip, nl_knee, nl_hock, nl_foot, 3.2, 2.3, 1.6, 1.8, C_MID, C_CLAW)

        # --- MASSIVE CLAWED ATTACK ARM (Primary weapon!) ---
        sh = (cx - 2, cy - 6)
        if windup:
            el = (cx + 2, cy - 14)
            wrist = (cx - 2, cy - 22)
            near.tapered_limb([(sh[0], sh[1], 3.0), (el[0], el[1], 2.5), (wrist[0], wrist[1], 2.4)], C_MID)
            # 3 splayed talons reared overhead
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] - 6, wrist[1] - 5, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] - 2, wrist[1] - 7, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] + 3, wrist[1] - 6, 0.4)], C_CLAW)
        elif attack:
            el = (cx - 10, cy - 4)
            wrist = (cx - 17, cy + 1)
            near.tapered_limb([(sh[0], sh[1], 3.2), (el[0], el[1], 2.6), (wrist[0], wrist[1], 2.6)], C_MID)
            # Savage forward claw rake!
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] - 7, wrist[1] - 4, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] - 8, wrist[1] + 0, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 2.0), (wrist[0] - 7, wrist[1] + 5, 0.4)], C_CLAW)
            # Slash arc
            fx.bezier_limb((cx - 14, cy - 12), (cx - 23, cy), (cx - 19, cy + 11), 0.5, 1.5, 0.4, C_SLASH)
        else:
            el = (cx - 5, cy + 3)
            wrist = (cx - 11, cy + 11)
            near.tapered_limb([(sh[0], sh[1], 3.0), (el[0], el[1], 2.4), (wrist[0], wrist[1], 2.3)], C_MID)
            # Hanging gnarled claws
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 4, wrist[1] + 5, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 1, wrist[1] + 7, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] + 2, wrist[1] + 6, 0.4)], C_CLAW)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_CLAW, C_CLAW_HI, C_CLAW_HI, C_CLAW_SH, C_SHADOW)
        near.line([(sh[0], sh[1] + 1), (el[0], el[1])], C_SINEW)
        near.add_outline(C_OUT, ground_y + 1)

        sheet.paste(composite_layers(W, H, [far, body, near, fx]), (f * W, 0))

    sheet.save("assets/sprites/enemies/normal/normal_mutant.png")
    print("Saved redesigned normal_mutant.png (288x64)")

# =========================================================================
# 4. TANK MUTANT (72x96, Hulking Gorilla-like Flesh Behemoth, NOT a fridge!)
# Frame size: 72 x 96, 6 frames (0:Idle, 1:Walk1, 2:Walk2, 3:Windup, 4:Smash, 5:Hurt)
# =========================================================================
def generate_tank_mutant():
    W, H = 72, 96
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (16, 14, 20, 255)
    C_DEEP = (38, 34, 44, 255)
    C_SHADOW = (74, 70, 82, 255)
    C_MID = (138, 134, 146, 255)
    C_LIGHT = (190, 186, 198, 255)
    C_HI = (238, 236, 246, 255)
    C_VEIN = (112, 42, 62, 255)
    C_BONE = (234, 226, 206, 255)
    C_BONE_SH = (134, 126, 112, 255)
    C_BONE_HI = (255, 252, 242, 255)
    C_EYE = (255, 95, 30, 255)
    C_IMPACT = (240, 210, 170, 200)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)
        fx = PixelLayer(W, H)

        ground_y = 92
        bob_y = 0
        lean_x = 0
        step = 0
        windup = (f == 3)
        smash = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -2
            step = 5
        elif f == 2:
            bob_y = -2
            step = -5
        elif windup:
            bob_y = -4
            lean_x = 4
        elif smash:
            bob_y = 6
            lean_x = -6
        elif hurt:
            bob_y = -2
            lean_x = 5

        cx = 38 + lean_x
        cy = 48 + bob_y

        # --- FAR LEG (Thick plantigrade/digitigrade pillar) ---
        far.tapered_limb([
            (cx + 8, cy + 16, 7.0),
            (cx + 4 - step // 2, cy + 28, 5.5),
            (cx + 10 - step // 2, cy + 36, 4.5),
            (cx + 7 - step, ground_y - 2, 5.0),
        ], C_SHADOW)
        far.tapered_limb([(cx + 6 - step, ground_y - 1, 3.0), (cx + 1 - step, ground_y, 1.2)], C_BONE_SH)

        # Far Massive Club Arm
        if windup:
            far.tapered_limb([(cx + 2, cy - 12, 6.0), (cx + 6, cy - 26, 5.5), (cx + 2, cy - 36, 6.5)], C_SHADOW)
        elif smash:
            far.tapered_limb([(cx - 4, cy - 8, 6.0), (cx - 14, cy + 12, 6.0), (cx - 18, ground_y - 4, 7.0)], C_SHADOW)
        else:
            far.tapered_limb([(cx + 2, cy - 10, 5.5), (cx - 8, cy + 8, 5.5), (cx - 12, cy + 24, 6.5)], C_SHADOW)
        far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
        far.add_outline(C_OUT, ground_y + 1)

        # --- COLOSSAL HUNCHED BEHEMOTH TORSO (Sloping shoulders towering over sunken head!) ---
        # Pelvis -> Huge hunched upper back -> Massive forward traps
        body.bezier_limb((cx + 8, cy + 16), (cx + 14, cy - 2), (cx - 2, cy - 18), 10.0, 14.5, 13.0, C_MID)
        # Bulging chest forward
        body.disc(cx - 8, cy - 6, 9.5, C_MID)
        # Asymmetrical jagged bone osteoderms/ridges along spine/back (NOT 3 birthday candles!)
        body.tapered_limb([(cx + 4, cy - 24, 3.2), (cx + 7, cy - 33, 0.8)], C_BONE)
        body.tapered_limb([(cx + 11, cy - 18, 2.8), (cx + 16, cy - 25, 0.7)], C_BONE)
        body.tapered_limb([(cx - 3, cy - 26, 2.5), (cx - 2, cy - 32, 0.7)], C_BONE)

        # --- SUNKEN BRUTISH HEAD (tucked low in front of massive shoulders) ---
        body.tapered_limb([(cx - 8, cy - 14, 7.0), (cx - 16, cy - 11, 6.2)], C_MID)
        # Heavy protruding brow & underbite jaw
        body.tapered_limb([(cx - 13, cy - 7, 4.0), (cx - 19, cy - 6, 3.0)], C_MID)

        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        body.auto_shade(C_BONE, C_BONE_HI, C_BONE_HI, C_BONE_SH, C_SHADOW)

        # Bursting calcified ribs & thick necrotic veins
        for r in range(3):
            ry = cy - 4 + r * 6
            body.tapered_limb([(cx - 11, ry, 1.3), (cx - 2, ry + 2, 0.7)], C_BONE)
        body.line([(cx - 6, cy + 6), (cx + 8, cy + 10)], C_VEIN, width=2)
        body.line([(cx + 2, cy - 8), (cx + 10, cy + 2)], C_VEIN, width=2)

        # Sunken eye & jagged underbite fangs
        body.disc(cx - 16, cy - 12, 2.0, C_DEEP)
        body.set_px(cx - 17, cy - 12, C_EYE)
        body.set_px(cx - 16, cy - 12, (255, 190, 90, 255))
        body.set_px(cx - 18, cy - 8, C_BONE_HI)
        body.set_px(cx - 16, cy - 8, C_BONE_HI)
        body.add_outline(C_OUT, ground_y + 1)

        # --- NEAR LEG ---
        near.tapered_limb([
            (cx + 4, cy + 16, 7.5),
            (cx - 2 + step // 2, cy + 28, 5.8),
            (cx + 5 + step // 2, cy + 36, 4.8),
            (cx + 1 + step, ground_y - 2, 5.5),
        ], C_MID)
        near.tapered_limb([(cx + 1 + step, ground_y - 1, 3.2), (cx - 5 + step, ground_y, 1.2)], C_BONE)

        # --- TITANIC NEAR CLUB-ARM & SPIKED BONE FIST ---
        sh = (cx - 5, cy - 10)
        if windup:
            el = (cx - 10, cy - 20)
            fist = (cx - 6, cy - 31)
            near.tapered_limb([(sh[0], sh[1], 7.0), (el[0], el[1], 6.0), (fist[0], fist[1], 7.0)], C_MID)
            # Spiked bone knuckle armor on raised fist
            near.tapered_limb([(fist[0] - 5, fist[1] - 3, 2.8), (fist[0] + 5, fist[1] - 3, 2.8)], C_BONE)
            near.tapered_limb([(fist[0] - 4, fist[1] - 4, 1.8), (fist[0] - 7, fist[1] - 8, 0.5)], C_BONE)
            near.tapered_limb([(fist[0] + 2, fist[1] - 4, 1.8), (fist[0] + 3, fist[1] - 9, 0.5)], C_BONE)
        elif smash:
            el = (cx - 16, cy + 4)
            fist = (cx - 24, ground_y - 5)
            near.tapered_limb([(sh[0], sh[1], 7.2), (el[0], el[1], 6.2), (fist[0], fist[1], 7.8)], C_MID)
            # Bone knuckle plate slamming ground!
            near.tapered_limb([(fist[0] - 5, fist[1] + 2, 3.2), (fist[0] + 4, fist[1] + 2, 3.2)], C_BONE)
            near.tapered_limb([(fist[0] - 5, fist[1], 2.0), (fist[0] - 9, fist[1] - 3, 0.5)], C_BONE)
            near.tapered_limb([(fist[0] - 1, fist[1] - 3, 2.0), (fist[0] - 2, fist[1] - 7, 0.5)], C_BONE)
            # Impact shockwave pixels
            fx.line([(fist[0] - 10, ground_y), (fist[0] - 2, ground_y - 4)], C_IMPACT, width=2)
            fx.line([(fist[0] + 5, ground_y), (fist[0] + 10, ground_y - 5)], C_IMPACT, width=2)
        else:
            el = (cx - 12, cy + 6)
            fist = (cx - 17, cy + 23)
            near.tapered_limb([(sh[0], sh[1], 7.0), (el[0], el[1], 6.0), (fist[0], fist[1], 7.2)], C_MID)
            # Elbow bone spur
            near.tapered_limb([(el[0] + 3, el[1] - 1, 2.0), (el[0] + 8, el[1] - 3, 0.5)], C_BONE)
            # Heavy bone knuckles & claws
            near.tapered_limb([(fist[0] - 4, fist[1] + 3, 2.8), (fist[0] + 4, fist[1] + 3, 2.8)], C_BONE)
            near.tapered_limb([(fist[0] - 4, fist[1] + 4, 1.5), (fist[0] - 6, fist[1] + 9, 0.4)], C_BONE)
            near.tapered_limb([(fist[0] + 1, fist[1] + 4, 1.5), (fist[0] + 1, fist[1] + 9, 0.4)], C_BONE)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_BONE, C_BONE_HI, C_BONE_HI, C_BONE_SH, C_SHADOW)
        near.line([(sh[0], sh[1] + 2), (el[0], el[1])], C_VEIN, width=2)
        near.add_outline(C_OUT, ground_y + 1)

        sheet.paste(composite_layers(W, H, [far, body, near, fx]), (f * W, 0))

    sheet.save("assets/sprites/enemies/tank/tank_mutant.png")
    print("Saved redesigned tank_mutant.png (432x96)")

# =========================================================================
# 5. DUMMY MUTANT (Stitched Bio-Horror Totem - NO traffic cone / clown nose!)
# Frame size: 48 x 64, 6 frames (0:Idle1, 1:Idle2, 2:HighBlock, 3:LowBlock, 4:Hurt1, 5:Hurt2)
# =========================================================================
def generate_dummy_mutant():
    W, H = 48, 64
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (16, 16, 22, 255)
    C_DEEP = (38, 36, 46, 255)
    C_SHADOW = (72, 72, 84, 255)
    C_MID = (132, 134, 146, 255)
    C_LIGHT = (186, 190, 202, 255)
    C_HI = (236, 240, 248, 255)
    C_VEIN = (104, 44, 66, 255)
    C_CORE = (176, 32, 46, 255)
    C_CORE_HI = (242, 108, 108, 255)
    C_BONE = (228, 222, 206, 255)
    C_BONE_SH = (128, 122, 108, 255)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)

        ground_y = 60
        wobble_x = 0
        bob_y = 0
        if f == 1:
            bob_y = 1
        elif f == 4:
            wobble_x = -3
        elif f == 5:
            wobble_x = 3

        cx = 24 + wobble_x
        cy = 33 + bob_y

        # --- TWISTED BONE & FLESH ROOT PEDESTAL ---
        far.tapered_limb([(cx + 2, cy + 12, 3.5), (cx + 7, ground_y - 1, 2.2)], C_SHADOW)
        far.tapered_limb([(cx - 2, cy + 12, 3.5), (cx - 7, ground_y - 1, 2.2)], C_SHADOW)
        far.add_outline(C_OUT, ground_y + 1)

        # --- IMPALED / STITCHED TORSO & SLACK HEAD ---
        body.tapered_limb([(cx, ground_y - 2, 2.4), (cx, cy + 12, 3.5)], C_BONE)  # Central spine stake
        body.bezier_limb((cx, cy + 13), (cx + 1, cy + 2), (cx - 1, cy - 9), 4.5, 7.0, 6.2, C_MID)
        # Slack tilted stitched head
        body.tapered_limb([(cx - 1, cy - 10, 3.0), (cx - 3, cy - 17, 4.8)], C_MID)
        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        body.auto_shade(C_BONE, C_HI, C_HI, C_BONE_SH, C_SHADOW)

        # Exposed thoracic cavity with visceral pulsing organ + rib cage bars
        body.disc(cx - 2, cy - 1, 3.6, C_DEEP)
        body.disc(cx - 2, cy - 1, 2.4, C_CORE)
        body.set_px(cx - 3, cy - 2, C_CORE_HI)
        # Ribs framing cavity
        body.line([(cx - 6, cy - 3), (cx - 3, cy - 2)], C_BONE)
        body.line([(cx - 6, cy + 1), (cx - 3, cy + 1)], C_BONE)
        # Autopsy stitches on torso & sewn-shut eyes
        body.line([(cx + 3, cy - 7), (cx + 3, cy + 9)], C_DEEP)
        for sy in range(cy - 5, cy + 8, 4):
            body.line([(cx + 1, sy), (cx + 5, sy)], C_BONE)
        body.line([(cx - 6, cy - 18), (cx - 3, cy - 17)], C_DEEP)  # Sewn eye
        body.set_px(cx - 5, cy - 19, C_BONE)
        body.set_px(cx - 4, cy - 16, C_BONE)
        body.add_outline(C_OUT, ground_y + 1)

        # --- GRAFTED MUTANT BLOCKING ARMS ---
        if f == 2:  # High Block
            near.tapered_limb([(cx + 2, cy - 6, 2.6), (cx - 5, cy - 10, 2.4), (cx - 9, cy - 19, 3.0)], C_MID)
            near.tapered_limb([(cx - 9, cy - 21, 2.5), (cx - 9, cy - 11, 2.5)], C_BONE)
        elif f == 3:  # Low Block
            near.tapered_limb([(cx + 2, cy - 4, 2.6), (cx - 6, cy + 3, 2.4), (cx - 10, cy + 10, 3.0)], C_MID)
            near.tapered_limb([(cx - 10, cy + 2, 2.5), (cx - 10, cy + 13, 2.5)], C_BONE)
        else:
            near.tapered_limb([(cx + 3, cy - 6, 2.4), (cx + 6, cy + 3, 2.0), (cx + 3, cy + 11, 1.8)], C_MID)
            near.tapered_limb([(cx + 3, cy + 11, 1.4), (cx, cy + 15, 0.4)], C_BONE)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_BONE, C_HI, C_HI, C_BONE_SH, C_SHADOW)
        near.add_outline(C_OUT, ground_y + 1)

        sheet.paste(composite_layers(W, H, [far, body, near]), (f * W, 0))

    sheet.save("assets/sprites/enemies/dummy/dummy_mutant.png")
    print("Saved redesigned dummy_mutant.png (288x64)")

# =========================================================================
# 6. LURKER MUTANT (Low-slung Stalker/Crawler - fixes opposite legs, face sticks & missing attack!)
# Frame size: 48 x 64, 6 frames (0:Idle, 1:Walk1, 2:Walk2, 3:Windup, 4:PounceSlash, 5:Hurt)
# =========================================================================
def generate_lurker_mutant():
    W, H = 48, 64
    NFRAMES = 6
    sheet = Image.new("RGBA", (W * NFRAMES, H), (0, 0, 0, 0))

    C_OUT = (14, 16, 22, 255)
    C_DEEP = (34, 38, 46, 255)
    C_SHADOW = (68, 74, 86, 255)
    C_MID = (128, 134, 146, 255)
    C_LIGHT = (184, 192, 206, 255)
    C_HI = (236, 244, 254, 255)
    C_VEIN = (98, 46, 68, 255)
    C_CLAW = (238, 234, 214, 255)
    C_CLAW_SH = (132, 128, 114, 255)
    C_CLAW_HI = (255, 252, 242, 255)
    C_EYE = (255, 40, 35, 255)
    C_SLASH = (255, 80, 60, 200)

    for f in range(NFRAMES):
        far = PixelLayer(W, H)
        body = PixelLayer(W, H)
        near = PixelLayer(W, H)
        fx = PixelLayer(W, H)

        ground_y = 60
        bob_y = 0
        lean_x = 0
        step = 0
        windup = (f == 3)
        attack = (f == 4)
        hurt = (f == 5)

        if f == 1:
            bob_y = -1
            step = 4
        elif f == 2:
            bob_y = -1
            step = -4
        elif windup:
            bob_y = 2
            lean_x = 4
        elif attack:
            bob_y = -1
            lean_x = -5
        elif hurt:
            bob_y = -2
            lean_x = 4

        cx = 26 + lean_x
        cy = 39 + bob_y

        # --- FAR HIND LEG (SAME anatomical direction as near leg: knee forward, hock back, foot forward!) ---
        fl_hip = (cx + 7, cy + 4)
        fl_knee = (cx + 2 - step // 2, cy + 11)
        fl_hock = (cx + 9 - step // 2, cy + 16)
        fl_foot = (cx + 5 - step, ground_y)
        draw_digitigrade_leg(far, fl_hip, fl_knee, fl_hock, fl_foot, 2.8, 2.0, 1.4, 1.6, C_SHADOW, C_CLAW_SH)

        # Far Grasping Claw Arm
        if windup:
            far.tapered_limb([(cx - 4, cy - 2, 2.2), (cx + 2, cy - 10, 1.8), (cx - 2, cy - 16, 1.8)], C_SHADOW)
        elif attack:
            far.tapered_limb([(cx - 6, cy - 2, 2.2), (cx - 15, cy - 4, 1.8), (cx - 21, cy + 1, 1.8)], C_SHADOW)
        else:
            far.tapered_limb([(cx - 5, cy - 1, 2.2), (cx - 11, cy + 6, 1.8), (cx - 15, cy + 13, 1.8)], C_SHADOW)
        far.auto_shade(C_SHADOW, C_MID, C_MID, C_DEEP, C_DEEP)
        far.add_outline(C_OUT, ground_y + 1)

        # --- LOW-SLUNG HORIZONTAL STALKER TORSO ---
        body.bezier_limb((cx + 8, cy + 4), (cx + 2, cy - 4), (cx - 7, cy - 3), 4.4, 5.2, 4.4, C_MID)

        # --- PREDATOR SKULL (Elongated snout + jagged lower jaw, NO silly sticks on forehead!) ---
        body.bezier_limb((cx - 5, cy - 5), (cx - 11, cy - 7), (cx - 17, cy - 3), 4.0, 3.5, 2.0, C_MID)
        jaw_open = 3 if (windup or attack) else 1
        body.tapered_limb([(cx - 7, cy - 1, 2.2), (cx - 14, cy + 1 + jaw_open, 1.4)], C_MID)

        body.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)

        # Spine ridges & veins
        for s in range(3):
            body.set_px(cx + 4 - s * 4, cy - 9 + s, C_CLAW)
        body.line([(cx - 1, cy), (cx + 4, cy + 2)], C_VEIN)
        # Mouth + fangs
        body.polygon([(cx - 15, cy - 2), (cx - 10, cy - 2), (cx - 10, cy + jaw_open), (cx - 14, cy + jaw_open)], C_DEEP)
        body.set_px(cx - 15, cy - 2, C_CLAW_HI)
        body.set_px(cx - 13, cy - 2, C_CLAW_HI)
        # 3 clustered crimson eyes on skull side
        body.set_px(cx - 11, cy - 5, C_EYE)
        body.set_px(cx - 13, cy - 5, C_EYE)
        body.set_px(cx - 12, cy - 3, C_EYE)
        body.add_outline(C_OUT, ground_y + 1)

        # --- NEAR HIND LEG (Proper digitigrade bend!) ---
        nl_hip = (cx + 5, cy + 4)
        nl_knee = (cx - 1 + step // 2, cy + 11)
        nl_hock = (cx + 6 + step // 2, cy + 16)
        nl_foot = (cx + 2 + step, ground_y)
        draw_digitigrade_leg(near, nl_hip, nl_knee, nl_hock, nl_foot, 3.2, 2.2, 1.5, 1.7, C_MID, C_CLAW)

        # --- NEAR PREDATORY CLAWED ARM (Clearly attacks!) ---
        sh = (cx - 5, cy - 1)
        if windup:
            # Coiled back, claws raised high ready to pounce-slash!
            el = (cx, cy - 9)
            wrist = (cx - 5, cy - 17)
            near.tapered_limb([(sh[0], sh[1], 2.8), (el[0], el[1], 2.2), (wrist[0], wrist[1], 2.2)], C_MID)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 6, wrist[1] - 4, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 3, wrist[1] - 6, 0.4)], C_CLAW)
        elif attack:
            # Fierce forward pounce-slash!
            el = (cx - 12, cy - 4)
            wrist = (cx - 19, cy + 1)
            near.tapered_limb([(sh[0], sh[1], 3.0), (el[0], el[1], 2.4), (wrist[0], wrist[1], 2.4)], C_MID)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 7, wrist[1] - 4, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 8, wrist[1] + 1, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.8), (wrist[0] - 6, wrist[1] + 5, 0.4)], C_CLAW)
            fx.bezier_limb((cx - 14, cy - 10), (cx - 24, cy), (cx - 19, cy + 10), 0.5, 1.4, 0.4, C_SLASH)
        else:
            # Stalking crouch: elbow bent back, hooked claws poised low in front
            el = (cx - 2, cy + 5)
            wrist = (cx - 10, cy + 12)
            near.tapered_limb([(sh[0], sh[1], 2.8), (el[0], el[1], 2.2), (wrist[0], wrist[1], 2.0)], C_MID)
            near.tapered_limb([(wrist[0], wrist[1], 1.6), (wrist[0] - 5, wrist[1] + 4, 0.4)], C_CLAW)
            near.tapered_limb([(wrist[0], wrist[1], 1.6), (wrist[0] - 2, wrist[1] + 6, 0.4)], C_CLAW)

        near.auto_shade(C_MID, C_LIGHT, C_HI, C_SHADOW, C_DEEP)
        near.auto_shade(C_CLAW, C_CLAW_HI, C_CLAW_HI, C_CLAW_SH, C_SHADOW)
        near.add_outline(C_OUT, ground_y + 1)

        sheet.paste(composite_layers(W, H, [far, body, near, fx]), (f * W, 0))

    sheet.save("assets/sprites/enemies/lurker/lurker_mutant.png")
    print("Saved redesigned lurker_mutant.png (288x64)")

generate_duelist_aggressive()
generate_duelist_passive()
generate_normal_mutant()
generate_tank_mutant()
generate_dummy_mutant()
generate_lurker_mutant()

# Regenerate all tile icons and canvas previews from frame 0
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
        icon32.save(os.path.join(dir_path, f'{name}.png'))
    icon32.save(prev_path)

print("All 6 mutant spritesheets & icons regenerated!")
