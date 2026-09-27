"""Personajes: hojas de sprites 16x24 (3 frames x 4 direcciones) y retratos 48x48."""
from artlib import *

FW, FH = 16, 24
DIRS = ["down", "left", "right", "up"]

TEAL = (52, 118, 128)
TEAL_D = (34, 82, 94)
LEATHER = (110, 72, 48)
LEATHER_D = (78, 50, 36)
GREY = (120, 118, 124)
GREY_D = (84, 82, 92)

SPECS = {
    "player": dict(hair=(110, 70, 42), hair_d=(78, 48, 30), style="spiky", top=TEAL, top_d=TEAL_D,
                   pants=(96, 70, 52), pants_d=(70, 50, 38), boots=LEATHER_D, belt=LEATHER),
    "kaelen": dict(hair=(44, 46, 74), hair_d=(28, 28, 50), style="swept", top=(116, 104, 96), top_d=(84, 74, 70),
                   pants=(58, 56, 72), pants_d=(40, 38, 52), boots=(50, 36, 30), scarf=CLOTH_R, scarf_d=CLOTH_R_D,
                   belt=LEATHER_D),
    "yara": dict(hair=(190, 84, 50), hair_d=(140, 56, 36), style="long", top=CREAM, top_d=CREAM_S,
                 dress=True, sash=CLOTH_G, boots=(120, 90, 60)),
    "mother": dict(hair=(96, 60, 40), hair_d=(66, 40, 28), style="bun", top=(138, 58, 60), top_d=(100, 40, 46),
                   dress=True, apron=CREAM, boots=(70, 50, 40)),
    "bartolo": dict(hair=(208, 204, 196), hair_d=(160, 156, 150), style="bald", beard=(226, 222, 214),
                    top=(112, 88, 70), top_d=(82, 62, 50), dress=True, boots=(60, 44, 36), skin=(226, 176, 140)),
    "roc": dict(hair=(60, 40, 30), hair_d=(40, 26, 20), style="bald", beard=(70, 44, 30), top=(150, 140, 128),
                top_d=(112, 104, 96), pants=(70, 60, 56), pants_d=(50, 42, 40), boots=(44, 34, 30), apron=LEATHER),
    "nil": dict(hair=(200, 150, 70), hair_d=(160, 110, 46), style="spiky", top=CLOTH_R, top_d=CLOTH_R_D,
                pants=(80, 90, 120), pants_d=(56, 64, 90), boots=LEATHER_D, kid=True),
    "remei": dict(hair=(186, 170, 140), hair_d=(150, 134, 108), style="scarf", scarf=(90, 120, 150),
                  scarf_d=(64, 88, 116), top=CLOTH_G, top_d=CLOTH_G_D, dress=True, boots=(70, 50, 40)),
}


def draw_frame(spec, direction, frame):
    c = Canvas(FW, FH)
    skin = spec.get("skin", SKIN)
    skin_s = darken(skin, 0.18)
    hair, hair_d = spec["hair"], spec["hair_d"]
    top, top_d = spec["top"], spec["top_d"]
    dress = spec.get("dress", False)
    kid = spec.get("kid", False)
    oy = 3 if kid else 0
    step = [0, 1, -1][frame]  # 0 quieto, 1 pierna A, -1 pierna B
    side = direction in ("left", "right")

    # --- Piernas ---
    legs_top = 18 + oy
    if dress:
        # falda hasta y=21, pies asomando
        skirt = spec.get("top", CREAM) if "sash" in spec else top
        for y in range(15 + oy, 22):
            half = 4 + (y - 15 - oy) // 3
            x0 = 8 - half
            for x in range(x0, 8 + half):
                col = skirt if x > x0 else darken(skirt, 0.15)
                if y == 21:
                    col = darken(skirt, 0.2)
                c.px(x, y, col)
        boots = spec["boots"]
        if side:
            c.rect(7 + step, 22, 3, 1, boots)
            c.rect(6 - step, 22, 2, 1, darken(boots, 0.2))
        else:
            c.rect(5, 22, 2, 1, boots if step >= 0 else darken(boots, 0.3))
            c.rect(9, 22, 2, 1, boots if step <= 0 else darken(boots, 0.3))
    else:
        pants, pants_d, boots = spec["pants"], spec["pants_d"], spec["boots"]
        leg_len = 3 if kid else 5
        if side:
            fx, bx = (9, 5) if step != 0 else (8, 6)
            if step == -1:
                fx, bx = 6, 8
            for (lx, col) in ((bx, pants_d), (fx, pants)):
                c.rect(lx, legs_top, 2, leg_len - 1, col)
                c.rect(lx, legs_top + leg_len - 1, 2, 1, boots)
                c.px(lx + (1 if direction == "right" else -1) + (1 if direction == "left" else 0), legs_top + leg_len - 1, boots)
        else:
            la = -1 if step == 1 else 0
            lb = -1 if step == -1 else 0
            c.rect(5, legs_top, 3, leg_len - 1 + la, pants)
            c.rect(5, legs_top + leg_len - 1 + la, 3, 1, boots)
            c.rect(8, legs_top, 3, leg_len - 1 + lb, pants_d if direction == "up" else pants)
            c.rect(8, legs_top + leg_len - 1 + lb, 3, 1, boots)
            c.vline(7, legs_top, 2, pants_d)

    # --- Torso ---
    t0 = 11 + oy
    tw = 6 if side else 8
    tx = 5 if side else 4
    th = 6 if not kid else 5
    for y in range(t0, t0 + th):
        for x in range(tx, tx + tw):
            col = top
            if x == tx or (not side and x == tx + tw - 1):
                col = top_d
            c.px(x, y, col)
    if "belt" in spec and not dress:
        c.hline(tx, t0 + th - 1, tw, spec["belt"])
        if not side and direction == "down":
            c.px(7, t0 + th - 1, GOLD)
    if "sash" in spec:
        c.hline(tx, t0 + 3, tw, spec["sash"])
        c.hline(tx, t0 + 4, tw, darken(spec["sash"], 0.2))
    if "apron" in spec and direction != "up":
        ax = tx + (2 if side and direction == "right" else 0) + (0 if side else 1)
        aw = 3 if side else 6
        for y in range(t0 + 2, 21 if dress else t0 + th + 3):
            c.hline(ax if not side else (tx + (3 if direction == "right" else 0)), y, aw, spec["apron"])
    # --- Brazos ---
    swing = step if not side else 0
    if side:
        ax = 7 if direction == "right" else 8
        arm_y = t0 + 1
        off = [0, 1, -1][frame]
        c.rect(ax + off, arm_y, 2, 4, top_d)
        c.rect(ax + off, arm_y + 4, 2, 1, skin)
    else:
        c.rect(3, t0 + 1 - swing, 1, 4, top_d)
        c.px(3, t0 + 5 - swing, skin)
        c.rect(12, t0 + 1 + swing, 1, 4, top_d)
        c.px(12, t0 + 5 + swing, skin)

    # --- Bufanda ---
    if "scarf" in spec and spec.get("style") != "scarf":
        c.hline(tx, t0, tw, spec["scarf"])
        if direction == "up":
            c.rect(9, t0 + 1, 2, 4, spec["scarf_d"])
        elif side:
            bx = 5 if direction == "right" else 9
            c.rect(bx, t0 + 1, 2, 3 + (frame % 2), spec["scarf_d"])
        else:
            c.rect(9, t0 + 1, 2, 3, spec["scarf_d"])

    # --- Cabeza ---
    h0 = 2 + oy
    hx = 4 if not side else 5
    hw = 8 if not side else 7
    for y in range(h0, h0 + 9):
        for x in range(hx, hx + hw):
            corner = (y in (h0, h0 + 8)) and (x in (hx, hx + hw - 1))
            if not corner:
                c.px(x, y, skin)
    # sombra de mandíbula
    c.hline(hx + 1, h0 + 8, hw - 2, skin_s)

    style = spec["style"]
    # --- Pelo ---
    def hair_px(x, y, dark=False):
        c.px(x, y, hair_d if dark else hair)

    if style != "bald":
        for x in range(hx - 1, hx + hw + 1):
            for y in range(h0 - 1, h0 + 3):
                if (x in (hx - 1, hx + hw)) and y == h0 - 1:
                    continue
                hair_px(x, y, dark=(y == h0 + 2))
    if style == "spiky":
        for x in range(hx, hx + hw, 2):
            hair_px(x, h0 - 2)
        if direction == "down":
            hair_px(hx + 1, h0 + 3)
            hair_px(hx + hw - 2, h0 + 3)
    if style == "swept":
        if direction == "down":
            for x in range(hx, hx + 5):
                hair_px(x, h0 + 3, dark=True)
            hair_px(hx, h0 + 4, dark=True)
    if style in ("long", "bun", "scarf", "swept", "spiky"):
        # laterales
        if direction == "down":
            for y in range(h0 + 3, h0 + (9 if style == "long" else 6)):
                hair_px(hx - 1, y, dark=True)
                hair_px(hx + hw, y, dark=True)
            if style == "long":
                for y in range(h0 + 9, h0 + 13):
                    hair_px(hx - 1, y)
                    hair_px(hx + hw, y)
    if style == "bun":
        c.rect(hx + 2, h0 - 3, 4, 2, hair)
        c.hline(hx + 2, h0 - 2, 4, hair_d)
    if style == "scarf":
        sc, scd = spec["scarf"], spec["scarf_d"]
        for x in range(hx - 1, hx + hw + 1):
            for y in range(h0 - 1, h0 + 2):
                if (x in (hx - 1, hx + hw)) and y == h0 - 1:
                    continue
                c.px(x, y, sc if y < h0 + 1 else scd)
        if direction == "up":
            c.rect(hx + 2, h0 + 2, hw - 4, 5, scd)
    if style == "bald":
        # pelo solo a los lados
        if direction != "up":
            for y in range(h0 + 2, h0 + 6):
                hair_px(hx - 1 if direction != "right" else hx - 1, y)
                if direction == "down":
                    hair_px(hx + hw, y)
        else:
            c.rect(hx, h0 + 3, hw, 3, hair)
        c.px(hx + 2, h0 + 1, lighten(skin, 0.4))

    if direction == "up":
        # Nuca cubierta
        y_end = h0 + (13 if style == "long" else 7)
        x0b = hx - (1 if style != "bald" else 0)
        x1b = hx + hw + (1 if style != "bald" else 0)
        for y in range(h0 + 3, y_end):
            for x in range(x0b, x1b):
                if style == "bald" and y > h0 + 5:
                    continue
                if style == "scarf":
                    continue
                edge = x in (x0b, x1b - 1) or y == y_end - 1
                c.px(x, y, hair_d if edge else hair)
        if style == "long":
            c.vline(8, h0 + 4, y_end - h0 - 5, hair_d)
    elif side:
        back = hx - 1 if direction == "right" else hx + hw
        front_eye = hx + hw - 2 if direction == "right" else hx + 1
        if style != "bald":
            rng = range(hx - 1, hx + 3) if direction == "right" else range(hx + hw - 3, hx + hw + 1)
            for x in rng:
                for y in range(h0 + 3, h0 + (13 if style == "long" else 7)):
                    if style == "scarf" and y > h0 + 5:
                        continue
                    hair_px(x, y, dark=(x == back))
        c.px(front_eye, h0 + 5, OUT)
        nose = hx + hw if direction == "right" else hx - 1
        c.px(nose, h0 + 6, skin)
        if "beard" in spec:
            for y in range(h0 + 6, h0 + 10):
                for x in range(hx + 2, hx + hw) if direction == "right" else range(hx, hx + hw - 2):
                    c.px(x, y, spec["beard"])
    else:
        # Cara de frente
        c.px(hx + 2, h0 + 5, OUT)
        c.px(hx + hw - 3, h0 + 5, OUT)
        c.px(hx + 2, h0 + 4, lighten(skin, 0.1))
        if style in ("long", "bun", "scarf") or spec.get("blush"):
            c.px(hx + 1, h0 + 6, (230, 150, 140))
            c.px(hx + hw - 2, h0 + 6, (230, 150, 140))
        c.px(hx + 3, h0 + 7, skin_s)
        c.px(hx + 4, h0 + 7, skin_s)
        if "beard" in spec:
            for y in range(h0 + 6, h0 + 11):
                w = hw if y < h0 + 9 else hw - 2
                x0 = hx if y < h0 + 9 else hx + 1
                for x in range(x0, x0 + w):
                    if y == h0 + 6 and x in (hx + 2, hx + hw - 3):
                        continue
                    c.px(x, y, spec["beard"])
            c.px(hx + 3, h0 + 7, darken(spec["beard"], 0.3))
            c.px(hx + 4, h0 + 7, darken(spec["beard"], 0.3))

    c.outline(OUT)
    return c


def build_sheet(name, spec, out_dir):
    sheet = Canvas(FW * 3, FH * 4)
    for r, d in enumerate(DIRS):
        for f in range(3):
            if d == "left":
                fr = draw_frame(spec, "right", f).flip_h()
            else:
                fr = draw_frame(spec, d, f)
            sheet.paste(fr, f * FW, r * FH)
    sheet.save(f"{out_dir}/chars/{name}.png")


# --- Retratos -----------------------------------------------------------------
def portrait(name, spec, out_dir):
    S = 48
    c = Canvas(S, S)
    skin = spec.get("skin", SKIN)
    skin_s = darken(skin, 0.18)
    hair, hair_d = spec["hair"], spec["hair_d"]
    top, top_d = spec["top"], spec["top_d"]
    style = spec["style"]
    # hombros
    for y in range(36, 48):
        half = 14 + (y - 36) // 2
        for x in range(24 - half, 24 + half):
            c.px(x, y, top if abs(x - 24) < half - 2 else top_d)
    if "scarf" in spec and style != "scarf":
        c.rect(14, 35, 20, 4, spec["scarf"])
        c.rect(26, 38, 5, 8, spec["scarf_d"])
    if "apron" in spec:
        c.rect(17, 40, 14, 8, spec["apron"])
    if "sash" in spec:
        c.rect(10, 44, 28, 2, spec["sash"])
    # cuello
    c.rect(20, 31, 8, 6, skin_s)
    # pelo trasero largo
    if style == "long":
        c.ellipse(24, 26, 15, 17, hair_d)
        c.rect(9, 26, 30, 16, hair_d)
    if style == "bun":
        c.ellipse(24, 5, 6, 5, hair)
        c.ellipse(24, 6, 4, 2, hair_d)
    # cara
    c.ellipse(24, 21, 11, 13, skin)
    for y in range(26, 34):
        for x in range(13, 36):
            if c.get(x, y)[3] and c.get(x, y)[:3] == skin and (x < 16 or x > 32):
                c.px(x, y, skin_s)
    # orejas
    c.ellipse(12.5, 22, 2, 3, skin_s)
    c.ellipse(35.5, 22, 2, 3, skin_s)
    # pelo
    if style == "scarf":
        sc, scd = spec["scarf"], spec["scarf_d"]
        c.ellipse(24, 14, 14, 10, sc)
        c.rect(10, 14, 28, 5, sc)
        c.rect(9, 18, 4, 16, scd)
        c.rect(35, 18, 4, 16, scd)
        c.hline(12, 18, 24, scd)
        c.rect(15, 18, 18, 2, hair)
    elif style == "bald":
        c.rect(11, 17, 3, 9, hair)
        c.rect(34, 17, 3, 9, hair)
        c.px(20, 11, lighten(skin, 0.4))
        c.px(21, 11, lighten(skin, 0.4))
        c.px(19, 12, lighten(skin, 0.3))
    else:
        c.ellipse(24, 13, 13, 8, hair)
        c.rect(11, 12, 26, 5, hair)
        if style == "spiky":
            for i, x in enumerate(range(12, 37, 4)):
                c.rect(x, 4 + (i % 2), 3, 4, hair)
            c.rect(13, 17, 4, 3, hair)
            c.rect(22, 17, 3, 2, hair)
            c.rect(31, 17, 4, 3, hair)
        if style == "swept":
            for x in range(12, 30):
                c.px(x, 17 + (x - 12) // 5, hair)
                c.px(x, 18 + (x - 12) // 5, hair_d)
            c.rect(11, 17, 3, 10, hair_d)
            c.rect(34, 17, 3, 6, hair_d)
        if style in ("long", "bun"):
            c.rect(10, 15, 4, 18 if style == "long" else 8, hair)
            c.rect(34, 15, 4, 18 if style == "long" else 8, hair)
            c.rect(16, 16, 16, 2, hair)
        # brillos
        for x in range(16, 22):
            c.px(x, 8 + (x - 16) // 3, lighten(hair, 0.35))
        c.hline(11, 16, 26, hair_d) if style not in ("swept",) else None
    # ojos
    for ex in (18, 28):
        c.rect(ex, 21, 3, 3, WHITE)
        c.rect(ex + 1, 21, 2, 3, OUT if name != "yara" else (60, 110, 80))
        c.px(ex + 1, 21, WHITE)
        c.hline(ex - 1, 20, 5, hair_d if style != "bald" else darken(skin, 0.4))
    if name == "yara":
        for ex in (18, 28):
            c.px(ex + 2, 23, OUT)
    # nariz, boca
    c.px(24, 25, skin_s)
    c.px(24, 26, skin_s)
    c.hline(22, 29, 5, (170, 90, 80))
    if name in ("kaelen",):
        c.px(27, 28, (170, 90, 80))
    if style in ("long", "bun", "scarf"):
        c.px(16, 26, (232, 150, 140))
        c.px(17, 26, (232, 150, 140))
        c.px(31, 26, (232, 150, 140))
        c.px(32, 26, (232, 150, 140))
    if "beard" in spec:
        b = spec["beard"]
        for y in range(26, 38):
            half = 11 - max(0, y - 30)
            for x in range(24 - half, 24 + half):
                if y < 28 and abs(x - 24) < 6:
                    continue
                c.px(x, y, b if (x + y) % 4 else darken(b, 0.12))
        c.hline(21, 28, 7, darken(b, 0.3))
    if name == "kaelen":
        # cicatriz en la ceja
        c.px(29, 19, skin_s)
        c.px(30, 18, skin_s)
    c.outline(OUT)
    c.save(f"{out_dir}/portraits/{name}.png")


def build_all(out_dir):
    for name, spec in SPECS.items():
        build_sheet(name, spec, out_dir)
        portrait(name, spec, out_dir)
