"""Acto 4: Las Tres Lágrimas. Montes y Minas de Khazgurim, Pantano de Selen,
Ruinas de Cristal y Paso del Sur."""
import random
from artlib import *
from maps import Map, T, flowers_decal
from maps2 import grid, to_rows, cave_mouth_decal, scorch_decal


def dungeon(w, h, rooms, exit_cols):
    g = grid(w, h, " ")
    for (x0, y0, x1, y1) in rooms:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if 0 < x < w - 1 and 0 < y < h - 1:
                    g[y][x] = "."
    for y in range(h):
        for x in range(w):
            if g[y][x] == ".":
                continue
            if any(0 <= y + dy < h and 0 <= x + dx < w and g[y + dy][x + dx] == "." for dx in (-1, 0, 1)
                   for dy in (-1, 0, 1, 2, 3)):
                g[y][x] = "#"
    for x in range(w):
        for y in range(1, h - 2):
            if g[y - 1][x] == "#" and g[y][x] == "." and g[y + 1][x] == ".":
                g[y][x] = "W"
                g[y + 1][x] = "W"
    for x in exit_cols:
        g[h - 1][x] = "."
    return g


def outdoor(mid, w, h, base, border, display, music, modulate):
    m = Map(mid, [base * w] * h, False, display, music, modulate=modulate)
    for y in range(h):
        for x in range(w):
            if x < 2 or x > w - 3 or y < 2 or y > h - 3:
                m.grid[y][x] = border
    return m


def scatter(m, rng, n, names, allowed, cols=True):
    for i in range(n):
        x = rng.randint(8, m.w * T - 8)
        y = rng.randint(16, m.h * T - 4)
        if m.t(x // T, y // T) in allowed:
            m.prop(rng.choice(names), x, y, col=cols)


# ------------------------------------------------------------ Montes de Khazgurim
def montes():
    W, H = 40, 26
    m = outdoor("montes", W, H, "k", "K", "Montes de Khazgurim", "mountain", [0.92, 0.95, 1.05])
    m.fill(0, 12, 22, 13, "p")
    m.fill(21, 4, 22, 13, "p")
    m.fill(22, 4, 30, 5, "p")
    # boca de la mina en la pared norte
    for x in range(27, 34):
        m.grid[2][x] = "k"
        m.grid[3][x] = "k"
    m.decals.append(cave_mouth_decal(30 * T + 8, 2 * T + 6))
    rng = random.Random(21)
    for (cx, cy, rx, ry) in ((10, 5, 4, 2), (33, 18, 4, 3), (14, 20, 5, 2)):
        for y in range(H):
            for x in range(W):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1:
                    m.grid[y][x] = "K"
    scatter(m, rng, 26, ["rock", "rock_big", "pine", "pine2"], "k")
    m.prop("minecart", 400, 110)
    m.prop("mine_beam", 488, 64, col=False)
    m.prop("sign", 330, 200, "sign_montes")
    m.prop("campfire", 150, 240, "campfire", light={"r": 90, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    m.prop("chest", 580, 380, "chest_montes1")
    m.encounters = {"rate": 30, "table": [["golem"], ["salamandra", "salamandra"], ["boar", "salamandra"],
                                          ["bat", "bat", "salamandra"], ["golem", "bat"]]}
    m.battle_bg = "forest"
    m.marker("from_camp", 24, 200)
    m.marker("from_mine", 488, 80)
    m.marker("durgan", 470, 96)
    m.exit([0, 188, 8, 36], "elf_camp", "from_montes")
    m.exit([27 * T, 0, 7 * T, 40], "minas", "from_montes")
    return m


# ------------------------------------------------------------ Minas de Khazgurim
def minas():
    w, h = 36, 34
    rooms = [(15, 26, 21, 32), (17, 18, 19, 26), (6, 16, 30, 19), (4, 8, 9, 16), (27, 8, 33, 16),
             (10, 9, 26, 11), (15, 2, 21, 9)]
    g = dungeon(w, h, rooms, range(16, 21))
    m = Map("minas", to_rows(g), True, "Minas de Khazgurim", "deep", modulate=[0.62, 0.55, 0.5])
    m.style = "mine"
    m.battle_bg = "crypt"
    m.encounters = {"rate": 28, "table": [["golem", "bat"], ["salamandra", "salamandra", "bat"], ["gusano_cria", "gusano_cria"],
                                          ["golem", "salamandra"], ["gusano_cria", "bat", "bat"]]}
    for (x, y) in ((280, 470), (120, 280), (470, 280), (200, 170), (380, 170), (290, 90)):
        m.prop("torch", x, y - 14, col=False, light={"r": 70, "color": [1, 0.7, 0.4], "e": 0.9, "flicker": True, "oy": -8})
    for (x, y) in ((96, 200), (500, 210), (150, 300), (450, 300), (260, 60)):
        m.prop("ore_rock", x, y, light={"r": 30, "color": [0.5, 0.8, 1], "e": 0.5, "oy": -6})
    m.prop("ore_gold", 520, 150)
    m.prop("minecart", 330, 300)
    m.prop("minecart", 160, 150)
    m.prop("forge", 110, 140, "forge", light={"r": 80, "color": [1, 0.6, 0.3], "e": 1.0, "flicker": True, "oy": -10})
    m.prop("anvil", 140, 150)
    m.prop("rubble", 488, 136, "trapped_dwarves")
    m.prop("chest", 90, 250, "chest_mina1")
    m.prop("chest", 510, 250, "chest_mina2")
    m.prop("chest", 300, 50, "chest_mina3")
    m.marker("from_montes", 288, 520)
    m.marker("worm", 288, 60)
    m.marker("dwarves", 488, 160)
    m.exit([16 * T, h * T - 8, 5 * T, 8], "montes", "from_mine")
    m.trigger("worm_zone", [15 * T, 9 * T, 7 * T, 8])
    return m


# ------------------------------------------------------------ Pantano de Selen
def pantano():
    W, H = 42, 30
    m = outdoor("pantano", W, H, "b", "f", "Pantano de Selen", "swamp", [0.72, 0.82, 0.74])
    rng = random.Random(33)
    for k in range(9):
        cx, cy = rng.randint(6, W - 7), rng.randint(6, H - 7)
        rx, ry = rng.uniform(2, 4.5), rng.uniform(1.5, 3)
        for y in range(H):
            for x in range(W):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1 and m.grid[y][x] == "b":
                    m.grid[y][x] = "B"
    # caminos: norte -> centro -> oeste (ruinas) y este (paso)
    m.fill(20, 0, 21, 16, "p")
    m.fill(2, 15, 39, 16, "p")
    m.fill(0, 15, 2, 16, "p")
    m.fill(39, 15, 41, 16, "p")
    scatter(m, rng, 40, ["dead_tree", "dead_tree2", "reeds", "reeds", "mushroom"], "b")
    scatter(m, rng, 30, ["dark_tree", "dark_tree2"], "f")
    for (x, y) in ((120, 200), (500, 330), (400, 120), (180, 400)):
        m.prop("mushroom", x, y, light={"r": 34, "color": [0.6, 1, 0.6], "e": 0.6, "flicker": True, "oy": -6})
    m.prop("campfire", 360, 280, "campfire", light={"r": 90, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    m.prop("statue_broken", 60, 220, "selen_statue")
    m.prop("chest", 620, 420, "chest_pantano1")
    m.prop("sign", 360, 230, "sign_pantano")
    m.encounters = {"rate": 30, "table": [["lodo", "lodo"], ["wisp", "lodo"], ["spectre", "lodo"], ["boar", "wisp"],
                                          ["lodo", "lodo", "wisp"]]}
    m.battle_bg = "heart"
    m.marker("from_camp", 328, 20)
    m.marker("from_ruins", 24, 256)
    m.marker("from_pass", W * T - 24, 256)
    m.exit([20 * T, 0, 2 * T, 8], "elf_camp", "from_pantano")
    m.exit([0, 15 * T, 8, 2 * T], "ruinas", "from_pantano")
    m.exit([W * T - 8, 15 * T, 8, 2 * T], "paso_sur", "from_pantano", requires="tear_crystal",
           blocked="Una niebla densa cubre el camino del este. Selen dice que antes debéis visitar sus ruinas.")
    return m


# ------------------------------------------------------------ Ruinas de Cristal
def ruinas():
    w, h = 36, 32
    rooms = [(15, 25, 21, 30), (6, 18, 30, 21), (3, 10, 10, 18), (26, 10, 33, 18), (16, 13, 20, 18),
             (11, 2, 25, 10)]
    g = dungeon(w, h, rooms, range(16, 21))
    m = Map("ruinas", to_rows(g), True, "Ruinas de Cristal", "deep", modulate=[0.55, 0.7, 0.8])
    m.style = "ruin"
    m.battle_bg = "crypt"
    m.encounters = {"rate": 26, "table": [["spectre", "wisp"], ["lodo", "spectre"], ["wisp", "wisp", "wisp"],
                                          ["custodian"], ["spectre", "spectre"]]}
    for (x, y) in ((110, 320), (470, 320), (290, 420), (230, 60), (350, 60)):
        m.prop("crystal", x, y, light={"r": 60, "color": [0.55, 0.85, 1], "e": 0.8, "flicker": True, "oy": -14})
    for (x, y) in ((140, 330), (440, 330), (200, 150), (380, 150)):
        m.prop("pillar_broken", x, y)
    m.prop("mirror_off", 90, 200, "mirror1")
    m.prop("mirror_off", 500, 200, "mirror2")
    m.prop("mirror_off", 288, 250, "mirror3")
    m.prop("chest", 70, 280, "chest_ruina1")
    m.prop("chest", 520, 280, "chest_ruina2")
    m.prop("chest", 220, 50, "chest_ruina3")
    m.prop("statue", 288, 60, "empress_throne")
    m.marker("from_pantano", 288, 480)
    m.marker("empress", 288, 90)
    m.exit([16 * T, h * T - 8, 5 * T, 8], "pantano", "from_ruins")
    m.trigger("empress_zone", [11 * T, 11 * T, 15 * T, 8])
    return m


# ------------------------------------------------------------ Paso del Sur
def paso_sur():
    W, H = 42, 28
    m = outdoor("paso_sur", W, H, "q", "K", "Paso del Sur", "war", [0.95, 0.72, 0.62])
    m.fill(0, 14, 30, 15, "p")
    rng = random.Random(55)
    for (cx, cy, rx, ry) in ((10, 6, 5, 2), (12, 22, 6, 2)):
        for y in range(H):
            for x in range(W):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1:
                    m.grid[y][x] = "K"
    scatter(m, rng, 18, ["dead_tree", "rock", "rock_big", "bones"], "q")
    for (x, y) in ((420, 120), (420, 360)):
        m.prop("palisade", x, y)
    for (x, y) in ((470, 90), (470, 400), (560, 180), (560, 320)):
        m.prop("war_banner", x, y)
    for (x, y) in ((380, 170), (380, 310)):
        m.prop("totem", x, y)
    m.prop("cage_full", 590, 150, "cage1")
    m.prop("cage_full", 620, 250, "cage2")
    m.prop("cage_full", 590, 350, "cage3")
    m.prop("campfire", 520, 250, col=False, light={"r": 110, "color": [1, 0.5, 0.25], "e": 1.1, "flicker": True, "oy": -8})
    m.prop("tent_red", 610, 60)
    m.prop("tent_red", 640, 420)
    m.prop("campfire", 100, 300, "campfire", light={"r": 90, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    m.prop("chest", 60, 60, "chest_paso1")
    m.prop("chest", 640, 150, "chest_paso2")
    m.decals += [scorch_decal(300, 200, 20), scorch_decal(200, 330, 16), scorch_decal(470, 250, 26)]
    m.encounters = {"rate": 28, "table": [["soldado", "soldado"], ["soldado", "thrall"], ["brute"], ["soldado", "salamandra"],
                                          ["soldado", "soldado", "thrall"]]}
    m.battle_bg = "heart"
    m.marker("from_pantano", 24, 236)
    m.marker("kraag", 540, 250)
    m.exit([0, 14 * T, 8, 2 * T], "pantano", "from_pass")
    m.trigger("kraag_zone", [440, 150, 12, 200])
    return m


MAPS = (montes, minas, pantano, ruinas, paso_sur)
