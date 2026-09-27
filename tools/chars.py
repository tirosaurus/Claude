"""Personajes: hojas 16x24 (3 frames x 4 direcciones) y retratos 48x48.

Además de los PNJ a todo color, genera las capas del creador de personaje:
cuerpo (piel en grises + ropa con color marcador), pelo (en grises) y barba,
para cada raza y sexo. El juego las tiñe en tiempo real.
"""
from artlib import *

FW, FH = 16, 24
DIRS = ["down", "left", "right", "up"]

TEAL = (52, 118, 128)
TEAL_D = (34, 82, 94)
LEATHER = (110, 72, 48)
LEATHER_D = (78, 50, 36)

# Grises reservados para capas teñibles (deben ser r == g == b)
G_SKIN_L, G_SKIN, G_BLUSH, G_SKIN_S = (228,) * 3, (200,) * 3, (184,) * 3, (160,) * 3
G_HAIR_L, G_HAIR, G_HAIR_D = (236,) * 3, (170,) * 3, (110,) * 3
M_TOP, M_TOP_D = (255, 0, 255), (170, 0, 170)

HAIR_STYLES = ["short", "spiky", "long", "ponytail", "bun", "braids", "shaved"]
RACES = ["human", "elf", "dwarf"]
SEXES = ["m", "f"]


def skin_pal(skin):
    return dict(skin=skin, skin_s=darken(skin, 0.18), skin_l=lighten(skin, 0.12), blush=mix(skin, (230, 120, 120), 0.45))


def hair_pal(h):
    return dict(hair=h, hair_d=darken(h, 0.3), hair_l=lighten(h, 0.3))


def npc(race="human", sex="m", skin=SKIN, hair=(110, 70, 42), **kw):
    d = dict(race=race, sex=sex)
    d.update(skin_pal(skin))
    d.update(hair_pal(hair))
    d.update(kw)
    return d


SPECS = {
    "kaelen": npc(hair=(44, 46, 74), style="swept", top=(116, 104, 96), top_d=(84, 74, 70),
                  pants=(58, 56, 72), pants_d=(40, 38, 52), boots=(50, 36, 30), scarf=CLOTH_R, scarf_d=CLOTH_R_D,
                  belt=LEATHER_D),
    "kaelen_dark": npc(skin=(196, 170, 176), hair=(28, 24, 40), style="swept", top=(52, 40, 64), top_d=(34, 26, 44),
                       pants=(34, 30, 44), pants_d=(24, 20, 32), boots=(26, 20, 28), scarf=(90, 30, 110),
                       scarf_d=(60, 20, 76), belt=(20, 16, 24)),
    "yara": npc(sex="f", hair=(190, 84, 50), style="long", top=CREAM, top_d=CREAM_S, dress=True, sash=CLOTH_G,
                boots=(120, 90, 60)),
    "yara_dark": npc(sex="f", skin=(206, 178, 186), hair=(120, 36, 40), style="long", top=(46, 34, 58),
                     top_d=(30, 22, 40), dress=True, sash=(120, 40, 140), boots=(30, 22, 30)),
    "aelis": npc(race="elf", sex="f", skin=(240, 206, 180), hair=(226, 220, 190), style="ponytail",
                 top=(64, 110, 70), top_d=(44, 78, 50), pants=(96, 76, 56), pants_d=(70, 54, 40),
                 boots=(70, 52, 38), belt=LEATHER),
    "brom": npc(race="dwarf", sex="m", skin=(220, 168, 130), hair=(170, 70, 40), style="short", beard=(180, 76, 44),
                top=(120, 124, 136), top_d=(84, 88, 100), pants=(96, 70, 52), pants_d=(70, 50, 38),
                boots=(60, 44, 34), belt=LEATHER),
    "mother": npc(sex="f", hair=(96, 60, 40), style="bun", top=(138, 58, 60), top_d=(100, 40, 46), dress=True,
                  apron=CREAM, boots=(70, 50, 40)),
    "bartolo": npc(skin=(226, 176, 140), hair=(208, 204, 196), style="bald", beard=(226, 222, 214),
                   top=(112, 88, 70), top_d=(82, 62, 50), dress=True, boots=(60, 44, 36)),
    "roc": npc(hair=(60, 40, 30), style="bald", beard=(70, 44, 30), top=(150, 140, 128), top_d=(112, 104, 96),
               pants=(70, 60, 56), pants_d=(50, 42, 40), boots=(44, 34, 30), apron=LEATHER),
    "nil": npc(hair=(200, 150, 70), style="spiky", top=CLOTH_R, top_d=CLOTH_R_D, pants=(80, 90, 120),
               pants_d=(56, 64, 90), boots=LEATHER_D, kid=True),
    "remei": npc(sex="f", hair=(186, 170, 140), style="scarf", scarf=(90, 120, 150), scarf_d=(64, 88, 116),
                 top=CLOTH_G, top_d=CLOTH_G_D, dress=True, boots=(70, 50, 40)),
    "ilvanis": npc(race="elf", sex="f", skin=(236, 214, 196), hair=(236, 236, 240), style="long",
                   top=(70, 120, 130), top_d=(46, 86, 96), dress=True, sash=GOLD, boots=(90, 80, 60)),
    "elf_guard": npc(race="elf", skin=(226, 196, 170), hair=(90, 70, 50), style="short", top=(58, 96, 64),
                     top_d=(40, 68, 46), pants=(70, 60, 44), pants_d=(50, 42, 30), boots=(60, 44, 34), belt=GOLD_D),
    "emissary": npc(skin=(170, 160, 170), hair=(20, 18, 26), style="hood", hood=(34, 28, 40), hood_d=(22, 18, 28),
                    top=(34, 28, 40), top_d=(22, 18, 28), dress=True, sash=(150, 30, 40), boots=(20, 16, 22)),
    "thrall": npc(skin=(120, 96, 130), hair=(40, 30, 44), style="short", top=(84, 70, 60), top_d=(60, 48, 42),
                  pants=(60, 52, 50), pants_d=(44, 38, 36), boots=(40, 30, 30), eyes=(250, 140, 220)),
}


def draw_frame(spec, direction, frame):
    c = Canvas(FW, FH)
    race = spec.get("race", "human")
    sex = spec.get("sex", "m")
    dwarf, elf = race == "dwarf", race == "elf"
    skin, skin_s, skin_l, blush = spec["skin"], spec["skin_s"], spec["skin_l"], spec["blush"]
    hair, hair_d, hair_l = spec["hair"], spec["hair_d"], spec["hair_l"]
    top, top_d = spec["top"], spec["top_d"]
    dress = spec.get("dress", False)
    kid = spec.get("kid", False)
    style = spec.get("style", "none")
    eye_col = spec.get("eyes", OUT)
    step = [0, 1, -1][frame]
    side = direction in ("left", "right")

    h0 = 2 + (3 if kid else (2 if dwarf else 0))
    t0 = h0 + 9
    th = 5 if (kid or dwarf) else 6
    tx, tw = (5, 6) if side else (4, 8)
    if dwarf:
        tx, tw = (4, 8) if side else (3, 10)
    legs_top = t0 + th
    leg_rows = 23 - legs_top

    # --- Piernas / falda ---
    if dress:
        for y in range(t0 + 4, 22):
            half = 4 + (y - t0 - 4) // 3 + (1 if dwarf else 0)
            x0 = 8 - half
            for x in range(x0, 8 + half):
                col = top if x > x0 else top_d
                if y == 21:
                    col = top_d
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
        lw = 3 if not dwarf else 4
        if side:
            fx, bx = (9, 5) if step == 1 else ((6, 8) if step == -1 else (8, 6))
            for (lx, col) in ((bx, pants_d), (fx, pants)):
                c.rect(lx, legs_top, 2, leg_rows - 1, col)
                c.rect(lx, 22, 3 if direction == "right" else 2, 1, boots)
                if direction == "left":
                    c.px(lx - 1, 22, boots)
        else:
            la = -1 if step == 1 else 0
            lb = -1 if step == -1 else 0
            lx1, lx2 = (4, 8) if dwarf else (5, 8)
            c.rect(lx1, legs_top, lw, leg_rows - 1 + la, pants)
            c.rect(lx1, 22 + la, lw, 1, boots)
            c.rect(lx2, legs_top, lw, leg_rows - 1 + lb, pants_d if direction == "up" else pants)
            c.rect(lx2, 22 + lb, lw, 1, boots)
            c.vline(lx2 - 1, legs_top, 2, pants_d)

    # --- Torso ---
    for y in range(t0, t0 + th):
        for x in range(tx, tx + tw):
            col = top
            if x == tx or (not side and x == tx + tw - 1):
                col = top_d
            c.px(x, y, col)
    # túnica larga (femenina)
    if sex == "f" and not dress:
        for y in range(t0 + th, t0 + th + 2):
            for x in range(tx, tx + tw):
                c.px(x, y, top_d if (x == tx or x == tx + tw - 1 or y == t0 + th + 1) else top)
    if "belt" in spec and not dress:
        c.hline(tx, t0 + th - 1, tw, spec["belt"])
        if not side and direction == "down":
            c.px(tx + tw // 2 - 1, t0 + th - 1, GOLD)
    if "sash" in spec:
        c.hline(tx, t0 + 3, tw, spec["sash"])
        c.hline(tx, t0 + 4, tw, darken(spec["sash"], 0.2))
    if "apron" in spec and direction != "up":
        aw = 3 if side else tw - 2
        ax = (tx + (3 if direction == "right" else 0)) if side else tx + 1
        for y in range(t0 + 2, 21 if dress else t0 + th + 3):
            c.hline(ax, y, aw, spec["apron"])
    if dwarf and not dress and direction != "up":
        # hombreras
        c.px(tx, t0, STONE_L)
        c.px(tx + tw - 1, t0, STONE_L)

    # --- Brazos ---
    if side:
        ax = 7 if direction == "right" else 8
        off = [0, 1, -1][frame]
        c.rect(ax + off, t0 + 1, 2, th - 2, top_d)
        c.rect(ax + off, t0 + th - 1, 2, 1, skin)
    else:
        swing = step
        axl, axr = (tx - 1, tx + tw)
        c.rect(axl, t0 + 1 - swing, 1, th - 2, top_d)
        c.px(axl, t0 + th - 1 - swing, skin)
        c.rect(axr, t0 + 1 + swing, 1, th - 2, top_d)
        c.px(axr, t0 + th - 1 + swing, skin)

    # --- Bufanda ---
    if "scarf" in spec and style != "scarf":
        c.hline(tx, t0, tw, spec["scarf"])
        if direction == "up":
            c.rect(9, t0 + 1, 2, 4, spec["scarf_d"])
        elif side:
            bx = 5 if direction == "right" else 9
            c.rect(bx, t0 + 1, 2, 3 + (frame % 2), spec["scarf_d"])
        else:
            c.rect(9, t0 + 1, 2, 3, spec["scarf_d"])

    # --- Cabeza ---
    hx = 4 if not side else 5
    hw = 8 if not side else 7
    for y in range(h0, h0 + 9):
        for x in range(hx, hx + hw):
            corner = (y in (h0, h0 + 8)) and (x in (hx, hx + hw - 1))
            if not corner:
                c.px(x, y, skin)
    c.hline(hx + 1, h0 + 8, hw - 2, skin_s)
    if not side and direction == "down":
        c.px(hx + 2, h0 + 4, skin_l)

    def hp(x, y, dark=False, light=False):
        c.px(x, y, hair_l if light else (hair_d if dark else hair))

    back_len = {"long": 13, "braids": 7, "ponytail": 7, "shaved": 4, "bun": 7}.get(style, 7)

    if style in HAIR_STYLES + ["swept"] and style != "shaved":
        for x in range(hx - 1, hx + hw + 1):
            for y in range(h0 - 1, h0 + 3):
                if (x in (hx - 1, hx + hw)) and y == h0 - 1:
                    continue
                hp(x, y, dark=(y == h0 + 2))
        hp(hx + 1, h0 - 1, light=True)
        hp(hx + 2, h0 - 1, light=True)
    if style == "shaved":
        for x in range(hx, hx + hw):
            hp(x, h0, dark=True)
            if (x + frame) % 2 == 0:
                hp(x, h0 + 1, dark=True)
    if style == "spiky":
        for x in range(hx, hx + hw, 2):
            hp(x, h0 - 2)
        if direction == "down":
            hp(hx + 1, h0 + 3)
            hp(hx + hw - 2, h0 + 3)
    if style == "swept" and direction == "down":
        for x in range(hx, hx + 5):
            hp(x, h0 + 3, dark=True)
        hp(hx, h0 + 4, dark=True)
    if style == "bun":
        c.rect(hx + 2, h0 - 3, 4, 2, hair)
        c.hline(hx + 2, h0 - 2, 4, hair_d)
    if direction == "down" and style in ("long", "bun", "swept", "spiky", "short", "ponytail", "braids"):
        side_len = {"long": 9, "braids": 5, "ponytail": 5, "bun": 5}.get(style, 5 if sex == "f" else 4)
        for y in range(h0 + 3, h0 + side_len):
            hp(hx - 1, y, dark=True)
            hp(hx + hw, y, dark=True)
        if style == "long":
            for y in range(h0 + 9, h0 + 13):
                hp(hx - 1, y)
                hp(hx + hw, y)
        if style == "braids":
            for y in range(h0 + 5, h0 + 13):
                hp(hx - 1, y, dark=(y % 2 == 0))
                hp(hx + hw, y, dark=(y % 2 == 0))
            hp(hx - 1, h0 + 13, light=True)
            hp(hx + hw, h0 + 13, light=True)
    if style == "scarf":
        sc, scd = spec["scarf"], spec["scarf_d"]
        for x in range(hx - 1, hx + hw + 1):
            for y in range(h0 - 1, h0 + 2):
                if (x in (hx - 1, hx + hw)) and y == h0 - 1:
                    continue
                c.px(x, y, sc if y < h0 + 1 else scd)
        if direction == "up":
            c.rect(hx + 2, h0 + 2, hw - 4, 5, scd)
    if style == "hood":
        hd, hdd = spec["hood"], spec["hood_d"]
        for x in range(hx - 2, hx + hw + 2):
            for y in range(h0 - 2, h0 + 10):
                inside_face = (not side and direction == "down" and hx + 1 <= x <= hx + hw - 2 and h0 + 3 <= y <= h0 + 8)
                inside_face_side = side and ((direction == "right" and x >= hx + hw - 3) or (direction == "left" and x <= hx + 2)) and h0 + 3 <= y <= h0 + 8
                if inside_face or inside_face_side:
                    continue
                if (x in (hx - 2, hx + hw + 1)) and y < h0 + 1:
                    continue
                c.px(x, y, hd if y < h0 + 5 else hdd)
        c.px(hx + hw // 2, h0 - 3, hd)
    if style == "bald":
        if direction != "up":
            for y in range(h0 + 2, h0 + 6):
                hp(hx - 1, y)
                if direction == "down":
                    hp(hx + hw, y)
        else:
            c.rect(hx, h0 + 3, hw, 3, hair)
        c.px(hx + 2, h0 + 1, skin_l)

    if direction == "up":
        if style in HAIR_STYLES + ["swept", "bald"] and style != "shaved":
            y_end = h0 + back_len
            x0b = hx - (1 if style != "bald" else 0)
            x1b = hx + hw + (1 if style != "bald" else 0)
            for y in range(h0 + 3, y_end):
                for x in range(x0b, x1b):
                    if style == "bald" and y > h0 + 5:
                        continue
                    edge = x in (x0b, x1b - 1) or y == y_end - 1
                    hp(x, y, dark=edge)
            if style == "long":
                c.vline(8, h0 + 4, y_end - h0 - 5, hair_d)
            if style == "ponytail":
                for y in range(h0 + 7, h0 + 14):
                    c.hline(7, y, 2, hair if y < h0 + 13 else hair_d)
                c.hline(7, h0 + 6, 2, GOLD_D)
            if style == "braids":
                for y in range(h0 + 7, h0 + 13):
                    hp(hx, y, dark=(y % 2 == 0))
                    hp(hx + hw - 1, y, dark=(y % 2 == 0))
        elif style == "shaved":
            for x in range(hx, hx + hw):
                for y in range(h0 + 1, h0 + 4):
                    if (x + y) % 2 == 0:
                        hp(x, y, dark=True)
    elif side:
        back = hx - 1 if direction == "right" else hx + hw
        front_eye = hx + hw - 2 if direction == "right" else hx + 1
        if style in HAIR_STYLES + ["swept", "bald"] and style not in ("shaved", "bald"):
            rng = range(hx - 1, hx + 3) if direction == "right" else range(hx + hw - 3, hx + hw + 1)
            ln = {"long": 13, "braids": 6, "ponytail": 6}.get(style, 7)
            for x in rng:
                for y in range(h0 + 3, h0 + ln):
                    hp(x, y, dark=(x == back))
            if style == "ponytail":
                tx0 = hx - 2 if direction == "right" else hx + hw + 1
                sw = [0, 1, 0][frame]
                for y in range(h0 + 4, h0 + 11):
                    c.px(tx0 + (-sw if direction == "right" else sw), y, hair if y < h0 + 10 else hair_d)
            if style == "braids":
                bx = hx + 1 if direction == "right" else hx + hw - 2
                for y in range(h0 + 6, h0 + 13):
                    hp(bx, y, dark=(y % 2 == 0))
        if style != "hood":
            c.px(front_eye, h0 + 5, eye_col)
            if sex == "f":
                c.px(front_eye, h0 + 4, eye_col)
            nose = hx + hw if direction == "right" else hx - 1
            c.px(nose, h0 + 6, skin)
            if dwarf:
                c.px(nose, h0 + 5, skin)
                c.px(nose + (1 if direction == "right" else -1), h0 + 6, skin_s)
        if elf:
            ex = hx + 2 if direction == "right" else hx + hw - 3
            c.px(ex, h0 + 4, skin)
            c.px(ex - (1 if direction == "right" else -1), h0 + 3, skin)
            c.px(ex - (2 if direction == "right" else -2), h0 + 2, skin_s)
        if "beard" in spec:
            bd = spec["beard"]
            bdd = spec.get("beard_d", darken(bd, 0.3))
            rows = 11 if dwarf else 10
            for y in range(h0 + 6, h0 + rows):
                for x in (range(hx + 2, hx + hw) if direction == "right" else range(hx, hx + hw - 2)):
                    c.px(x, y, bd if y < h0 + rows - 1 else bdd)
    else:
        if style != "hood" or True:
            ey = h0 + 5
            c.px(hx + 2, ey, eye_col)
            c.px(hx + hw - 3, ey, eye_col)
            if sex == "f":
                c.px(hx + 2, ey - 1, eye_col)
                c.px(hx + hw - 3, ey - 1, eye_col)
                c.px(hx + 1, h0 + 6, blush)
                c.px(hx + hw - 2, h0 + 6, blush)
            c.px(hx + 3, h0 + 7, skin_s)
            c.px(hx + 4, h0 + 7, skin_s)
            if dwarf:
                c.rect(hx + 3, h0 + 5, 2, 2, skin_s)
                c.px(hx + 3, h0 + 5, skin)
        if elf:
            for (x, y, col) in ((hx - 1, h0 + 4, skin), (hx - 2, h0 + 3, skin), (hx - 2, h0 + 2, skin_s),
                                (hx + hw, h0 + 4, skin), (hx + hw + 1, h0 + 3, skin), (hx + hw + 1, h0 + 2, skin_s)):
                c.px(x, y, col)
        if "beard" in spec:
            bd = spec["beard"]
            bdd = spec.get("beard_d", darken(bd, 0.3))
            rows = 12 if dwarf else 11
            for y in range(h0 + 6, h0 + rows):
                w = hw if y < h0 + 9 else hw - 2
                x0 = hx if y < h0 + 9 else hx + 1
                for x in range(x0, x0 + w):
                    if y == h0 + 6 and x in (hx + 2, hx + hw - 3, hx + 3, hx + 4):
                        continue
                    c.px(x, y, bd if (x + y) % 5 else bdd)
            c.px(hx + 3, h0 + 7, bdd)
            c.px(hx + 4, h0 + 7, bdd)
            if dwarf:
                c.rect(hx + 3, h0 + 11, 2, 2, bd)
                c.px(hx + 3, h0 + 12, bdd)
    c.outline(OUT)
    return c


def build_sheet(spec):
    sheet = Canvas(FW * 3, FH * 4)
    for r, d in enumerate(DIRS):
        for f in range(3):
            fr = draw_frame(spec, "right", f).flip_h() if d == "left" else draw_frame(spec, d, f)
            sheet.paste(fr, f * FW, r * FH)
    return sheet


# --- Retratos -------------------------------------------------------------------
def portrait(spec):
    S = 48
    c = Canvas(S, S)
    race = spec.get("race", "human")
    sex = spec.get("sex", "m")
    dwarf, elf = race == "dwarf", race == "elf"
    skin, skin_s, skin_l, blush = spec["skin"], spec["skin_s"], spec["skin_l"], spec["blush"]
    hair, hair_d, hair_l = spec["hair"], spec["hair_d"], spec["hair_l"]
    top, top_d = spec["top"], spec["top_d"]
    style = spec.get("style", "none")
    eye = spec.get("eye_color", (70, 50, 40) if not elf else (60, 120, 90))
    fw = 12 if dwarf else (10 if (elf or sex == "f") else 11)

    for y in range(36, 48):
        half = (15 if dwarf else 14) + (y - 36) // 2
        for x in range(24 - half, 24 + half):
            c.px(x, y, top if abs(x - 24) < half - 2 else top_d)
    if dwarf:
        c.rect(8, 36, 6, 4, STONE_L)
        c.rect(34, 36, 6, 4, STONE_L)
        c.hline(8, 36, 6, STONE_H)
        c.hline(34, 36, 6, STONE_H)
    if "scarf" in spec and style != "scarf":
        c.rect(14, 35, 20, 4, spec["scarf"])
        c.rect(26, 38, 5, 8, spec["scarf_d"])
    if "apron" in spec:
        c.rect(17, 40, 14, 8, spec["apron"])
    if "sash" in spec:
        c.rect(10, 44, 28, 2, spec["sash"])
    c.rect(20, 31, 8, 6, skin_s)
    if style in ("long",):
        c.ellipse(24, 26, 15, 17, hair_d)
        c.rect(9, 26, 30, 16, hair_d)
    if style == "braids":
        for bx in (11, 35):
            for y in range(24, 44):
                c.rect(bx, y, 3, 1, hair if y % 3 else hair_d)
    if style == "bun":
        c.ellipse(24, 5, 6, 5, hair)
        c.ellipse(24, 6, 4, 2, hair_d)
    if style == "ponytail":
        c.ellipse(36, 16, 5, 5, hair)
        c.rect(36, 16, 4, 20, hair_d)
    # cara
    c.ellipse(24, 21 + (1 if dwarf else 0), fw, 13 if not dwarf else 12, skin)
    for y in range(26, 34):
        for x in range(12, 37):
            p = c.get(x, y)
            if p[3] and p[:3] == skin and (x < 16 or x > 32):
                c.px(x, y, skin_s)
    # orejas
    if not elf:
        c.ellipse(24 - fw - 0.5, 22, 2, 3, skin_s)
        c.ellipse(24 + fw + 0.5, 22, 2, 3, skin_s)
    # pelo
    if style == "scarf":
        sc, scd = spec["scarf"], spec["scarf_d"]
        c.ellipse(24, 14, 14, 10, sc)
        c.rect(10, 14, 28, 5, sc)
        c.rect(9, 18, 4, 16, scd)
        c.rect(35, 18, 4, 16, scd)
        c.hline(12, 18, 24, scd)
        c.rect(15, 18, 18, 2, hair)
    elif style == "hood":
        hd, hdd = spec["hood"], spec["hood_d"]
        c.ellipse(24, 16, 17, 16, hd)
        c.rect(7, 16, 34, 22, hd)
        c.ellipse(24, 22, fw - 1, 11, skin)
        c.rect(12, 8, 24, 8, hdd)
        c.ellipse(24, 14, 11, 4, hdd)
    elif style == "bald":
        c.rect(11, 17, 3, 9, hair)
        c.rect(34, 17, 3, 9, hair)
        c.px(20, 11, skin_l)
        c.px(21, 11, skin_l)
    elif style == "shaved":
        for y in range(8, 16):
            for x in range(14, 35):
                if c.get(x, y)[3] and (x + y) % 2 == 0:
                    c.px(x, y, hair_d)
    elif style != "none":
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
        if style in ("long", "bun", "braids", "ponytail"):
            c.rect(10, 15, 4, 18 if style == "long" else 8, hair)
            c.rect(34, 15, 4, 18 if style == "long" else 8, hair)
            c.rect(16, 16, 16, 2, hair)
        if style == "short":
            c.rect(12, 16, 5, 3, hair)
            c.rect(31, 16, 5, 3, hair)
            c.rect(18, 16, 8, 2, hair)
        for x in range(16, 22):
            c.px(x, 8 + (x - 16) // 3, hair_l)
        if style != "swept":
            c.hline(11, 16, 26, hair_d)
    if elf and style != "hood":
        for i in range(9):
            for (sx, dirn) in ((24 - fw, -1), (24 + fw - 1, 1)):
                c.px(sx + dirn * (1 + i // 2), 23 - i, skin if i < 7 else skin_s)
                c.px(sx + dirn * (i // 2), 23 - i, skin)
                if i < 6:
                    c.px(sx + dirn * (i // 2), 24 - i, skin_s)
    # ojos
    for ex in (18, 28):
        c.rect(ex, 21, 3, 3, WHITE)
        c.rect(ex + 1, 21, 2, 3, eye)
        c.px(ex + 1, 21, WHITE)
        c.hline(ex - 1, 20, 5, hair_d if style not in ("bald", "hood", "none", "shaved") else darken(skin, 0.4))
        if sex == "f":
            c.px(ex - 1, 21, OUT)
            c.px(ex + 3, 20, OUT)
    c.px(20, 23, OUT)
    c.px(30, 23, OUT)
    # nariz, boca
    if dwarf:
        c.ellipse(24, 26, 2.5, 2, skin_s)
        c.px(23, 25, skin_l)
    else:
        c.px(24, 25, skin_s)
        c.px(24, 26, skin_s)
    c.hline(22, 29, 5, (170, 90, 80) if sex == "m" else (196, 96, 100))
    if sex == "f":
        for (x, y) in ((16, 26), (17, 26), (31, 26), (32, 26)):
            c.px(x, y, blush)
    if "beard" in spec:
        b = spec["beard"]
        bdd = spec.get("beard_d", darken(b, 0.12))
        for y in range(26, 40 if dwarf else 38):
            half = 11 - max(0, y - (32 if dwarf else 30))
            for x in range(24 - half, 24 + half):
                if y < 28 and abs(x - 24) < 6:
                    continue
                c.px(x, y, b if (x + y) % 4 else bdd)
        c.hline(21, 28, 7, bdd)
    c.outline(OUT)
    return c


# --- Capas del creador ------------------------------------------------------------
def layer_spec(race, sex, style="none", beard=False):
    d = dict(race=race, sex=sex, style=style, skin=G_SKIN, skin_s=G_SKIN_S, skin_l=G_SKIN_L, blush=G_BLUSH,
             hair=G_HAIR, hair_d=G_HAIR_D, hair_l=G_HAIR_L, top=M_TOP, top_d=M_TOP_D,
             pants=(96, 70, 52), pants_d=(70, 50, 38), boots=LEATHER_D, belt=LEATHER)
    if beard:
        d["beard"] = G_HAIR
        d["beard_d"] = G_HAIR_D
    return d


def diff_layer(full, base):
    out = Canvas(full.w, full.h)
    for y in range(full.h):
        for x in range(full.w):
            a, b = full.p[x, y], base.p[x, y]
            if a != b and a[3] > 0:
                out.p[x, y] = a
    return out


def build_creator(out):
    d = f"{out}/creator"
    for race in RACES:
        for sex in SEXES:
            base_spec = layer_spec(race, sex)
            base = build_sheet(base_spec)
            base.save(f"{d}/body_{race}_{sex}.png")
            pbase = portrait(base_spec)
            pbase.save(f"{d}/pbody_{race}_{sex}.png")
            for style in HAIR_STYLES:
                s = layer_spec(race, sex, style)
                diff_layer(build_sheet(s), base).save(f"{d}/hair_{race}_{sex}_{style}.png")
                diff_layer(portrait(s), pbase).save(f"{d}/phair_{race}_{sex}_{style}.png")
            bs = layer_spec(race, sex, beard=True)
            diff_layer(build_sheet(bs), base).save(f"{d}/beard_{race}_{sex}.png")
            diff_layer(portrait(bs), pbase).save(f"{d}/pbeard_{race}_{sex}.png")


def build_all(out_dir):
    import chars2
    for name, spec in SPECS.items():
        portrait(spec).save(f"{out_dir}/portraits/{name}.png")
    build_creator(out_dir)
    # v3: sprites de mapa y de combate con el nuevo rig
    chars2.build_all(out_dir, SPECS, HAIR_STYLES, RACES, SEXES)
