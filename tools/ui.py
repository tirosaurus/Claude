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
