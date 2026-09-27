"""Mapas: pinta el suelo de cada mapa en un PNG y exporta colisiones/props/salidas a JSON."""
import json
import random
from artlib import *
from PIL import Image

T = 16

# Colisión (dx, dy, w, h) relativa al ancla (abajo-centro) de cada sprite.
COLS = {
    "bed": (-11, -30, 22, 30), "nightstand": (-6, -9, 12, 9), "wardrobe": (-12, -12, 24, 12),
    "chest": (-8, -9, 16, 9), "bookshelf": (-14, -11, 28, 11), "table": (-15, -12, 30, 10),
    "table_food": (-15, -12, 30, 10), "chair": (-5, -6, 10, 6), "barrel": (-6, -8, 12, 8),
    "crate": (-7, -10, 14, 10), "plant": (-4, -6, 8, 6), "sack": (-5, -6, 10, 6),
    "fireplace": (-20, -18, 40, 18), "cathedral": (-92, -58, 184, 56),
    "oak": (-5, -6, 10, 6), "oak2": (-5, -6, 10, 6), "pine": (-4, -5, 8, 5), "pine2": (-4, -5, 8, 5),
    "dark_tree": (-6, -7, 12, 7), "dark_tree2": (-6, -7, 12, 7), "bush": (-8, -7, 16, 7),
    "bush_berries": (-8, -7, 16, 7), "rock": (-7, -6, 14, 6), "rock_big": (-10, -9, 20, 9),
    "stump": (-6, -6, 12, 6), "fence_h": (-8, -6, 16, 5), "fence_v": (-2, -16, 4, 16), "well": (-14, -14, 28, 14),
    "house_red": (-32, -30, 64, 22), "house_blue": (-32, -30, 64, 22), "house_brown": (-28, -30, 56, 22),
    "house_smithy": (-36, -30, 72, 22),
    "sign": (-3, -4, 6, 4), "stall": (-19, -10, 38, 10), "anvil": (-8, -6, 16, 6),
    "woodpile": (-13, -8, 26, 8), "hay": (-9, -8, 18, 8), "shrine": (-10, -6, 20, 6), "torch": (-2, -3, 4, 3),
}
FRAMES = {"fireplace": 3, "torch": 3, "shrine": 2}
SIZES = {}


def sprite_size(name):
    if name not in SIZES:
        im = Image.open(f"{OUT_DIR}/sprites/{name}.png")
        SIZES[name] = (im.width // FRAMES.get(name, 1), im.height)
    return SIZES[name]


OUT_DIR = None


class Map:
    def __init__(self, mid, grid, interior, display, music, modulate=None):
        self.id = mid
        self.grid = [list(r) for r in grid]
        self.h = len(grid)
        self.w = len(grid[0])
        for r in grid:
            assert len(r) == self.w, (mid, r, len(r), self.w)
        self.interior = interior
        self.display = display
        self.music = music
        self.modulate = modulate
        self.props = []
        self.markers = {}
        self.exits = []
        self.triggers = []
        self.lights = []
        self.decals = []  # funciones f(canvas) pintadas tras el suelo
        self.extra_solids = []

    def t(self, x, y):
        x = min(max(x, 0), self.w - 1)
        y = min(max(y, 0), self.h - 1)
        return self.grid[y][x]

    def fill(self, x0, y0, x1, y1, ch):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if 0 <= x < self.w and 0 <= y < self.h:
                    self.grid[y][x] = ch

    def prop(self, name, x, y, pid=None, col=True, light=None, flip=False, extra=None):
        e = {"sprite": name, "x": x, "y": y}
        if pid:
            e["id"] = pid
        if col and name in COLS:
            e["col"] = list(COLS[name])
        if FRAMES.get(name, 1) > 1:
            e["frames"] = FRAMES[name]
        if light:
            e["light"] = light
        if flip:
            e["flip"] = True
        if extra:
            e.update(extra)
        self.props.append(e)
        return e

    def marker(self, name, x, y):
        self.markers[name] = [x, y]

    def exit(self, rect, to, spawn, requires=None, blocked=None):
        e = {"rect": rect, "to": to, "spawn": spawn}
        if requires:
            e["requires"] = requires
            e["blocked"] = blocked or ""
        self.exits.append(e)

    def trigger(self, name, rect):
        self.triggers.append({"id": name, "rect": rect})


# ------------------------------------------------------------ Pintores de suelo
def grass_px(x, y, seed=0, pal=(GRASS_D, GRASS_M, GRASS_L, GRASS_H)):
    d, m, l, hi = pal
    n = fbm(x, y, 28, seed)
    base = mix(d, m, min(1, max(0, n * 1.9 - 0.45)))
    if n > 0.62:
        base = mix(base, l, (n - 0.62) * 2.2)
    h = hash2(x, y, seed + 1)
    if h > 0.975:
        return l
    if h < 0.025:
        return d
    if hash2(x, y, seed + 5) > 0.986 or hash2(x, y - 1, seed + 5) > 0.986:
        return hi if n > 0.45 else l
    return base


def dirt_px(x, y, seed=0):
    n = fbm(x, y, 18, seed + 20)
    base = mix(DIRT_D, DIRT_M, min(1, max(0, n * 1.6 - 0.2)))
    if n > 0.66:
        base = mix(base, DIRT_L, 0.5)
    h = hash2(x, y, seed + 21)
    if h > 0.972:
        return DIRT_L
    if h < 0.02:
        return DIRT_D
    return base


def cobble_px(x, y, seed=0):
    cs = 7
    cx, cy = x // cs, y // cs
    best = [(1e9, None), (1e9, None)]
    for oy in (-1, 0, 1):
        for ox in (-1, 0, 1):
            gx, gy = cx + ox, cy + oy
            fx = gx * cs + hash2(gx, gy, 31) * cs
            fy = gy * cs + hash2(gx, gy, 32) * cs
            d = (fx - x) ** 2 + (fy - y) ** 2
            if d < best[0][0]:
                best = [(d, (gx, gy, fx, fy)), best[0]]
            elif d < best[1][0]:
                best[1] = (d, (gx, gy, fx, fy))
    d1, d2 = math.sqrt(best[0][0]), math.sqrt(best[1][0])
    if d2 - d1 < 1.3:
        return darken(STONE_D, 0.15)
    gx, gy, fx, fy = best[0][1]
    k = hash2(gx, gy, 33)
    col = mix(STONE_M, STONE_L, k * 0.8)
    if x < fx - 1 and y < fy - 1:
        col = lighten(col, 0.08)
    if x > fx + 1 and y > fy + 1:
        col = darken(col, 0.1)
    if fbm(x, y, 14, 40) > 0.66:
        col = mix(col, GRASS_D, 0.35)
    return col


def forest_px(x, y, seed=0):
    n = fbm(x, y, 22, seed + 50)
    base = mix(FGRASS_D, FGRASS_M, min(1, max(0, n * 1.8 - 0.4)))
    if n > 0.64:
        base = mix(base, FGRASS_L, (n - 0.64) * 2)
    h = hash2(x, y, seed + 51)
    if h > 0.985:
        return random.Random(int(h * 1e6)).choice([(126, 84, 46), (150, 104, 52), (96, 64, 40)])
    if h < 0.03:
        return FGRASS_D
    if hash2(x, y, seed + 55) > 0.99 or hash2(x, y - 1, seed + 55) > 0.99:
        return FGRASS_L
    return base


def clearing_px(x, y):
    return grass_px(x, y, 9, pal=((50, 96, 58), (74, 128, 66), (108, 158, 80), (150, 190, 100)))


def wood_floor_px(x, y):
    row = y // 4
    off = int(hash2(row, 0, 60) * 32)
    plank = (x + off) // 32
    seam = (x + off) % 32 == 0
    if y % 4 == 3:
        return darken(WOOD_D, 0.1)
    if seam:
        return WOOD_D
    k = hash2(plank, row, 61)
    col = mix(WOOD_M, WOOD_L, k * 0.55)
    if y % 4 == 0:
        col = lighten(col, 0.06)
    h = hash2(x, y, 62)
    if h > 0.94:
        col = darken(col, 0.12)
    elif h > 0.9:
        col = lighten(col, 0.08)
    if (x + off) % 32 == 30 and y % 4 == 1:
        col = darken(WOOD_D, 0.2)  # clavo
    return col


def wall_face_px(x, y, local_y, face_h):
    # yeso arriba, zócalo de madera abajo, vigas verticales
    wainscot = face_h - 10
    if local_y < 3:
        return WOOD_D if local_y < 2 else darken(WOOD_D, 0.3)
    if local_y >= wainscot:
        ly = local_y - wainscot
        if ly == 0:
            return WOOD_L
        if ly == 1:
            return WOOD_D
        col = WOOD_M if (x % 6) else WOOD_D
        if ly == 9:
            col = darken(WOOD_D, 0.2)
        return col
    if x % 80 < 4:
        return WOOD_D if x % 80 in (1, 2) else darken(WOOD_D, 0.2)
    col = PLASTER
    n = fbm(x, y, 10, 70)
    if n > 0.6:
        col = PLASTER_S
    if hash2(x, y, 71) > 0.96:
        col = PLASTER_S
    if local_y > wainscot - 4:
        col = mix(col, PLASTER_D, 0.4)
    return col


def wall_top_px(x, y):
    col = (66, 46, 40)
    if hash2(x, y, 80) > 0.9:
        col = (78, 56, 46)
    return col


# ------------------------------------------------------------ Render
def render_ground(m):
    W, H = m.w * T, m.h * T
    c = Canvas(W, H, BLACK + (255,))
    for y in range(H):
        for x in range(W):
            tx, ty = x // T, y // T
            ch = m.grid[ty][tx]
            if m.interior:
                if ch == "#":
                    col = wall_top_px(x, y)
                    # borde inferior del muro superior
                    if m.t(tx, ty + 1) == "W" and y % T >= T - 2:
                        col = (44, 30, 28)
                elif ch == "W":
                    top = ty
                    while m.t(tx, top - 1) == "W":
                        top -= 1
                    bot = ty
                    while m.t(tx, bot + 1) == "W":
                        bot += 1
                    col = wall_face_px(x, y, y - top * T, (bot - top + 1) * T)
                elif ch in ".d":
                    col = wood_floor_px(x, y)
                    # oclusión ambiental junto a muros
                    ly = y % T
                    if m.t(tx, ty - 1) in "W#" and ly < 5:
                        col = darken(col, 0.35 - ly * 0.07)
                    lx = x % T
                    if m.t(tx - 1, ty) == "#" and lx < 3:
                        col = darken(col, 0.25 - lx * 0.08)
                    if m.t(tx + 1, ty) == "#" and lx > T - 4:
                        col = darken(col, 0.25 - (T - 1 - lx) * 0.08)
                else:
                    col = BLACK
                c.p[x, y] = col + (255,)
                continue
            # exterior: bordes orgánicos por desplazamiento con ruido
            jx = x + int((smooth_noise(x, y, 7, 90) - 0.5) * 9)
            jy = y + int((smooth_noise(x, y, 7, 91) - 0.5) * 9)
            ch = m.t(jx // T, jy // T)
            if ch == "p":
                col = dirt_px(x, y)
                near = [m.t((jx + dx) // T, (jy + dy) // T) for dx, dy in ((3, 0), (-3, 0), (0, 3), (0, -3))]
                if any(n not in "pc" for n in near):
                    col = darken(col, 0.12)
            elif ch == "c":
                col = cobble_px(x, y)
            elif ch == "f":
                col = forest_px(x, y)
            elif ch == "l":
                col = clearing_px(x, y)
            else:
                col = grass_px(x, y)
            c.p[x, y] = col + (255,)
    return c


def bake_shadows(m, c):
    for p in m.props:
        name = p["sprite"]
        if name in ("sign", "torch", "rail") or p.get("noshadow"):
            continue
        w, h = sprite_size(name)
        rx = min(w * 0.42, 40)
        ry = max(2.5, min(rx * 0.3, 7))
        if name.startswith(("oak", "pine", "dark_tree")):
            rx, ry = w * 0.45, 6
        if name == "cathedral":
            rx, ry = 96, 8
        if name.startswith("house"):
            rx, ry = w * 0.5, 5
        c.darken_ellipse(p["x"], p["y"] - 1, rx, ry, 0.32 if not m.interior else 0.25)


def solids_from_grid(m):
    rects = []
    if not m.interior:
        return rects
    for ty in range(m.h):
        tx = 0
        while tx < m.w:
            if m.grid[ty][tx] in "#W ":
                start = tx
                while tx < m.w and m.grid[ty][tx] in "#W ":
                    tx += 1
                rects.append([start * T, ty * T, (tx - start) * T, T])
            else:
                tx += 1
    return rects


def save_map(m):
    ground = render_ground(m)
    for dfn in m.decals:
        dfn(ground)
    bake_shadows(m, ground)
    ground.save(f"{OUT_DIR}/maps/{m.id}.png")
    data = {
        "id": m.id, "display": m.display, "music": m.music, "interior": m.interior,
        "size": [m.w * T, m.h * T], "ground": f"res://assets/maps/{m.id}.png",
        "solids": solids_from_grid(m) + m.extra_solids, "props": m.props, "markers": m.markers,
        "exits": m.exits, "triggers": m.triggers, "lights": m.lights,
    }
    if m.modulate:
        data["modulate"] = m.modulate
    with open(f"{OUT_DIR}/maps/{m.id}.json", "w") as f:
        json.dump(data, f, ensure_ascii=False, indent=1)


# ------------------------------------------------------------ Decals
def window_decal(x, y, night=False):
    def f(c):
        c.rect(x - 1, y - 1, 18, 22, WOOD_D)
        c.rect(x, y, 16, 20, (120, 170, 210))
        c.rect(x, y + 12, 16, 8, (150, 190, 150))
        c.rect(x + 2, y + 2, 5, 4, (200, 230, 245))
        c.vline(x + 7, y, 20, WOOD_M)
        c.vline(x + 8, y, 20, WOOD_D)
        c.hline(x, y + 9, 16, WOOD_M)
        c.rect(x - 2, y + 20, 20, 2, WOOD_L)
        # rayo de luz en el suelo
        for k in range(26):
            for w in range(16):
                c.px(x + w + k // 2, y + 26 + k, (255, 236, 180), 34 - k)
    return f


def rug_decal(x, y, w, h, col=CLOTH_R, border=GOLD):
    def f(c):
        c.rect(x, y, w, h, darken(col, 0.3))
        c.rect(x + 1, y + 1, w - 2, h - 2, col)
        c.rect(x + 3, y + 3, w - 6, h - 6, darken(col, 0.15))
        for xx in range(x + 3, x + w - 3):
            if (xx - x) % 4 < 2:
                c.px(xx, y + 2, border)
                c.px(xx, y + h - 3, border)
        for yy in range(y + 3, y + h - 3):
            if (yy - y) % 4 < 2:
                c.px(x + 2, yy, border)
                c.px(x + w - 3, yy, border)
        cx, cy = x + w // 2, y + h // 2
        for i in range(-4, 5):
            c.px(cx + i, cy - (4 - abs(i)), border)
            c.px(cx + i, cy + (4 - abs(i)), border)
        for xx in range(x - 2, x + w + 2, 2):
            c.px(xx, y - 1, CREAM_S)
            c.px(xx, y + h, CREAM_S)
    return f


def stairs_down_decal(x, y):
    def f(c):
        c.rect(x - 1, y - 1, 18, 34, WOOD_D)
        for k in range(8):
            shade = 1 - k / 8.0
            col = mix(BLACK, WOOD_L, shade * 0.9)
            c.rect(x, y + k * 4, 16, 3, col)
            c.hline(x, y + k * 4 + 3, 16, mix(BLACK, WOOD_D, shade))
    return f


def stairs_up_decal(x, y):
    def f(c):
        c.rect(x - 2, y, 20, 34, (44, 30, 28))
        for k in range(8):
            shade = 0.35 + k / 11.0
            yy = y + 32 - k * 4 - 4
            c.rect(x, yy, 16, 3, mix(BLACK, WOOD_L, shade))
            c.hline(x, yy + 3, 16, mix(BLACK, WOOD_D, shade))
        c.vline(x - 1, y, 34, WOOD_M)
        c.vline(x + 16, y, 34, WOOD_M)
    return f


def sword_decal(x, y):
    def f(c):
        s = Canvas(9, 26)
        s.rect(3, 1, 3, 16, WOOD_H)
        s.vline(3, 1, 16, lighten(WOOD_H, 0.3))
        s.px(4, 0, WOOD_H)
        s.rect(0, 17, 9, 2, CLOTH_R)
        s.rect(3, 19, 3, 5, WOOD_D)
        s.rect(3, 24, 3, 1, GOLD)
        s.outline()
        c.paste(s, x - 1, y - 1)
        c.px(x + 3, y - 3, STONE_D)
        c.vline(x + 3, y - 2, 2, (120, 110, 90))
    return f


def shelf_decal(x, y, w):
    def f(c):
        c.rect(x, y + 10, w, 2, WOOD_M)
        c.hline(x, y + 12, w, WOOD_D)
        rng = random.Random(x)
        xx = x + 1
        while xx < x + w - 4:
            kind = rng.choice(["jar", "jar", "pot", "bottle"])
            if kind == "jar":
                col = rng.choice([(180, 120, 60), (120, 150, 90), (200, 180, 120)])
                c.rect(xx, y + 4, 5, 6, col)
                c.rect(xx + 1, y + 3, 3, 1, WOOD_D)
                c.px(xx + 1, y + 5, lighten(col, 0.3))
                xx += 7
            elif kind == "pot":
                c.ellipse(xx + 3, y + 7, 3, 3, (150, 90, 60))
                xx += 7
            else:
                c.rect(xx + 1, y + 3, 2, 7, (90, 140, 120))
                c.px(xx + 1, y + 2, WOOD_D)
                xx += 5
    return f


def herbs_decal(x, y):
    def f(c):
        c.hline(x, y, 20, WOOD_D)
        for i, col in enumerate([LEAF_M, (160, 120, 170), LEAF_L, GOLD]):
            bx = x + 2 + i * 5
            c.vline(bx + 1, y + 1, 2, WOOD_D)
            c.rect(bx, y + 3, 3, 6, col)
            c.px(bx + 1, y + 9, darken(col, 0.3))
    return f


def mat_decal(x, y, w, h):
    def f(c):
        c.rect(x, y, w, h, (150, 110, 60))
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                if (xx + yy) % 3 == 0:
                    c.px(xx, yy, (120, 86, 48))
    return f


def door_light_decal(x, y, w):
    def f(c):
        for k in range(10):
            c.rect(x, y + k, w, 1, (255, 240, 200), 60 - k * 5)
    return f


def flowers_decal(m, count, seed, allowed="g", palette=None):
    palette = palette or [(240, 230, 120), (240, 240, 240), (220, 90, 90), (150, 150, 230), (230, 150, 200)]

    def f(c):
        rng = random.Random(seed)
        for i in range(count):
            x = rng.randint(0, m.w * T - 4)
            y = rng.randint(0, m.h * T - 4)
            if m.t(x // T, y // T) not in allowed:
                continue
            col = rng.choice(palette)
            for k in range(rng.randint(2, 5)):
                fx, fy = x + rng.randint(-4, 4), y + rng.randint(-3, 3)
                c.px(fx, fy + 1, darken(GRASS_D, 0.2))
                c.px(fx, fy, col)
    return f


def sunbeam_decal(cx, cy, rx, ry):
    def f(c):
        for yy in range(int(cy - ry), int(cy + ry)):
            for xx in range(int(cx - rx), int(cx + rx)):
                dx, dy = (xx - cx) / rx, (yy - cy) / ry
                d = dx * dx + dy * dy
                if d < 1:
                    c.px(xx, yy, (255, 245, 190), int(40 * (1 - d)))
    return f


def moonflower_decal(points):
    def f(c):
        mf = Image.open(f"{OUT_DIR}/sprites/moonflower.png")
        for (x, y) in points:
            c.paste(mf, x - 4, y - 4)
    return f


# ------------------------------------------------------------ Mapas
def bedroom():
    g = ["############",
         "#WWWWWWWWWW#",
         "#WWWWWWWWWW#",
         "#..........#",
         "#..........#",
         "#..........#",
         "#..........#",
         "#..........#",
         "#..........#",
         "############"]
    m = Map("bedroom", g, True, "Tu habitación", "home", modulate=[0.86, 0.8, 0.74])
    m.decals += [rug_decal(66, 88, 52, 34), window_decal(100, 20), window_decal(140, 20), sword_decal(123, 24),
                 stairs_down_decal(160, 112)]
    m.prop("bed", 30, 82, "bed")
    m.prop("nightstand", 53, 62, "nightstand", light={"r": 40, "color": [1, 0.8, 0.5], "e": 0.6, "flicker": True, "oy": -18})
    m.prop("wardrobe", 76, 62, "wardrobe")
    m.prop("chest", 30, 98, "chest")
    m.prop("table", 160, 82, "desk")
    m.prop("chair", 160, 96, col=True)
    m.prop("plant", 22, 144)
    m.prop("rail", 157, 146, col=False)
    m.extra_solids += [[152, 108, 5, 40]]
    m.marker("start", 60, 104)
    m.marker("from_downstairs", 142, 128)
    m.exit([162, 118, 12, 22], "housemain", "from_upstairs")
    m.trigger("window", [100, 44, 56, 8])
    m.trigger("sword", [122, 44, 12, 8])
    m.lights.append({"x": 108, "y": 60, "r": 70, "color": [1, 0.95, 0.8], "e": 0.35})
    m.lights.append({"x": 148, "y": 60, "r": 70, "color": [1, 0.95, 0.8], "e": 0.35})
    return m


def housemain():
    g = ["##############",
         "#WWWWWWWWWWWW#",
         "#WWWWWWWWWWWW#",
         "#............#",
         "#............#",
         "#............#",
         "#............#",
         "#............#",
         "#............#",
         "#............#",
         "######..######"]
    m = Map("housemain", g, True, "Casa", "home", modulate=[0.86, 0.8, 0.74])
    m.decals += [shelf_decal(88, 20, 34), herbs_decal(150, 22), window_decal(126, 20), stairs_up_decal(192, 14),
                 rug_decal(80, 96, 64, 40, CLOTH_B, CREAM), mat_decal(98, 150, 28, 10), door_light_decal(96, 160, 32)]
    m.prop("fireplace", 52, 66, "fireplace",
           light={"r": 110, "color": [1, 0.6, 0.3], "e": 0.9, "flicker": True, "oy": -12})
    m.prop("table_food", 112, 118, "kitchen_table")
    m.prop("chair", 90, 120)
    m.prop("chair", 134, 120)
    m.prop("bookshelf", 168, 62, "cupboard")
    m.prop("barrel", 26, 150)
    m.prop("sack", 40, 154)
    m.prop("crate", 204, 150)
    m.prop("barrel", 188, 152)
    m.prop("plant", 22, 72)
    m.marker("from_upstairs", 200, 70)
    m.marker("from_village", 112, 150)
    m.marker("mother", 86, 84)
    m.exit([196, 46, 10, 10], "bedroom", "from_downstairs")
    m.exit([100, 166, 24, 10], "village", "home_door", requires="mother_talked",
           blocked="Tu madre carraspea desde la cocina. Quizá deberías darle los buenos días.")
    m.lights.append({"x": 134, "y": 60, "r": 70, "color": [1, 0.95, 0.8], "e": 0.3})
    m.lights.append({"x": 112, "y": 170, "r": 60, "color": [1, 0.95, 0.85], "e": 0.35})
    return m


def village():
    W, H = 46, 34
    g = [["g"] * W for _ in range(H)]
    m = Map("village", ["g" * W] * H, False, "Tortosa", "village")
    # plaza y caminos
    m.fill(14, 12, 31, 19, "c")
    m.fill(21, 19, 23, 26, "p")
    m.fill(3, 25, 45, 26, "p")
    m.fill(9, 22, 10, 25, "p")       # casa del jugador
    m.fill(35, 21, 36, 25, "p")      # casa de Kaelen
    m.fill(8, 15, 13, 16, "p")       # herrería
    m.fill(32, 15, 38, 16, "p")      # casa marrón
    m.fill(22, 26, 23, 30, "p")      # hacia el sur
    m.fill(20, 30, 28, 31, "p")
    # catedral
    m.prop("cathedral", 368, 196, "cathedral")
    # plaza
    m.prop("well", 368, 272, "well")
    m.prop("stall", 290, 250, "stall")
    m.prop("crate", 262, 252)
    m.prop("barrel", 318, 252)
    for tx in (232, 504):
        m.prop("torch", tx, 206)
        m.prop("torch", tx, 318)
    # casas
    m.prop("house_red", 160, 356, "house_player")
    m.prop("house_blue", 568, 340, "house_kaelen")
    m.prop("house_smithy", 132, 238, "smithy")
    m.prop("house_brown", 600, 238, "house_brown")
    m.prop("house_brown", 408, 520, "house_south", extra={"noshadow": False})
    m.prop("anvil", 196, 258, "anvil")
    m.prop("woodpile", 84, 256)
    m.prop("barrel", 188, 236)
    # jardín del jugador
    for fx in range(88, 138, 16):
        m.prop("fence_h", fx, 404)
    for fx in range(184, 234, 16):
        m.prop("fence_h", fx, 404)
    m.prop("hay", 212, 376)
    m.prop("crate", 108, 372)
    m.prop("barrel", 94, 374)
    m.prop("bush_berries", 60, 350)
    # casa de Kaelen
    m.prop("woodpile", 628, 364)
    for fx in range(500, 540, 16):
        m.prop("fence_h", fx, 388)
    m.prop("bush", 612, 392)
    # cartel del bosque
    m.prop("sign", 690, 398, "sign_forest")
    # árboles del borde
    rng = random.Random(7)
    border = []
    for x in range(8, W * T, 30):
        border.append((x + rng.randint(-6, 6), 34 + rng.randint(0, 10)))
        border.append((x + rng.randint(-6, 6), H * T - 6 - rng.randint(0, 6)))
    for y in range(60, H * T - 20, 28):
        border.append((10 + rng.randint(0, 8), y + rng.randint(-4, 4)))
        if not (380 <= y <= 440):
            border.append((W * T - 12 - rng.randint(0, 6), y + rng.randint(-4, 4)))
    for (x, y) in border:
        if 250 < x < 490 and y < 100:
            continue  # tras la catedral dejamos pinos lejanos
        if 330 < x < 400 and y > 480:
            continue
        name = rng.choice(["oak", "oak2", "pine", "pine2", "oak"])
        m.prop(name, x, y)
    for (x, y) in ((240, 60), (270, 44), (470, 56), (500, 40), (210, 90), (530, 86)):
        m.prop("pine" if x % 2 else "pine2", x, y)
    for (x, y, n) in ((60, 150, "oak"), (250, 470, "oak2"), (520, 470, "oak"), (660, 180, "pine"),
                      (620, 460, "pine2"), (100, 470, "oak"), (300, 400, "bush"), (450, 300, "bush_berries"),
                      (470, 420, "rock"), (80, 300, "rock_big"), (640, 290, "stump"), (40, 420, "bush")):
        m.prop(n, x, y)
    m.decals.append(flowers_decal(m, 260, 3))
    # puntos
    m.marker("home_door", 160, 372)
    m.marker("from_forest", 700, 418)
    m.marker("kaelen_start", 540, 380)
    m.marker("bartolo", 462, 214)
    m.marker("remei", 398, 284)
    m.marker("roc", 214, 262)
    m.marker("nil", 300, 330)
    m.exit([150, 344, 20, 10], "housemain", "from_village")
    m.exit([W * T - 10, 392, 10, 48], "forest", "from_village", requires="kaelen_joined",
           blocked="Kaelen quería decirte algo antes de salir del pueblo.")
    return m


def forest():
    W, H = 50, 32
    m = Map("forest", ["f" * W] * H, False, "Bosque Santo", "forest", modulate=[0.82, 0.9, 0.84])
    # sendero serpenteante
    pts = [(0, 15), (6, 15), (10, 13), (15, 14), (20, 16), (26, 15), (31, 13), (36, 14), (41, 15), (45, 15)]
    for (a, b) in zip(pts, pts[1:]):
        steps = max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2
        for i in range(steps + 1):
            x = a[0] + (b[0] - a[0]) * i / steps
            y = a[1] + (b[1] - a[1]) * i / steps
            m.fill(int(x), int(y), int(x) + 1, int(y) + 1, "p")
    # claro
    for y in range(H):
        for x in range(W):
            if ((x - 20) / 7.5) ** 2 + ((y - 15) / 6) ** 2 < 1 and m.grid[y][x] != "p":
                m.grid[y][x] = "l"
    for y in range(H):
        for x in range(W):
            if ((x - 42) / 5) ** 2 + ((y - 15) / 4) ** 2 < 1 and m.grid[y][x] != "p":
                m.grid[y][x] = "l"

    def near_open(tx, ty, r):
        for yy in range(ty - r, ty + r + 1):
            for xx in range(tx - r, tx + r + 1):
                if m.t(xx, yy) in "pl":
                    return True
        return False

    rng = random.Random(11)
    placed = []
    def blocks_view(tx, ty):
        for yy in range(ty - 4, ty + 2):
            for xx in range(tx - 1, tx + 2):
                if m.t(xx, yy) in "pl":
                    return True
        return False

    for ty in range(0, H + 2):
        for tx in range(-1, W + 1):
            if blocks_view(tx, ty):
                continue
            if rng.random() > 0.62:
                continue
            x = tx * T + rng.randint(0, 12)
            y = ty * T + rng.randint(4, 14)
            if any((x - px) ** 2 + (y - py) ** 2 < 24 ** 2 for px, py in placed):
                continue
            placed.append((x, y))
            name = rng.choice(["dark_tree", "dark_tree2", "pine", "pine2", "dark_tree", "dark_tree2", "pine"])
            m.prop(name, x, y)
    # detalles del borde del sendero
    for i in range(70):
        tx, ty = rng.randint(1, W - 2), rng.randint(1, H - 2)
        if m.t(tx, ty) == "f" and near_open(tx, ty, 1) and not near_open(tx, ty, 0):
            x, y = tx * T + 8, ty * T + 12
            if any((x - px) ** 2 + (y - py) ** 2 < 18 ** 2 for px, py in placed):
                continue
            placed.append((x, y))
            m.prop(rng.choice(["bush", "bush", "bush_berries", "rock", "stump", "rock_big"]), x, y)
    # santuario
    m.prop("shrine", 322, 206, "shrine", light={"r": 90, "color": [0.55, 0.85, 1.0], "e": 0.8, "flicker": True, "oy": -20})
    m.decals += [sunbeam_decal(300, 240, 70, 40), sunbeam_decal(680, 240, 40, 26),
                 moonflower_decal([(280, 262), (292, 270), (270, 276), (300, 284), (262, 262), (286, 290),
                                   (356, 250), (348, 262), (340, 280), (250, 240), (364, 276)]),
                 flowers_decal(m, 120, 9, allowed="l")]
    m.marker("from_village", 20, 250)
    m.marker("yara", 282, 268)
    m.marker("wolf", 700, 240)
    m.marker("party_vs_wolf", 612, 250)
    m.exit([0, 224, 8, 56], "village", "from_forest")
    m.trigger("meet_yara", [220, 170, 20, 170])
    m.trigger("wolf_zone", [590, 150, 16, 200])
    return m


def build_all(out):
    global OUT_DIR
    OUT_DIR = out
    import os
    os.makedirs(f"{out}/maps", exist_ok=True)
    for fn in (bedroom, housemain, village, forest):
        save_map(fn())
