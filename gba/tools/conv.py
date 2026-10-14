"""Conversor de recursos de Vaelmoor (Godot) a datos de Game Boy Advance.

Genera gba/build/gen/*.c y *.h a partir de:
  - game/assets/maps/<id>.png + .json  (suelo + objetos horneados, colisiones, salidas, zonas)
  - hojas de personajes (20x30 por frame, 8 columnas x 4 filas)
  - sprites de combate (32x32), enemigos, fondos de combate, retratos
  - la fuente PixelifySans (ancho variable)

Formato de mapa en la ROM:
  - banco de tiles 4bpp únicos (32 bytes cada uno)
  - por celda de 8x8: u16 = índice de tile (12 bits) | paleta (4 bits)
  - 16 paletas de 16 colores (BGR555)
  - rejilla de colisión 1 bit por celda
"""
import json
import os
import sys
import numpy as np
from PIL import Image, ImageFont, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
GAME = os.path.join(ROOT, "game", "assets")
SRC = os.path.join(ROOT, "gba", "assets_src")
OUT = os.path.join(ROOT, "gba", "build", "gen")


# --------------------------------------------------------------------------- utilidades
def c_ident(s):
    return "".join(ch if ch.isalnum() else "_" for ch in s)


def to555(rgb):
    r, g, b = [int(v) >> 3 for v in rgb[:3]]
    return r | (g << 5) | (b << 10)


def c_array(name, ctype, values, per_line=16, static=False):
    vals = list(values)
    lines = []
    for i in range(0, len(vals), per_line):
        lines.append("\t" + ", ".join(str(v) for v in vals[i:i + per_line]) + ",")
    pre = "static " if static else ""
    return "%sconst %s %s[%d] = {\n%s\n};\n" % (pre, ctype, name, len(vals), "\n".join(lines))


def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def kmeans(data, k, weights=None, iters=12, seed=1):
    """k-means sencillo sobre colores (N,3)."""
    data = np.asarray(data, dtype=np.float32)
    n = len(data)
    if n <= k:
        return data.copy(), np.arange(n)
    w = np.ones(n, dtype=np.float32) if weights is None else np.asarray(weights, dtype=np.float32)
    rng = np.random.default_rng(seed)
    # k-means++ sencillo
    cent = [data[rng.choice(n, p=w / w.sum())]]
    for _ in range(1, k):
        d = np.min(((data[:, None, :] - np.array(cent)[None]) ** 2).sum(-1), axis=1) * w
        if d.sum() <= 0:
            break
        cent.append(data[rng.choice(n, p=d / d.sum())])
    cent = np.array(cent)
    for _ in range(iters):
        lab = np.argmin(((data[:, None, :] - cent[None]) ** 2).sum(-1), axis=1)
        for j in range(len(cent)):
            m = lab == j
            if m.any():
                cent[j] = (data[m] * w[m, None]).sum(0) / w[m].sum()
    lab = np.argmin(((data[:, None, :] - cent[None]) ** 2).sum(-1), axis=1)
    return cent, lab


# --------------------------------------------------------------------------- cuantización por tiles
def quantize_tiles(img, n_pal=16, colors_per_pal=15, transparent_key=False):
    """Divide la imagen (RGBA) en tiles 8x8 y les asigna una de n_pal paletas de 15 colores.

    Devuelve (tiles_bytes_unicos, celdas[(tile, pal)], paletas[n_pal][16] BGR555, ancho_celdas, alto_celdas).
    Si transparent_key, los píxeles con alfa < 128 usan el índice 0 (transparente)."""
    a = np.array(img.convert("RGBA"), dtype=np.int32)
    h, w = a.shape[:2]
    cw, ch = (w + 7) // 8, (h + 7) // 8
    pad = np.zeros((ch * 8, cw * 8, 4), dtype=np.int32)
    pad[:h, :w] = a
    rgb = (pad[..., :3] >> 3) << 3          # precisión de la GBA
    alpha = pad[..., 3]
    cells = []
    for cy in range(ch):
        for cx in range(cw):
            px = rgb[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8].reshape(64, 3)
            al = alpha[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8].reshape(64)
            cells.append((px, al))
    # 1) rasgos por celda: histograma grueso de color -> agrupar en n_pal grupos
    feats = []
    for px, al in cells:
        op = px[al >= 128] if transparent_key else px
        if len(op) == 0:
            feats.append(np.zeros(6))
            continue
        feats.append(np.concatenate([op.mean(0), op.std(0) * 0.5]))
    feats = np.array(feats)
    used_pal = min(n_pal, max(1, len(cells)))
    _, groups = kmeans(feats, used_pal, iters=10)
    # 2) paleta de cada grupo: k-means de sus píxeles a 15 colores
    palettes = []
    for gi in range(used_pal):
        idx = np.where(groups == gi)[0]
        if len(idx) == 0:
            palettes.append(np.zeros((colors_per_pal, 3)))
            continue
        allpx = np.concatenate([cells[i][0][cells[i][1] >= 128] if transparent_key else cells[i][0] for i in idx])
        if len(allpx) == 0:
            palettes.append(np.zeros((colors_per_pal, 3)))
            continue
        uniq, counts = np.unique(allpx, axis=0, return_counts=True)
        if len(uniq) <= colors_per_pal:
            pal = np.zeros((colors_per_pal, 3))
            pal[:len(uniq)] = uniq
        else:
            pal, _ = kmeans(uniq, colors_per_pal, weights=counts, iters=10)
        palettes.append(np.clip(np.round(pal / 8) * 8, 0, 248))
    while len(palettes) < n_pal:
        palettes.append(np.zeros((colors_per_pal, 3)))
    pals = np.array(palettes)  # (n_pal, 15, 3)
    # 3) cada celda: elegir la paleta con menor error y mapear píxeles
    tiles = {}
    tile_list = []
    out_cells = []
    for px, al in cells:
        best = None
        for pi in range(used_pal):
            d = ((px[:, None, :] - pals[pi][None]) ** 2).sum(-1)
            if transparent_key:
                err = d.min(1)[al >= 128].sum()
            else:
                err = d.min(1).sum()
            if best is None or err < best[0]:
                best = (err, pi, d.argmin(1))
        _, pi, ids = best
        ids = ids + 1
        if transparent_key:
            ids = np.where(al >= 128, ids, 0)
        data = bytearray(32)
        for i in range(64):
            v = int(ids[i]) & 15
            if i & 1:
                data[i >> 1] |= v << 4
            else:
                data[i >> 1] |= v
        key = bytes(data)
        if key not in tiles:
            tiles[key] = len(tile_list)
            tile_list.append(key)
        out_cells.append((tiles[key], pi))
    pal555 = []
    for pi in range(n_pal):
        row = [0] + [to555(c) for c in pals[pi]]
        pal555.append(row[:16])
    return tile_list, out_cells, pal555, cw, ch


# --------------------------------------------------------------------------- mapas
def sprite_img(name):
    return Image.open(os.path.join(GAME, "sprites", name + ".png")).convert("RGBA")


def compose_map(data):
    im = Image.open(os.path.join(GAME, "maps", data["id"] + ".png")).convert("RGBA")
    for p in sorted(data["props"], key=lambda p: p["y"]):
        try:
            spr = sprite_img(p["sprite"])
        except FileNotFoundError:
            continue
        fr = p.get("frames", 1)
        fw = spr.width // fr
        spr = spr.crop((0, 0, fw, spr.height))
        if p.get("flip"):
            spr = spr.transpose(Image.FLIP_LEFT_RIGHT)
        im.alpha_composite(spr, (int(p["x"] - fw // 2), int(p["y"] - spr.height - p.get("lift", 0))))
    return im


def collision_grid(data, cw, ch):
    """1 = bloqueado. Una celda de 8x8 es sólida si su centro cae dentro de un sólido."""
    rects = list(data["solids"])
    for p in data["props"]:
        if "col" in p:
            cx, cy, w, h = p["col"]
            rects.append([p["x"] + cx, p["y"] + cy, w, h])
    grid = np.zeros((ch, cw), dtype=np.uint8)
    for (x, y, w, h) in rects:
        x0, y0 = int(np.floor(x / 8)), int(np.floor(y / 8))
        x1, y1 = int(np.ceil((x + w) / 8)), int(np.ceil((y + h) / 8))
        for gy in range(max(0, y0), min(ch, y1)):
            for gx in range(max(0, x0), min(cw, x1)):
                # centro de la celda
                mx, my = gx * 8 + 4, gy * 8 + 4
                if x <= mx <= x + w and y <= my <= y + h:
                    grid[gy, gx] = 1
    # bordes del mapa
    grid[0, :] |= 0
    bits = bytearray((cw * ch + 7) // 8)
    for i, v in enumerate(grid.reshape(-1)):
        if v:
            bits[i >> 3] |= 1 << (i & 7)
    return bits


def export_maps(map_ids, out_c, out_h):
    c, h = [], []
    h.append("typedef struct { short x, y, w, h; unsigned char to, spawn; short req; } MapExit;")
    h.append("typedef struct { short x, y, w, h; const char* id; } MapZone;")
    h.append("typedef struct { short x, y; const char* id; } MapMarker;")
    h.append("typedef struct { short x, y; const char* id; } MapObj;")
    h.append("typedef struct {\n\tconst char* id; const char* name; unsigned short wc, hc, ntiles;\n"
             "\tconst unsigned int* tiles; const unsigned short* cells; const unsigned char* cpal; const unsigned short* pals;\n"
             "\tconst unsigned char* coll;\n\tconst MapExit* exits; unsigned char nexits;\n"
             "\tconst MapZone* zones; unsigned char nzones;\n\tconst MapMarker* markers; unsigned char nmarkers;\n"
             "\tconst MapObj* objs; unsigned char nobjs;\n\tunsigned char music; unsigned char indoor;\n} MapData;")
    for i, mid in enumerate(map_ids):
        h.append("#define MAP_%s %d" % (c_ident(mid).upper(), i))
    h.append("#define MAP_COUNT %d" % len(map_ids))
    h.append("extern const MapData g_maps[MAP_COUNT];")
    names = []
    for mid in map_ids:
        data = json.load(open(os.path.join(GAME, "maps", mid + ".json")))
        img = compose_map(data)
        tiles, cells, pals, cw, ch = quantize_tiles(img, n_pal=MAP_PALS)
        ident = c_ident(mid)
        words = []
        for t in tiles:
            for k in range(0, 32, 4):
                words.append("0x%08X" % int.from_bytes(t[k:k + 4], "little"))
        c.append(c_array("map_%s_tiles" % ident, "unsigned int", words, 8, static=True))
        c.append(c_array("map_%s_cells" % ident, "unsigned short", [t for t, p in cells], 16, static=True))
        c.append(c_array("map_%s_cpal" % ident, "unsigned char", [p for t, p in cells], 32, static=True))
        preview_map(mid, tiles, cells, pals, cw, ch)
        c.append(c_array("map_%s_pals" % ident, "unsigned short", [v for row in pals[:MAP_PALS] for v in row], 16, static=True))
        c.append(c_array("map_%s_coll" % ident, "unsigned char", collision_grid(data, cw, ch), 32, static=True))
        exits = []
        for e in data["exits"]:
            x, y, w_, h_ = e["rect"]
            to = map_ids.index(e["to"]) if e["to"] in map_ids else 255
            exits.append("{%d,%d,%d,%d,%d,0,-1}" % (x, y, w_, h_, to))
            names.append((mid, e))
        c.append("static const MapExit map_%s_exits[] = {%s};\n" % (ident, ", ".join(exits) or "{0}"))
        zones = ["{%d,%d,%d,%d,%s}" % (t["rect"][0], t["rect"][1], t["rect"][2], t["rect"][3], c_string(t["id"]))
                 for t in data["triggers"]]
        c.append("static const MapZone map_%s_zones[] = {%s};\n" % (ident, ", ".join(zones) or "{0}"))
        marks = ["{%d,%d,%s}" % (v[0], v[1], c_string(k)) for k, v in data["markers"].items()]
        c.append("static const MapMarker map_%s_markers[] = {%s};\n" % (ident, ", ".join(marks) or "{0}"))
        objs = ["{%d,%d,%s}" % (p["x"], p["y"], c_string(p["id"])) for p in data["props"] if "id" in p]
        c.append("static const MapObj map_%s_objs[] = {%s};\n" % (ident, ", ".join(objs) or "{0}"))
        print("  mapa %-14s %3dx%-3d celdas, %4d tiles únicos" % (mid, cw, ch, len(tiles)))
        data["_counts"] = (len(tiles), cw, ch, len(exits), len(zones), len(marks), len(objs))
        data["_ident"] = ident
        json_cache[mid] = data
    entries = []
    for mid in map_ids:
        d = json_cache[mid]
        nt, cw, ch, ne, nz, nm, no = d["_counts"]
        ident = d["_ident"]
        entries.append("\t{%s, %s, %d, %d, %d, map_%s_tiles, map_%s_cells, map_%s_cpal, map_%s_pals, map_%s_coll, "
                       "map_%s_exits, %d, map_%s_zones, %d, map_%s_markers, %d, map_%s_objs, %d, 0, %d}" % (
                           c_string(mid), c_string(d["display"]), cw, ch, nt, ident, ident, ident, ident, ident,
                           ident, ne, ident, nz, ident, nm, ident, no, 1 if d["interior"] else 0))
    c.append("const MapData g_maps[MAP_COUNT] = {\n%s\n};\n" % ",\n".join(entries))
    # spawn: índice del marcador de destino dentro del mapa destino (se resuelve en C por nombre)
    spawn_names = []
    for mid in map_ids:
        d = json_cache[mid]
        spawn_names.append("static const char* const map_%s_spawns[] = {%s};" % (
            d["_ident"], ", ".join(c_string(e["spawn"]) for e in d["exits"]) or "0"))
    c.extend(spawn_names)
    c.append("const char* const* const g_map_exit_spawns[MAP_COUNT] = {%s};" % ", ".join(
        "map_%s_spawns" % json_cache[m]["_ident"] for m in map_ids))
    h.append("extern const char* const* const g_map_exit_spawns[MAP_COUNT];")
    open(out_c, "w").write('#include "maps.h"\n\n' + "\n".join(c))
    open(out_h, "w").write("#pragma once\n" + "\n".join(h) + "\n")


json_cache = {}
MAP_PALS = 14   # paletas 14 y 15 quedan para retratos e interfaz


def preview_map(mid, tiles, cells, pals, cw, ch):
    """Reconstruye el mapa a partir de los datos de GBA (para revisar la calidad)."""
    os.makedirs(os.path.join(OUT, "..", "preview"), exist_ok=True)
    im = np.zeros((ch * 8, cw * 8, 3), dtype=np.uint8)
    for i, (t, p) in enumerate(cells):
        cy, cx = divmod(i, cw)
        data = tiles[t]
        for k in range(64):
            v = (data[k >> 1] >> (4 if k & 1 else 0)) & 15
            c = pals[p][v]
            im[cy * 8 + (k >> 3), cx * 8 + (k & 7)] = ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)
    Image.fromarray(im).save(os.path.join(OUT, "..", "preview", mid + ".png"))


# --------------------------------------------------------------------------- sprites
def sheet_tiles(img, fw, fh, box=32, ox=None, oy=None, max_frames=None, pal=None, parts=1):
    """Convierte una hoja (frames fw x fh en filas) en frames de box x box, 4bpp, mapeo 1D.
    Devuelve (palabras u32, paleta[16])."""
    img = img.convert("RGBA")
    cols, rows = img.width // fw, img.height // fh
    frames = []
    for r in range(rows):
        for c in range(cols):
            fr = Image.new("RGBA", (box * parts, box), (0, 0, 0, 0))
            px = (box - fw) // 2 if ox is None else ox
            py = (box - fh) if oy is None else oy
            fr.alpha_composite(img.crop((c * fw, r * fh, c * fw + fw, r * fh + fh)), (px, py))
            frames.append(fr)
    if max_frames:
        frames = frames[:max_frames]
    # paleta común a toda la hoja
    allpx = np.concatenate([np.array(f)[..., :4].reshape(-1, 4) for f in frames])
    op = allpx[allpx[:, 3] >= 128][:, :3]
    op = (op >> 3) << 3
    if pal is None:
        uniq, counts = np.unique(op, axis=0, return_counts=True)
        if len(uniq) <= 15:
            pal = uniq.astype(np.float32)
        else:
            pal, _ = kmeans(uniq, 15, weights=counts)
        pal = np.clip(np.round(pal / 8) * 8, 0, 248)
    words = []
    subframes = []
    for f in frames:
        for pi in range(parts):
            subframes.append(f.crop((pi * box, 0, pi * box + box, box)))
    for f in subframes:
        a = np.array(f).astype(np.int32)
        for ty in range(box // 8):
            for tx in range(box // 8):
                blk = a[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].reshape(64, 4)
                d = (((blk[:, None, :3] >> 3 << 3) - pal[None]) ** 2).sum(-1)
                ids = np.where(blk[:, 3] >= 128, d.argmin(1) + 1, 0)
                data = bytearray(32)
                for i in range(64):
                    v = int(ids[i]) & 15
                    data[i >> 1] |= (v << 4) if (i & 1) else v
                for k in range(0, 32, 4):
                    words.append("0x%08X" % int.from_bytes(data[k:k + 4], "little"))
    pal16 = [0] + [to555(c) for c in pal]
    pal16 += [0] * (16 - len(pal16))
    return words, pal16[:16], len(frames)


def export_sprites(specs, out_c, out_h):
    c, h = ['#include "sprites.h"\n'], ["#pragma once"]
    h.append("typedef struct { const unsigned int* tiles; const unsigned short* pal; unsigned short frames; unsigned char size; } SpriteSheet;")
    for name, path, fw, fh, box, kw in specs:
        img = Image.open(path)
        words, pal, n = sheet_tiles(img, fw, fh, box, **kw)
        ident = c_ident(name)
        c.append(c_array("spr_%s_tiles" % ident, "unsigned int", words, 8, static=True))
        c.append(c_array("spr_%s_pal" % ident, "unsigned short", pal, 16, static=True))
        c.append("const SpriteSheet g_spr_%s = {spr_%s_tiles, spr_%s_pal, %d, %d};\n" % (ident, ident, ident, n, box))
        h.append("extern const SpriteSheet g_spr_%s;" % ident)
        print("  sprite %-16s %2d frames %dx%d" % (name, n, box, box))
    open(out_c, "w").write("\n".join(c))
    open(out_h, "w").write("\n".join(h) + "\n")


# --------------------------------------------------------------------------- imágenes de fondo (combate, retratos, título)
def export_images(specs, out_c, out_h):
    c, h = ['#include "images.h"\n'], ["#pragma once"]
    h.append("typedef struct { unsigned short wc, hc, ntiles; const unsigned int* tiles; const unsigned short* cells; const unsigned short* pals; unsigned char npal; } BgImage;")
    for name, img, npal in specs:
        tiles, cells, pals, cw, ch = quantize_tiles(img, n_pal=npal)
        ident = c_ident(name)
        words = []
        for t in tiles:
            for k in range(0, 32, 4):
                words.append("0x%08X" % int.from_bytes(t[k:k + 4], "little"))
        c.append(c_array("img_%s_tiles" % ident, "unsigned int", words, 8, static=True))
        c.append(c_array("img_%s_cells" % ident, "unsigned short", [(t & 0x0FFF) | (p << 12) for t, p in cells], 16, static=True))
        c.append(c_array("img_%s_pals" % ident, "unsigned short", [v for row in pals[:npal] for v in row], 16, static=True))
        c.append("const BgImage g_img_%s = {%d, %d, %d, img_%s_tiles, img_%s_cells, img_%s_pals, %d};\n" % (
            ident, cw, ch, len(tiles), ident, ident, ident, npal))
        h.append("extern const BgImage g_img_%s;" % ident)
        print("  imagen %-14s %dx%d celdas, %d tiles" % (name, cw, ch, len(tiles)))
    open(out_c, "w").write("\n".join(c))
    open(out_h, "w").write("\n".join(h) + "\n")


# --------------------------------------------------------------------------- fuente
CHARS = (" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
         "áéíóúÁÉÍÓÚñÑüÜ¿¡«»—…·ªº")


def export_font(out_c, out_h, size=10):
    f = ImageFont.truetype(os.path.join(GAME, "fonts", "PixelifySans.ttf"), size)
    H = 12
    glyphs = []
    for ch in CHARS:
        im = Image.new("L", (16, H), 0)
        d = ImageDraw.Draw(im)
        d.text((0, -1), ch, font=f, fill=255)
        a = np.array(im) >= 128
        if ch == " ":
            w = 3
        else:
            cols = np.where(a.any(0))[0]
            w = (cols.max() + 1) if len(cols) else 3
            w = int(min(w, 12))
        rows = []
        for y in range(H):
            bits = 0
            for x in range(min(16, 12)):
                if a[y, x]:
                    bits |= 1 << x
            rows.append(bits)
        glyphs.append((ord(ch), w + 1, rows))
    c = ['#include "font.h"\n']
    c.append(c_array("font_codes", "unsigned short", [g[0] for g in glyphs], 16, static=True))
    c.append(c_array("font_widths", "unsigned char", [g[1] for g in glyphs], 32, static=True))
    c.append(c_array("font_rows", "unsigned short", [r for g in glyphs for r in g[2]], 16, static=True))
    c.append("const Font g_font = {font_codes, font_widths, font_rows, %d, %d};\n" % (len(glyphs), H))
    open(out_c, "w").write("\n".join(c))
    open(out_h, "w").write("#pragma once\ntypedef struct { const unsigned short* codes; const unsigned char* widths; "
                           "const unsigned short* rows; unsigned short count; unsigned char height; } Font;\n"
                           "extern const Font g_font;\n")
    print("  fuente: %d glifos" % len(glyphs))


# --------------------------------------------------------------------------- principal
DEMO_MAPS = ["bedroom", "housemain", "village", "forest"]


def main():
    os.makedirs(OUT, exist_ok=True)
    maps = sys.argv[1:] or DEMO_MAPS
    print("Mapas:")
    export_maps(maps, os.path.join(OUT, "maps.c"), os.path.join(OUT, "maps.h"))
    print("Sprites:")
    ch = os.path.join(GAME, "chars")
    bt = os.path.join(GAME, "battle")
    specs = [
        ("player_m", os.path.join(SRC, "player_m.png"), 20, 30, 32, {}),
        ("player_f", os.path.join(SRC, "player_f.png"), 20, 30, 32, {}),
        ("kaelen", os.path.join(ch, "kaelen.png"), 20, 30, 32, {}),
        ("yara", os.path.join(ch, "yara.png"), 20, 30, 32, {}),
        ("mother", os.path.join(ch, "mother.png"), 20, 30, 32, {}),
        ("villager_m", os.path.join(ch, "villager_m.png"), 20, 30, 32, {}),
        ("villager_f", os.path.join(ch, "villager_f.png"), 20, 30, 32, {}),
        ("villager_old", os.path.join(ch, "villager_old.png"), 20, 30, 32, {}),
        ("roc", os.path.join(ch, "roc.png"), 20, 30, 32, {}),
        ("nil", os.path.join(ch, "nil.png"), 20, 30, 32, {}),
        ("bartolo", os.path.join(ch, "bartolo.png"), 20, 30, 32, {}),
        ("remei", os.path.join(ch, "remei.png"), 20, 30, 32, {}),
        ("b_player_m", os.path.join(SRC, "player_m_battle.png"), 32, 32, 32, {"ox": 0, "oy": 0}),
        ("b_kaelen", os.path.join(bt, "kaelen.png"), 32, 32, 32, {"ox": 0, "oy": 0}),
        ("b_yara", os.path.join(bt, "yara.png"), 32, 32, 32, {"ox": 0, "oy": 0}),
        ("e_wolf", os.path.join(GAME, "sprites", "wolf_big.png"), 96, 64, 64, {"ox": 16, "oy": 0, "parts": 2}),
    ]
    export_sprites(specs, os.path.join(OUT, "sprites.c"), os.path.join(OUT, "sprites.h"))
    print("Imágenes:")
    bf = Image.open(os.path.join(GAME, "bg", "battle_forest.png")).convert("RGBA").resize((240, 135), Image.LANCZOS)
    title = Image.open(os.path.join(GAME, "ui", "title_bg.png")).convert("RGBA") if os.path.exists(os.path.join(GAME, "ui", "title_bg.png")) else None
    imgs = [("battle_forest", bf, 8)]
    if title is not None:
        imgs.append(("title", title.resize((240, 160), Image.LANCZOS), 8))
    for who in ["kaelen", "yara", "mother", "bartolo", "roc", "nil", "remei"]:
        p = os.path.join(GAME, "portraits", who + ".png")
        if os.path.exists(p):
            imgs.append(("por_" + who, Image.open(p).convert("RGBA"), 1))
    imgs.append(("por_player_m", Image.open(os.path.join(SRC, "player_m_portrait.png")).convert("RGBA"), 1))
    imgs.append(("por_player_f", Image.open(os.path.join(SRC, "player_f_portrait.png")).convert("RGBA"), 1))
    export_images(imgs, os.path.join(OUT, "images.c"), os.path.join(OUT, "images.h"))
    print("Fuente:")
    export_font(os.path.join(OUT, "font.c"), os.path.join(OUT, "font.h"))


if __name__ == "__main__":
    main()
