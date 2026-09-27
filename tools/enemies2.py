"""Enemigos v3: esculpido con volúmenes (elipsoides y cápsulas) y sombreado
por rampa de colores con luz desde arriba-izquierda, más detalles a mano.

Cada enemigo: 2 frames en horizontal (respiración / aleteo).
"""
import math
from artlib import Canvas, OUT, mix, darken, lighten

LIGHT = (-0.55, -0.75, 0.65)
_l = math.sqrt(sum(v * v for v in LIGHT))
LIGHT = tuple(v / _l for v in LIGHT)
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def ramp(dark, mid, light, n=5):
    out = []
    for i in range(n):
        t = i / (n - 1)
        out.append(mix(dark, mid, t * 2) if t < 0.5 else mix(mid, light, (t - 0.5) * 2))
    return out


MAT = {
    "purple": ramp((34, 18, 48), (104, 52, 128), (196, 140, 214)),
    "flesh": ramp((48, 22, 44), (140, 70, 110), (230, 160, 190)),
    "bark": ramp((26, 18, 16), (86, 56, 42), (166, 124, 90)),
    "bark_dark": ramp((20, 12, 20), (70, 40, 66), (140, 96, 130)),
    "stone": ramp((36, 36, 48), (108, 106, 120), (206, 204, 210)),
    "moss": ramp((20, 40, 28), (58, 100, 58), (140, 180, 100)),
    "crystal": ramp((30, 60, 110), (90, 170, 220), (230, 250, 255)),
    "bat": ramp((22, 18, 30), (74, 60, 84), (150, 130, 150)),
    "wing": ramp((28, 18, 34), (92, 56, 88), (168, 120, 150)),
    "bone": ramp((70, 60, 50), (190, 176, 150), (250, 244, 226)),
    "dark": ramp((16, 10, 22), (56, 30, 70), (130, 80, 150)),
    "chitin": ramp((20, 14, 26), (66, 40, 86), (160, 110, 190)),
}


class Sculpt:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.depth = [[-1e9] * w for _ in range(h)]
        self.mat = [[None] * w for _ in range(h)]
        self.norm = [[None] * w for _ in range(h)]

    def _put(self, x, y, z, m, n):
        if 0 <= x < self.w and 0 <= y < self.h and z > self.depth[y][x]:
            self.depth[y][x] = z
            self.mat[y][x] = m
            self.norm[y][x] = n

    def ellipsoid(self, cx, cy, rx, ry, m, cz=0.0, rz=None, flat=1.0):
        rz = rz if rz is not None else min(rx, ry)
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                nx = (x + 0.5 - cx) / rx
                ny = (y + 0.5 - cy) / ry
                d = nx * nx + ny * ny
                if d <= 1.0:
                    nz = math.sqrt(1 - d)
                    n = (nx * flat, ny * flat, nz)
                    self._put(x, y, cz + nz * rz, m, n)

    def capsule(self, x0, y0, x1, y1, r0, r1, m, cz=0.0):
        dx, dy = x1 - x0, y1 - y0
        L2 = dx * dx + dy * dy or 1
        rmax = max(r0, r1)
        for y in range(int(min(y0, y1) - rmax - 1), int(max(y0, y1) + rmax + 2)):
            for x in range(int(min(x0, x1) - rmax - 1), int(max(x0, x1) + rmax + 2)):
                px, py = x + 0.5, y + 0.5
                t = max(0, min(1, ((px - x0) * dx + (py - y0) * dy) / L2))
                qx, qy = x0 + dx * t, y0 + dy * t
                r = r0 + (r1 - r0) * t
                ox, oy = px - qx, py - qy
                d = math.sqrt(ox * ox + oy * oy)
                if d <= r:
                    nx, ny = ox / r, oy / r
                    nz = math.sqrt(max(0, 1 - nx * nx - ny * ny))
                    self._put(x, y, cz + nz * r, m, (nx, ny, nz))

    def polygon(self, pts, m, cz=0.0, normal=(0, 0, 1)):
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        for y in range(int(min(ys)), int(max(ys)) + 1):
            for x in range(int(min(xs)), int(max(xs)) + 1):
                if _inside(x + 0.5, y + 0.5, pts):
                    self._put(x, y, cz, m, normal)

    def render(self, outline=OUT, dither=True, rim=True):
        c = Canvas(self.w, self.h)
        for y in range(self.h):
            for x in range(self.w):
                m = self.mat[y][x]
                if m is None:
                    continue
                n = self.norm[y][x]
                ln = math.sqrt(n[0] ** 2 + n[1] ** 2 + n[2] ** 2) or 1
                nd = (n[0] * LIGHT[0] + n[1] * LIGHT[1] + n[2] * LIGHT[2]) / ln
                v = max(0.0, nd) * 0.85 + 0.12
                # oclusión por diferencia de profundidad con vecinos
                if rim:
                    for ddx, ddy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        xx, yy = x + ddx, y + ddy
                        if 0 <= xx < self.w and 0 <= yy < self.h and self.mat[yy][xx] is not None:
                            if self.depth[yy][xx] - self.depth[y][x] > 3.0:
                                v -= 0.18
                r = MAT[m]
                t = v * (len(r) - 1)
                if dither:
                    t += (BAYER[y % 4][x % 4] / 16.0 - 0.5) * 0.55
                i = max(0, min(len(r) - 1, int(round(t))))
                c.px(x, y, r[i])
        # contorno interior entre piezas a distinta profundidad
        for y in range(self.h):
            for x in range(self.w):
                if self.mat[y][x] is None:
                    continue
                for ddx, ddy in ((1, 0), (0, 1), (-1, 0), (0, -1)):
                    xx, yy = x + ddx, y + ddy
                    if 0 <= xx < self.w and 0 <= yy < self.h and self.mat[yy][xx] is not None:
                        if self.depth[yy][xx] - self.depth[y][x] > 5.0 and self.mat[yy][xx] != self.mat[y][x]:
                            p = c.get(x, y)
                            c.px(x, y, darken(p[:3], 0.45))
                            break
        if outline:
            c.outline(outline)
        return c


def _inside(x, y, pts):
    ins = False
    j = len(pts) - 1
    for i in range(len(pts)):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi + 1e-9) + xi:
            ins = not ins
        j = i
    return ins


def glow_eye(c, x, y, col, size=1):
    core = lighten(col, 0.6)
    if size == 1:
        c.px(x, y, core)
        c.px(x + 1, y, col)
    else:
        c.rect(x, y, size + 1, size, col)
        c.px(x, y, core)
        c.px(x + 1, y, core)


def halo(c, x, y, col, r=3, alpha=90):
    for yy in range(y - r, y + r + 1):
        for xx in range(x - r, x + r + 1):
            d = math.hypot(xx - x, yy - y)
            if d <= r and c.get(xx, yy)[3] > 0:
                c.px(xx, yy, col, int(alpha * (1 - d / r)))


def sheet(frames):
    w, h = frames[0].w, frames[0].h
    s = Canvas(w * len(frames), h)
    for i, f in enumerate(frames):
        s.paste(f, i * w, 0)
    return s


# =========================================================================== Enemigos
def larva(f):
    W, H = 48, 26
    s = Sculpt(W, H)
    bob = [0, 1][f]
    segs = 6
    for i in range(segs):
        t = i / (segs - 1)
        cx = 40 - i * 6.2
        cy = 15 + math.sin(i * 1.3 + f * 1.5) * 1.2
        r = 6.5 - abs(t - 0.25) * 4
        s.ellipsoid(cx, cy + (bob if i % 2 else 0), r * 0.95, r, "chitin", cz=i * -0.3)
    # cabeza
    s.ellipsoid(42, 13, 6.5, 6, "flesh", cz=2)
    # mandíbulas
    s.capsule(45, 16, 48, 20, 1.3, 0.6, "bone", cz=4)
    s.capsule(41, 17, 44, 21, 1.3, 0.6, "bone", cz=4)
    c = s.render()
    # manchas brillantes y ojos
    for i in range(1, segs):
        cx = int(40 - i * 6.2)
        c.px(cx, 11 + (bob if i % 2 else 0), (240, 120, 230))
        c.px(cx - 1, 11 + (bob if i % 2 else 0), (170, 60, 190))
    glow_eye(c, 42, 11, (255, 90, 200))
    glow_eye(c, 45, 12, (255, 90, 200))
    # patitas
    for i in range(1, segs):
        cx = int(40 - i * 6.2)
        c.px(cx - 1 + f, 21, OUT)
        c.px(cx + 1 - f, 21, OUT)
    return c


def bat(f):
    W, H = 56, 36
    s = Sculpt(W, H)
    up = f == 0
    # alas (membrana)
    for side in (-1, 1):
        cx = 28
        tip_y = 4 if up else 24
        pts = [(cx + side * 4, 14), (cx + side * 12, 6 if up else 16), (cx + side * 24, tip_y),
               (cx + side * 20, 18 if up else 26), (cx + side * 15, 17 if up else 22), (cx + side * 10, 20),
               (cx + side * 5, 19)]
        s.polygon(pts, "wing", cz=-2, normal=(side * 0.3, 0.2 if up else -0.3, 0.9))
        # huesos del ala
        s.capsule(cx + side * 4, 14, cx + side * 24, tip_y, 1.0, 0.6, "bat", cz=0)
        s.capsule(cx + side * 12, 6 if up else 16, cx + side * 15, 17 if up else 22, 0.8, 0.5, "bat", cz=0)
    s.ellipsoid(28, 17, 6, 7.5, "bat", cz=3)
    s.ellipsoid(28, 10, 4.5, 4, "bat", cz=5)
    # orejas
    s.polygon([(24, 8), (25, 2), (27, 7)], "bat", cz=4, normal=(-0.3, -0.3, 0.9))
    s.polygon([(29, 7), (31, 2), (32, 8)], "bat", cz=4, normal=(0.3, -0.3, 0.9))
    c = s.render()
    glow_eye(c, 26, 10, (170, 255, 120))
    glow_eye(c, 29, 10, (170, 255, 120))
    c.px(27, 13, (240, 240, 230))
    c.px(29, 13, (240, 240, 230))
    # esporas
    for (x, y) in ((10, 28), (44, 30), (20, 33), (38, 26), (50, 12), (6, 16)):
        yy = y - f * 2
        c.px(x, yy, (190, 240, 140), 200)
    return c


def brute(f):
    W, H = 80, 84
    s = Sculpt(W, H)
    br = [0, 1][f]
    # piernas
    s.capsule(30, 58, 27, 76, 7, 6, "purple", cz=-4)
    s.capsule(50, 58, 53, 76, 7, 6, "purple", cz=-4)
    s.ellipsoid(26, 79, 8, 4, "dark", cz=-2)
    s.ellipsoid(54, 79, 8, 4, "dark", cz=-2)
    # torso masivo
    s.ellipsoid(40, 42 + br, 22, 20, "purple", cz=0)
    s.ellipsoid(40, 56 + br, 16, 9, "purple", cz=2)
    # hombros y brazos largos
    for side in (-1, 1):
        sx = 40 + side * 20
        s.ellipsoid(sx, 30 + br, 10, 9, "purple", cz=6)
        s.capsule(sx + side * 3, 34 + br, sx + side * 6, 54 + br, 7, 6, "purple", cz=8)
        s.capsule(sx + side * 6, 54 + br, sx + side * 4, 68 + br, 6, 8, "purple", cz=10)
        s.ellipsoid(sx + side * 4, 70 + br, 8, 6, "dark", cz=12)
    # cabeza hundida
    s.ellipsoid(40, 24 + br, 10, 9, "purple", cz=10)
    s.ellipsoid(40, 29 + br, 8, 4, "dark", cz=13)
    # cristales en la espalda
    for (x, y, h) in ((26, 20, 10), (34, 14, 12), (46, 14, 12), (54, 20, 10)):
        s.polygon([(x - 3, y + 6 + br), (x, y - h + 6 + br), (x + 3, y + 6 + br)], "crystal", cz=4,
                  normal=(-0.5, -0.3, 0.8))
    c = s.render()
    glow_eye(c, 35, 22 + br, (255, 80, 220), 2)
    glow_eye(c, 43, 22 + br, (255, 80, 220), 2)
    for (x, y) in ((36, 30), (39, 31), (42, 30), (45, 31)):
        c.px(x, y + br, (236, 226, 200))
    # venas brillantes
    for (x, y) in ((30, 44), (31, 45), (32, 47), (50, 40), (49, 42), (48, 43), (40, 52), (41, 53)):
        c.px(x, y + br, (230, 110, 240))
    halo(c, 36, 22 + br, (255, 120, 230), 4, 70)
    halo(c, 44, 22 + br, (255, 120, 230), 4, 70)
    return c


def root(f):
    W, H = 64, 52
    s = Sculpt(W, H)
    sw = [0, 1][f]
    for i, (x0, x1, y1) in enumerate(((20, 6, 50), (26, 18, 51), (36, 44, 51), (42, 58, 49), (30, 30, 51))):
        s.capsule(32, 30, x1 + (sw if i % 2 else -sw), y1, 5, 1.5, "bark", cz=-2)
    # zarcillos superiores
    s.capsule(24, 24, 12 - sw * 2, 8, 3, 1, "bark", cz=-1)
    s.capsule(40, 24, 54 + sw * 2, 10, 3, 1, "bark", cz=-1)
    s.ellipsoid(32, 28, 14, 12, "bark_dark", cz=2)
    s.ellipsoid(32, 30, 8, 7, "flesh", cz=10)
    c = s.render()
    # ojo central
    c.ellipse(32, 30, 4, 3.5, (250, 230, 200))
    c.ellipse(32, 30, 2, 3, (200, 40, 160))
    c.px(32, 30, (20, 10, 20))
    c.px(31, 28, (255, 255, 255))
    for (x, y) in ((22, 20), (44, 22), (28, 40), (38, 38)):
        c.px(x, y, (230, 120, 240))
    return c


def spectre(f):
    W, H = 48, 58
    s = Sculpt(W, H)
    fl = [0, 2][f]
    s.ellipsoid(24, 18 + fl, 11, 12, "crystal", cz=4)
    s.polygon([(13, 20 + fl), (35, 20 + fl), (38, 42 + fl), (32, 50 + fl), (28, 44 + fl), (24, 55 + fl),
               (20, 45 + fl), (15, 52 + fl), (10, 42 + fl)], "crystal", cz=0, normal=(-0.2, 0.1, 0.95))
    for side in (-1, 1):
        s.capsule(24 + side * 9, 24 + fl, 24 + side * 17, 36 + fl - side * 2, 3, 2, "crystal", cz=2)
    c = s.render()
    # translúcido
    for y in range(H):
        for x in range(W):
            p = c.get(x, y)
            if p[3] and p[:3] != OUT:
                c.p[x, y] = (p[0], p[1], p[2], 220)
    c.rect(18, 15 + fl, 3, 4, (20, 30, 60))
    c.rect(27, 15 + fl, 3, 4, (20, 30, 60))
    c.px(19, 16 + fl, (200, 255, 255))
    c.px(28, 16 + fl, (200, 255, 255))
    c.ellipse(24, 24 + fl, 2.5, 2, (20, 30, 60))
    for (x, y) in ((12, 10), (38, 14), (8, 30), (42, 34)):
        c.px(x, y - fl, (220, 250, 255), 200)
    return c


def custodian(f):
    W, H = 104, 116
    s = Sculpt(W, H)
    br = [0, 1][f]
    # piernas pilar
    for side in (-1, 1):
        s.capsule(52 + side * 16, 78, 52 + side * 18, 106, 10, 11, "stone", cz=-6)
        s.ellipsoid(52 + side * 18, 108, 13, 6, "stone", cz=-4)
    s.ellipsoid(52, 56 + br, 30, 28, "stone", cz=0, flat=0.8)
    s.ellipsoid(52, 78 + br, 22, 10, "stone", cz=2)
    for side in (-1, 1):
        sx = 52 + side * 32
        s.ellipsoid(sx, 38 + br, 14, 12, "stone", cz=8)
        s.capsule(sx + side * 4, 44 + br, sx + side * 8, 72 + br, 10, 9, "stone", cz=10)
        s.ellipsoid(sx + side * 8, 80 + br, 13, 12, "stone", cz=12)
    s.ellipsoid(52, 24 + br, 13, 11, "stone", cz=12)
    # musgo
    s.ellipsoid(40, 34 + br, 8, 3, "moss", cz=26)
    s.ellipsoid(70, 70 + br, 6, 3, "moss", cz=24)
    # núcleo
    s.ellipsoid(52, 56 + br, 9, 9, "crystal", cz=30)
    c = s.render()
    # grietas
    for (x0, y0, pts) in ((34, 48, ((1, 1), (1, 2), (2, 3), (2, 4))), (66, 62, ((1, 0), (2, 1), (2, 2), (3, 3))),
                          (84, 34, ((0, 1), (1, 2), (1, 3)))):
        for dx, dy in pts:
            c.px(x0 + dx, y0 + dy + br, (40, 40, 54))
    glow_eye(c, 45, 22 + br, (160, 240, 255), 3)
    glow_eye(c, 56, 22 + br, (160, 240, 255), 3)
    halo(c, 52, 56 + br, (190, 250, 255), 16, 70)
    for (x, y) in ((30, 40), (74, 44), (48, 90), (60, 30)):
        c.px(x, y + br, (170, 240, 255))
    return c


def mother_root(f):
    W, H = 184, 156
    s = Sculpt(W, H)
    pulse = [0, 2][f]
    import random
    rnd = random.Random(7)
    # tentáculos de fondo
    for i in range(14):
        x0 = 20 + i * 11
        sway = math.sin(i * 1.7 + f) * 4
        s.capsule(92, 90, x0 + sway, 150, 7, 2, "bark_dark", cz=-10)
    for i in range(8):
        a = -math.pi + i * math.pi / 7
        x1 = 92 + math.cos(a) * 86 + math.sin(i + f) * 3
        y1 = 70 + math.sin(a) * 60
        s.capsule(92, 70, x1, y1, 8, 2, "bark_dark", cz=-8)
        s.capsule(x1, y1, x1 + math.cos(a + 0.8) * 16, y1 + math.sin(a + 0.8) * 14 + 6, 2.2, 1, "bark_dark", cz=-7)
    # cuerpo bulboso
    s.ellipsoid(92, 78, 50 + pulse, 44 + pulse, "bark_dark", cz=0, flat=0.9)
    for (x, y, r) in ((60, 60, 16), (124, 62, 15), (70, 104, 18), (116, 104, 17), (92, 44, 18)):
        s.ellipsoid(x, y, r, r * 0.85, "flesh", cz=18)
    s.ellipsoid(92, 78, 24 + pulse, 22 + pulse, "flesh", cz=26)
    c = s.render()
    # ojo gigante
    c.ellipse(92, 78, 16 + pulse, 13 + pulse, (240, 222, 196))
    c.ellipse(92, 78, 9 + pulse, 11 + pulse, (200, 40, 170))
    c.ellipse(92, 78, 4, 9, (26, 8, 30))
    c.rect(86, 70, 3, 3, (255, 255, 255))
    for k in range(12):
        a = k * math.pi / 6
        x = int(92 + math.cos(a) * (19 + pulse))
        y = int(78 + math.sin(a) * (16 + pulse))
        c.px(x, y, (140, 20, 90))
    # ojos menores
    for (x, y) in ((60, 58), (124, 60), (70, 102), (116, 102), (92, 42)):
        c.ellipse(x, y, 3, 2.5, (255, 200, 90))
        c.px(x, y, (40, 10, 20))
        halo(c, x, y, (255, 200, 120), 6, 60)
    for _ in range(40):
        x, y = rnd.randint(30, 154), rnd.randint(30, 140)
        if c.get(x, y)[3]:
            c.px(x, y, (230, 120, 240))
    halo(c, 92, 78, (255, 120, 220), 30, 50)
    return c


# Nuevos enemigos para variedad
def wisp(f):
    """Fuego fatuo corrupto."""
    W, H = 32, 36
    s = Sculpt(W, H)
    fl = [0, 1][f]
    s.ellipsoid(16, 20 + fl, 8, 8, "purple", cz=2)
    s.polygon([(9, 18 + fl), (12, 6 + fl * 2), (15, 12 + fl), (17, 2 + fl), (20, 12 + fl), (23, 7 - fl), (24, 18 + fl)],
              "purple", cz=1, normal=(0, -0.4, 0.9))
    c = s.render()
    c.px(13, 20 + fl, (255, 255, 255))
    c.px(18, 20 + fl, (255, 255, 255))
    c.px(13, 21 + fl, (40, 10, 50))
    c.px(18, 21 + fl, (40, 10, 50))
    halo(c, 16, 18 + fl, (255, 160, 255), 10, 80)
    return c


def boar(f):
    """Jabalí corrompido."""
    W, H = 64, 40
    s = Sculpt(W, H)
    st = [0, 1][f]
    for (x, dx) in ((18, -1), (24, 1), (42, -1), (48, 1)):
        s.capsule(x, 26, x + dx * st, 37, 3, 2.5, "bark", cz=-3)
    s.ellipsoid(32, 22, 20, 12, "bark", cz=0)
    s.ellipsoid(50, 22, 10, 9, "bark", cz=4)
    s.ellipsoid(58, 25, 4, 4, "flesh", cz=8)
    for i in range(7):
        x = 18 + i * 4
        s.polygon([(x - 2, 12), (x, 4 + (i % 2) * 2), (x + 2, 12)], "crystal" if i % 3 == 1 else "dark", cz=3,
                  normal=(-0.4, -0.5, 0.7))
    s.capsule(56, 28, 60, 22, 1.2, 0.6, "bone", cz=10)
    c = s.render()
    glow_eye(c, 51, 19, (255, 80, 200))
    return c


def nhalzur(f):
    """Nhal'Zur, el Hambre bajo el Mundo: jefe secreto."""
    W, H = 200, 176
    s = Sculpt(W, H)
    br = [0, 2][f]
    # tentáculos de sombra al fondo
    for i in range(12):
        a = math.pi * (0.1 + 0.8 * i / 11)
        x1 = 100 + math.cos(a) * 96 + math.sin(i * 2 + f) * 4
        y1 = 120 - math.sin(a) * 100
        s.capsule(100, 100, x1, y1, 9, 2, "dark", cz=-12)
    for i in range(8):
        x0 = 30 + i * 20
        s.capsule(100, 130, x0 + math.sin(i + f) * 5, 172, 8, 3, "dark", cz=-10)
    # cuerpo
    s.ellipsoid(100, 104 + br, 58, 50, "dark", cz=0, flat=0.9)
    s.ellipsoid(100, 128 + br, 44, 20, "chitin", cz=10)
    # corona de cristal negro
    for k in range(7):
        x = 58 + k * 14
        hgt = 26 + (10 if k == 3 else (6 if k in (2, 4) else 0))
        s.polygon([(x - 6, 62 + br), (x, 62 - hgt + br), (x + 6, 62 + br)], "chitin", cz=18, normal=(-0.4, -0.6, 0.7))
    # brazos con garras
    for side in (-1, 1):
        s.capsule(100 + side * 46, 100 + br, 100 + side * 84, 132 + br, 12, 8, "dark", cz=14)
        for c in range(3):
            s.capsule(100 + side * 84, 132 + br, 100 + side * (90 + c * 5), 150 + c * 3 + br, 3, 1, "bone", cz=16)
    c = s.render()
    # fauces
    c.ellipse(100, 128 + br, 26, 9 + br, (12, 2, 12))
    for k in range(9):
        x = 78 + k * 5.5
        c.px(int(x), 121 + br, (240, 230, 210))
        c.px(int(x), 122 + br, (240, 230, 210))
        c.px(int(x) + 2, 135 + br, (240, 230, 210))
    # ojos
    for (x, y, r) in ((100, 92, 9), (74, 84, 5), (126, 84, 5), (62, 104, 3), (138, 104, 3), (88, 70, 3), (112, 70, 3)):
        c.ellipse(x, y + br, r, r * 0.75, (255, 60, 90))
        c.ellipse(x, y + br, max(1, r * 0.35), r * 0.7, (30, 0, 10))
        c.px(int(x - r * 0.4), int(y - r * 0.3 + br), (255, 220, 230))
        halo(c, x, y + br, (255, 60, 110), int(r * 2.2), 70)
    halo(c, 100, 104 + br, (180, 40, 200), 50, 40)
    return c


ENEMIES = {"larva": larva, "bat": bat, "brute": brute, "root": root, "spectre": spectre, "custodian": custodian,
           "mother_root": mother_root, "wisp": wisp, "boar": boar, "nhalzur": nhalzur}


def build_all(out):
    for name, fn in ENEMIES.items():
        sheet([fn(0), fn(1)]).save(f"{out}/enemies/{name}.png")
