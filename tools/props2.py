"""Props y enemigos de los actos 2-4: catedral, cripta, bosque corrupto,
campamento élfico, pueblo en llamas y criaturas de la savia."""
import random
from artlib import *
from props import wood_plank_fill, sheet

CORR_D = (40, 22, 48)
CORR_M = (70, 40, 86)
CORR_L = (112, 66, 130)
CORR_GLOW = (220, 120, 255)
CRYSTAL = (150, 220, 255)
CRYSTAL_D = (80, 140, 200)
CRYSTAL_L = (220, 246, 255)


def stone_block(xx, yy, base=STONE_M, seed=0, bw=8, bh=5):
    sx = xx + (bw // 2 if (yy // bh) % 2 else 0)
    if yy % bh == bh - 1 or sx % bw == bw - 1:
        return darken(base, 0.3)
    n = hash2(sx // bw, yy // bh, seed)
    if n > 0.7:
        return lighten(base, 0.1)
    if n < 0.2:
        return darken(base, 0.1)
    return base


# ------------------------------------------------------------ Catedral
def pew():
    c = Canvas(40, 20)
    c.rect(1, 2, 38, 7, WOOD_D)
    wood_plank_fill(c, 2, 3, 36, 5, WOOD_M, seed=31)
    c.hline(2, 3, 36, WOOD_L)
    c.rect(1, 10, 38, 5, WOOD_M)
    c.hline(1, 10, 38, WOOD_L)
    for x in (2, 36):
        c.rect(x, 9, 2, 10, WOOD_D)
    c.outline()
    return c


def pillar(broken=False):
    c = Canvas(24, 64 if not broken else 40)
    h = c.h
    for yy in range(4, h - 4):
        for xx in range(4, 20):
            col = STONE_M
            if xx < 7:
                col = STONE_L
            elif xx > 16:
                col = STONE_D
            if (xx - 4) % 4 == 0:
                col = darken(col, 0.15)
            if fbm(xx, yy, 8, 7) > 0.66:
                col = mix(col, (80, 110, 70), 0.4)
            c.px(xx, yy, col)
    c.rect(1, h - 6, 22, 6, STONE_M)
    c.hline(1, h - 6, 22, STONE_L)
    if not broken:
        c.rect(1, 0, 22, 6, STONE_L)
        c.hline(1, 5, 22, STONE_D)
    else:
        for x in range(4, 20):
            c.px(x, 4 + int(3 * math.sin(x * 1.3)), STONE_D)
    c.outline()
    return c


def altar():
    c = Canvas(48, 34)
    for yy in range(8, 34):
        for xx in range(2, 46):
            c.px(xx, yy, stone_block(xx, yy, STONE_L, 3))
    c.rect(0, 6, 48, 4, STONE_H)
    c.rect(4, 12, 40, 2, GOLD_D)
    c.rect(8, 10, 32, 18, (120, 30, 40))
    c.rect(8, 10, 32, 2, (160, 50, 60))
    for x in range(10, 38, 4):
        c.px(x, 26, GOLD)
    c.rect(22, 0, 4, 8, GOLD)
    c.rect(19, 2, 10, 2, GOLD)
    c.outline()
    return c


def statue(broken=False):
    c = Canvas(24, 52)
    c.rect(2, 40, 20, 12, STONE_D)
    c.rect(3, 40, 18, 3, STONE_L)
    body = STONE_L
    c.ellipse(12, 26, 6, 13, body)
    c.rect(6, 28, 12, 12, body)
    for yy in range(18, 40):
        c.px(8, yy, STONE_M)
    if not broken:
        c.ellipse(12, 10, 4.5, 5, body)
        c.rect(6, 4, 12, 2, STONE_M)
    else:
        c.rect(9, 12, 6, 2, STONE_D)
    c.rect(3, 22, 3, 12, body)
    c.rect(18, 20, 3, 8, body)
    c.outline()
    return c


def candelabra(frame):
    c = Canvas(16, 34)
    c.rect(7, 8, 2, 24, GOLD_D)
    c.rect(4, 30, 8, 3, GOLD_D)
    c.hline(2, 12, 12, GOLD)
    for x in (2, 7, 13):
        c.rect(x, 6, 2, 6, CREAM)
        f = (frame + x) % 3
        c.px(x, 4 - (f == 1), FIRE_1)
        c.px(x + 1, 5, FIRE_2)
        c.px(x, 5, FIRE_1)
    c.outline()
    return c


def rune_pedestal(lit):
    c = Canvas(20, 28)
    for yy in range(6, 28):
        for xx in range(3, 17):
            c.px(xx, yy, stone_block(xx, yy, STONE_M, 5, 6, 4))
    c.rect(1, 4, 18, 4, STONE_L)
    c.ellipse(10, 4, 7, 3, STONE_H)
    glow = (140, 230, 255) if lit else (80, 90, 110)
    for (x, y) in ((9, 12), (10, 12), (8, 14), (11, 14), (9, 16), (10, 16), (10, 18), (9, 20), (10, 20)):
        c.px(x, y, glow)
    if lit:
        c.ellipse(10, 3, 4, 2, (200, 250, 255))
    c.outline()
    return c


def rubble():
    c = Canvas(40, 22)
    rng = random.Random(14)
    for i in range(22):
        c.ellipse(rng.randint(4, 36), rng.randint(8, 18), rng.randint(3, 6), rng.randint(2, 4),
                  rng.choice([STONE_M, STONE_L, STONE_D]))
    c.outline()
    return c


def sarcophagus():
    c = Canvas(24, 40)
    for yy in range(2, 38):
        for xx in range(2, 22):
            c.px(xx, yy, stone_block(xx, yy, STONE_M, 8, 6, 6))
    c.rect(4, 4, 16, 30, STONE_L)
    c.ellipse(12, 10, 4, 4, STONE_M)
    c.rect(9, 14, 6, 16, STONE_M)
    c.hline(4, 4, 16, STONE_H)
    c.outline()
    return c


def bones():
    c = Canvas(20, 10)
    c.rect(2, 5, 12, 2, CREAM)
    c.ellipse(15, 4, 3.5, 3, CREAM)
    c.px(14, 4, OUT)
    c.px(16, 4, OUT)
    c.rect(4, 2, 2, 7, CREAM_S)
    return c


def crystal(frame):
    c = Canvas(24, 32)
    glow = [CRYSTAL, CRYSTAL_L][frame % 2]
    for (cx, h, w) in ((12, 28, 5), (6, 18, 3), (18, 20, 3)):
        for yy in range(32 - h, 31):
            half = max(1, int(w * (1 - (32 - yy) / h * 0.3)))
            if yy < 32 - h + 4:
                half = max(1, yy - (32 - h))
            for xx in range(cx - half, cx + half):
                c.px(xx, yy, CRYSTAL_D if xx > cx else glow)
        c.px(cx - 1, 32 - h + 3, WHITE)
    c.outline()
    return c


def seed_pedestal(frame, with_seed=True):
    c = Canvas(28, 40)
    for yy in range(14, 40):
        for xx in range(6, 22):
            c.px(xx, yy, stone_block(xx, yy, STONE_L, 9, 6, 4))
    c.rect(2, 12, 24, 4, STONE_H)
    if with_seed:
        g = [CRYSTAL, CRYSTAL_L][frame % 2]
        c.ellipse(14, 6, 5, 6, g)
        c.ellipse(13, 5, 2, 2, WHITE)
        c.px(14, 0, (120, 220, 140))
        c.px(15, -1, (120, 220, 140))
        for i in range(3):
            c.px(16 + i, 1 - i // 2, (120, 220, 140))
    c.outline()
    return c


# ------------------------------------------------------------ Bosque corrupto
def corrupt_tree(seed=0):
    c = Canvas(52, 64)
    rng = random.Random(seed + 70)
    c.rect(22, 34, 9, 29, (44, 30, 40))
    c.rect(23, 34, 3, 29, (66, 44, 60))
    for rx in (16, 34):
        c.rect(rx, 58, 5, 4, (44, 30, 40))
    blobs = [(26, 22, 20, 16), (14, 28, 11, 9), (38, 28, 11, 9), (26, 10, 14, 9)]
    for (x, y, rx, ry) in blobs:
        c.ellipse(x, y, rx, ry, CORR_D)
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 1, y - 2, rx - 3, ry - 3, CORR_M)
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 4, y - 5, rx * 0.4, ry * 0.35, CORR_L)
    for i in range(70):
        x, y = rng.randint(6, 46), rng.randint(2, 38)
        if c.get(x, y)[3]:
            c.px(x, y, rng.choice([CORR_L, CORR_D, (90, 50, 100)]))
    for i in range(8):
        x, y = rng.randint(10, 42), rng.randint(8, 34)
        if c.get(x, y)[3]:
            c.px(x, y, CORR_GLOW)
    # venas en el tronco
    for y in range(36, 62, 3):
        c.px(24 + (y % 5) // 2, y, SAP_L)
    c.outline()
    return c


def mushroom(frame):
    c = Canvas(16, 14)
    g = [(120, 230, 200), (170, 255, 230)][frame % 2]
    for (x, h, r) in ((5, 8, 4), (11, 5, 3)):
        c.rect(x - 1, 13 - h, 2, h, CREAM_S)
        c.ellipse(x, 13 - h, r, r * 0.6, g)
        c.px(x - 1, 12 - h, WHITE)
    c.outline()
    return c


def thorns():
    c = Canvas(28, 18)
    rng = random.Random(8)
    for i in range(7):
        x0 = rng.randint(2, 24)
        x, y = x0, 17
        for k in range(rng.randint(8, 15)):
            c.px(x, y, CORR_D)
            c.px(x + 1, y, (54, 30, 60))
            if k % 3 == 0:
                c.px(x + rng.choice([-1, 2]), y, (150, 120, 160))
            x += rng.choice([-1, 0, 1])
            y -= 1
    c.outline()
    return c


def campfire(frame):
    c = Canvas(24, 22)
    for i, (x, y) in enumerate(((5, 17), (12, 19), (19, 17), (8, 20), (16, 20))):
        c.ellipse(x, y, 3, 2, STONE_M)
    c.rect(6, 15, 12, 3, WOOD_D)
    c.rect(8, 13, 8, 2, WOOD_M)
    rng = random.Random(frame * 5 + 2)
    for i in range(28):
        fx = 8 + rng.randint(0, 8)
        fy = 14 - rng.randint(0, 9 - abs(fx - 12))
        c.px(fx, fy, FIRE_1 if fy < 9 else (FIRE_2 if fy < 12 else FIRE_3))
    c.outline()
    return c


def fire(frame):
    c = Canvas(28, 34)
    rng = random.Random(frame * 13 + 1)
    for i in range(110):
        fx = 4 + rng.randint(0, 20)
        spread = 14 - abs(fx - 14)
        fy = 33 - rng.randint(0, max(1, spread * 2 + rng.randint(0, 6)))
        col = FIRE_3 if fy > 28 else (FIRE_2 if fy > 20 else FIRE_1)
        c.px(fx, fy, col)
        c.px(fx, fy + 1, FIRE_3 if col != FIRE_1 else FIRE_2)
    return c


def broken_cart():
    c = Canvas(40, 26)
    wood_plank_fill(c, 4, 6, 30, 10, WOOD_M, seed=44)
    c.rect(4, 6, 30, 2, WOOD_L)
    for i in range(10):
        c.px(20 + i, 8 + i // 2, WOOD_D)
    c.ellipse(10, 19, 6, 6, WOOD_D)
    c.ellipse(10, 19, 4, 4, WOOD_M)
    c.ellipse(10, 19, 1.5, 1.5, WOOD_D)
    c.rect(28, 20, 10, 3, WOOD_D)
    c.rect(34, 2, 3, 8, WOOD_D)
    c.outline()
    return c


# ------------------------------------------------------------ Campamento élfico
def tent(color=(70, 120, 90)):
    c = Canvas(48, 40)
    d = darken(color, 0.3)
    l = lighten(color, 0.2)
    for yy in range(4, 38):
        half = int((yy - 4) * 0.66) + 2
        for xx in range(24 - half, 24 + half):
            col = color if xx < 24 else d
            if (xx - 24 + half) % 8 == 0:
                col = l
            c.px(xx, yy, col)
    for yy in range(22, 38):
        half = (yy - 22) // 3
        c.hline(24 - half, yy, half * 2 + 1, (30, 26, 30))
    c.vline(24, 0, 6, WOOD_D)
    c.px(25, 0, GOLD)
    c.outline()
    return c


def lantern_post(frame):
    c = Canvas(12, 36)
    c.rect(5, 6, 2, 30, WOOD_D)
    c.hline(3, 6, 6, WOOD_D)
    g = [(250, 230, 150), (255, 246, 190)][frame % 2]
    c.rect(2, 8, 5, 7, (80, 60, 40))
    c.rect(3, 9, 3, 5, g)
    c.outline()
    return c


def sacred_tree():
    W, H = 120, 140
    c = Canvas(W, H)
    rng = random.Random(90)
    for yy in range(70, H - 4):
        spread = 10 + (yy - 70) // 5
        for xx in range(60 - spread, 60 + spread):
            col = (98, 74, 56) if abs(xx - 60) < spread - 3 else (70, 52, 42)
            if (xx * 3 + yy) % 11 == 0:
                col = (120, 92, 70)
            c.px(xx, yy, col)
    for k in range(6):
        x = 60 + (k - 3) * 10
        for yy in range(H - 12, H):
            c.hline(x - 3 + (yy - H + 12) * (k - 3) // 3, yy, 6, (80, 60, 46))
    blobs = [(60, 44, 50, 34), (30, 58, 24, 18), (90, 58, 24, 18), (60, 20, 34, 18), (40, 30, 20, 14), (82, 30, 20, 14)]
    for (x, y, rx, ry) in blobs:
        c.ellipse(x, y, rx, ry, (40, 96, 70))
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 2, y - 3, rx - 4, ry - 4, (62, 132, 84))
    for (x, y, rx, ry) in blobs:
        c.ellipse(x - 6, y - 7, rx * 0.4, ry * 0.35, (104, 170, 100))
    for i in range(60):
        x, y = rng.randint(14, 106), rng.randint(6, 76)
        if c.get(x, y)[3]:
            c.px(x, y, rng.choice([(200, 240, 200), (250, 230, 150), (130, 190, 110)]))
    c.outline()
    return c


def dummy():
    c = Canvas(16, 30)
    c.rect(7, 10, 2, 20, WOOD_D)
    c.rect(2, 12, 12, 3, WOOD_M)
    c.ellipse(8, 7, 5, 6, (210, 176, 90))
    c.rect(4, 14, 8, 10, (196, 164, 84))
    c.hline(4, 18, 8, CLOTH_R)
    c.outline()
    return c


# ------------------------------------------------------------ Enemigos
def larva(frame):
    c = Canvas(40, 30)
    b = frame % 2
    for i in range(5):
        x = 8 + i * 6
        y = 18 + (1 if (i + b) % 2 else 0)
        c.ellipse(x, y, 7 - i * 0.6, 6 - i * 0.5, CORR_M if i else CORR_L)
    c.ellipse(8, 17, 7, 6, CORR_L)
    c.ellipse(6, 15, 2.5, 2, CORR_GLOW)
    c.px(6, 15, WHITE)
    c.ellipse(11, 14, 1.5, 1.5, CORR_GLOW)
    c.hline(3, 21, 6, SAP)
    for i in range(4):
        c.px(12 + i * 6, 24, SAP_L)
    c.outline()
    return c


def bat(frame):
    c = Canvas(40, 30)
    up = frame % 2 == 0
    c.ellipse(20, 15, 6, 6, (70, 60, 76))
    c.ellipse(20, 17, 4, 4, (100, 88, 104))
    for side in (-1, 1):
        for i in range(14):
            y = 12 + (i // 3 if up else -i // 4) + (0 if up else 4)
            for k in range(6 - i // 3):
                c.px(20 + side * (6 + i), y + k, (56, 46, 62) if k else (90, 76, 96))
    c.px(18, 14, (200, 255, 120))
    c.px(22, 14, (200, 255, 120))
    for i in range(6):
        c.px(14 + i * 2, 24 + (i % 2), (150, 200, 90))
    c.outline()
    return c


def brute(frame):
    c = Canvas(72, 80)
    b = frame % 2
    c.rect(20, 56, 12, 22, CORR_D)
    c.rect(40, 56, 12, 22, CORR_D)
    c.ellipse(36, 44 + b, 26, 22, CORR_M)
    c.ellipse(34, 40 + b, 20, 16, CORR_L)
    c.ellipse(36, 18 + b, 13, 12, CORR_M)
    c.ellipse(34, 16 + b, 9, 8, CORR_L)
    c.rect(28, 16 + b, 4, 3, CORR_GLOW)
    c.rect(38, 16 + b, 4, 3, CORR_GLOW)
    c.hline(30, 24 + b, 10, SAP)
    for x in (31, 35, 39):
        c.px(x, 25 + b, CREAM)
    for (ax, d) in ((10, -1), (62, 1)):
        c.ellipse(ax, 42 + b, 8, 14, CORR_M)
        c.ellipse(ax + d * 2, 58 + b, 9, 7, CORR_D)
    for i in range(14):
        c.px(20 + i * 2, 36 + (i % 3) + b, SAP)
    for (x, y) in ((24, 30), (48, 34), (30, 52), (44, 50)):
        c.ellipse(x, y + b, 2, 2, CORR_GLOW)
    c.outline()
    return c


def root_creature(frame):
    c = Canvas(56, 52)
    b = frame % 2
    rng = random.Random(4)
    for k in range(7):
        x = 8 + k * 7
        for y in range(50, 18 + (k % 3) * 4 + b, -1):
            x += rng.choice([-1, 0, 1]) if y % 3 == 0 else 0
            c.rect(x, y, 4 - (50 - y) // 14, 1, (66, 48, 44) if k % 2 else (84, 60, 50))
    c.ellipse(28, 24 + b, 12, 10, (84, 60, 50))
    c.ellipse(24, 22 + b, 2, 2, CORR_GLOW)
    c.ellipse(32, 22 + b, 2, 2, CORR_GLOW)
    c.hline(24, 28 + b, 9, SAP)
    for i in range(6):
        c.px(18 + i * 4, 14 + (i % 2) + b, LEAF_D)
    c.outline()
    return c


def spectre(frame):
    c = Canvas(44, 54)
    b = frame % 2
    for yy in range(6, 50):
        half = 12 - max(0, (yy - 30)) // 3
        wave = int(2 * math.sin(yy * 0.5 + b * 2))
        for xx in range(22 - half + wave, 22 + half + wave):
            a = 200 if yy < 40 else max(40, 200 - (yy - 40) * 18)
            col = CRYSTAL if xx < 22 + wave else CRYSTAL_D
            c.px(xx, yy, col, a)
    c.ellipse(22, 14, 8, 8, CRYSTAL_L, 230)
    c.rect(17, 12, 3, 4, (20, 40, 80))
    c.rect(24, 12, 3, 4, (20, 40, 80))
    c.ellipse(22, 20, 2, 3, (20, 40, 80))
    for (x, y) in ((8, 28), (36, 26), (12, 40), (32, 44)):
        c.px(x, y + b, WHITE)
    return c


def custodian(frame):
    c = Canvas(96, 110)
    b = frame % 2
    for yy in range(70, 108):
        for xx in (range(24, 40), range(56, 72)):
            for x in xx:
                c.px(x, yy, stone_block(x, yy, STONE_M, 12, 8, 6))
    for yy in range(26, 76):
        for xx in range(16, 80):
            if abs(xx - 48) < 32 - max(0, 40 - yy) // 3:
                c.px(xx, yy + b, stone_block(xx, yy, STONE_L, 13, 10, 7))
    c.ellipse(48, 46 + b, 10, 10, CRYSTAL_D)
    c.ellipse(48, 46 + b, 7, 7, [CRYSTAL, CRYSTAL_L][b])
    c.ellipse(46, 44 + b, 2, 2, WHITE)
    for yy in range(4, 28):
        for xx in range(34, 62):
            if abs(xx - 48) < 14 - max(0, 10 - yy) // 2:
                c.px(xx, yy + b, stone_block(xx, yy, STONE_L, 14, 7, 5))
    c.rect(38, 14 + b, 6, 3, CRYSTAL_L)
    c.rect(52, 14 + b, 6, 3, CRYSTAL_L)
    for (ax, d) in ((8, -1), (88, 1)):
        for yy in range(30, 80):
            for xx in range(ax - 8, ax + 8):
                c.px(xx, yy + b, stone_block(xx, yy, STONE_M, 15, 6, 6))
        for k in range(4):
            c.px(ax - 2 + k, 82 + b, CRYSTAL)
    for (x, y) in ((24, 30), (70, 36), (30, 62), (64, 60)):
        c.rect(x, y + b, 3, 3, CRYSTAL)
    c.outline()
    return c


def mother_root(frame):
    W, H = 176, 150
    c = Canvas(W, H)
    b = frame % 2
    rng = random.Random(77)
    for k in range(14):
        x = 10 + k * 12
        for y in range(H - 1, 30 + (k % 4) * 8, -1):
            x += rng.choice([-1, 0, 1]) if y % 4 == 0 else 0
            w = 6 - (H - y) // 30
            c.rect(x, y, max(2, w), 1, (58, 40, 50) if k % 2 else (76, 52, 62))
            if rng.random() > 0.97:
                c.px(x + 1, y, CORR_GLOW)
    pulse = 2 if b else 0
    c.ellipse(88, 70, 44 + pulse, 38 + pulse, CORR_D)
    c.ellipse(86, 66, 38 + pulse, 32 + pulse, CORR_M)
    c.ellipse(82, 60, 22, 18, CORR_L)
    for a in range(0, 360, 24):
        r = 30
        x = 88 + int(r * math.cos(math.radians(a)))
        y = 70 + int(r * 0.8 * math.sin(math.radians(a)))
        c.ellipse(x, y, 3, 3, SAP)
    c.ellipse(88, 64, 12, 12, (30, 10, 30))
    c.ellipse(88, 64, 8, 8, CORR_GLOW if b else (190, 90, 230))
    c.ellipse(86, 62, 3, 3, WHITE)
    for (x, y) in ((58, 50), (118, 52), (70, 94), (108, 92), (88, 34)):
        c.ellipse(x, y, 4, 3, (250, 180, 80))
        c.px(x, y, OUT)
    for i in range(30):
        x = rng.randint(40, 136)
        y = rng.randint(30, 110)
        if c.get(x, y)[3]:
            c.px(x, y, SAP_L)
    c.outline()
    return c


def build_all(out):
    d = f"{out}/sprites"
    items = {
        "pew": pew(), "pillar": pillar(), "pillar_broken": pillar(True), "altar": altar(), "statue": statue(),
        "statue_broken": statue(True), "rune_off": rune_pedestal(False), "rune_on": rune_pedestal(True),
        "rubble": rubble(), "sarcophagus": sarcophagus(), "bones": bones(), "thorns": thorns(),
        "broken_cart": broken_cart(), "tent_green": tent(), "tent_blue": tent((70, 90, 140)),
        "tent_red": tent((140, 60, 60)), "sacred_tree": sacred_tree(), "dummy": dummy(),
        "corrupt_tree": corrupt_tree(1), "corrupt_tree2": corrupt_tree(2),
        "seed_empty": seed_pedestal(0, False),
    }
    for name, cv in items.items():
        cv.save(f"{d}/{name}.png")
    anims = {
        "candelabra": [candelabra(i) for i in range(3)], "crystal": [crystal(i) for i in range(2)],
        "seed_pedestal": [seed_pedestal(i) for i in range(2)], "mushroom": [mushroom(i) for i in range(2)],
        "campfire": [campfire(i) for i in range(3)], "fire": [fire(i) for i in range(3)],
        "lantern": [lantern_post(i) for i in range(2)],
    }
    for name, frames in anims.items():
        sheet(frames).save(f"{d}/{name}.png")
    e = f"{out}/enemies"
    sheet([mother_root(0), mother_root(1)]).save(f"{d}/mother_root_map.png")
    for name, fn in (("larva", larva), ("bat", bat), ("brute", brute), ("root", root_creature),
                     ("spectre", spectre), ("custodian", custodian), ("mother_root", mother_root)):
        sheet([fn(0), fn(1)]).save(f"{e}/{name}.png")
