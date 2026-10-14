"""Sprites, retratos, enemigos, fondos, interfaz, capas del creador de personajes y fuente."""
import json
import os
import sys
import numpy as np
from PIL import Image, ImageFont, ImageDraw
from lib import *

C = ['#include "../../src/gfxdata.h"', ""]
H = ["#pragma once", '#include "../../src/gfxdata.h"', ""]
OBJ_SHAPES = [(64, 64), (64, 32), (32, 64), (32, 32), (32, 16), (16, 32), (16, 16), (32, 8), (8, 32), (16, 8), (8, 16), (8, 8)]


def obj_shape_size(w, h):
    table = {(8, 8): (0, 0), (16, 16): (0, 1), (32, 32): (0, 2), (64, 64): (0, 3),
             (16, 8): (1, 0), (32, 8): (1, 1), (32, 16): (1, 2), (64, 32): (1, 3),
             (8, 16): (2, 0), (8, 32): (2, 1), (16, 32): (2, 2), (32, 64): (2, 3)}
    return table[(w, h)]


def words(b):
    return np.frombuffer(b, dtype=np.uint32)


def blob_u32(name, b):
    write_blob(name, b)
    C.append("extern const u32 %s[];" % name)


def blob_u8(name, b):
    write_blob(name, b)
    C.append("extern const u8 %s[];" % name)


def emit_pal(name, pal16):
    C.append(c_array(name, "u16", pal16, 16))


# ------------------------------------------------------------------ hojas de personaje (mapa)
def char_frames(img):
    """160x120 (8x4 frames de 20x30) -> 32 imágenes 32x32 con el frame en (6,2)."""
    out = []
    for r in range(4):
        for c in range(8):
            fr = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
            fr.alpha_composite(img.crop((c * 20, r * 30, c * 20 + 20, r * 30 + 30)), (6, 2))
            out.append(fr)
    return out


def battle_frames(img):
    return [img.crop((k * 32, 0, k * 32 + 32, 32)) for k in range(img.width // 32)]


def export_sheet(name, frames, w=32, h=32):
    data, pal = sheet_quant(frames)
    ident = "sh_" + c_ident(name)
    blob_u32(ident + "_t", data)
    emit_pal(ident + "_p", pal)
    C.append("const Sheet %s = {%s_t, %s_p, %d, %d, %d};" % (ident, ident, ident, len(frames), w, h))
    H.append("extern const Sheet %s;" % ident)
    return ident


# ------------------------------------------------------------------ metasprites (enemigos, monstruos de escena)
def decompose(mask_cells, cw, ch):
    """Cubre las celdas ocupadas con rectángulos OBJ válidos sin solaparse."""
    todo = set(mask_cells)
    covered = set()
    pieces = []
    while todo:
        cy, cx = min((c[1], c[0]) for c in todo)
        for pw, ph in OBJ_SHAPES:
            w8, h8 = pw // 8, ph // 8
            if cx + w8 > cw or cy + h8 > ch:
                continue
            cells = {(cx + i, cy + j) for i in range(w8) for j in range(h8)}
            if cells & covered:
                continue
            pieces.append((cx * 8, cy * 8, pw, ph))
            covered |= cells
            todo -= cells
            break
        else:
            raise RuntimeError("no se pudo descomponer")
    return pieces


def export_metasprite(name, frames, anchor="center"):
    """frames: lista de RGBA del mismo tamaño. anchor: 'center' (combate) o 'bottom' (mapa)."""
    w0, h0 = frames[0].size
    cw, ch = (w0 + 7) // 8, (h0 + 7) // 8
    pad = []
    for f in frames:
        p = Image.new("RGBA", (cw * 8, ch * 8), (0, 0, 0, 0))
        p.alpha_composite(f, (0, 0))
        pad.append(p)
    occ = set()
    for p in pad:
        a = np.array(p)[..., 3]
        for cy in range(ch):
            for cx in range(cw):
                if (a[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] >= 128).any():
                    occ.add((cx, cy))
    pieces = decompose(occ, cw, ch)
    crops = []
    for p in pad:
        for (x, y, pw, ph) in pieces:
            crops.append(p.crop((x, y, x + pw, y + ph)))
    data, pal = sheet_quant(crops)
    ftiles = sum(pw * ph // 64 for (_, _, pw, ph) in pieces)
    ident = "ms_" + c_ident(name)
    blob_u32(ident + "_t", data)
    emit_pal(ident + "_p", pal)
    ox, oy = (w0 // 2, h0 // 2) if anchor == "center" else (w0 // 2, h0)
    pdefs = []
    off = 0
    for (x, y, pw, ph) in pieces:
        sh, sz = obj_shape_size(pw, ph)
        pdefs.append("{%d, %d, %d, %d, %d}" % (x - ox, y - oy, sh, sz, off))
        off += pw * ph // 64
    C.append("static const ObjPiece %s_pc[] = {%s};" % (ident, ", ".join(pdefs)))
    C.append("const MetaSprite %s = {%s_t, %s_p, %d, %d, %d, %d, %d, %s_pc};" % (
        ident, ident, ident, len(frames), len(pieces), ftiles, w0, h0, ident))
    H.append("extern const MetaSprite %s;" % ident)
    return ident


def scaled_frames(path, nframes, scale, flip=False):
    img = load_rgba(path)
    fw = img.width // nframes
    out = []
    for k in range(nframes):
        f = img.crop((k * fw, 0, (k + 1) * fw, img.height))
        if flip:
            f = f.transpose(Image.FLIP_LEFT_RIGHT)
        if abs(scale - 1.0) > 1e-3:
            nw, nh = max(1, round(fw * scale)), max(1, round(img.height * scale))
            if scale >= 1.0:
                f = f.resize((nw, nh), Image.NEAREST)
            else:
                # reducción: suavizado + umbral de alfa
                a = f.resize((nw, nh), Image.LANCZOS)
                arr = np.array(a)
                arr[..., 3] = np.where(arr[..., 3] >= 110, 255, 0)
                f = Image.fromarray(arr, "RGBA")
        out.append(f)
    return out


# ------------------------------------------------------------------ fondos
def export_bg(name, img, npal):
    rgb, al, cw, ch = img_cells(img)
    pals = build_palettes(rgb, al, npal, transparent=False)
    asg = assign_palettes(rgb, al, pals, transparent=False)
    enc = encode_cells(rgb, al, pals, asg, transparent=False)
    bank = TileBank(reserve_blank=True)
    cells = [bank.add(t) | (int(a) << 12) for t, a in zip(enc, asg)]
    ident = "bg_" + c_ident(name)
    blob_u32(ident + "_t", bank.blob())
    write_blob(ident + "_c", np.array(cells, np.uint16))
    C.append("extern const u16 %s_c[];" % ident)
    emit_pal(ident + "_p", [v for p in pals for v in pal555(p)])
    C.append("const BgImage %s = {%d, %d, %d, %s_t, %s_c, %s_p, %d};" % (ident, cw, ch, len(bank.tiles), ident, ident, ident, npal))
    H.append("extern const BgImage %s;" % ident)


# ------------------------------------------------------------------ capas del creador (composición en la GBA)
def rle(px):
    out = bytearray()
    i, n = 0, len(px)
    while i < n:
        j = i
        while j < n and j - i < 128 and px[j] == px[i]:
            j += 1
        if j - i >= 3:
            out.append(0x80 | (j - i - 1))
            out.append(px[i])
            i = j
            continue
        j = i
        while j < n and j - i < 128 and not (j + 2 < n and px[j] == px[j + 1] == px[j + 2]):
            j += 1
        out.append(j - i - 1)
        out += bytes(px[i:j])
        i = j
    return bytes(out)


LAYERS = []


def export_layer(key, path):
    img = np.array(load_rgba(path), dtype=np.int32)
    h, w = img.shape[:2]
    flat = img.reshape(-1, 4)
    op = flat[:, 3] >= 128
    cols = {}
    idx = np.zeros(len(flat), np.uint8)
    for i in np.nonzero(op)[0]:
        c = tuple(int(v) for v in flat[i, :3])
        if c not in cols:
            cols[c] = len(cols) + 1
        idx[i] = cols[c]
    if len(cols) > 255:
        raise SystemExit("capa con demasiados colores: " + path)
    rgb = bytearray()
    for c in sorted(cols, key=lambda c: cols[c]):
        rgb += bytes(c)
    ident = "ly_" + c_ident(key)
    blob_u8(ident + "_c", bytes(rgb))
    blob_u8(ident + "_r", rle(idx.tobytes()))
    LAYERS.append((key, "{%s, %d, %d, %d, %s_c, %s_r}" % (c_string(key), w, h, len(cols), ident, ident)))


# ------------------------------------------------------------------ interfaz
def rgbf(r, g, b):
    return (int(r * 255), int(g * 255), int(b * 255))


def nine_src(name, pal_rgb):
    """Imagen de interfaz -> índices en una paleta fija (0 transparente)."""
    a = np.array(load_rgba(os.path.join(GAME, "ui", name + ".png")), dtype=np.int32)
    h, w = a.shape[:2]
    p = np.array(pal_rgb, dtype=np.int32)
    out = np.zeros((h, w), np.uint8)
    for y in range(h):
        for x in range(w):
            if a[y, x, 3] >= 128:
                d = (((a[y, x, :3] >> 3 << 3) - p) ** 2).sum(1)
                d[0] = 1 << 30
                out[y, x] = int(d.argmin())
    ident = "ui_" + c_ident(name)
    blob_u8(ident + "_px", out.tobytes())
    return ident, w, h


def ui_palettes():
    def cols(name):
        a = np.array(load_rgba(os.path.join(GAME, "ui", name + ".png")), dtype=np.int32).reshape(-1, 4)
        a = a[a[:, 3] >= 128][:, :3] >> 3 << 3
        u, cnt = np.unique(a, axis=0, return_counts=True)
        return [tuple(int(v) for v in c) for c in u[np.argsort(-cnt)]]

    defs = {}
    # Paleta clara (pergamino): panel + textos oscuros
    light_text = [("TEXT", rgbf(0.24, 0.15, 0.11)), ("SEL", rgbf(0.6, 0.2, 0.1)), ("ARM", rgbf(0.85, 0.22, 0.08)),
                  ("HI2", rgbf(0.55, 0.2, 0.12)), ("HINT", rgbf(0.45, 0.35, 0.3)), ("TITLE", rgbf(0.35, 0.18, 0.1)),
                  ("GOOD", rgbf(0.15, 0.5, 0.15))]
    dark_text = [("TEXT", rgbf(1, 0.93, 0.75)), ("NAME", rgbf(0.98, 0.86, 0.55)), ("WHITE", (248, 248, 248)),
                 ("OUT", rgbf(0.1, 0.07, 0.1)), ("DIM", rgbf(0.65, 0.6, 0.55)), ("GOLD", rgbf(1, 0.93, 0.6))]
    win_text = [("TEXT", rgbf(0.97, 0.97, 1.0)), ("HI", rgbf(1.0, 0.86, 0.35)), ("DIS", rgbf(0.55, 0.57, 0.68)),
                ("OUT", rgbf(0.1, 0.07, 0.1)), ("HP", rgbf(0.36, 0.86, 0.42)), ("MP", rgbf(0.42, 0.66, 1.0)),
                ("LOW", rgbf(0.95, 0.3, 0.28))]
    btn_text = [("TEXT", rgbf(1, 0.93, 0.75)), ("OUT", rgbf(0.12, 0.08, 0.08)), ("DIS", rgbf(0.6, 0.55, 0.5)),
                ("GOLD", rgbf(1, 0.93, 0.6))]

    def build(name, base, text, nbase):
        base = palette_from_pixels(np.array(base), nbase) if len(base) > nbase else np.array(base)
        pal = [(0, 0, 0)] + [tuple(int(v) for v in c) for c in base]
        names = {}
        for k, c in text:
            names[k] = len(pal)
            pal.append(c)
        assert len(pal) <= 16, (name, len(pal))
        pal += [(0, 0, 0)] * (16 - len(pal))
        defs[name] = (pal, names, len(base))
        return pal

    build("LIGHT", cols("panel"), light_text, 8)
    build("DARK", cols("panel_dark"), dark_text, 9)
    wc = cols("window") + cols("select_bar")
    build("WIN", wc, win_text, 8)
    build("WINRED", cols("window_red"), win_text, 8)
    bc = cols("button_normal") + cols("button_hover") + cols("button_disabled") + cols("button_pressed")
    build("BTN", bc, btn_text, 11)
    out = []
    for k, (pal, names, nb) in defs.items():
        emit_pal("uipal_%s" % k.lower(), [to555(c) for c in pal])
        C[-1] = C[-1].replace("static const", "const")
        H.append("extern const u16 uipal_%s[16];" % k.lower())
        for n, i in names.items():
            H.append("#define UC_%s_%s %d" % (k, n, i))
    return defs


def export_ui():
    defs = ui_palettes()
    srcs = [("panel", "LIGHT", 8), ("panel_dark", "DARK", 8), ("window", "WIN", 6), ("window_red", "WINRED", 6),
            ("button_normal", "BTN", 4), ("button_hover", "BTN", 4), ("button_pressed", "BTN", 4),
            ("button_disabled", "BTN", 4), ("select_bar", "WIN", 0)]
    rows = []
    for name, pal, margin in srcs:
        ident, w, h = nine_src(name, defs[pal][0])
        rows.append("{%s_px, %d, %d, %d}" % (ident, w, h, margin))
        H.append("#define NP_%s %d" % (name.upper(), len(rows) - 1))
    C.append("const NineSrc g_nine[] = {%s};" % ", ".join(rows))
    H.append("extern const NineSrc g_nine[];")
    # iconos -> OBJ 16x16 con paletas compartidas
    icons = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "ui")) if f.endswith(".png") and f.split("_")[0] in
                   ("cmd", "el", "eq", "icon", "it", "st"))
    imgs = []
    for n in icons:
        im = load_rgba(os.path.join(GAME, "ui", n + ".png"))
        p = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        p.alpha_composite(im, ((16 - im.width) // 2, (16 - im.height) // 2))
        imgs.append(p)
    # agrupar iconos en 4 paletas por color
    feats = []
    for p in imgs:
        a = np.array(p, dtype=np.int32).reshape(-1, 4)
        a = a[a[:, 3] >= 128][:, :3]
        feats.append(np.concatenate([a.mean(0), a.std(0)]) if len(a) else np.zeros(6))
    NP = 4
    _, grp = kmeans(np.array(feats), NP, iters=20)
    pals = []
    for g in range(NP):
        px = []
        for i in np.nonzero(grp == g)[0]:
            a = np.array(imgs[i], dtype=np.int32).reshape(-1, 4)
            px.append(quant5(a[a[:, 3] >= 128][:, :3]))
        pals.append(palette_from_pixels(np.concatenate(px) if px else np.zeros((0, 3)), 15))
    data = bytearray()
    rows = []
    for i, (n, p) in enumerate(zip(icons, imgs)):
        # mejor paleta para este icono
        cr, ca, _, _ = img_cells(p)
        best, berr = 0, None
        for g in range(NP):
            asg = np.full(len(cr), g)
            e = 0
            pp = np.array(pals[g], np.float32)
            for k in range(len(cr)):
                d = ((cr[k][:, None, :] - pp[None]) ** 2).sum(-1).min(1)
                e += (d * (ca[k] >= 128)).sum()
            if berr is None or e < berr:
                best, berr = g, e
        enc = encode_cells(cr, ca, [pals[best]], np.zeros(len(cr), np.int32))
        data += b"".join(enc)
        rows.append('{"%s", %d}' % (n, best))
    blob_u32("icons_t", bytes(data))
    for g in range(NP):
        emit_pal("iconpal_%d" % g, pal555(pals[g]))
    C.append("const u16* const g_icon_pals[] = {%s};" % ", ".join("iconpal_%d" % g for g in range(NP)))
    C.append("const IconDef g_icons[] = {%s};" % ", ".join(rows))
    C.append("const int g_nicons = %d;" % len(icons))
    H.append("extern const u16* const g_icon_pals[]; extern const IconDef g_icons[]; extern const int g_nicons;")
    H.append("extern const u32 icons_t[];")
    H.append("#define ICON_NPALS %d" % NP)
    # cursores: mano, cursor, flecha de turno y burbuja de interacción
    small = []
    for n in ("hand", "cursor", "turn_arrow"):
        im = load_rgba(os.path.join(GAME, "ui", n + ".png"))
        p = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
        p.alpha_composite(im, (0, 0))
        small.append(p)
    # flecha de turno a doble tamaño (en Godot se dibuja con escala 2)
    ta = load_rgba(os.path.join(GAME, "ui", "turn_arrow.png")).resize((18, 14), Image.NEAREST)
    p = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    p.alpha_composite(ta, (0, 0))
    small2 = [p]
    bub = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(bub)
    d.rectangle((0, 0, 10, 10), fill=(31, 23, 31, 255))
    d.rectangle((1, 1, 9, 9), fill=(242, 230, 199, 255))
    for (x, y) in [(5, 3), (4, 4), (6, 4), (4, 5), (6, 5), (3, 6), (4, 6), (5, 6), (6, 6), (7, 6), (3, 7), (7, 7)]:
        bub.putpixel((x, y), (61, 38, 28, 255))
    small.append(bub)
    allf = small + [small2[0].crop((0, 0, 16, 16)), small2[0].crop((16, 0, 32, 16))]
    data, pal = sheet_quant(allf)
    blob_u32("cursors_t", data)
    emit_pal("cursors_p", pal)
    C[-1] = C[-1].replace("static const", "const")
    H.append("extern const u32 cursors_t[]; extern const u16 cursors_p[16];")
    H.append("#define CUR_HAND 0\n#define CUR_CURSOR 1\n#define CUR_ARROW 2\n#define CUR_BUBBLE 3\n#define CUR_ARROW2 4")


# ------------------------------------------------------------------ fuente
FONT_CHARS = [chr(c) for c in range(32, 127)] + list("¡¿áéíóúÁÉÍÓÚñÑüÜ«»…—–·×ºª“”‘’¨´çÇàèìòùâêîôû")


def export_font():
    f = ImageFont.truetype(os.path.join(GAME, "fonts", "PixelifySans.ttf"), 10)
    Hh = 12
    glyphs = []
    for ch in FONT_CHARS:
        im = Image.new("L", (24, 16), 0)
        d = ImageDraw.Draw(im)
        d.text((0, -1), ch, font=f, fill=255)
        a = np.array(im) > 100
        bbox = f.getbbox(ch)
        adv = int(round(f.getlength(ch)))
        if ch == " ":
            adv = 3
        w = max(adv, (np.nonzero(a.any(0))[0].max() + 1) if a.any() else 0)
        if not a.any() and ch != " ":
            continue
        rows = []
        for y in range(Hh):
            v = 0
            for x in range(min(16, w)):
                if y < a.shape[0] and a[y, x]:
                    v |= 1 << x
            rows.append(v)
        glyphs.append((ord(ch), adv, rows))
    glyphs.sort()
    C.append(c_array("font_codes", "u16", [g[0] for g in glyphs], 16))
    C.append(c_array("font_adv", "u8", [g[1] for g in glyphs], 32))
    C.append(c_array("font_rows", "u16", [r for g in glyphs for r in g[2]], 16))
    C.append("const Font g_font = {font_codes, font_adv, font_rows, %d, %d};" % (len(glyphs), Hh))
    H.append("extern const Font g_font;")


# ------------------------------------------------------------------ principal
SCENE_SPRITES = [
    # nombre, ruta, frames, escala (mapa, 1:1), voltear
    ("wolf_map", "sprites/wolf.png", 2, 1.0, False),
    ("sap_puddle", "sprites/sap_puddle.png", 1, 1.0, False),
    ("root_08", "enemies/root.png", 2, 0.8, True),
    ("aleixolo_05", "enemies/aleixolo.png", 2, 0.5, False),
    ("kraag_04", "enemies/kraag.png", 2, 0.4, True),
    ("soldado_042", "enemies/soldado.png", 2, 0.42, True),
    ("larva_09", "enemies/larva.png", 2, 0.9, False),
    ("brute_075", "enemies/brute.png", 2, 0.75, False),
]


def main():
    db = json.load(open(os.path.join(GBA, "data", "db.json")))
    print("  hojas de personajes", flush=True)
    for f in sorted(os.listdir(os.path.join(GAME, "chars"))):
        if not f.endswith(".png") or f == "player.png":
            continue
        export_sheet("c_" + f[:-4], char_frames(load_rgba(os.path.join(GAME, "chars", f))))
    for f in sorted(os.listdir(os.path.join(GAME, "battle"))):
        if f.endswith(".png"):
            export_sheet("b_" + f[:-4], battle_frames(load_rgba(os.path.join(GAME, "battle", f))))
    print("  retratos", flush=True)
    for f in sorted(os.listdir(os.path.join(GAME, "portraits"))):
        if not f.endswith(".png"):
            continue
        im = load_rgba(os.path.join(GAME, "portraits", f))
        data, pal = sheet_quant([im])
        ident = "por_" + c_ident(f[:-4])
        blob_u32(ident + "_t", data)
        emit_pal(ident + "_p", pal)
        C.append("const Portrait %s = {%s_t, %s_p};" % (ident, ident, ident))
        H.append("extern const Portrait %s;" % ident)
    print("  enemigos", flush=True)
    rows = []
    done = {}
    for key, e in sorted(db["ENEMIES"].items()):
        spr, fr, sc = e["sprite"], int(e["frames"]), float(e["scale"]) / 2.0
        k2 = (spr, fr, sc)
        if k2 not in done:
            path = os.path.join(GAME, spr + ".png")
            if fr == -1:
                frames = scaled_frames(path, 9, sc * 2.0 / 2.0 if False else sc, flip=True)
            else:
                frames = scaled_frames(path, max(1, fr), sc)
            done[k2] = export_metasprite("e_" + c_ident(os.path.basename(spr)) + "_%d" % int(sc * 100), frames, "center")
    C.append("// enemigos: clave -> metasprite")
    for key, e in sorted(db["ENEMIES"].items()):
        spr, fr, sc = e["sprite"], int(e["frames"]), float(e["scale"]) / 2.0
        rows.append('{"%s", &%s}' % (key, done[(spr, fr, sc)]))
    C.append("const EnemyGfx g_enemy_gfx[] = {%s};" % ", ".join(rows))
    C.append("const int g_nenemy_gfx = %d;" % len(rows))
    H.append("extern const EnemyGfx g_enemy_gfx[]; extern const int g_nenemy_gfx;")
    print("  sprites de escena", flush=True)
    rows = []
    for (n, path, fr, sc, flip) in SCENE_SPRITES:
        ident = export_metasprite("s_" + n, scaled_frames(os.path.join(GAME, path), fr, sc, flip), "bottom")
        rows.append('{"%s", &%s}' % (n, ident))
    C.append("const SceneSprite g_scene_sprites[] = {%s};" % ", ".join(rows))
    C.append("const int g_nscene_sprites = %d;" % len(rows))
    H.append("extern const SceneSprite g_scene_sprites[]; extern const int g_nscene_sprites;")
    print("  fondos", flush=True)
    for f in sorted(os.listdir(os.path.join(GAME, "bg"))):
        if f.endswith(".png"):
            export_bg(f[:-4], load_rgba(os.path.join(GAME, "bg", f)), 11)
    print("  capas del creador", flush=True)
    for f in sorted(os.listdir(os.path.join(GAME, "creator"))):
        if not f.endswith(".png"):
            continue
        parts = f[:-4].split("_")
        if parts[0] == "weapon" and not parts[-1].isdigit() and parts[-1] not in ("dark", "light"):
            continue
        export_layer("creator/" + f[:-4], os.path.join(GAME, "creator", f))
    for n in ("kaelen", "yara", "aelis", "brom", "kaelen_dark", "yara_dark"):
        export_layer("chars/" + n, os.path.join(GAME, "chars", n + ".png"))
    for f in sorted(os.listdir(os.path.join(GAME, "battle"))):
        if f.endswith("_nw.png"):
            export_layer("battle/" + f[:-4], os.path.join(GAME, "battle", f))
    LAYERS.sort()
    C.append("const Layer g_layers[] = {%s};" % ",\n\t".join(r for _, r in LAYERS))
    C.append("const int g_nlayers = %d;" % len(LAYERS))
    H.append("extern const Layer g_layers[]; extern const int g_nlayers;")
    print("  logotipo", flush=True)
    f = ImageFont.truetype(os.path.join(GAME, "fonts", "PixelifySans.ttf"), 30)
    txt = "VAELMOOR"
    w = int(f.getlength(txt)) + 8
    logo = Image.new("RGBA", (w, 40), (0, 0, 0, 0))
    d = ImageDraw.Draw(logo)
    for ox in range(-2, 3):
        for oy in range(-2, 3):
            if abs(ox) + abs(oy) <= 3:
                d.text((4 + ox, 2 + oy), txt, font=f, fill=(26, 18, 26, 255))
    d.text((4, 2), txt, font=f, fill=(250, 219, 140, 255))
    arr = np.array(logo)
    arr[..., 3] = np.where(arr[..., 3] >= 128, 255, 0)
    logo = Image.fromarray(arr, "RGBA")
    bb = logo.getbbox()
    logo = logo.crop(bb)
    export_metasprite("logo", [logo], "center")
    print("  interfaz y fuente", flush=True)
    export_ui()
    export_font()
    # tablas de nombres para appearance.c
    names = ["// generado por conv_gfx.py"]
    pors = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "portraits")) if f.endswith(".png"))
    names.append("static const struct { const char* n; const Portrait* p; } por_tab[] = {%s};" % ", ".join(
        '{"%s", &por_%s}' % (n, c_ident(n)) for n in pors))
    chs = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "chars")) if f.endswith(".png") and f != "player.png")
    names.append("static const struct { const char* n; const Sheet* s; } char_tab[] = {%s};" % ", ".join(
        '{"%s", &sh_c_%s}' % (n, c_ident(n)) for n in chs))
    bts = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "battle")) if f.endswith(".png"))
    names.append("static const struct { const char* n; const Sheet* s; } bat_tab[] = {%s};" % ", ".join(
        '{"%s", &sh_b_%s}' % (n, c_ident(n)) for n in bts))
    names.append("""const Portrait* portrait_by_name(const char* n) {
	for (int i = 0; i < (int)(sizeof por_tab / sizeof por_tab[0]); ++i) if (!strcmp(por_tab[i].n, n)) return por_tab[i].p;
	return 0;
}
const Sheet* char_sheet(const char* n) {
	for (int i = 0; i < (int)(sizeof char_tab / sizeof char_tab[0]); ++i) if (!strcmp(char_tab[i].n, n)) return char_tab[i].s;
	return 0;
}
const Sheet* battle_sheet_static(const char* n) {
	for (int i = 0; i < (int)(sizeof bat_tab / sizeof bat_tab[0]); ++i) if (!strcmp(bat_tab[i].n, n)) return bat_tab[i].s;
	return 0;
}""")
    write_if_changed(os.path.join(OUT, "gfx_names.h"), "\n".join(names) + "\n")
    write_if_changed(os.path.join(OUT, "gfx.c"), "\n".join(C) + "\n")
    write_if_changed(os.path.join(OUT, "gfx.h"), "\n".join(H) + "\n")
    blobs_asm()


if __name__ == "__main__":
    main()
