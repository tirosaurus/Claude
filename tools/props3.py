"""Objetos del Acto 4: minas de Khazgurim, pantano de Selen y paso del sur."""
import random
from artlib import *
from props import sheet


def minecart():
    c = Canvas(28, 22)
    c.rect(3, 6, 22, 10, (96, 70, 52))
    c.rect(3, 6, 22, 2, (130, 98, 70))
    c.rect(2, 5, 24, 2, (70, 70, 84))
    for x in (6, 12, 18):
        c.vline(x, 8, 7, (78, 56, 40))
    c.rect(5, 3, 18, 3, (60, 54, 60))            # mineral encima
    for (x, y, col) in ((8, 3, (180, 160, 90)), (14, 2, (200, 220, 240)), (19, 3, (150, 100, 60))):
        c.px(x, y, col)
        c.px(x + 1, y, col)
    for x in (7, 21):
        c.ellipse(x, 17, 3, 3, (50, 50, 58))
        c.px(x, 17, (140, 140, 150))
    c.outline(OUT)
    return c


def ore_rock(gem=(120, 200, 255)):
    c = Canvas(24, 18)
    c.ellipse(12, 11, 11, 7, (86, 78, 76))
    c.ellipse(10, 9, 7, 5, (116, 106, 100))
    for (x, y) in ((6, 10), (14, 7), (17, 12), (9, 13)):
        c.rect(x, y, 2, 2, gem)
        c.px(x, y, lighten(gem, 0.5))
    c.outline(OUT)
    return c


def mine_beam():
    c = Canvas(40, 40)
    for x in (3, 33):
        c.rect(x, 6, 4, 34, WOOD_M)
        c.vline(x, 6, 34, WOOD_D)
    c.rect(1, 3, 38, 5, WOOD_L)
    c.hline(1, 7, 38, WOOD_D)
    c.px(20, 9, (255, 220, 120))
    c.rect(19, 8, 3, 3, (255, 200, 90))       # farolillo
    c.outline(OUT)
    return c


def dead_tree(seed=0):
    rnd = random.Random(seed)
    c = Canvas(40, 56)
    col, cold = (70, 60, 50), (44, 36, 30)
    c.rect(18, 20, 5, 36, col)
    c.vline(22, 20, 36, cold)
    for k in range(5):
        x0, y0 = 20, 22 + k * 5
        dx = rnd.choice([-1, 1]) * rnd.randint(6, 14)
        for i in range(abs(dx)):
            c.px(x0 + (i if dx > 0 else -i), y0 - i // 2, col)
    c.ellipse(20, 54, 8, 2, cold)
    c.outline(OUT)
    return c


def reeds(seed=0):
    rnd = random.Random(seed)
    c = Canvas(18, 20)
    for k in range(7):
        x = 2 + k * 2 + rnd.randint(0, 1)
        h = rnd.randint(9, 17)
        c.vline(x, 20 - h, h, (96, 120, 60) if k % 2 else (70, 96, 50))
        if k % 3 == 0:
            c.rect(x, 20 - h, 1, 3, (110, 76, 46))
    return c


def mirror(lit):
    c = Canvas(18, 30)
    c.rect(4, 22, 10, 7, (80, 110, 120))
    c.rect(4, 22, 10, 2, (130, 170, 180))
    c.rect(8, 12, 2, 10, (70, 100, 110))
    c.ellipse(9, 8, 6, 7, (60, 90, 110) if not lit else (160, 240, 255))
    c.ellipse(9, 8, 4, 5, (90, 130, 150) if not lit else (230, 255, 255))
    if lit:
        c.px(7, 5, (255, 255, 255))
    c.outline(OUT)
    return c


def cage(full):
    c = Canvas(30, 34)
    if full:
        c.ellipse(15, 22, 5, 8, (120, 90, 70))
        c.ellipse(15, 12, 4, 4, SKIN)
        c.rect(12, 9, 7, 2, (80, 56, 40))
    for x in range(3, 28, 5):
        c.vline(x, 4, 28, (70, 66, 72))
    c.rect(1, 2, 28, 3, (96, 70, 52))
    c.rect(1, 30, 28, 3, (96, 70, 52))
    c.outline(OUT)
    return c


def palisade():
    c = Canvas(48, 34)
    for i in range(8):
        x = 2 + i * 6
        c.rect(x, 6, 5, 28, WOOD_M)
        c.vline(x + 4, 6, 28, WOOD_D)
        c.px(x + 2, 2, WOOD_L)
        c.rect(x + 1, 3, 3, 3, WOOD_L)
    c.hline(0, 16, 48, (60, 44, 34))
    c.outline(OUT)
    return c


def war_banner():
    c = Canvas(18, 44)
    c.rect(3, 2, 2, 42, WOOD_D)
    c.rect(5, 4, 11, 20, (130, 24, 30))
    for y in range(24, 28):
        c.hline(5 + (y - 24), y, 11 - 2 * (y - 24), (130, 24, 30))
    c.ellipse(10, 12, 3, 3, (20, 14, 20))
    c.px(10, 12, (200, 60, 70))
    c.outline(OUT)
    return c


def totem():
    c = Canvas(20, 44)
    c.rect(8, 10, 4, 34, WOOD_D)
    c.ellipse(10, 8, 6, 6, (226, 220, 200))       # calavera
    c.rect(7, 6, 2, 2, (20, 14, 20))
    c.rect(11, 6, 2, 2, (20, 14, 20))
    c.hline(8, 11, 5, (20, 14, 20))
    for k in range(3):
        c.px(4 + k, 18 + k * 4, (130, 24, 30))
        c.px(15 - k, 18 + k * 4, (130, 24, 30))
    c.outline(OUT)
    return c


def forge(frame):
    c = Canvas(34, 30)
    c.rect(2, 10, 30, 20, (80, 74, 80))
    c.rect(2, 10, 30, 3, (120, 114, 120))
    c.rect(9, 16, 16, 10, (30, 18, 16))
    fl = [(255, 200, 90), (255, 150, 60), (230, 90, 40)]
    for k in range(6):
        x = 11 + k * 2
        h = 3 + (k + frame) % 4
        c.vline(x, 25 - h, h, fl[(k + frame) % 3])
    c.rect(12, 2, 10, 8, (70, 66, 72))
    c.outline(OUT)
    return c


def thorn_wall():
    c = Canvas(96, 52)
    rnd = random.Random(9)
    for k in range(60):
        x0 = rnd.randint(0, 95)
        y0 = rnd.randint(8, 51)
        L = rnd.randint(6, 16)
        dx = rnd.choice([-1, 1])
        for i in range(L):
            c.px(x0 + dx * i, y0 - i, (60 + i * 3, 30, 60 + i * 2))
            if i % 4 == 2:
                c.px(x0 + dx * i + 1, y0 - i - 1, (200, 180, 200))
    c.outline(OUT)
    return c


def tear(kind):
    cols = {"stone": (170, 150, 120), "crystal": (160, 230, 255), "blood": (220, 60, 70)}
    c = Canvas(12, 16)
    col = cols[kind]
    for y in range(14):
        half = int(min(y, 13 - y + 4) * 0.45) + (0 if y < 3 else 1)
        c.hline(6 - half, 1 + y, half * 2, col if y > 2 else lighten(col, 0.3))
    c.px(5, 5, (255, 255, 255))
    c.outline(OUT)
    return c


def choco_icon():
    c = Canvas(16, 16)
    c.rect(2, 3, 12, 11, (40, 22, 12))
    c.rect(3, 4, 10, 9, (96, 54, 28))
    for (x, y) in ((3, 4), (8, 4), (3, 9), (8, 9)):
        c.rect(x, y, 4, 4, (120, 70, 38))
        c.px(x, y, (160, 100, 60))
    c.rect(2, 9, 12, 5, (200, 40, 50))      # envoltorio rojo
    c.hline(2, 9, 12, (240, 90, 90))
    c.px(7, 11, (250, 220, 120))
    c.px(8, 11, (250, 220, 120))
    c.outline(OUT)
    return c


def build_all(out):
    choco_icon().save(f"{out}/ui/it_choco.png")
    d = f"{out}/sprites"
    minecart().save(f"{d}/minecart.png")
    ore_rock().save(f"{d}/ore_rock.png")
    ore_rock((230, 190, 90)).save(f"{d}/ore_gold.png")
    mine_beam().save(f"{d}/mine_beam.png")
    dead_tree(1).save(f"{d}/dead_tree.png")
    dead_tree(5).save(f"{d}/dead_tree2.png")
    reeds(2).save(f"{d}/reeds.png")
    mirror(False).save(f"{d}/mirror_off.png")
    mirror(True).save(f"{d}/mirror_on.png")
    cage(True).save(f"{d}/cage_full.png")
    cage(False).save(f"{d}/cage_empty.png")
    palisade().save(f"{d}/palisade.png")
    war_banner().save(f"{d}/war_banner.png")
    totem().save(f"{d}/totem.png")
    sheet([forge(f) for f in range(3)]).save(f"{d}/forge.png")
    thorn_wall().save(f"{d}/thorn_wall.png")
    for k in ("stone", "crystal", "blood"):
        tear(k).save(f"{out}/ui/it_tear_{k}.png")
