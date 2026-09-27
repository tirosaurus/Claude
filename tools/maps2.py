"""Mapas de los actos 2-4."""
import random
from artlib import *
import maps
from maps import (Map, T, window_decal, rug_decal, stairs_down_decal, stairs_up_decal, flowers_decal,
                  sunbeam_decal, mat_decal, door_light_decal)

maps.COLS.update({
    "pew": (-19, -12, 38, 10), "pillar": (-9, -8, 18, 8), "pillar_broken": (-9, -8, 18, 8),
    "altar": (-23, -20, 46, 18), "statue": (-10, -10, 20, 10), "statue_broken": (-10, -10, 20, 10),
    "candelabra": (-3, -3, 6, 3), "rune_off": (-8, -10, 16, 10), "rune_on": (-8, -10, 16, 10),
    "rubble": (-18, -12, 36, 10), "sarcophagus": (-10, -30, 20, 30), "crystal": (-8, -8, 16, 8),
    "seed_pedestal": (-10, -14, 20, 14), "seed_empty": (-10, -14, 20, 14),
    "corrupt_tree": (-6, -7, 12, 7), "corrupt_tree2": (-6, -7, 12, 7), "thorns": (-12, -8, 24, 8),
    "campfire": (-9, -8, 18, 8), "fire": (-10, -8, 20, 8), "broken_cart": (-18, -10, 36, 10),
    "tent_green": (-20, -18, 40, 18), "tent_blue": (-20, -18, 40, 18), "tent_red": (-20, -18, 40, 18),
    "lantern": (-2, -3, 4, 3), "sacred_tree": (-22, -16, 44, 14), "dummy": (-4, -4, 8, 4),
    "mushroom": (-6, -4, 12, 4), "cathedral_open": (-92, -58, 184, 56),
})
maps.FRAMES.update({"candelabra": 3, "crystal": 2, "seed_pedestal": 2, "mushroom": 2, "campfire": 3, "fire": 3,
                    "lantern": 2, "mother_root_map": 2})


def grid(w, h, fill):
    return [[fill] * w for _ in range(h)]


def to_rows(g):
    return ["".join(r) for r in g]


def room(w, h, face=2):
    g = grid(w, h, ".")
    for x in range(w):
        g[0][x] = "#"
        g[h - 1][x] = "#"
        for k in range(1, face + 1):
            g[k][x] = "W"
    for y in range(h):
        g[y][0] = "#"
        g[y][w - 1] = "#"
    return g


def stained_glass(x, y, hue):
    def f(c):
        c.rect(x - 1, y - 1, 18, 26, (40, 36, 48))
        c.ellipse(x + 8, y + 3, 8, 5, (40, 36, 48))
        cols = [(200, 60, 70), (70, 100, 200), (230, 190, 80), (80, 170, 110)]
        for yy in range(y, y + 24):
            for xx in range(x, x + 16):
                k = ((xx - x) // 4 + (yy - y) // 5 + hue) % 4
                col = cols[k]
                if (xx - x) % 4 == 0 or (yy - y) % 5 == 0:
                    col = (30, 28, 36)
                c.px(xx, yy, col)
        for k in range(40):
            for w in range(16):
                col = cols[(w // 4 + hue) % 4]
                c.px(x + w + k // 3, y + 30 + k, col, 26 - k // 2)
    return f


def sap_decal(x, y, rx, ry):
    def f(c):
        c.ellipse(x, y, rx, ry, (20, 10, 26))
        c.ellipse(x - rx * 0.3, y - ry * 0.2, rx * 0.3, ry * 0.3, (70, 40, 80))
    return f


def scorch_decal(x, y, r):
    def f(c):
        c.darken_ellipse(x, y, r, r * 0.6, 0.55)
    return f


# ------------------------------------------------------------ Pueblo de noche / amanecer
def _village_variant(mid, display, music, modulate, fires, open_door):
    m = maps.village()
    m.id = mid
    m.display = display
    m.music = music
    m.modulate = modulate
    m.battle_bg = "village_night"
    for p in m.props:
        if p["sprite"] == "cathedral" and open_door:
            p["sprite"] = "cathedral_open"
        if p["sprite"] == "torch" and fires:
            p["light"] = {"r": 60, "color": [1, 0.65, 0.3], "e": 0.9, "flicker": True, "oy": -26}
    m.props = [p for p in m.props if p["sprite"] not in ("stall",)]
    m.prop("broken_cart", 290, 252, "cart")
    m.prop("rubble", 360, 222, "rubble") if not open_door else None
    for (x, y, r) in ((300, 330, 18), (470, 300, 14), (240, 420, 16), (560, 420, 12), (420, 250, 20)):
        m.decals.append(scorch_decal(x, y, r))
    for (x, y) in ((330, 300, ), (500, 280), (210, 300), (420, 360), (620, 300)):
        m.decals.append(sap_decal(x, y, 10, 5))
    if fires:
        for (x, y, lift) in ((140, 358, 40), (186, 358, 30), (580, 342, 38), (612, 240, 30), (128, 240, 36),
                             (420, 522, 30)):
            m.prop("fire", x, y, col=False,
                   light={"r": 90, "color": [1, 0.55, 0.25], "e": 1.1, "flicker": True, "oy": -14 - lift},
                   extra={"noshadow": True, "lift": lift})
    m.exits = [e for e in m.exits if e["to"] != "forest"]
    if fires:
        m.exit([W - 10, 392, 10, 48], "forest", "from_village", requires="never",
               blocked="No puedes abandonar Tortosa ahora. ¡Tu gente te necesita!")
        m.exit([356, 196, 24, 8], "cathedral", "from_village", requires="brute_defeated",
               blocked="La entrada de la Catedral sigue bloqueada por los escombros.")
    else:
        m.exit([W - 10, 392, 10, 48], "forest", "from_village", requires="act3_started",
               blocked="Todavía no. Deberías hablar con los demás antes de partir.")
        m.exit([356, 196, 24, 8], "cathedral", "from_village")
    m.marker("cathedral_front", 368, 214)
    m.marker("plaza", 368, 300)
    return m


W = 46 * 16


def village_night():
    return _village_variant("village_night", "Tortosa en llamas", "night", [0.42, 0.42, 0.66], True, False)


def village_dawn():
    return _village_variant("village_dawn", "Tortosa, al alba", "sad", [0.82, 0.76, 0.84], False, True)


# ------------------------------------------------------------ Catedral
def cathedral():
    w, h = 26, 25
    g = room(w, h)
    for y in range(4, h - 1):
        g[y][12] = "r"
        g[y][13] = "r"
    g[h - 1][12] = "d"
    g[h - 1][13] = "d"
    m = Map("cathedral", to_rows(g), True, "La Catedral Vieja", "cathedral", modulate=[0.62, 0.62, 0.8])
    m.style = "stone"
    m.battle_bg = "cathedral"
    m.encounters = {"rate": 34, "table": [["larva", "larva"], ["bat", "bat"], ["spectre"], ["larva", "bat"],
                                          ["spectre", "larva"], ["wisp", "bat"], ["wisp"]]}
    m.decals += [stained_glass(40, 18, 0), stained_glass(88, 18, 1), stained_glass(280, 18, 2),
                 stained_glass(328, 18, 3), door_light_decal(192, 384, 32)]
    m.prop("altar", 208, 90, "altar")
    for (x, y) in ((164, 84), (252, 84), (180, 110), (236, 110)):
        m.prop("candelabra", x, y, light={"r": 50, "color": [1, 0.8, 0.5], "e": 0.7, "flicker": True, "oy": -26})
    m.prop("statue", 144, 70, "statue")
    m.prop("statue_broken", 272, 70, "statue")
    for row in range(5):
        y = 150 + row * 40
        m.prop("pew", 88, y)
        m.prop("pew", 328, y) if row != 2 else m.prop("rubble", 330, y)
    for (x, y) in ((168, 140), (248, 140), (168, 220), (248, 220), (168, 300), (248, 300)):
        m.prop("pillar" if (x + y) % 3 else "pillar_broken", x, y)
    m.prop("rune_off", 32, 132, "rune_1")
    m.prop("rune_off", 384, 132, "rune_2")
    m.prop("rune_off", 40, 360, "rune_3")
    m.prop("rubble", 372, 70, "crypt_rubble")
    m.prop("chest", 380, 368, "chest_cath1")
    m.prop("chest", 40, 200, "chest_cath2")
    m.prop("bones", 300, 360, col=False)
    m.prop("bones", 120, 330, col=False)
    m.decals.append(stairs_down_decal(364, 50))
    m.marker("from_village", 208, 380)
    m.marker("from_crypt", 372, 90)
    m.exit([196, 394, 24, 6], "village_night_or_dawn", "cathedral_front")
    m.exit([364, 50, 16, 14], "crypt", "from_cathedral", requires="runes_done",
           blocked="Un montón de escombros tapa algo que parecen unas escaleras.")
    m.lights += [{"x": 48, "y": 60, "r": 70, "color": [0.9, 0.6, 0.6], "e": 0.4},
                 {"x": 96, "y": 60, "r": 70, "color": [0.6, 0.7, 1.0], "e": 0.4},
                 {"x": 288, "y": 60, "r": 70, "color": [1.0, 0.9, 0.5], "e": 0.4},
                 {"x": 336, "y": 60, "r": 70, "color": [0.6, 1.0, 0.7], "e": 0.4}]
    return m


# ------------------------------------------------------------ Cripta
def crypt():
    w, h = 20, 30
    g = grid(w, h, " ")
    # pasillo
    for y in range(0, 16):
        for x in range(6, 14):
            g[y][x] = "."
    for y in range(15, h):
        for x in range(1, w - 1):
            g[y][x] = "."
    for y in range(h):
        for x in range(w):
            if g[y][x] == ".":
                continue
            near = any(0 <= y + dy < h and 0 <= x + dx < w and g[y + dy][x + dx] == "." for dx in (-1, 0, 1) for dy in (-1, 0, 1, 2, 3))
            if near:
                g[y][x] = "#"
    for y in range(h):
        for x in range(w):
            if g[y][x] == "." and (y + 1 < h) and g[y - 1][x] == "#" if y > 0 else False:
                pass
    # caras de muro: dos filas bajo cada muro superior
    for x in range(w):
        for y in range(1, h - 2):
            if g[y - 1][x] == "#" and g[y][x] == "." and g[y + 1][x] == ".":
                g[y][x] = "W"
                g[y + 1][x] = "W"
    for x in range(6, 14):
        g[0][x] = "#"
    for x in range(1, w - 1):
        g[h - 1][x] = "#"
    m = Map("crypt", to_rows(g), True, "La Cripta", "cathedral", modulate=[0.5, 0.52, 0.7])
    m.style = "crypt"
    m.battle_bg = "crypt"
    m.encounters = {"rate": 30, "table": [["spectre"], ["spectre", "bat"], ["thrall"], ["thrall", "larva"]]}
    m.decals.append(stairs_up_decal(152, 12))
    for y in (70, 118, 166):
        m.prop("sarcophagus", 110, y)
        m.prop("sarcophagus", 210, y)
    m.prop("bones", 150, 200, col=False)
    for (x, y) in ((40, 300), (280, 300), (60, 420), (260, 420), (110, 260), (210, 260)):
        m.prop("crystal", x, y, light={"r": 60, "color": [0.55, 0.85, 1], "e": 0.7, "flicker": True, "oy": -14})
    m.prop("seed_pedestal", 160, 420, "seed", light={"r": 110, "color": [0.6, 0.95, 1], "e": 1.0, "flicker": True, "oy": -30})
    m.prop("chest", 40, 380, "chest_crypt1")
    m.marker("from_cathedral", 160, 60)
    m.marker("chamber", 160, 350)
    m.marker("custodian", 160, 400)
    m.exit([152, 20, 16, 12], "cathedral", "from_crypt")
    m.trigger("custodian_zone", [16, 270, 288, 12])
    m.trigger("seed_zone", [130, 400, 60, 24])
    return m


# ------------------------------------------------------------ Bosque profundo
def forest_deep():
    Wt, Ht = 50, 30
    m = Map("forest_deep", ["v" * Wt] * Ht, False, "Bosque Profundo", "deep", modulate=[0.72, 0.66, 0.82])
    m.battle_bg = "deep"
    m.encounters = {"rate": 38, "table": [["wolf"], ["root"], ["larva", "larva"], ["bat", "larva"], ["root", "larva"],
                                          ["wolf", "wolf"], ["boar"], ["boar", "wisp"], ["wisp", "wisp", "larva"]]}
    pts = [(0, 15), (6, 16), (12, 13), (18, 12), (24, 14), (30, 17), (36, 16), (42, 13), (49, 13)]
    for (a, b) in zip(pts, pts[1:]):
        steps = max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2
        for i in range(steps + 1):
            x = a[0] + (b[0] - a[0]) * i / steps
            y = a[1] + (b[1] - a[1]) * i / steps
            m.fill(int(x), int(y), int(x) + 1, int(y) + 1, "p")
    m.fill(36, 4, 38, 16, "p")
    for (cx, cy, rx, ry) in ((24, 14, 6, 4), (40, 14, 4, 3)):
        for y in range(Ht):
            for x in range(Wt):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1 and m.grid[y][x] != "p":
                    m.grid[y][x] = "s" if cx == 24 else "v"
    for (cx, cy) in ((10, 20), (33, 8), (44, 21)):
        for y in range(cy - 1, cy + 2):
            for x in range(cx - 2, cx + 3):
                if m.grid[y][x] == "v":
                    m.grid[y][x] = "z"

    def blocks(tx, ty):
        for yy in range(ty - 4, ty + 2):
            for xx in range(tx - 1, tx + 2):
                if m.t(xx, yy) in "psz":
                    return True
        return False

    rng = random.Random(21)
    placed = []
    for ty in range(0, Ht + 2):
        for tx in range(-1, Wt + 1):
            if blocks(tx, ty) or rng.random() > 0.6:
                continue
            x, y = tx * T + rng.randint(0, 12), ty * T + rng.randint(4, 14)
            if any((x - a) ** 2 + (y - b) ** 2 < 24 ** 2 for a, b in placed):
                continue
            placed.append((x, y))
            m.prop(rng.choice(["corrupt_tree", "corrupt_tree2", "corrupt_tree", "dark_tree"]), x, y)
    for i in range(80):
        tx, ty = rng.randint(1, Wt - 2), rng.randint(1, Ht - 2)
        if m.t(tx, ty) == "v" and any(m.t(tx + dx, ty + dy) == "p" for dx in (-1, 0, 1) for dy in (-1, 0, 1)) is False:
            near = any(m.t(tx + dx, ty + dy) in "ps" for dx in (-2, -1, 0, 1, 2) for dy in (-2, -1, 0, 1, 2))
            if not near:
                continue
            x, y = tx * T + 8, ty * T + 12
            if any((x - a) ** 2 + (y - b) ** 2 < 18 ** 2 for a, b in placed):
                continue
            placed.append((x, y))
            kind = rng.choice(["thorns", "mushroom", "mushroom", "rock", "stump", "thorns"])
            if kind == "mushroom":
                m.prop(kind, x, y, light={"r": 36, "color": [0.5, 1, 0.9], "e": 0.6, "flicker": True, "oy": -6})
            else:
                m.prop(kind, x, y)
    m.prop("campfire", 360, 216, "campfire", light={"r": 90, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    m.prop("chest", 420, 190, "chest_deep1")
    # boca de la Cueva de los Susurros (al norte del sendero)
    m.decals.append(cave_mouth_decal(36 * T + 8, 4 * T + 8))
    m.marker("from_cave", 36 * T + 8, 7 * T)
    m.exit([36 * T - 4, 4 * T, 24, 12], "cave", "from_deep")
    m.marker("from_forest", 18, 258)
    m.marker("from_camp", W2(Wt) - 20, 226)
    m.marker("brom", 640, 226)
    m.marker("campfire", 360, 240)
    m.exit([0, 232, 8, 48], "forest", "from_deep")
    m.exit([Wt * T - 8, 196, 8, 48], "elf_camp", "from_deep", requires="brom_event_done",
           blocked="Algo se mueve entre los árboles justo delante...")
    m.trigger("brom_zone", [580, 150, 16, 150])
    return m


def cave_mouth_decal(x, y):
    def f(c):
        for (dx, dy, r) in ((-14, 4, 7), (14, 4, 7), (-10, -6, 6), (10, -6, 6), (0, -10, 7)):
            c.ellipse(x + dx, y + dy, r, r * 0.8, (70, 66, 80))
            c.ellipse(x + dx - 1, y + dy - 1, r * 0.6, r * 0.5, (104, 100, 116))
        c.ellipse(x, y + 2, 10, 9, (10, 8, 16))
        c.rect(x - 10, y + 2, 20, 8, (10, 8, 16))
        c.ellipse(x, y + 1, 6, 5, (4, 2, 8))
    return f


# ------------------------------------------------------------ Cueva de los Susurros (mazmorra opcional)
def cave():
    w, h = 34, 32
    g = grid(w, h, " ")
    rooms = [(14, 24, 20, 30), (16, 13, 18, 24), (2, 14, 13, 22), (13, 17, 16, 19), (18, 16, 22, 18), (22, 11, 32, 21),
             (16, 8, 18, 13), (9, 1, 25, 9)]
    for (x0, y0, x1, y1) in rooms:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if 0 < x < w - 1 and 0 < y < h - 1:
                    g[y][x] = "."
    for y in range(h):
        for x in range(w):
            if g[y][x] == ".":
                continue
            near = any(0 <= y + dy < h and 0 <= x + dx < w and g[y + dy][x + dx] == "." for dx in (-1, 0, 1)
                       for dy in (-1, 0, 1, 2, 3))
            if near:
                g[y][x] = "#"
    for x in range(w):
        for y in range(1, h - 2):
            if g[y - 1][x] == "#" and g[y][x] == "." and g[y + 1][x] == ".":
                g[y][x] = "W"
                g[y + 1][x] = "W"
    for x in range(15, 20):
        g[h - 1][x] = "."
    m = Map("cave", to_rows(g), True, "Cueva de los Susurros", "deep", modulate=[0.46, 0.5, 0.66])
    m.style = "crypt"
    m.battle_bg = "crypt"
    m.encounters = {"rate": 26, "table": [["spectre", "wisp"], ["boar", "boar"], ["thrall", "spectre"],
                                          ["wisp", "wisp", "bat"], ["root", "boar"], ["spectre", "spectre", "wisp"]]}
    rng = random.Random(44)
    for (x, y) in ((60, 250), (190, 330), (400, 200), (480, 300), (200, 60), (330, 60), (270, 440), (500, 190),
                   (170, 120), (40, 330)):
        m.prop("crystal", x, y, light={"r": 60, "color": [0.55, 0.85, 1], "e": 0.8, "flicker": True, "oy": -14})
    for (x, y) in ((100, 300), (440, 240), (240, 400), (300, 140), (150, 40), (370, 110)):
        m.prop("mushroom", x, y, light={"r": 36, "color": [0.5, 1, 0.9], "e": 0.6, "flicker": True, "oy": -6})
    for (x, y) in ((80, 270), (460, 320), (230, 470), (380, 30)):
        m.prop("bones", x, y, col=False)
    m.prop("chest", 44, 240, "chest_cave1")
    m.prop("chest", 500, 200, "chest_cave2")
    m.prop("chest", 500, 320, "chest_cave3")
    m.prop("chest", 200, 44, "chest_cave4")
    m.prop("chest", 350, 44, "chest_cave5")
    m.prop("statue_broken", 276, 40)
    m.marker("from_deep", 280, 490)
    m.marker("guardian", 280, 120)
    m.exit([15 * T, h * T - 8, 5 * T, 8], "forest_deep", "from_cave")
    m.trigger("guardian_zone", [16 * T, 12 * T, 3 * T, 8])
    return m


def W2(wt):
    return wt * T


# ------------------------------------------------------------ Campamento élfico
def elf_camp():
    Wt, Ht = 36, 26
    m = Map("elf_camp", ["l" * Wt] * Ht, False, "Claro de la Savia", "elves", modulate=[0.9, 0.95, 1.0])
    for y in range(Ht):
        for x in range(Wt):
            if x < 2 or x > Wt - 3 or y < 2 or y > Ht - 3:
                m.grid[y][x] = "f"
    for y in range(15, 21):
        for x in range(3, 10):
            if ((x - 6.5) / 3.6) ** 2 + ((y - 17.5) / 2.6) ** 2 < 1:
                m.grid[y][x] = "w"
    m.fill(0, 12, 18, 13, "p")
    m.fill(17, 8, 18, 25, "p")
    m.fill(17, 0, 18, 8, "p")
    m.prop("sacred_tree", 290, 150, "sacred_tree")
    m.prop("tent_green", 110, 110, "tent1")
    m.prop("tent_blue", 180, 90, "tent2")
    m.prop("tent_red", 470, 300, "tent3")
    m.prop("tent_green", 520, 250)
    m.prop("stall", 420, 340, "elf_shop")
    m.prop("campfire", 290, 250, "campfire", light={"r": 100, "color": [1, 0.7, 0.35], "e": 1.0, "flicker": True, "oy": -8})
    for (x, y) in ((250, 200), (330, 200), (230, 300), (350, 300), (150, 180), (420, 180)):
        m.prop("lantern", x, y, light={"r": 60, "color": [1, 0.9, 0.6], "e": 0.8, "flicker": True, "oy": -28})
    m.prop("dummy", 470, 150)
    m.prop("dummy", 500, 150)
    m.prop("chest", 540, 150, "chest_elf1")
    rng = random.Random(3)
    for i in range(46):
        x = rng.randint(0, Wt * T)
        y = rng.randint(0, Ht * T)
        if m.t(x // T, y // T) == "f":
            m.prop(rng.choice(["oak", "pine", "oak2", "pine2"]), x, y)
    m.decals += [flowers_decal(m, 180, 5, allowed="l"), sunbeam_decal(290, 220, 90, 50)]
    m.marker("from_deep", 20, 200)
    m.marker("from_heart", 280, 30)
    m.marker("ilvanis", 290, 176)
    m.marker("aelis", 200, 220)
    m.marker("brom", 360, 260)
    m.marker("shop", 420, 364)
    m.marker("lake", 150, 300)
    m.marker("fire", 290, 276)
    m.exit([0, 188, 8, 36], "forest_deep", "from_camp")
    m.exit([270, 0, 36, 8], "heart", "from_camp", requires="ready_for_heart",
           blocked="Ilvanis quiere hablar contigo antes de que os adentréis en el Corazón.")
    return m


# ------------------------------------------------------------ Corazón del Bosque
def heart():
    Wt, Ht = 34, 44
    g = grid(Wt, Ht, "x")
    rng = random.Random(9)
    path = [(17, 43), (17, 38), (9, 34), (9, 28), (24, 24), (26, 17), (14, 13), (17, 7), (17, 2)]
    for (a, b) in zip(path, path[1:]):
        steps = max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2
        for i in range(steps + 1):
            x = a[0] + (b[0] - a[0]) * i / steps
            y = a[1] + (b[1] - a[1]) * i / steps
            r = 2 + (1 if rng.random() > 0.5 else 0)
            for yy in range(int(y) - r, int(y) + r + 1):
                for xx in range(int(x) - r - 1, int(x) + r + 2):
                    if 0 <= xx < Wt and 0 <= yy < Ht:
                        g[yy][xx] = "v"
    for (cx, cy, rx, ry) in ((17, 6, 9, 5), (9, 31, 4, 4), (24, 20, 5, 4)):
        for y in range(Ht):
            for x in range(Wt):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1:
                    g[y][x] = "v"
    for (cx, cy) in ((6, 30), (28, 21), (11, 6), (23, 6)):
        for y in range(cy - 1, cy + 1):
            for x in range(cx - 1, cx + 2):
                g[y][x] = "z"
    m = Map("heart", to_rows(g), False, "Corazón del Bosque", "heart", modulate=[0.62, 0.52, 0.74])
    m.battle_bg = "heart"
    m.encounters = {"rate": 34, "table": [["root", "root"], ["thrall", "larva"], ["spectre", "bat"],
                                          ["thrall", "thrall"], ["root", "larva", "larva"], ["brute"], ["boar", "boar"], ["wisp", "thrall", "wisp"]]}
    for i in range(60):
        tx, ty = rng.randint(1, Wt - 2), rng.randint(1, Ht - 2)
        if g[ty][tx] == "v" and any(g[ty + dy][tx + dx] == "x" for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
            x, y = tx * T + 8, ty * T + 12
            if rng.random() > 0.5:
                m.prop("mushroom", x, y, light={"r": 34, "color": [0.8, 0.5, 1], "e": 0.7, "flicker": True, "oy": -6})
            else:
                m.prop("thorns", x, y)
    m.prop("mother_root_map", 272, 94, "heart_core", col=False,
           light={"r": 140, "color": [0.85, 0.4, 1], "e": 1.1, "flicker": True, "oy": -70},
           extra={"col": [-70, -40, 140, 36], "frames": 2})
    m.prop("chest", 150, 500, "chest_heart1")
    m.prop("chest", 400, 330, "chest_heart2")
    m.marker("from_camp", 272, 680)
    m.marker("kaelen_spot", 150, 470)
    m.marker("final_spot", 272, 160)
    m.exit([256, 694, 32, 10], "elf_camp", "from_heart")
    m.trigger("kaelen_zone", [96, 400, 96, 12])
    m.trigger("final_zone", [180, 186, 184, 12])
    return m


MAPS = (cave, village_night, village_dawn, cathedral, crypt, forest_deep, elf_camp, heart)
