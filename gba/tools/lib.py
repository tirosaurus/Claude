"""Utilidades comunes del conversor de recursos (Godot -> GBA)."""
import hashlib
import json
import os
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
GAME = os.path.join(ROOT, "game", "assets")
GBA = os.path.join(ROOT, "gba")
SRC = os.path.join(GBA, "assets_src")
OUT = os.path.join(GBA, "build", "gen")
BIN = os.path.join(OUT, "bin")
PREVIEW = os.path.join(GBA, "build", "preview")
CACHE = os.path.join(GBA, "build", "cache")
for d in (OUT, BIN, PREVIEW, CACHE):
    os.makedirs(d, exist_ok=True)


def c_ident(s):
    return "".join(ch if ch.isalnum() else "_" for ch in s)


def to555(rgb):
    r, g, b = [int(v) >> 3 for v in rgb[:3]]
    return r | (g << 5) | (b << 10)


def c_string(s):
    if s is None:
        return "0"
    out = s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return '"' + out + '"'


def c_array(name, ctype, values, per_line=16, static=True):
    vals = list(values)
    lines = []
    for i in range(0, len(vals), per_line):
        lines.append("\t" + ", ".join(str(v) for v in vals[i:i + per_line]) + ",")
    pre = "static " if static else ""
    if not vals:
        return "%sconst %s %s[1] = {0};\n" % (pre, ctype, name)
    return "%sconst %s %s[%d] = {\n%s\n};\n" % (pre, ctype, name, len(vals), "\n".join(lines))


# ------------------------------------------------------------------ blobs binarios (.incbin)
def write_blob(name, data):
    """Escribe build/gen/bin/<name>.bin; el símbolo C se declara con blob_decl()."""
    if isinstance(data, np.ndarray):
        data = data.tobytes()
    data = bytes(data)
    while len(data) % 4:
        data += b"\0"
    path = os.path.join(BIN, name + ".bin")
    if not os.path.exists(path) or open(path, "rb").read() != data:
        open(path, "wb").write(data)
    return name


def blobs_asm():
    """Genera blobs.s con todos los .bin presentes."""
    names = sorted(f[:-4] for f in os.listdir(BIN) if f.endswith(".bin"))
    lines = ['\t.section .rodata', '\t.align 2']
    for n in names:
        lines.append('\t.global %s\n\t.align 2\n%s:\n\t.incbin "bin/%s.bin"' % (n, n, n))
    txt = "\n".join(lines) + "\n"
    p = os.path.join(OUT, "blobs.s")
    if not os.path.exists(p) or open(p).read() != txt:
        open(p, "w").write(txt)


def file_hash(paths, extra=""):
    h = hashlib.sha1(extra.encode())
    for p in paths:
        if os.path.exists(p):
            h.update(open(p, "rb").read())
    return h.hexdigest()


def write_if_changed(path, text):
    if not os.path.exists(path) or open(path).read() != text:
        open(path, "w").write(text)


# ------------------------------------------------------------------ color
def kmeans(data, k, weights=None, iters=12, seed=1):
    data = np.asarray(data, dtype=np.float32)
    n = len(data)
    if n <= k:
        return data.copy(), np.arange(n)
    w = np.ones(n, dtype=np.float32) if weights is None else np.asarray(weights, dtype=np.float32)
    rng = np.random.default_rng(seed)
    cent = [data[rng.choice(n, p=w / w.sum())]]
    dmin = ((data - cent[0]) ** 2).sum(-1)
    for _ in range(1, k):
        d = dmin * w
        if d.sum() <= 0:
            break
        c = data[rng.choice(n, p=d / d.sum())]
        cent.append(c)
        dmin = np.minimum(dmin, ((data - c) ** 2).sum(-1))
    cent = np.array(cent)
    for _ in range(iters):
        lab = np.argmin(((data[:, None, :] - cent[None]) ** 2).sum(-1), axis=1)
        for j in range(len(cent)):
            m = lab == j
            if m.any():
                cent[j] = (data[m] * w[m, None]).sum(0) / w[m].sum()
    lab = np.argmin(((data[:, None, :] - cent[None]) ** 2).sum(-1), axis=1)
    return cent, lab


def quant5(px):
    """Reduce a la precisión de la GBA (5 bits por canal), devolviendo valores 0..248."""
    return (np.asarray(px, dtype=np.int32) >> 3) << 3


def palette_from_pixels(px, ncol=15, weights=None):
    """px (N,3) en 0..248 -> paleta (ncol,3)."""
    px = np.asarray(px, dtype=np.int32)
    if len(px) == 0:
        return np.zeros((ncol, 3))
    uniq, counts = np.unique(px, axis=0, return_counts=True)
    if len(uniq) <= ncol:
        pal = np.zeros((ncol, 3))
        pal[:len(uniq)] = uniq
        return pal
    pal, _ = kmeans(uniq, ncol, weights=counts, iters=12)
    return np.clip(np.round(pal / 8) * 8, 0, 248)


def pal555(pal, n=15):
    return [0] + [to555(c) for c in pal[:n]] + [0] * max(0, 15 - len(pal[:n]))


# ------------------------------------------------------------------ tiles
def img_cells(img):
    """RGBA -> (cells_rgb (N,64,3) int, cells_alpha (N,64), cw, ch)."""
    a = np.array(img.convert("RGBA"), dtype=np.int32)
    h, w = a.shape[:2]
    cw, ch = (w + 7) // 8, (h + 7) // 8
    pad = np.zeros((ch * 8, cw * 8, 4), dtype=np.int32)
    pad[:h, :w] = a
    t = pad.reshape(ch, 8, cw, 8, 4).transpose(0, 2, 1, 3, 4).reshape(ch * cw, 64, 4)
    return quant5(t[..., :3]), t[..., 3], cw, ch


def build_palettes(cells_rgb, cells_alpha, n_pal, ncol=15, transparent=True, seed=1):
    """Agrupa celdas en n_pal grupos por color y saca una paleta de ncol colores para cada uno."""
    feats = []
    keep = []
    for i in range(len(cells_rgb)):
        m = cells_alpha[i] >= 128 if transparent else np.ones(64, bool)
        op = cells_rgb[i][m]
        if len(op) == 0:
            continue
        keep.append(i)
        feats.append(np.concatenate([op.mean(0), op.std(0) * 0.6]))
    if not keep:
        return np.zeros((n_pal, ncol, 3))
    feats = np.array(feats)
    used = min(n_pal, len(keep))
    _, groups = kmeans(feats, used, iters=12, seed=seed)
    pals = []
    for g in range(used):
        idx = [keep[j] for j in np.where(groups == g)[0]]
        if not idx:
            pals.append(np.zeros((ncol, 3)))
            continue
        allpx = np.concatenate([cells_rgb[i][(cells_alpha[i] >= 128) if transparent else slice(None)] for i in idx])
        pals.append(palette_from_pixels(allpx, ncol))
    # refinar: reasignar celdas a la mejor paleta y recalcular (2 pasadas)
    pals = np.array(pals)
    for _ in range(2):
        assign = assign_palettes(cells_rgb, cells_alpha, pals, transparent)
        newp = []
        for g in range(len(pals)):
            idx = [i for i in keep if assign[i] == g]
            if not idx:
                newp.append(pals[g])
                continue
            allpx = np.concatenate([cells_rgb[i][(cells_alpha[i] >= 128) if transparent else slice(None)] for i in idx])
            newp.append(palette_from_pixels(allpx, ncol))
        pals = np.array(newp)
    out = np.zeros((n_pal, ncol, 3))
    out[:len(pals)] = pals
    return out


def assign_palettes(cells_rgb, cells_alpha, pals, transparent=True):
    """Para cada celda, el índice de la paleta con menor error."""
    pals = np.asarray(pals, dtype=np.float32)
    best = np.zeros(len(cells_rgb), dtype=np.int32)
    chunk = 512
    for s in range(0, len(cells_rgb), chunk):
        px = cells_rgb[s:s + chunk].astype(np.float32)          # (C,64,3)
        al = cells_alpha[s:s + chunk]
        m = (al >= 128) if transparent else np.ones_like(al, bool)
        errs = []
        for p in pals:
            d = ((px[:, :, None, :] - p[None, None]) ** 2).sum(-1).min(-1)   # (C,64)
            errs.append((d * m).sum(-1))
        best[s:s + chunk] = np.argmin(np.array(errs), axis=0)
    return best


def encode_cells(cells_rgb, cells_alpha, pals, assign, transparent=True):
    """Devuelve lista de bytes(32) 4bpp por celda (índices 1..15; 0 transparente)."""
    out = []
    pals = np.asarray(pals, dtype=np.float32)
    for i in range(len(cells_rgb)):
        p = pals[assign[i]]
        d = ((cells_rgb[i][:, None, :].astype(np.float32) - p[None]) ** 2).sum(-1)
        ids = d.argmin(1) + 1
        if transparent:
            ids = np.where(cells_alpha[i] >= 128, ids, 0)
        lo = ids[0::2] & 15
        hi = ids[1::2] & 15
        out.append(bytes((lo | (hi << 4)).astype(np.uint8)))
    return out


class TileBank:
    """Banco de tiles 4bpp con deduplicación."""

    def __init__(self, reserve_blank=False):
        self.index = {}
        self.tiles = []
        if reserve_blank:
            self.add(bytes(32))

    def add(self, t):
        k = bytes(t)
        if k not in self.index:
            self.index[k] = len(self.tiles)
            self.tiles.append(k)
        return self.index[k]

    def blob(self):
        return b"".join(self.tiles)


def decode_tile(t, pal16):
    """bytes(32) + paleta (16 BGR555) -> array (8,8,4) RGBA (para vistas previas)."""
    out = np.zeros((8, 8, 4), np.uint8)
    for k in range(64):
        v = (t[k >> 1] >> (4 if k & 1 else 0)) & 15
        if v:
            c = pal16[v]
            out[k >> 3, k & 7] = ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3, 255)
    return out


def sheet_quant(frames, ncol=15, pal=None):
    """Lista de imágenes RGBA (mismo tamaño, múltiplo de 8) -> (bytes 4bpp en orden 1D por frame, pal16).
    Una paleta para toda la hoja."""
    allpx = []
    for f in frames:
        a = np.array(f.convert("RGBA"), dtype=np.int32).reshape(-1, 4)
        allpx.append(quant5(a[a[:, 3] >= 128][:, :3]))
    allpx = np.concatenate(allpx) if allpx else np.zeros((0, 3))
    if pal is None:
        pal = palette_from_pixels(allpx, ncol)
    data = bytearray()
    for f in frames:
        cr, ca, cw, ch = img_cells(f)
        enc = encode_cells(cr, ca, [pal], np.zeros(len(cr), np.int32))
        data += b"".join(enc)
    return bytes(data), pal555(pal)


def load_rgba(path):
    return Image.open(path).convert("RGBA")
