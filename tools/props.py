"""Props (muebles, edificios, árboles, criaturas). Todos anclados abajo-centro."""
import random
from artlib import *


def wood_plank_fill(c, x, y, w, h, base=WOOD_M, vertical=False, seed=0):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            k = (xx - x) if vertical else (yy - y)
            col = base
            if k % 4 == 3:
                col = darken(base, 0.28)
            elif hash2(xx, yy, seed) > 0.9:
                col = lighten(base, 0.12)
            elif hash2(xx, yy, seed + 1) > 0.92:
                col = darken(base, 0.12)
            c.px(xx, yy, col)


# ---------------------------------------------------------------- Interior
def bed():
    c = Canvas(24, 34)
    c.rect(1, 0, 22, 8, WOOD_D)
    c.rect(2, 1, 20, 6, WOOD_M)
    c.hline(2, 1, 20, WOOD_L)
    c.rect(4, 2, 2, 4, WOOD_D)
    c.rect(18, 2, 2, 4, WOOD_D)
    c.rect(1, 8, 22, 24, WOOD_D)
    c.rect(2, 8, 20, 6, CREAM)
    c.rect(3, 9, 8, 4, WHITE)
    c.rect(13, 9, 8, 4, WHITE)
    c.hline(2, 13, 20, CREAM_S)
    # manta de retazos
    for yy in range(14, 30):
        for xx in range(2, 22):
            cell = ((xx - 2) // 5 + (yy - 14) // 5) % 3
            col = [CLOTH_R, CLOTH_B, (186, 140, 70)][cell]
            if (xx - 2) % 5 == 0 or (yy - 14) % 5 == 0:
                col = darken(col, 0.2)
            c.px(xx, yy, col)
    c.hline(2, 14, 20, lighten(CLOTH_R, 0.2))
    c.rect(1, 30, 22, 4, WOOD_M)
    c.hline(1, 30, 22, WOOD_L)
    c.outline()
    return c


def nightstand():
    c = Canvas(14, 20)
    c.rect(1, 8, 12, 11, WOOD_M)
    c.hline(1, 8, 12, WOOD_L)
    c.rect(2, 12, 10, 5, WOOD_D)
    c.px(7, 14, GOLD)
    # vela
    c.rect(6, 3, 2, 5, CREAM)
    c.px(6, 1, FIRE_1)
    c.px(7, 2, FIRE_2)
    c.px(6, 2, FIRE_1)
    c.rect(4, 7, 6, 1, STONE_L)
    c.outline()
    return c


def wardrobe():
    c = Canvas(24, 38)
    c.rect(1, 2, 22, 35, WOOD_D)
    c.rect(0, 0, 24, 4, WOOD_M)
    c.hline(0, 0, 24, WOOD_L)
    wood_plank_fill(c, 2, 5, 9, 30, WOOD_M, vertical=True, seed=3)
    wood_plank_fill(c, 13, 5, 9, 30, WOOD_M, vertical=True, seed=4)
    c.vline(11, 5, 30, WOOD_D)
    c.vline(12, 5, 30, WOOD_D)
    c.px(10, 19, GOLD)
    c.px(13, 19, GOLD)
    c.rect(2, 35, 20, 2, WOOD_D)
    c.outline()
    return c


def chest():
    c = Canvas(18, 16)
    c.rect(1, 2, 16, 13, WOOD_M)
    c.rect(1, 2, 16, 5, WOOD_L)
    c.hline(1, 2, 16, WOOD_H)
    c.hline(1, 7, 16, WOOD_D)
    for x in (3, 14):
        c.vline(x, 2, 13, STONE_M)
    c.rect(8, 6, 2, 3, GOLD)
    c.px(8, 8, GOLD_D)
    c.outline()
    return c


def bookshelf():
    c = Canvas(28, 38)
    c.rect(0, 0, 28, 38, WOOD_D)
    c.rect(1, 1, 26, 2, WOOD_L)
    rng = random.Random(5)
    for shelf in range(4):
        y0 = 4 + shelf * 8
        c.rect(2, y0, 24, 6, (40, 26, 22))
        x = 2
        while x < 25:
            w = rng.choice([2, 2, 3])
            h = rng.choice([4, 5, 6])
            col = rng.choice([CLOTH_R, CLOTH_B, CLOTH_G, (150, 110, 60), (120, 60, 110), CREAM_S])
            if x + w > 26:
                break
            c.rect(x, y0 + 6 - h, w, h, col)
            c.px(x, y0 + 6 - h, lighten(col, 0.3))
            x += w
        c.hline(1, y0 + 6, 26, WOOD_M)
        c.hline(1, y0 + 7, 26, WOOD_D)
    c.outline()
    return c


def table(items=False):
    c = Canvas(32, 24)
    c.rect(2, 12, 2, 11, WOOD_D)
    c.rect(28, 12, 2, 11, WOOD_D)
    wood_plank_fill(c, 0, 2, 32, 12, WOOD_L, seed=9)
    c.hline(0, 2, 32, WOOD_H)
    c.rect(0, 14, 32, 2, WOOD_D)
    if items:
        c.ellipse(9, 7, 4, 2.5, (196, 150, 90))
        c.hline(7, 6, 4, (220, 180, 120))
        c.rect(19, 2, 4, 6, STONE_L)
        c.hline(19, 2, 4, STONE_H)
        c.rect(23, 4, 1, 2, STONE_L)
        c.ellipse(27, 8, 3, 2, CREAM_S)
        c.ellipse(27, 7.5, 2, 1, (170, 110, 70))
    c.outline()
    return c


def chair():
    c = Canvas(12, 18)
    c.rect(1, 0, 10, 8, WOOD_M)
    c.rect(2, 1, 8, 2, WOOD_L)
    c.rect(1, 8, 10, 4, WOOD_L)
    c.rect(1, 12, 2, 5, WOOD_D)
    c.rect(9, 12, 2, 5, WOOD_D)
    c.outline()
    return c


def fireplace(frame):
    c = Canvas(40, 44)
    for yy in range(0, 44):
        for xx in range(0, 40):
            bx = (xx + (4 if (yy // 5) % 2 else 0)) // 8
            col = STONE_M
            if yy % 5 == 4 or (xx + (4 if (yy // 5) % 2 else 0)) % 8 == 7:
                col = STONE_D
            elif hash2(bx, yy // 5, 2) > 0.6:
                col = STONE_L
            c.px(xx, yy, col)
    c.rect(0, 0, 40, 3, WOOD_D)
    c.rect(0, 22, 40, 3, WOOD_M)
    c.hline(0, 22, 40, WOOD_L)
    # boca
    c.rect(8, 26, 24, 18, BLACK)
    c.ellipse(20, 27, 12, 4, BLACK)
    # leña
    c.rect(11, 40, 18, 3, WOOD_D)
    c.rect(13, 38, 14, 2, WOOD_M)
    # fuego
    rng = random.Random(frame * 17 + 3)
    for i in range(40):
        fx = 12 + rng.randint(0, 16)
        fy = 39 - rng.randint(0, 11 - abs(fx - 20) // 2)
        col = FIRE_3 if fy > 36 else (FIRE_2 if fy > 32 else FIRE_1)
        c.px(fx, fy, col)
        c.px(fx, fy + 1, FIRE_2 if col == FIRE_1 else FIRE_3)
    # olla
    c.vline(20, 26, 5, STONE_D)
    c.ellipse(20, 33, 5, 3.5, (50, 50, 56))
    c.hline(16, 31, 9, (80, 80, 90))
    c.outline()
    return c


def barrel():
    c = Canvas(14, 18)
    for yy in range(1, 17):
        bulge = 1 if 4 < yy < 13 else 0
        for xx in range(1 - bulge, 13 + bulge):
            col = WOOD_M if (xx % 3) else WOOD_D
            if xx < 3:
                col = darken(col, 0.15)
            c.px(xx, yy, col)
    for yy in (3, 13):
        c.hline(0, yy, 14, STONE_M)
    c.ellipse(7, 2, 6, 2, WOOD_L)
    c.outline()
    return c


def crate():
    c = Canvas(16, 16)
    wood_plank_fill(c, 1, 1, 14, 14, WOOD_L, seed=11)
    c.rect(1, 1, 14, 2, WOOD_D)
    c.rect(1, 13, 14, 2, WOOD_D)
    for i in range(12):
        c.px(2 + i, 3 + i * 10 // 12, WOOD_D)
    c.outline()
    return c


def plant():
    c = Canvas(14, 18)
    c.rect(3, 11, 8, 6, ROOF_M)
    c.hline(2, 11, 10, ROOF_L)
    rng = random.Random(4)
    for i in range(40):
        a = rng.random() * 3.14
        r = rng.random() * 6
        x = 7 + int(r * math.cos(a) * 1.1)
        y = 10 - int(r * math.sin(a))
        c.px(x, y, rng.choice([LEAF_M, LEAF_L, LEAF_D]))
    c.outline()
    return c


def sack():
    c = Canvas(14, 14)
    c.ellipse(7, 8.5, 6, 5, (196, 170, 120))
    c.rect(5, 1, 4, 3, (196, 170, 120))
    c.hline(4, 4, 6, (140, 110, 70))
    c.ellipse(5, 9, 2, 3, (220, 196, 150))
    c.outline()
    return c


def stool():
    c = Canvas(12, 12)
    c.ellipse(6, 3, 5, 2.5, WOOD_L)
    c.rect(2, 4, 2, 7, WOOD_D)
    c.rect(8, 4, 2, 7, WOOD_D)
    c.outline()
    return c


def rail():
    c = Canvas(4, 34)
    c.rect(1, 0, 2, 34, WOOD_M)
    c.vline(1, 0, 34, WOOD_L)
    for y in (0, 16, 32):
        c.rect(0, y, 4, 2, WOOD_D)
    c.outline()
    return c


# ---------------------------------------------------------------- Exterior
def house(roof=(ROOF_D, ROOF_M, ROOF_L), w=64, wall_h=30, roof_h=44, seed=1, sign=None):
    H = wall_h + roof_h
    c = Canvas(w + 8, H + 2)
    ox = 4
    # muro
    wy = roof_h - 4
    for yy in range(wy, wy + wall_h):
        for xx in range(ox + 2, ox + w - 2):
            col = PLASTER
            if hash2(xx, yy, seed) > 0.93:
                col = PLASTER_S
            c.px(xx, yy, col)
    # zócalo de piedra
    for yy in range(wy + wall_h - 8, wy + wall_h):
        for xx in range(ox + 2, ox + w - 2):
            sx = (xx + (3 if (yy // 4) % 2 else 0)) % 7
            col = STONE_M if yy % 4 != 3 and sx != 6 else STONE_D
            if hash2(xx // 7, yy // 4, seed) > 0.6 and col == STONE_M:
                col = STONE_L
            c.px(xx, yy, col)
    # entramado de madera
    c.hline(ox + 2, wy, w - 4, WOOD_D)
    c.hline(ox + 2, wy + 1, w - 4, WOOD_M)
    c.hline(ox + 2, wy + wall_h - 9, w - 4, WOOD_D)
    for xx in (ox + 2, ox + w // 2 - 1, ox + w - 4):
        c.rect(xx, wy, 2, wall_h - 8, WOOD_D)
    for i in range(wall_h - 10):
        c.px(ox + 5 + i, wy + 2 + i, WOOD_D)
        c.px(ox + w - 6 - i, wy + 2 + i, WOOD_D)
    # puerta
    dx = ox + w // 2 - 6
    dy = wy + wall_h - 18
    c.rect(dx - 1, dy - 1, 14, 19, WOOD_D)
    wood_plank_fill(c, dx, dy, 12, 18, WOOD_M, vertical=True, seed=seed)
    c.ellipse(dx + 6, dy, 6, 2, WOOD_D)
    c.px(dx + 9, dy + 9, GOLD)
    c.hline(dx, dy + 4, 12, WOOD_D)
    c.hline(dx, dy + 13, 12, WOOD_D)
    # ventanas
    for wxp in (ox + 8, ox + w - 20):
        wyp = wy + 6
        c.rect(wxp - 1, wyp - 1, 14, 12, WOOD_D)
        c.rect(wxp, wyp, 12, 10, (70, 90, 110))
        c.rect(wxp + 1, wyp + 1, 4, 3, (150, 180, 200))
        c.vline(wxp + 6, wyp, 10, WOOD_M)
        c.hline(wxp, wyp + 5, 12, WOOD_M)
        c.rect(wxp - 1, wyp + 10, 14, 2, WOOD_L)
        # macetas
        for fx in range(wxp, wxp + 12, 3):
            c.px(fx, wyp + 9, ROOF_L if (fx // 3) % 2 else GOLD)
    # tejado
    rd, rm, rl = roof
    for yy in range(0, roof_h):
        inset = max(0, (roof_h - yy - 30) // 1) if yy < 12 else 0
        inset = max(0, 12 - yy) if yy < 12 else 0
        for xx in range(ox - 3 + inset, ox + w + 3 - inset):
            row = yy // 4
            shift = 3 if row % 2 else 0
            col = rm
            if yy % 4 == 3:
                col = rd
            elif (xx + shift) % 6 == 0:
                col = rd
            elif yy % 4 == 0:
                col = rl
            if hash2(xx, yy, seed + 5) > 0.95:
                col = rl
            c.px(xx, yy, col)
    c.hline(ox - 3, roof_h - 1, w + 6, darken(rd, 0.3))
    c.hline(ox + 9, 0, w - 18, lighten(rl, 0.2))
    # chimenea
    cx = ox + w - 18
    c.rect(cx, 0, 8, 10, STONE_M)
    c.rect(cx - 1, 0, 10, 2, STONE_D)
    c.vline(cx + 7, 2, 8, STONE_D)
    if sign:
        c.rect(ox + w - 10, wy + 10, 10, 8, WOOD_L)
        c.rect(ox + w - 9, wy + 11, 8, 6, sign)
    c.outline()
    return c


def cathedral(open_door=False):
    W, H = 192, 184
    c = Canvas(W, H)
    rng = random.Random(21)

    def stone_block(xx, yy, base=STONE_M, seed=0):
        bw, bh = 8, 5
        sx = (xx + (4 if (yy // bh) % 2 else 0))
        col = base
        if yy % bh == bh - 1 or sx % bw == bw - 1:
            col = darken(base, 0.3)
        else:
            n = hash2(sx // bw, yy // bh, seed)
            if n > 0.7:
                col = lighten(base, 0.12)
            elif n < 0.2:
                col = darken(base, 0.1)
        # manchas de musgo
        if fbm(xx, yy, 12, seed + 3) > 0.62:
            col = mix(col, (80, 110, 70), 0.45)
        return col

    base_y = H - 1
    # nave central (fachada)
    for yy in range(56, H):
        for xx in range(40, 152):
            c.px(xx, yy, stone_block(xx, yy))
    # hastial triangular roto
    for yy in range(20, 56):
        half = int((yy - 20) * 1.55)
        for xx in range(96 - half, 96 + half):
            if xx > 104 and yy < 44 - (xx - 104) // 3:
                continue  # rotura
            c.px(xx, yy, stone_block(xx, yy, STONE_L, 1))
    # torres
    for (tx, top, broken) in ((12, 8, False), (148, 30, True)):
        for yy in range(top, H):
            for xx in range(tx, tx + 32):
                if broken and yy < top + 16 and (xx - tx) > 8 + (yy - top) * 2 and (xx - tx) < 30:
                    continue
                if broken and yy < top + 6 and (xx - tx) > 4:
                    continue
                col = stone_block(xx, yy, STONE_M, 2)
                if xx in (tx, tx + 31):
                    col = STONE_D
                c.px(xx, yy, col)
        # pináculo
        if not broken:
            for yy in range(0, top):
                half = (yy * 16) // top
                for xx in range(tx + 16 - half, tx + 16 + half):
                    c.px(xx, yy, stone_block(xx, yy, STONE_L, 4))
        # ventanas ojivales de torre
        for wy in (top + 22, top + 60):
            if wy + 18 < H - 40:
                c.rect(tx + 12, wy + 4, 8, 14, BLACK)
                c.ellipse(tx + 16, wy + 4, 4, 4, BLACK)
    # rosetón roto
    c.ellipse(96, 78, 17, 17, STONE_D)
    c.ellipse(96, 78, 15, 15, (28, 22, 40))
    for a in range(12):
        ang = a * 3.14159 / 6
        for r in range(3, 15):
            x = 96 + int(r * math.cos(ang))
            y = 78 + int(r * math.sin(ang))
            if not (a in (2, 3) and r > 7):
                c.px(x, y, STONE_L)
    for a in range(0, 360, 30):
        ang = math.radians(a + 15)
        c.px(96 + int(10 * math.cos(ang)), 78 + int(10 * math.sin(ang)), (120, 60, 110))
        c.px(96 + int(6 * math.cos(ang)), 78 + int(6 * math.sin(ang)), (70, 90, 150))
    c.ellipse(96, 78, 3, 3, GOLD_D)
    # portada ojival sellada
    px0, pw = 76, 40
    for yy in range(112, H):
        for xx in range(px0 - 4, px0 + pw + 4):
            c.px(xx, yy, stone_block(xx, yy, STONE_L, 6))
    for yy in range(116, H):
        for xx in range(px0, px0 + pw):
            c.px(xx, yy, (26, 20, 32))
    c.ellipse(96, 118, 20, 14, (26, 20, 32))
    for i in range(6):
        c.ellipse(96, 118, 22 + i % 2, 16 + i % 2, STONE_L) if False else None
    # escombros tapando la puerta
    for i in range(0 if open_door else 60):
        rx = px0 + rng.randint(-6, pw + 6)
        ry = H - rng.randint(2, 30)
        rr = rng.randint(2, 5)
        col = rng.choice([STONE_M, STONE_L, STONE_D])
        c.ellipse(rx, ry, rr, rr * 0.8, col)
        c.px(rx - 1, ry - rr + 1, STONE_H)
    # ventanas laterales de la nave
    for wx in (50, 130):
        c.rect(wx, 96, 12, 26, (26, 20, 32))
        c.ellipse(wx + 6, 96, 6, 6, (26, 20, 32))
        c.vline(wx + 6, 92, 30, STONE_L)
    # grietas
    for start in ((60, 60), (140, 70), (110, 130)):
        x, y = start
        for i in range(24):
            c.px(x, y, (40, 36, 48))
            x += rng.choice([-1, 0, 1])
            y += 1
    # enredaderas
    for vx in (14, 44, 150, 170, 120):
        x = vx
        for y in range(H - 1, H - rng.randint(50, 120), -1):
            x += rng.choice([-1, 0, 0, 1])
            c.px(x, y, LEAF_M)
            if rng.random() > 0.6:
                c.px(x + rng.choice([-1, 1]), y, LEAF_L)
            if rng.random() > 0.9:
                c.px(x + rng.choice([-2, 2]), y, LEAF_D)
    c.outline()
    return c


def oak(seed=0):
    c = Canvas(44, 52)
    rng = random.Random(seed)
    c.rect(19, 30, 7, 20, WOOD_D)
    c.rect(20, 30, 3, 20, WOOD_M)
    c.rect(16, 48, 13, 3, WOOD_D)
    blobs = [(22, 18, 16, 13), (12, 24, 9, 8), (32, 24, 9, 8), (22, 10, 11, 8), (16, 14, 8, 7), (29, 14, 8, 7)]
    for (x, y, rx, ry) in blobs:
        c.ellipse(x, y, rx, ry, LEAF_D)
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 1, y - 2, rx - 2, ry - 2, LEAF_M)
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 3, y - 4, rx * 0.45, ry * 0.4, LEAF_L)
    for i in range(60):
        x, y = rng.randint(6, 38), rng.randint(2, 32)
        if c.get(x, y)[3]:
            c.px(x, y, rng.choice([LEAF_H, LEAF_D, LEAF_L]))
    c.outline()
    return c


def pine(seed=0):
    c = Canvas(30, 52)
    c.rect(13, 40, 4, 11, WOOD_D)
    for tier in range(4):
        top = 2 + tier * 9
        for yy in range(top, top + 16):
            half = 3 + (yy - top) * (6 + tier * 2) // 16
            for xx in range(15 - half, 15 + half):
                col = PINE_M
                if xx < 15 - half + 2:
                    col = PINE_L
                if xx > 15 + half - 3 or yy == top + 15:
                    col = PINE_D
                if hash2(xx, yy, seed) > 0.9:
                    col = PINE_L
                c.px(xx, yy, col)
    c.outline()
    return c


def dark_tree(seed=0):
    c = Canvas(52, 64)
    rng = random.Random(seed + 40)
    c.rect(22, 34, 9, 29, (60, 40, 34))
    c.rect(23, 34, 3, 29, (84, 58, 44))
    for rx in (16, 34):
        c.rect(rx, 58, 5, 4, (60, 40, 34))
    blobs = [(26, 22, 20, 16), (14, 28, 11, 9), (38, 28, 11, 9), (26, 10, 14, 9)]
    for (x, y, rx, ry) in blobs:
        c.ellipse(x, y, rx, ry, (22, 52, 42))
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 1, y - 2, rx - 3, ry - 3, (32, 70, 50))
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 4, y - 5, rx * 0.4, ry * 0.35, (52, 96, 62))
    for i in range(80):
        x, y = rng.randint(6, 46), rng.randint(2, 38)
        if c.get(x, y)[3]:
            c.px(x, y, rng.choice([(66, 112, 70), (22, 52, 42), (40, 84, 56)]))
    c.outline()
    return c


def bush(seed=0, berries=False):
    c = Canvas(20, 16)
    c.ellipse(10, 9, 9, 6, LEAF_D)
    c.ellipse(9, 7, 7, 5, LEAF_M)
    c.ellipse(7, 5, 3, 2, LEAF_L)
    if berries:
        for (x, y) in ((5, 9), (12, 6), (14, 10), (9, 11)):
            c.px(x, y, CLOTH_R)
    c.outline()
    return c


def rock(big=False):
    w, h = (24, 18) if big else (16, 12)
    c = Canvas(w, h)
    c.ellipse(w / 2, h / 2 + 1, w / 2 - 1, h / 2 - 1, STONE_D)
    c.ellipse(w / 2 - 1, h / 2, w / 2 - 3, h / 2 - 3, STONE_M)
    c.ellipse(w / 2 - 3, h / 2 - 2, w / 4, h / 5, STONE_L)
    c.outline()
    return c


def stump():
    c = Canvas(16, 12)
    c.rect(2, 4, 12, 7, WOOD_D)
    c.ellipse(8, 4, 6, 3, WOOD_L)
    c.ellipse(8, 4, 3, 1.5, WOOD_M)
    c.outline()
    return c


def fence(kind="h"):
    c = Canvas(16, 16)
    if kind == "h":
        c.rect(0, 5, 16, 2, WOOD_L)
        c.rect(0, 10, 16, 2, WOOD_M)
        c.rect(1, 3, 3, 12, WOOD_M)
        c.hline(1, 3, 3, WOOD_L)
        c.rect(12, 3, 3, 12, WOOD_M)
        c.hline(12, 3, 3, WOOD_L)
    else:
        c.rect(6, 0, 3, 16, WOOD_M)
        c.vline(6, 0, 16, WOOD_L)
    c.outline()
    return c


def well():
    c = Canvas(32, 36)
    c.rect(3, 2, 2, 20, WOOD_D)
    c.rect(27, 2, 2, 20, WOOD_D)
    for yy in range(0, 6):
        half = 16 - yy
        c.hline(16 - half + 6 - yy, yy, (half - 6 + yy) * 2, ROOF_M if yy % 2 else ROOF_D)
    c.rect(1, 4, 30, 3, ROOF_D)
    c.hline(5, 9, 22, WOOD_M)
    c.vline(16, 9, 8, (120, 110, 90))
    c.rect(14, 16, 5, 4, WOOD_L)
    for yy in range(20, 35):
        for xx in range(2, 30):
            sx = (xx + (3 if (yy // 4) % 2 else 0)) % 7
            col = STONE_M if yy % 4 != 3 and sx != 6 else STONE_D
            c.px(xx, yy, col)
    c.ellipse(16, 21, 13, 3, STONE_L)
    c.ellipse(16, 21, 11, 2, (30, 40, 60))
    c.outline()
    return c


def signpost(arrow=True):
    c = Canvas(24, 26)
    c.rect(11, 8, 3, 18, WOOD_D)
    c.rect(1, 3, 22, 9, WOOD_L)
    c.hline(1, 3, 22, WOOD_H)
    c.hline(1, 11, 22, WOOD_M)
    if arrow:
        c.rect(22, 5, 2, 5, WOOD_L)
    for x in range(4, 18, 3):
        c.hline(x, 7, 2, WOOD_D)
    c.outline()
    return c


def torch_post(frame):
    c = Canvas(10, 30)
    c.rect(4, 8, 2, 22, WOOD_D)
    c.rect(2, 7, 6, 3, STONE_D)
    rng = random.Random(frame + 9)
    for i in range(14):
        x = 5 + rng.randint(-2, 1)
        y = 6 - rng.randint(0, 5)
        c.px(x, y, FIRE_1 if y < 3 else FIRE_2)
    c.px(5, 6, FIRE_3)
    c.outline()
    return c


def stall():
    c = Canvas(40, 38)
    c.rect(3, 12, 2, 24, WOOD_D)
    c.rect(35, 12, 2, 24, WOOD_D)
    for x in range(0, 40):
        col = CLOTH_R if (x // 5) % 2 else CREAM
        c.rect(x, 4, 1, 9, col)
        c.px(x, 13 + (x % 5 == 2), col)
    c.hline(0, 4, 40, lighten(CLOTH_R, 0.2))
    c.rect(2, 24, 36, 5, WOOD_L)
    c.hline(2, 24, 36, WOOD_H)
    c.rect(2, 29, 36, 7, WOOD_M)
    for i, col in enumerate([(220, 70, 60), (230, 180, 70), (120, 170, 80), (220, 70, 60), (180, 110, 60)]):
        c.ellipse(7 + i * 6, 23, 2.5, 2, col)
    c.outline()
    return c


def anvil():
    c = Canvas(18, 14)
    c.rect(6, 7, 6, 6, STONE_D)
    c.rect(2, 3, 14, 4, STONE_M)
    c.rect(0, 3, 4, 2, STONE_M)
    c.hline(2, 3, 14, STONE_H)
    c.rect(4, 12, 10, 2, STONE_D)
    c.outline()
    return c


def woodpile():
    c = Canvas(28, 18)
    for row in range(3):
        for i in range(4 - (row == 2)):
            x = 4 + i * 6 + row * 3
            y = 13 - row * 5
            c.ellipse(x, y, 3, 2.5, WOOD_M)
            c.ellipse(x, y, 1.5, 1.2, WOOD_L)
    c.outline()
    return c


def hay():
    c = Canvas(20, 16)
    c.rect(1, 3, 18, 12, (210, 176, 90))
    for x in range(1, 19, 2):
        c.vline(x, 3, 12, (184, 148, 70))
    c.hline(1, 3, 18, (234, 204, 120))
    c.hline(1, 8, 18, WOOD_D)
    c.outline()
    return c


def shrine(frame):
    c = Canvas(26, 42)
    for yy in range(4, 40):
        half = 9 - max(0, (10 - yy)) // 2
        for xx in range(13 - half, 13 + half):
            col = STONE_M
            if xx < 13 - half + 2:
                col = STONE_L
            if xx > 13 + half - 3:
                col = STONE_D
            if fbm(xx, yy, 6, 8) > 0.64:
                col = mix(col, (70, 110, 70), 0.5)
            c.px(xx, yy, col)
    glow = [(150, 220, 255), (190, 240, 255)][frame % 2]
    dim = (90, 150, 190)
    for (x, y) in ((11, 12), (12, 12), (13, 13), (14, 14), (11, 16), (13, 17), (15, 18), (12, 21), (13, 21),
                   (14, 22), (11, 25), (12, 26), (14, 26), (13, 29), (12, 30), (14, 31)):
        c.px(x, y, glow if (x + y + frame) % 3 else dim)
    c.rect(3, 38, 20, 4, STONE_D)
    c.outline()
    return c


def moonflower():
    c = Canvas(8, 8)
    for (x, y) in ((3, 1), (1, 3), (5, 3), (3, 5)):
        c.px(x, y, (220, 236, 255))
        c.px(x + 1, y, (220, 236, 255))
    c.px(3, 3, (250, 240, 150))
    c.px(4, 3, (250, 240, 150))
    return c


# ---------------------------------------------------------------- Lobo
WOLF = (116, 110, 118)
WOLF_D = (78, 72, 86)
WOLF_L = (166, 158, 156)
EYE = (250, 170, 60)


def wolf_small(frame):
    c = Canvas(28, 20)
    by = 7 + (frame % 2)
    c.ellipse(13, by + 3, 9, 4.5, WOLF)
    c.ellipse(13, by + 5, 7, 2.5, WOLF_L)
    for i in range(12):
        c.px(6 + i, by - 1 + (i % 2), WOLF_D)
    for (x, y) in ((9, 2), (10, 3), (11, 3), (12, 4), (15, 2), (16, 3), (14, 4)):
        c.px(x, by + y, SAP)
    # patas
    for lx, off in ((6, 0), (9, 1), (17, 1), (20, 0)):
        ly = by + 6
        c.rect(lx, ly, 2, 5 - ((frame + off) % 2), WOLF_D)
    # cabeza
    c.ellipse(23, by, 4, 3.5, WOLF)
    c.rect(25, by + 1, 3, 2, WOLF_L)
    c.px(27, by + 1, OUT)
    c.px(21, by - 4, WOLF_D)
    c.px(22, by - 4, WOLF)
    c.px(24, by - 4, WOLF_D)
    c.px(24, by - 1, EYE)
    # cola
    for i in range(6):
        c.px(4 - i // 2, by + 1 - i // 2, WOLF_D if i % 2 else WOLF)
    c.outline()
    return c


def wolf_big(frame):
    c = Canvas(96, 64)
    b = frame % 2
    by = 26 + b
    # cola tupida, caída
    for i in range(16):
        t = i / 15.0
        x = 20 - i * 1.0
        y = by + 2 + t * t * 18
        r = 4.5 - t * 1.5
        c.ellipse(x, y, r, r, WOLF_D if i > 10 else WOLF)
    c.ellipse(6, by + 20, 3, 3, WOLF_L)
    # patas traseras (anca + corvejón)
    for (hx, dark) in ((26, True), (32, False)):
        col = WOLF_D if dark else WOLF
        c.ellipse(hx, by + 13, 8, 9, col)
        for k in range(12):
            c.rect(hx - 3 + (k // 4), by + 20 + k, 4, 1, col)
        c.rect(hx - 1, by + 32, 6, 3, WOLF_D)
    # cuerpo esbelto
    c.ellipse(46, by + 8, 24, 10, WOLF_D)
    c.ellipse(46, by + 6, 22, 9, WOLF)
    c.ellipse(50, by + 13, 16, 3.5, WOLF_L)
    # patas delanteras
    for (fx, dark) in ((58, True), (64, False)):
        col = WOLF_D if dark else WOLF
        c.ellipse(fx, by + 12, 5, 6, col)
        c.rect(fx - 2, by + 14, 4, 18, col)
        c.vline(fx - 1, by + 16, 14, lighten(col, 0.15))
        c.rect(fx - 3, by + 32, 7, 3, WOLF_D)
    # lomo erizado
    for i in range(20):
        x = 26 + i * 2
        h = 3 + (i % 3)
        for k in range(h):
            c.px(x + k // 2, by - 2 - k + (i % 2), WOLF_D if k < h - 1 else WOLF)
    # venas de savia negra (corrupción)
    rng = random.Random(3)
    for start in ((32, by + 1), (42, by), (52, by + 2), (60, by + 1)):
        x, y = start
        for k in range(10):
            c.px(x, y, SAP if k < 7 else SAP_L)
            x += rng.choice([-1, 0, 1])
            y += 1
            if rng.random() > 0.7:
                c.px(x + 1, y, SAP)
    # cuello y cabeza agachada, amenazante
    c.ellipse(70, by + 2, 9, 9, WOLF)
    c.ellipse(78, by + 2, 11, 9, WOLF_D)
    c.ellipse(77, by + 1, 10, 8, WOLF)
    c.ellipse(87, by + 5, 8, 4.5, WOLF)
    c.ellipse(88, by + 7, 7, 2.5, WOLF_L)
    c.rect(93, by + 3, 3, 3, OUT)
    c.hline(82, by + 9, 13, OUT)
    for fx in (85, 88, 91):
        c.px(fx, by + 10, WHITE)
    # orejas hacia atrás
    for (ex, col) in ((70, WOLF_D), (76, WOLF)):
        for k in range(8):
            c.hline(ex - k // 2, by - 5 - k, 5 - k // 2, col)
    c.px(73, by - 8, (160, 100, 100))
    # ojo encendido
    c.rect(81, by - 2, 4, 2, EYE)
    c.px(84, by - 2, (255, 236, 160))
    c.hline(80, by - 3, 6, OUT)
    for (x, y) in ((74, by + 4), (73, by + 5), (72, by + 7)):
        c.px(x, y, SAP)
    c.outline()
    return c


def sap_puddle():
    c = Canvas(28, 12)
    c.ellipse(14, 6, 13, 5, SAP)
    c.ellipse(10, 5, 4, 2, SAP_L)
    c.ellipse(20, 7, 3, 1.5, (50, 28, 58))
    return c


def sheet(frames):
    w, h = frames[0].w, frames[0].h
    s = Canvas(w * len(frames), h)
    for i, f in enumerate(frames):
        s.paste(f, i * w, 0)
    return s


def build_all(out):
    d = f"{out}/sprites"
    items = {
        "bed": bed(), "nightstand": nightstand(), "wardrobe": wardrobe(), "chest": chest(),
        "bookshelf": bookshelf(), "table": table(), "table_food": table(True), "chair": chair(),
        "barrel": barrel(), "crate": crate(), "plant": plant(), "sack": sack(), "stool": stool(), "rail": rail(),
        "house_red": house(seed=1), "house_blue": house((ROOF_B_D, ROOF_B_M, ROOF_B_L), seed=2),
        "house_brown": house(((92, 64, 44), (128, 92, 60), (160, 122, 80)), w=56, seed=3),
        "house_smithy": house(((70, 70, 80), (96, 96, 108), (130, 130, 140)), w=72, seed=4, sign=STONE_D),
        "cathedral": cathedral(), "cathedral_open": cathedral(True),
        "oak": oak(1), "oak2": oak(2), "pine": pine(3), "pine2": pine(4),
        "dark_tree": dark_tree(5), "dark_tree2": dark_tree(6),
        "bush": bush(1), "bush_berries": bush(2, True), "rock": rock(), "rock_big": rock(True), "stump": stump(),
        "fence_h": fence("h"), "fence_v": fence("v"), "well": well(), "sign": signpost(),
        "stall": stall(), "anvil": anvil(), "woodpile": woodpile(), "hay": hay(),
        "moonflower": moonflower(), "sap_puddle": sap_puddle(),
    }
    for name, cv in items.items():
        cv.save(f"{d}/{name}.png")
    sheet([fireplace(i) for i in range(3)]).save(f"{d}/fireplace.png")
    sheet([torch_post(i) for i in range(3)]).save(f"{d}/torch.png")
    sheet([shrine(i) for i in range(2)]).save(f"{d}/shrine.png")
    sheet([wolf_small(i) for i in range(2)]).save(f"{d}/wolf.png")
    sheet([wolf_big(i) for i in range(2)]).save(f"{d}/wolf_big.png")
