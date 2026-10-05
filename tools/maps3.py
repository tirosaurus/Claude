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
    # campamento enano junto a la mina
    m.prop("tent_blue", 400, 168)
    m.prop("tent_red", 560, 170)
    m.prop("forge", 600, 120, "dwarf_forge", light={"r": 80, "color": [1, 0.6, 0.3], "e": 1.0, "flicker": True, "oy": -10})
    m.prop("anvil", 570, 128)
    m.prop("barrel", 430, 120)
    m.prop("barrel", 444, 124)
    m.prop("crate", 520, 124)
    m.marker("hilda", 420, 196)
    m.marker("torbin", 590, 200)
    m.marker("bori", 360, 140)
    m.marker("kazrik", 520, 70)
    m.encounters = {"rate": 30, "table": [["golem"], ["salamandra", "salamandra"], ["boar", "salamandra"],
                                          ["bat", "bat", "salamandra"], ["golem", "bat"]]}
    m.battle_bg = "forest"
    m.marker("from_camp", 24, 200)
    m.marker("from_mine", 488, 80)
    m.marker("durgan", 470, 96)
    m.exit([0, 188, 8, 36], "sendero_montes", "from_east")
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
    m.exit([20 * T, 0, 2 * T, 8], "sendero_pantano", "from_south")
    m.exit([0, 15 * T, 8, 2 * T], "ruinas", "from_pantano")
    m.exit([W * T - 8, 15 * T, 8, 2 * T], "camino_paso", "from_west", requires="tear_crystal",
           blocked="Una niebla densa cubre el camino del este. Selen dice que antes debéis visitar sus ruinas.")
    return m


# ------------------------------------------------------------ Ruinas de Cristal
def ruinas():
    w, h = 36, 32
    rooms = [(15, 25, 21, 30), (17, 19, 19, 26), (6, 18, 30, 21), (3, 10, 10, 18), (26, 10, 33, 18), (16, 13, 20, 18), (17, 8, 19, 14),
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
    m.prop("chest", 220, 84, "chest_ruina3")
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
    m.exit([0, 14 * T, 8, 2 * T], "camino_paso", "from_east")
    m.trigger("kraag_zone", [440, 150, 12, 200])
    return m


# ------------------------------------------------------------ Rutas entre zonas (para que el viaje no sea instantáneo)
def _solidify(m, chars):
    for ty in range(m.h):
        tx = 0
        while tx < m.w:
            if m.grid[ty][tx] in chars:
                st = tx
                while tx < m.w and m.grid[ty][tx] in chars:
                    tx += 1
                m.extra_solids.append([st * T, ty * T, (tx - st) * T, T])
            else:
                tx += 1


def _route(mid, display, music, modulate, horizontal, length, ground_a, ground_b, wall, trees_a, trees_b,
           encounters, bg, seed, chests):
    rng = random.Random(seed)
    across = 22
    W, H = (length, across) if horizontal else (across, length)
    m = Map(mid, [wall * W] * H, False, display, music, modulate=modulate)
    c = across / 2.0
    off = 0.0
    centers = []
    for k in range(length):
        off += rng.uniform(-0.9, 0.9)
        off = max(-5.5, min(5.5, off * 0.96))
        if k < 4 or k > length - 5:
            off *= 0.6
        centers.append(c + off)
        half = 2.6 + (1.2 if (k // 9) % 2 == 0 else 0)
        for j in range(across):
            if abs(j + 0.5 - (c + off)) < half:
                x, y = (k, j) if horizontal else (j, k)
                m.grid[y][x] = ground_a if k < length * (0.45 + rng.uniform(-0.05, 0.05)) else ground_b
    # bolsillos laterales con cofre
    pockets = []
    for n, frac in enumerate(chests):
        k = int(length * frac)
        side = 1 if n % 2 == 0 else -1
        cc = int(centers[k])
        for dk in range(-2, 3):
            for dj in range(0, 6):
                j = cc + side * (3 + dj)
                if 1 < j < across - 2:
                    x, y = (k + dk, j) if horizontal else (j, k + dk)
                    m.grid[y][x] = ground_a if k < length / 2 else ground_b
        j = cc + side * 7
        pockets.append(((k * T + 8, j * T + 12) if horizontal else (j * T + 8, k * T + 12)))
    if wall in "fx":
        _solidify(m, wall)
    # árboles/rocas en el borde del camino
    for ty in range(m.h):
        for tx in range(m.w):
            if m.grid[ty][tx] != wall:
                continue
            near = any(0 <= ty + dy < m.h and 0 <= tx + dx < m.w and m.grid[ty + dy][tx + dx] != wall
                       for dx in (-1, 0, 1) for dy in (-1, 0, 1, 2))
            if near and rng.random() < 0.55:
                kk = tx if horizontal else ty
                pool = trees_a if kk < length / 2 else trees_b
                m.prop(rng.choice(pool), tx * T + 8, ty * T + 14, col=False)
    for i, (x, y) in enumerate(pockets):
        m.prop("chest", x, y, "chest_%s%d" % (mid, i + 1))
    k = length // 2
    cx = int(centers[k])
    fx_, fy_ = ((k * T + 8, int(centers[k] * T) + 20) if horizontal else (int(centers[k] * T) + 8, k * T + 8))
    m.prop("campfire", fx_, fy_, "campfire", light={"r": 90, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    m.encounters = encounters
    m.battle_bg = bg
    if horizontal:
        y0 = int(centers[0] * T)
        y1 = int(centers[-1] * T)
        m.marker("from_west", 24, y0 + 8)
        m.marker("from_east", W * T - 24, y1 + 8)
        m.marker("mid", fx_, fy_ - 24)
    else:
        x0 = int(centers[0] * T)
        x1 = int(centers[-1] * T)
        m.marker("from_north", x0 + 8, 24)
        m.marker("from_south", x1 + 8, H * T - 24)
        m.marker("mid", fx_ + 24, fy_)
    m._ends = (int(centers[0] * T), int(centers[-1] * T))
    return m


def sendero_montes():
    m = _route("sendero_montes", "Sendero de los Pinos Altos", "mountain", [0.9, 0.95, 1.0], True, 64, "l", "k", "f",
               ["pine", "pine2", "oak"], ["pine", "pine2", "rock_big"],
               {"rate": 26, "table": [["boar", "boar"], ["wisp", "boar"], ["salamandra"], ["golem"], ["bat", "bat", "salamandra"]]},
               "forest", 61, [0.25, 0.75])
    a, b = m._ends
    m.exit([0, a - 24, 8, 48], "elf_camp", "from_montes")
    m.exit([m.w * T - 8, b - 24, 8, 48], "montes", "from_camp")
    m.prop("sign", 140, a + 30, "sign_route")
    return m


def sendero_pantano():
    m = _route("sendero_pantano", "Bajada de las Ciénagas", "swamp", [0.8, 0.88, 0.8], False, 60, "l", "b", "f",
               ["oak", "oak2", "pine"], ["dead_tree", "dead_tree2", "dark_tree"],
               {"rate": 26, "table": [["wisp", "wisp"], ["boar", "root"], ["lodo"], ["spectre", "wisp"], ["lodo", "wisp"]]},
               "forest", 62, [0.3, 0.7])
    a, b = m._ends
    m.exit([a - 24, 0, 48, 8], "elf_camp", "from_pantano")
    m.exit([b - 24, m.h * T - 8, 48, 8], "pantano", "from_camp")
    m.prop("sign", a + 40, 120, "sign_route")
    return m


def camino_paso():
    m = _route("camino_paso", "Camino de las Cenizas", "war", [0.9, 0.78, 0.7], True, 64, "b", "q", "K",
               ["dead_tree", "dead_tree2"], ["dead_tree", "rock", "rock_big"],
               {"rate": 24, "table": [["lodo", "wisp"], ["soldado"], ["soldado", "thrall"], ["salamandra", "soldado"], ["brute"]]},
               "heart", 63, [0.35, 0.8])
    a, b = m._ends
    m.exit([0, a - 24, 8, 48], "pantano", "from_pass")
    m.exit([m.w * T - 8, b - 24, 8, 48], "paso_sur", "from_pantano")
    for x in (520, 760):
        m.prop("war_banner", x, int(b) - 30)
    m.prop("sign", 140, a + 30, "sign_route")
    return m


def senda_corazon():
    m = _route("senda_corazon", "Senda de las Raíces", "heart", [0.7, 0.62, 0.8], False, 60, "l", "v", "x",
               ["oak", "dark_tree", "pine2"], ["corrupt_tree", "corrupt_tree2", "thorns"],
               {"rate": 28, "table": [["root", "root"], ["thrall", "larva"], ["spectre", "bat"], ["brute"], ["root", "larva", "larva"]]},
               "heart", 64, [0.3, 0.7])
    a, b = m._ends
    m.exit([a - 24, m.h * T - 8, 48, 8], "elf_camp", "from_heart")
    m.exit([b - 24, 0, 48, 8], "heart", "from_camp")
    m.prop("sign", a + 40, m.h * T - 120, "sign_route")
    return m


# ------------------------------------------------------------ Abismo de los Susurros (post-juego)
def abyss_stairs_decal(x, y, glow=(150, 90, 220)):
    def f(c):
        c.rect(x - 2, y - 2, 36, 28, (20, 12, 30))
        for k in range(6):
            shade = 1 - k / 6.0
            c.rect(x, y + k * 4, 32, 3, mix((8, 4, 12), (110, 90, 130), shade))
        c.ellipse(x + 16, y + 26, 18, 4, glow)
    return f


def _abyss_layout(seed):
    rng = random.Random(seed)
    w, h = 36, 30
    rooms = [(15, 23, 21, 28)]          # sala de entrada abajo
    cx, cy = 18, 25
    for k in range(7):
        rw, rh = rng.randint(5, 8), rng.randint(5, 7)
        x0 = max(2, min(w - rw - 3, cx + rng.randint(-12, 12) - rw // 2))
        y0 = max(2, min(h - rh - 4, cy - rng.randint(5, 10)))
        if k % 3 == 2:
            y0 = max(2, min(h - rh - 4, cy + rng.randint(-3, 3)))
        rooms.append((x0, y0, x0 + rw, y0 + rh))
        # pasillo en L: el tramo horizontal tiene 4 filas (las 2 de arriba se vuelven pared)
        nx, ny = x0 + rw // 2, y0 + rh // 2 + 1
        for xx in range(min(cx, nx) - 1, max(cx, nx) + 2):
            rooms.append((xx, cy - 3, xx, cy))
        for yy in range(min(cy, ny) - 3, max(cy, ny) + 1):
            rooms.append((nx - 1, yy, nx + 1, yy))
        cx, cy = nx, ny
    return w, h, rooms


def _walkable(g, x, y):
    return 0 <= y < len(g) and 0 <= x < len(g[0]) and g[y][x] == "."


def _reach(g, start):
    seen = {start}
    st = [start]
    while st:
        x, y = st.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in seen and _walkable(g, *n):
                seen.add(n)
                st.append(n)
    return seen


def _abyss_floor(mid, seed, style, tint):
    # busca una semilla cuyo piso sea totalmente transitable (escalera y cofres alcanzables)
    for tries in range(200):
        w, h, rooms = _abyss_layout(seed + tries * 7919)
        g = dungeon(w, h, rooms, range(16, 21))
        big = [r for r in rooms[1:] if r[2] - r[0] >= 4 and r[3] - r[1] >= 4]
        if len(big) < 5:
            continue
        last = big[-1]
        reach = _reach(g, (18, 27))
        sx_t, sy_t = (last[0] + last[2]) // 2, last[3] - 1
        spots = [(r[0] + 1, r[3] - 1) for r in big[:-1]][:7]
        if (sx_t, sy_t) in reach and all(sp in reach for sp in spots):
            break
    rng = random.Random(seed)
    m = Map(mid, to_rows(g), True, "Abismo de los Susurros", "deep", modulate=tint)
    m.style = style
    m.battle_bg = "crypt"
    m.encounters = {"rate": 22, "table": [["golem", "salamandra"], ["soldado", "soldado", "wisp"], ["custodian"], ["brute", "spectre"],
                                          ["lodo", "lodo", "salamandra"], ["boar", "boar", "golem"], ["spectre", "spectre", "wisp", "wisp"],
                                          ["soldado", "golem"]]}
    sx, sy = sx_t * T + 8, sy_t * T + 8
    m.decals.append(abyss_stairs_decal(sx - 16, sy - 14))
    m.exit([sx - 14, sy - 12, 28, 18], "abismo_next", "from_above")
    m.marker("stairs", sx, sy + 20)
    for i, (tx, ty) in enumerate(spots):
        m.prop("chest", tx * T + 8, ty * T + 12, "chest_ab%d" % i)
    for i, r in enumerate(big[:-1]):
        m.prop("crystal" if i % 2 else "mushroom", (r[2] - 1) * T + 8, (r[3] - 1) * T + 12, col=False,
               light={"r": 50, "color": [0.7, 0.45, 1.0], "e": 0.8, "flicker": True, "oy": -10})
    for k in range(10):
        r = rng.choice(big)
        tx, ty = rng.randint(r[0] + 1, r[2] - 1), r[3] - 1
        if (tx, ty) in reach:
            m.prop("bones", tx * T + 8, ty * T + 8, col=False)
    m.marker("from_above", 18 * T, 27 * T)
    m.exit([16 * T, h * T - 8, 5 * T, 8], "cave", "from_abyss")
    return m


def abismo_a():
    return _abyss_floor("abismo_a", 101, "crypt", [0.5, 0.4, 0.7])


def abismo_b():
    return _abyss_floor("abismo_b", 202, "ruin", [0.45, 0.5, 0.7])


def abismo_c():
    return _abyss_floor("abismo_c", 303, "mine", [0.55, 0.38, 0.55])


def abismo_d():
    return _abyss_floor("abismo_d", 404, "crypt", [0.42, 0.35, 0.6])


MAPS = (montes, minas, pantano, ruinas, paso_sur, sendero_montes, sendero_pantano, camino_paso, senda_corazon, abismo_a, abismo_b, abismo_c, abismo_d)
