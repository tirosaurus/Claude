"""Utilidades de pixel art: lienzo, paleta, ruido determinista."""
import math
import os
from PIL import Image

# --- Paleta -----------------------------------------------------------------
OUT = (30, 24, 34)
BLACK = (18, 14, 22)

WOOD_D = (86, 52, 38)
WOOD_M = (128, 82, 52)
WOOD_L = (170, 116, 70)
WOOD_H = (200, 146, 92)

STONE_D = (64, 64, 78)
STONE_M = (104, 102, 116)
STONE_L = (146, 144, 154)
STONE_H = (184, 182, 188)

PLASTER = (222, 204, 168)
PLASTER_S = (190, 170, 136)
PLASTER_D = (160, 138, 108)

ROOF_D = (112, 44, 40)
ROOF_M = (156, 66, 50)
ROOF_L = (194, 96, 66)
ROOF_B_D = (46, 62, 96)
ROOF_B_M = (66, 90, 132)
ROOF_B_L = (96, 124, 164)

GRASS_D = (56, 104, 58)
GRASS_M = (82, 138, 66)
GRASS_L = (118, 170, 82)
GRASS_H = (156, 196, 104)

FGRASS_D = (30, 62, 48)
FGRASS_M = (44, 88, 58)
FGRASS_L = (66, 116, 66)

DIRT_D = (116, 84, 58)
DIRT_M = (152, 114, 78)
DIRT_L = (184, 148, 104)

LEAF_D = (32, 74, 50)
LEAF_M = (52, 108, 60)
LEAF_L = (88, 144, 70)
LEAF_H = (132, 178, 90)

PINE_D = (22, 56, 50)
PINE_M = (34, 82, 64)
PINE_L = (58, 112, 78)

CLOTH_R = (168, 52, 56)
CLOTH_R_D = (118, 36, 44)
CLOTH_B = (62, 92, 158)
CLOTH_B_D = (42, 60, 110)
CLOTH_G = (70, 126, 90)
CLOTH_G_D = (46, 88, 64)
CREAM = (236, 224, 192)
CREAM_S = (200, 186, 152)
WHITE = (246, 242, 230)

SKIN = (236, 188, 150)
SKIN_S = (200, 144, 112)

GOLD = (226, 184, 82)
GOLD_D = (170, 124, 50)
FIRE_1 = (255, 222, 120)
FIRE_2 = (250, 150, 60)
FIRE_3 = (208, 76, 48)

SAP = (26, 16, 30)
SAP_L = (70, 40, 78)


def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def darken(c, t=0.25):
    return mix(c, BLACK, t)


def lighten(c, t=0.25):
    return mix(c, (255, 250, 235), t)


# --- Ruido determinista -------------------------------------------------------
def hash2(x, y, seed=0):
    h = (x * 374761393 + y * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    h ^= h >> 16
    return (h & 0xFFFFFF) / float(0xFFFFFF)


def smooth_noise(x, y, scale, seed=0):
    fx, fy = x / scale, y / scale
    ix, iy = math.floor(fx), math.floor(fy)
    tx, ty = fx - ix, fy - iy
    tx = tx * tx * (3 - 2 * tx)
    ty = ty * ty * (3 - 2 * ty)
    a = hash2(ix, iy, seed)
    b = hash2(ix + 1, iy, seed)
    c = hash2(ix, iy + 1, seed)
    d = hash2(ix + 1, iy + 1, seed)
    return a + (b - a) * tx + (c - a) * ty + (a - b - c + d) * tx * ty


def fbm(x, y, scale, seed=0):
    return (smooth_noise(x, y, scale, seed) * 0.6
            + smooth_noise(x, y, scale / 2, seed + 7) * 0.3
            + smooth_noise(x, y, scale / 4, seed + 13) * 0.1)


# --- Lienzo -------------------------------------------------------------------
class Canvas:
    def __init__(self, w, h, fill=(0, 0, 0, 0)):
        self.w, self.h = w, h
        self.im = Image.new("RGBA", (w, h), fill)
        self.p = self.im.load()

    def px(self, x, y, c, a=255):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            if len(c) == 4:
                a = c[3]
                c = c[:3]
            if a >= 255:
                self.p[x, y] = (c[0], c[1], c[2], 255)
            elif a > 0:
                o = self.p[x, y]
                t = a / 255.0
                if o[3] == 0:
                    self.p[x, y] = (c[0], c[1], c[2], a)
                else:
                    self.p[x, y] = (int(o[0] + (c[0] - o[0]) * t), int(o[1] + (c[1] - o[1]) * t),
                                    int(o[2] + (c[2] - o[2]) * t), max(o[3], a))

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.p[x, y]
        return (0, 0, 0, 0)

    def rect(self, x, y, w, h, c, a=255):
        for yy in range(int(y), int(y + h)):
            for xx in range(int(x), int(x + w)):
                self.px(xx, yy, c, a)

    def hline(self, x, y, w, c):
        self.rect(x, y, w, 1, c)

    def vline(self, x, y, h, c):
        self.rect(x, y, 1, h, c)

    def ellipse(self, cx, cy, rx, ry, c, a=255):
        for yy in range(int(cy - ry - 1), int(cy + ry + 2)):
            for xx in range(int(cx - rx - 1), int(cx + rx + 2)):
                dx = (xx + 0.5 - cx) / max(rx, 0.1)
                dy = (yy + 0.5 - cy) / max(ry, 0.1)
                if dx * dx + dy * dy <= 1.0:
                    self.px(xx, yy, c, a)

    def darken_ellipse(self, cx, cy, rx, ry, amount=0.35):
        for yy in range(int(cy - ry - 1), int(cy + ry + 2)):
            for xx in range(int(cx - rx - 1), int(cx + rx + 2)):
                dx = (xx + 0.5 - cx) / max(rx, 0.1)
                dy = (yy + 0.5 - cy) / max(ry, 0.1)
                d = dx * dx + dy * dy
                if d <= 1.0 and 0 <= xx < self.w and 0 <= yy < self.h:
                    o = self.p[xx, yy]
                    if o[3] > 0:
                        k = amount * (1.0 - d * 0.5)
                        self.p[xx, yy] = (int(o[0] * (1 - k)), int(o[1] * (1 - k)), int(o[2] * (1 - k)), o[3])

    def outline(self, c=OUT):
        solid = [[self.p[x, y][3] > 0 for x in range(self.w)] for y in range(self.h)]
        for y in range(self.h):
            for x in range(self.w):
                if solid[y][x]:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < self.w and 0 <= ny < self.h and solid[ny][nx]:
                        self.p[x, y] = c + (255,)
                        break

    def paste(self, other, x, y):
        img = other.im if isinstance(other, Canvas) else other
        self.im.alpha_composite(img, (int(x), int(y)))
        self.p = self.im.load()

    def flip_h(self):
        c = Canvas(self.w, self.h)
        c.im = self.im.transpose(Image.FLIP_LEFT_RIGHT)
        c.p = c.im.load()
        return c

    def save(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        self.im.save(path)
