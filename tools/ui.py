"""UI: marcos 9-slice, iconos, fondos de título y combate, textura de luz."""
import random
from artlib import *


def panel(inner=CREAM, inner_s=CREAM_S, frame=WOOD_M, frame_d=WOOD_D, frame_l=WOOD_L):
    c = Canvas(24, 24)
    c.rect(0, 0, 24, 24, OUT)
    c.rect(1, 1, 22, 22, frame_d)
    c.rect(2, 2, 20, 20, frame)
    c.hline(2, 2, 20, frame_l)
    c.vline(2, 2, 20, frame_l)
    c.rect(4, 4, 16, 16, frame_d)
    c.rect(5, 5, 14, 14, inner)
    c.hline(5, 5, 14, inner_s)
    c.vline(5, 5, 14, inner_s)
    for (x, y) in ((2, 2), (20, 2), (2, 20), (20, 20)):
        c.rect(x, y, 2, 2, GOLD)
        c.px(x, y, lighten(GOLD, 0.4))
    return c


def button(state):
    base = {"normal": WOOD_M, "hover": WOOD_L, "pressed": WOOD_D, "disabled": (90, 80, 76)}[state]
    c = Canvas(16, 16)
    c.rect(0, 0, 16, 16, OUT)
    c.rect(1, 1, 14, 14, darken(base, 0.25))
    c.rect(2, 2, 12, 11 if state != "pressed" else 12, base)
    if state != "pressed":
        c.hline(2, 2, 12, lighten(base, 0.25))
    if state == "hover":
        c.px(2, 2, GOLD)
        c.px(13, 2, GOLD)
    return c


def cursor():
    c = Canvas(8, 8)
    for y in range(7):
        w = 4 - abs(3 - y)
        c.hline(1, y, w + 1, GOLD)
    c.outline()
    return c


def icon(kind):
    c = Canvas(16, 16)
    if kind == "melee":  # espada
        for i in range(9):
            c.px(4 + i, 11 - i, STONE_H)
            c.px(5 + i, 11 - i, STONE_L)
        c.rect(3, 10, 5, 2, GOLD)
        c.rect(2, 12, 3, 3, WOOD_M)
    elif kind == "ranged":  # arco
        for y in range(2, 14):
            x = 5 + int(3 * math.sin((y - 2) / 11 * math.pi))
            c.px(x, y, WOOD_L)
            c.px(x - 1, y, WOOD_M)
        c.vline(5, 2, 12, CREAM)
        c.hline(5, 8, 8, STONE_L)
        c.px(13, 8, STONE_H)
        c.px(12, 7, STONE_H)
        c.px(12, 9, STONE_H)
    elif kind == "dps":  # dagas cruzadas
        for i in range(8):
            c.px(4 + i, 4 + i, STONE_H)
            c.px(11 - i, 4 + i, STONE_L)
        c.rect(2, 12, 3, 2, CLOTH_R)
        c.rect(11, 12, 3, 2, CLOTH_R)
    elif kind == "healer":  # hoja con luz
        c.ellipse(8, 8, 4, 6, (120, 200, 120))
        c.vline(8, 3, 11, (60, 130, 70))
        c.px(6, 5, WHITE)
        for (x, y) in ((2, 3), (13, 4), (3, 12), (13, 12)):
            c.px(x, y, (250, 240, 150))
    elif kind == "tank":  # escudo
        for y in range(2, 14):
            half = 6 if y < 9 else 6 - (y - 8)
            c.hline(8 - half, y, half * 2, CLOTH_B if y > 3 else lighten(CLOTH_B, 0.2))
        c.vline(8, 3, 10, GOLD)
        c.hline(3, 7, 10, GOLD)
    c.outline()
    return c


def light_tex():
    c = Canvas(128, 128)
    for y in range(128):
        for x in range(128):
            d = math.sqrt((x - 63.5) ** 2 + (y - 63.5) ** 2) / 64
            if d < 1:
                a = int(255 * (1 - d) ** 1.6)
                c.p[x, y] = (255, 255, 255, a)
    return c


def battle_bg():
    W, H = 320, 180
    c = Canvas(W, H)
    for y in range(H):
        t = y / H
        col = mix((120, 170, 150), (40, 70, 60), t * 1.2 if t < 0.6 else 0.72)
        c.hline(0, y, W, col)
    rng = random.Random(3)
    # troncos lejanos
    for i in range(18):
        x = rng.randint(0, W)
        w = rng.randint(4, 9)
        c.rect(x, 20, w, 90, (54, 78, 70))
    # copa superior
    for i in range(70):
        c.ellipse(rng.randint(-10, W + 10), rng.randint(-10, 30), rng.randint(14, 30), rng.randint(8, 16),
                  rng.choice([(28, 60, 46), (36, 76, 54), (22, 50, 40)]))
    # troncos cercanos
    for x, w in ((6, 18), (40, 12), (270, 16), (300, 22)):
        c.rect(x, 0, w, 140, (58, 40, 34))
        c.rect(x + 2, 0, 4, 140, (80, 56, 44))
    # suelo
    for y in range(100, H):
        for x in range(W):
            col = forest_ground(x, y)
            c.p[x, y] = col + (255,)
    # claro iluminado
    for y in range(110, 170):
        for x in range(40, 290):
            dx, dy = (x - 165) / 125, (y - 140) / 30
            d = dx * dx + dy * dy
            if d < 1:
                c.px(x, y, (255, 240, 180), int(50 * (1 - d)))
    # rayos de sol
    for k in range(4):
        x0 = 60 + k * 60
        for y in range(0, 150):
            for w in range(10):
                c.px(x0 + w + y // 3, y, (255, 250, 210), 22)
    # hierba delante
    for x in range(0, W, 2):
        h = rng.randint(3, 9)
        c.vline(x, H - h, h, (40, 84, 54))
    return c


def forest_ground(x, y):
    n = fbm(x, y, 20, 5)
    col = mix(FGRASS_D, FGRASS_L, n)
    if hash2(x, y, 6) > 0.97:
        col = (120, 90, 50)
    return col


def title_bg():
    W, H = 320, 180
    c = Canvas(W, H)
    for y in range(H):
        t = y / H
        c.hline(0, y, W, mix((20, 18, 48), (84, 52, 96), t ** 1.4))
    rng = random.Random(9)
    for i in range(140):
        x, y = rng.randint(0, W - 1), rng.randint(0, 110)
        b = rng.randint(150, 255)
        c.px(x, y, (b, b, min(255, b + 20)))
        if rng.random() > 0.93:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c.px(x + dx, y + dy, (b, b, b), 120)
    # luna
    c.ellipse(250, 38, 16, 16, (240, 234, 210))
    c.ellipse(244, 34, 4, 3, (216, 208, 186))
    c.ellipse(255, 45, 3, 2, (216, 208, 186))
    for r in range(18, 34):
        c.ellipse(250, 38, r, r, (240, 230, 200), 3)
    # montañas
    for layer, (col, base, amp, seed) in enumerate(
            [((58, 48, 92), 100, 40, 1), ((44, 38, 74), 118, 30, 2), ((32, 28, 56), 134, 22, 3)]):
        for x in range(W):
            h = base - amp * fbm(x, 0, 60 - layer * 15, seed)
            for y in range(int(h), H):
                c.px(x, y, col)
            if layer == 0 and fbm(x, 0, 60, seed) > 0.7:
                c.px(x, int(h), (180, 180, 210))
                c.px(x, int(h) + 1, (140, 140, 180))
    # silueta de Tortosa y la catedral
    sil = (18, 16, 32)
    c.rect(150, 96, 40, 50, sil)
    for y in range(80, 96):
        half = (y - 80) * 20 // 16
        c.hline(170 - half, y, half * 2, sil)
    c.rect(140, 70, 12, 76, sil)
    for y in range(58, 70):
        half = (y - 58) * 6 // 12
        c.hline(146 - half, y, half * 2, sil)
    c.rect(188, 84, 12, 62, sil)
    c.rect(188, 80, 5, 4, sil)
    c.ellipse(170, 104, 5, 5, (60, 46, 70))
    for (hx, hw, hh) in ((100, 22, 16), (122, 18, 12), (206, 20, 14), (228, 24, 18), (84, 16, 10), (254, 16, 12)):
        c.rect(hx, 146 - hh, hw, hh, sil)
        for y in range(10):
            c.hline(hx - 2 + y // 2, 146 - hh - 10 + y, hw + 4 - y, sil) if False else None
        for y in range(8):
            c.hline(hx + y, 146 - hh - 8 + y, max(1, hw - 2 * y), sil) if False else None
        for y in range(hw // 2):
            c.hline(hx + y, 146 - hh - y, hw - 2 * y, sil)
        c.rect(hx + hw // 3, 146 - hh + 4, 3, 3, (250, 196, 110))
    for x in range(0, W):
        h = 150 + int(10 * fbm(x, 3, 12, 8))
        for y in range(h, H):
            c.px(x, y, (12, 14, 24))
    for i in range(30):
        x = rng.randint(0, W)
        for y in range(8):
            half = y // 2
            c.hline(x - half, 146 + y * 2, half * 2 + 1, (12, 14, 24))
    return c


def build_all(out):
    d = f"{out}/ui"
    panel().save(f"{d}/panel.png")
    panel((34, 30, 48), (26, 22, 38), (84, 70, 60), (54, 42, 40), (120, 100, 80)).save(f"{d}/panel_dark.png")
    for s in ("normal", "hover", "pressed", "disabled"):
        button(s).save(f"{d}/button_{s}.png")
    cursor().save(f"{d}/cursor.png")
    for k in ("melee", "ranged", "dps", "healer", "tank"):
        icon(k).save(f"{d}/icon_{k}.png")
    light_tex().save(f"{d}/light.png")
    battle_bg().save(f"{out}/bg/battle_forest.png")
    title_bg().save(f"{out}/bg/title.png")


# ------------------------------------------------------------ v2: iconos y fondos
def _ic():
    return Canvas(16, 16)


def icons_v2():
    I = {}
    c = _ic()
    for i in range(9):
        c.px(4 + i, 11 - i, STONE_H)
        c.px(5 + i, 11 - i, STONE_L)
    c.rect(3, 10, 5, 2, GOLD)
    c.rect(2, 12, 3, 3, WOOD_M)
    I["cmd_attack"] = c
    c = _ic()
    for a in range(5):
        ang = -math.pi / 2 + a * 2 * math.pi / 5
        for r in range(7):
            c.px(8 + int(r * math.cos(ang)), 8 + int(r * math.sin(ang)), (200, 150, 255))
    c.ellipse(8, 8, 3, 3, (240, 220, 255))
    I["cmd_skill"] = c
    c = _ic()
    c.ellipse(8, 10, 5, 5, (200, 50, 60))
    c.rect(6, 2, 4, 4, CREAM_S)
    c.rect(5, 1, 6, 2, WOOD_M)
    c.px(6, 8, WHITE)
    I["cmd_item"] = c
    I["it_potion"] = c
    c = _ic()
    for y in range(2, 14):
        half = 6 if y < 9 else 6 - (y - 8)
        c.hline(8 - half, y, half * 2, CLOTH_B if y > 3 else lighten(CLOTH_B, 0.2))
    c.vline(8, 3, 10, GOLD)
    c.hline(3, 7, 10, GOLD)
    I["cmd_defend"] = c
    I["st_protect"] = c
    c = _ic()
    c.rect(5, 3, 5, 8, WOOD_M)
    c.rect(5, 10, 9, 3, WOOD_D)
    for k in range(3):
        c.hline(0, 5 + k * 3, 4 - k, (220, 220, 230))
    I["cmd_flee"] = c
    # elementos
    c = _ic()
    for y in range(3, 15):
        half = int(5 * math.sin((y - 3) / 12 * math.pi) + 0.5)
        c.hline(8 - half, y, half * 2, FIRE_2 if y > 8 else FIRE_1)
    c.ellipse(8, 11, 2, 2, FIRE_1)
    I["el_fire"] = c
    c = _ic()
    for ang in range(0, 180, 60):
        a = math.radians(ang)
        for r in range(-6, 7):
            c.px(8 + int(r * math.cos(a)), 8 + int(r * math.sin(a)), (170, 230, 255))
    c.ellipse(8, 8, 1.5, 1.5, WHITE)
    I["el_ice"] = c
    c = _ic()
    pts = [(10, 1), (5, 8), (8, 8), (5, 15), (12, 6), (9, 6), (11, 1)]
    for y in range(1, 15):
        x = 10 - y // 2 if y < 8 else 8 - (y - 8) // 2
        c.hline(x, y, 3, (255, 236, 90))
    I["el_bolt"] = c
    c = _ic()
    c.ellipse(8, 8, 3, 3, (255, 250, 200))
    for a in range(8):
        ang = a * math.pi / 4
        for r in range(4, 7):
            c.px(8 + int(r * math.cos(ang)), 8 + int(r * math.sin(ang)), (255, 230, 140))
    I["el_light"] = c
    c = _ic()
    c.ellipse(8, 8, 6, 6, (140, 80, 180))
    c.ellipse(10, 6, 5, 5, (0, 0, 0, 0))
    for yy in range(16):
        for xx in range(16):
            if ((xx - 10.5) ** 2 + (yy - 6.5) ** 2) < 20:
                c.p[xx, yy] = (0, 0, 0, 0)
    I["el_dark"] = c
    c = _ic()
    c.ellipse(8, 8, 4, 6, (110, 200, 110))
    c.vline(8, 3, 11, (50, 120, 60))
    I["el_nature"] = c
    I["el_phys"] = I["cmd_attack"]
    # estados
    c = _ic()
    c.ellipse(8, 10, 4, 4, (120, 200, 80))
    for y in range(3, 8):
        c.hline(8 - (y - 3) // 2, y, (y - 3) + 1, (120, 200, 80))
    c.px(7, 9, WHITE)
    I["st_poison"] = c
    c = _ic()
    for (x, y) in ((3, 4), (12, 3), (8, 8), (4, 12), (12, 12)):
        c.px(x, y, (255, 240, 120))
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            c.px(x + dx, y + dy, (255, 210, 80))
    I["st_stun"] = c
    c = _ic()
    c.rect(6, 2, 4, 12, (110, 220, 120))
    c.rect(2, 6, 12, 4, (110, 220, 120))
    c.rect(7, 3, 2, 10, (190, 255, 190))
    I["st_regen"] = c
    c = _ic()
    c.ellipse(8, 9, 5, 5, (220, 70, 60))
    c.rect(4, 3, 8, 4, (220, 70, 60))
    for x in (5, 7, 9, 11):
        c.vline(x, 3, 3, (160, 40, 40))
    I["st_rage"] = c
    c = _ic()
    for (x0, y0) in ((3, 3), (9, 3), (3, 9), (9, 9)):
        c.rect(x0, y0 + 1, 4, 1, (240, 80, 70))
        c.rect(x0 + 1, y0, 1, 4, (240, 80, 70))
    I["st_taunt"] = c
    c = _ic()
    for y in range(3, 13):
        c.hline(7, y, 2, (150, 150, 220))
    for k in range(5):
        c.hline(3 + k, 9 + k, 10 - 2 * k, (150, 150, 220))
    I["st_weak"] = c
    # objetos
    def flask(col, lab=None):
        c = _ic()
        c.ellipse(8, 10, 5, 5, col)
        c.rect(6, 2, 4, 4, CREAM_S)
        c.rect(5, 1, 6, 2, WOOD_M)
        c.px(6, 8, WHITE)
        if lab:
            c.hline(5, 11, 6, lab)
        return c
    I["it_potion2"] = flask((230, 90, 60), GOLD)
    I["it_ether"] = flask((70, 110, 220))
    I["it_antidote"] = flask((90, 180, 90))
    c = _ic()
    for i in range(12):
        c.px(3 + i, 13 - i, (250, 120, 60))
        c.px(4 + i, 13 - i, (255, 200, 90))
        c.px(2 + i, 12 - i, (240, 80, 60))
    I["it_feather"] = c
    c = _ic()
    for (x, y) in ((8, 4), (4, 8), (12, 8), (8, 12)):
        c.ellipse(x, y, 3, 3, (220, 236, 255))
    c.ellipse(8, 8, 2, 2, (250, 240, 150))
    I["it_flower"] = c
    c = _ic()
    c.ellipse(8, 10, 5, 5, (60, 60, 70))
    c.px(6, 8, (140, 140, 150))
    c.vline(10, 2, 4, WOOD_L)
    c.px(11, 1, FIRE_1)
    I["it_bomb"] = c
    c = _ic()
    c.ellipse(8, 9, 6, 4, (196, 150, 90))
    c.hline(5, 8, 6, (220, 180, 120))
    I["it_bread"] = c
    c = _ic()
    c.ellipse(8, 8, 5, 6, (150, 220, 255))
    c.px(6, 6, WHITE)
    I["it_seed"] = c
    c = Canvas(14, 12)
    c.rect(0, 4, 8, 4, (250, 240, 220))
    c.rect(6, 2, 3, 8, (250, 240, 220))
    c.rect(9, 3, 4, 2, (250, 240, 220))
    c.rect(9, 6, 4, 2, (250, 240, 220))
    c.outline()
    I["hand"] = c
    for k, v in I.items():
        if k != "hand":
            v.outline()
    return I


def bg_generic(sky_top, sky_bot, ground_fn, deco_fn, seed=1):
    W, H = 320, 180
    c = Canvas(W, H)
    for y in range(H):
        c.hline(0, y, W, mix(sky_top, sky_bot, y / H))
    deco_fn(c, random.Random(seed))
    for y in range(110, H):
        for x in range(W):
            c.p[x, y] = ground_fn(x, y) + (255,)
    for y in range(118, 168):
        for x in range(40, 290):
            dx, dy = (x - 165) / 125, (y - 142) / 26
            d = dx * dx + dy * dy
            if d < 1:
                c.px(x, y, (255, 240, 200), int(30 * (1 - d)))
    return c


def bgs_v2(out):
    def night_deco(c, rng):
        for i in range(60):
            c.px(rng.randint(0, 319), rng.randint(0, 50), (200, 200, 230))
        for (x, w, h) in ((20, 50, 40), (90, 40, 30), (220, 60, 44), (280, 40, 30)):
            c.rect(x, 110 - h, w, h, (30, 24, 40))
            for yy in range(w // 2):
                c.hline(x + yy, 110 - h - yy, w - 2 * yy, (30, 24, 40))
            for k in range(30):
                fx, fy = x + rng.randint(0, w), 110 - h - rng.randint(0, w // 2 + 16)
                c.px(fx, fy, rng.choice([FIRE_1, FIRE_2, FIRE_3]))
        c.rect(140, 30, 40, 80, (26, 22, 34))
        c.rect(128, 40, 10, 70, (26, 22, 34))
        c.rect(182, 50, 10, 60, (26, 22, 34))
        for y in range(0, 110):
            for x in range(0, 320, 3):
                if rng.random() > 0.995:
                    c.px(x, y, FIRE_2)
    bg_generic((20, 18, 50), (120, 50, 50), lambda x, y: mix((40, 34, 36), (70, 56, 50), fbm(x, y, 14, 3)),
               night_deco, 3).save(f"{out}/bg/battle_village_night.png")

    def cath_deco(c, rng):
        for x in range(0, 320, 64):
            c.rect(x + 20, 0, 22, 110, (90, 88, 100))
            c.rect(x + 22, 0, 6, 110, (120, 118, 130))
        for x in (60, 124, 188, 252):
            cols = [(200, 60, 70), (70, 100, 200), (230, 190, 80), (80, 170, 110)]
            for yy in range(20, 70):
                for xx in range(x, x + 18):
                    col = cols[((xx - x) // 4 + (yy - 20) // 6) % 4]
                    if (xx - x) % 4 == 0 or (yy - 20) % 6 == 0:
                        col = (30, 28, 36)
                    c.px(xx, yy, col)
            for k in range(60):
                for w in range(18):
                    c.px(x + w + k // 3, 70 + k, cols[w // 5 % 4], 20)
    bg_generic((40, 38, 56), (70, 66, 86), lambda x, y: mix((90, 88, 100), (120, 118, 130), hash2(x // 16, y // 8, 4)),
               cath_deco, 4).save(f"{out}/bg/battle_cathedral.png")

    def crypt_deco(c, rng):
        for i in range(14):
            x = rng.randint(0, 310)
            h = rng.randint(20, 60)
            for yy in range(110 - h, 110):
                half = max(1, (yy - (110 - h)) // 4)
                c.hline(x - half, yy, half * 2, (120, 200, 240) if rng.random() > 0.2 else (200, 240, 255))
        for x in range(0, 320, 80):
            c.rect(x + 30, 60, 28, 50, (60, 62, 80))
    bg_generic((14, 14, 26), (40, 44, 66), lambda x, y: mix((46, 48, 62), (70, 72, 88), hash2(x // 16, y // 8, 5)),
               crypt_deco, 5).save(f"{out}/bg/battle_crypt.png")

    def deep_deco(c, rng):
        for i in range(20):
            x = rng.randint(0, 320)
            c.rect(x, 20, rng.randint(5, 10), 90, (40, 26, 44))
        for i in range(60):
            c.ellipse(rng.randint(-10, 330), rng.randint(-10, 30), rng.randint(14, 30), rng.randint(8, 16),
                      rng.choice([(40, 22, 48), (70, 40, 86), (56, 30, 66)]))
        for i in range(40):
            c.px(rng.randint(0, 319), rng.randint(20, 110), (220, 130, 255))
    bg_generic((60, 40, 80), (30, 20, 40), lambda x, y: mix((34, 20, 40), (70, 40, 78), fbm(x, y, 20, 6)),
               deep_deco, 6).save(f"{out}/bg/battle_deep.png")

    def heart_deco(c, rng):
        for k in range(24):
            x = rng.randint(0, 320)
            for y in range(0, 112):
                x += rng.choice([-1, 0, 1]) if y % 3 == 0 else 0
                c.rect(x, y, 4, 1, (58, 40, 50) if k % 2 else (76, 52, 62))
        c.ellipse(160, 50, 60, 40, (60, 20, 70), 120)
        c.ellipse(160, 50, 30, 20, (160, 70, 200), 80)
    bg_generic((30, 10, 30), (60, 20, 50), lambda x, y: mix((30, 16, 34), (80, 40, 90), fbm(x, y, 16, 7)),
               heart_deco, 7).save(f"{out}/bg/battle_heart.png")

    def ending_deco(c, rng):
        c.ellipse(160, 120, 90, 50, (255, 220, 150), 90)
    bg_generic((250, 180, 120), (120, 90, 140), lambda x, y: mix((60, 110, 70), (90, 140, 80), fbm(x, y, 20, 9)),
               ending_deco, 8).save(f"{out}/bg/ending_dawn.png")


def build_v2(out):
    for k, v in icons_v2().items():
        v.save(f"{out}/ui/{k}.png")
    bgs_v2(out)


# --- v3: ventanas estilo JRPG clásico ------------------------------------------------
def window_v3(top=(52, 70, 160), bot=(14, 18, 62)):
    W, H = 32, 48
    c = Canvas(W, H)
    for y in range(H):
        col = mix(top, bot, y / (H - 1))
        c.hline(0, y, W, col)
    # bordes: exterior oscuro, plata, sombra interior
    for x in range(W):
        for y in range(H):
            edge = min(x, y, W - 1 - x, H - 1 - y)
            if edge == 0:
                c.px(x, y, (20, 18, 30))
            elif edge == 1:
                c.px(x, y, (236, 236, 244) if (x < W / 2 or y < H / 2) else (170, 172, 190))
            elif edge == 2:
                c.px(x, y, (120, 124, 150))
    for (x, y) in ((0, 0), (1, 0), (0, 1), (W - 1, 0), (W - 2, 0), (W - 1, 1), (0, H - 1), (1, H - 1), (0, H - 2),
                   (W - 1, H - 1), (W - 2, H - 1), (W - 1, H - 2)):
        c.px(x, y, (0, 0, 0), 0)
        c.p[x, y] = (0, 0, 0, 0)
    for (x, y) in ((1, 1), (W - 2, 1), (1, H - 2), (W - 2, H - 2)):
        c.p[x, y] = (20, 18, 30, 255)
    return c


def build_v3(out):
    d = f"{out}/ui"
    window_v3().save(f"{d}/window.png")
    window_v3((90, 40, 60), (30, 12, 24)).save(f"{d}/window_red.png")
    # barra de selección
    s = Canvas(16, 16)
    for y in range(16):
        s.hline(0, y, 16, mix((120, 150, 240), (60, 80, 180), y / 15))
    s.save(f"{d}/select_bar.png")
    # triángulo indicador de turno
    t = Canvas(9, 7)
    for y in range(5):
        t.hline(y, y, 9 - 2 * y, (255, 230, 120))
    t.outline(OUT)
    t.save(f"{d}/turn_arrow.png")


def equip_icons(out):
    d = f"{out}/ui"
    ST, STD, STL = (196, 206, 218), (120, 128, 146), (240, 244, 250)
    WD, WDD, GD = (122, 84, 52), (84, 56, 36), (226, 184, 82)

    def new():
        return Canvas(16, 16)

    c = new()
    for i in range(10):
        c.px(4 + i, 11 - i, ST)
        c.px(5 + i, 11 - i, STD)
    c.px(13, 2, STL)
    for (x, y) in ((3, 10), (4, 11), (5, 12), (2, 9)):
        c.px(x, y, GD)
    c.px(3, 12, WDD)
    c.px(2, 13, GD)
    c.outline(OUT)
    c.save(f"{d}/eq_sword.png")
    c = new()
    for i in range(11):
        c.px(3 + i, 13 - i, WD if i % 3 else WDD)
    c.rect(9, 2, 5, 5, ST)
    c.vline(13, 2, 5, STL)
    c.px(9, 6, STD)
    c.outline(OUT)
    c.save(f"{d}/eq_axe.png")
    c = new()
    for off in (0, 5):
        for i in range(6):
            c.px(3 + off + i // 2, 12 - i, ST if i < 5 else STL)
        c.px(2 + off, 13, WDD)
        c.px(3 + off, 13, GD)
    c.outline(OUT)
    c.save(f"{d}/eq_daggers.png")
    c = new()
    for i in range(12):
        c.px(4 + i // 3, 14 - i, WD if i % 4 else WDD)
    c.rect(7, 1, 3, 3, (130, 230, 160))
    c.px(7, 1, (230, 255, 240))
    c.outline(OUT)
    c.save(f"{d}/eq_staff.png")
    c = new()
    for k in range(-6, 7):
        c.px(9 - (k * k) // 8, 8 + k, WD)
    for k in range(-5, 6):
        c.px(10, 8 + k, (230, 230, 220))
    c.outline(OUT)
    c.save(f"{d}/eq_bow.png")
    c = new()
    for y in range(2, 14):
        half = 5 if y < 10 else 5 - (y - 9)
        for x in range(8 - half, 8 + half):
            c.px(x, y, STD if x in (8 - half, 8 + half - 1) else ST)
    c.vline(7, 3, 9, GD)
    c.hline(4, 6, 8, GD)
    c.outline(OUT)
    c.save(f"{d}/eq_shield.png")
    c = new()
    c.ellipse(8, 8, 6, 6, ST)
    c.rect(2, 8, 12, 6, (0, 0, 0), 0)
    for y in range(8, 14):
        for x in range(2, 14):
            c.p[x, y] = (0, 0, 0, 0)
    c.rect(2, 8, 12, 2, STD)
    c.rect(2, 10, 2, 4, ST)
    c.rect(12, 10, 2, 4, ST)
    c.px(5, 4, STL)
    c.outline(OUT)
    c.save(f"{d}/eq_helm.png")
    c = new()
    c.ellipse(8, 8, 6, 7, (86, 70, 96))
    c.ellipse(8, 10, 3, 4, (30, 22, 34))
    c.outline(OUT)
    c.save(f"{d}/eq_hood.png")
    c = new()
    c.hline(2, 9, 12, (236, 236, 246))
    c.hline(3, 10, 10, (184, 186, 200))
    c.rect(7, 7, 2, 2, (90, 170, 230))
    c.outline(OUT)
    c.save(f"{d}/eq_circlet.png")
    c = new()
    c.rect(4, 3, 8, 10, ST)
    c.rect(2, 3, 3, 4, STD)
    c.rect(11, 3, 3, 4, STD)
    c.hline(4, 8, 8, STD)
    c.px(6, 5, STL)
    c.outline(OUT)
    c.save(f"{d}/eq_armor.png")
    c = new()
    for y in range(2, 15):
        half = 3 + (y - 2) // 3
        c.hline(8 - half, y, half * 2, (72, 70, 140))
    c.hline(4, 6, 8, GD)
    c.outline(OUT)
    c.save(f"{d}/eq_robe.png")
    c = new()
    c.ellipse(8, 9, 5, 5, GD)
    c.ellipse(8, 9, 3, 3, (0, 0, 0), 0)
    for y in range(6, 13):
        for x in range(5, 12):
            if (x + 0.5 - 8) ** 2 + (y + 0.5 - 9) ** 2 < 6:
                c.p[x, y] = (0, 0, 0, 0)
    c.rect(7, 2, 3, 3, (220, 60, 90))
    c.outline(OUT)
    c.save(f"{d}/eq_ring.png")


def aleixolo_icons(out):
    d = f"{out}/ui"
    c = Canvas(16, 16)
    c.rect(3, 2, 10, 12, (236, 226, 200))
    c.rect(3, 2, 10, 1, (200, 186, 156))
    for y in (5, 7, 9, 11):
        c.hline(5, y, 6, (120, 110, 100))
    c.ellipse(10, 11, 2.5, 2, (110, 62, 34))
    c.outline(OUT)
    c.save(f"{d}/it_scroll.png")
    c = Canvas(12, 12)
    c.rect(2, 2, 8, 8, (110, 62, 34))
    c.rect(2, 2, 8, 1, (160, 100, 60))
    c.px(5, 5, (70, 38, 20))
    c.px(5, 6, (70, 38, 20))
    c.outline(OUT)
    c.save(f"{d}/st_choco.png")
