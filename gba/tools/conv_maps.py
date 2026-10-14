"""Mapas de Vaelmoor -> GBA.

Cada mapa se convierte en:
  * suelo: el PNG del mapa con TODOS los objetos estáticos horneados encima (BG1).
  * capa de oclusión: para cada celda 8x8 cubierta por objetos, una lista de variantes
    "objetos con y > profundidad" ya compuestas; en tiempo de ejecución se elige la variante
    según la y del personaje que pisa la celda y se dibuja por encima de los sprites (BG2).
    Así los personajes pasan por detrás de árboles, casas, etc. exactamente como en Godot.
  * parches: objetos que cambian durante la partida (cofres abiertos, runas, escombros que
    desaparecen, catedral abierta, hogueras animadas, piedras de retorno) guardan sus celdas
    alternativas para cada estado.
"""
import json
import os
import sys
import numpy as np
from PIL import Image
from lib import *

MAP_PALS = 12

ALT = {
    "village_night": {"cathedral": ["cathedral_open"]},
    "village_dawn": {"cathedral": ["cathedral_open"]},
    "village": {"cathedral": ["cathedral_open"]},
    "cathedral": {"rune_1": ["rune_on"], "rune_2": ["rune_on"], "rune_3": ["rune_on"]},
    "crypt": {"seed": ["seed_empty"]},
    "ruinas": {"mirror1": ["mirror_on"], "mirror2": ["mirror_on"], "mirror3": ["mirror_on"]},
    "paso_sur": {"cage1": ["cage_empty"], "cage2": ["cage_empty"], "cage3": ["cage_empty"]},
}
REMOVABLE = {
    "village_night": ["rubble"], "village_dawn": ["rubble"], "village": ["rubble"], "cathedral": ["crypt_rubble"],
}
WAYSTONE = {"minas": (320, 120), "ruinas": (340, 124), "paso_sur": (490, 200), "elf_camp": (360, 150),
            "village_dawn": (410, 320)}
WAYSTONE_PROP = {"sprite": "rune_on", "id": "waystone", "col": [-6, -6, 12, 6], "frames": 1,
                 "light": {"r": 56, "color": [0.55, 0.85, 1.0], "e": 0.9, "flicker": True, "oy": -10}, "_added": True}

_sprite_cache = {}


def sprite(name):
    if name not in _sprite_cache:
        _sprite_cache[name] = np.array(load_rgba(os.path.join(GAME, "sprites", name + ".png")), dtype=np.int32)
    return _sprite_cache[name]


def render_prop(p, state):
    """-> (rgba int array, x0, y0) o None si el objeto no se ve en ese estado."""
    kind = state[0]
    if kind in ("removed", "hidden"):
        return None
    name = state[1] if kind == "sprite" else p["sprite"]
    img = sprite(name)
    frames = int(p.get("frames", 1))
    fw = img.shape[1] // frames
    k = state[1] if kind == "frame" else 0
    img = img[:, k * fw:(k + 1) * fw].copy()
    if p.get("flip"):
        img = img[:, ::-1].copy()
    if kind == "dark":
        img[..., 0] = img[..., 0] * 55 // 100
        img[..., 1] = img[..., 1] * 50 // 100
        img[..., 2] = img[..., 2] * 50 // 100
    x0 = int(p["x"]) - fw // 2
    y0 = int(p["y"]) - img.shape[0] - int(p.get("lift", 0))
    return img, x0, y0


def prop_states(mid, p):
    if p.get("_added"):
        return [("hidden",), ("default",)]
    pid = p.get("id", "")
    st = [("default",)]
    if int(p.get("frames", 1)) > 1:
        for k in range(1, int(p["frames"])):
            st.append(("frame", k))
        return st
    for s in ALT.get(mid, {}).get(pid, []):
        st.append(("sprite", s))
    if pid.startswith("chest_"):
        st.append(("dark",))
    if pid in REMOVABLE.get(mid, []) or pid.startswith("chest_ab"):
        st.append(("removed",))
    return st


def state_name(s):
    if s[0] == "sprite":
        return s[1]
    if s[0] == "frame":
        return "f%d" % s[1]
    return s[0]


def convert(mid):
    data = json.load(open(os.path.join(GAME, "maps", mid + ".json")))
    ground = np.array(load_rgba(os.path.join(GAME, "maps", mid + ".png")), dtype=np.int32)
    H, W = ground.shape[:2]
    cw, ch = (W + 7) // 8, (H + 7) // 8
    props = [dict(p) for p in data["props"]]
    # las casas solo chocaban por la franja de abajo y se podía caminar tras medio tejado: más volumen
    for p in props:
        if str(p.get("sprite", "")).startswith("house") and p.get("col"):
            c = list(p["col"])
            bottom = max(c[1] + c[3], -2)   # hasta la base: si no, el personaje queda oculto tras la fachada
            c[1] = min(c[1], -56)
            c[3] = bottom - c[1]
            p["col"] = c
            # la puerta queda dentro del nuevo bloque: bajar la salida hasta el borde inferior
            x0, y0, x1, y1 = p["x"] + c[0], p["y"] + c[1], p["x"] + c[0] + c[2], p["y"] + c[1] + c[3]
            for e in data.get("exits", []):
                r = e["rect"]
                if r[0] < x1 and r[0] + r[2] > x0 and r[1] < y1 and r[1] + r[3] > y0:
                    e["rect"] = [r[0], y1 - 3, r[2], 10]
    if mid in WAYSTONE:
        wp = dict(WAYSTONE_PROP)
        wp["x"], wp["y"] = WAYSTONE[mid]
        props.append(wp)
    for i, p in enumerate(props):
        p["_i"] = i
        p["_states"] = prop_states(mid, p)
    order = sorted(range(len(props)), key=lambda i: (props[i]["y"], i))
    rendered = {}

    def rend(i, st):
        key = (i, st)
        if key not in rendered:
            rendered[key] = render_prop(props[i], st)
        return rendered[key]

    def compose(x, y, w, h, over):
        """Suelo + objetos (orden de profundidad) en el rectángulo dado."""
        out = np.zeros((h, w, 4), np.int32)
        gx0, gy0 = max(0, x), max(0, y)
        gx1, gy1 = min(W, x + w), min(H, y + h)
        if gx1 > gx0 and gy1 > gy0:
            out[gy0 - y:gy1 - y, gx0 - x:gx1 - x] = ground[gy0:gy1, gx0:gx1]
        out[..., 3] = 255
        for i in order:
            st = over.get(i, props[i]["_states"][0])
            r = rend(i, st)
            if r is None:
                continue
            img, x0, y0 = r
            blit(out, img, x0 - x, y0 - y)
        return out

    def blit(dst, img, ox, oy):
        h, w = img.shape[:2]
        dx0, dy0 = max(0, ox), max(0, oy)
        dx1, dy1 = min(dst.shape[1], ox + w), min(dst.shape[0], oy + h)
        if dx1 <= dx0 or dy1 <= dy0:
            return
        src = img[dy0 - oy:dy1 - oy, dx0 - ox:dx1 - ox]
        m = src[..., 3] >= 128
        reg = dst[dy0:dy1, dx0:dx1]
        reg[m] = src[m]

    # ------------------------------------------------ celdas cubiertas por cada objeto
    def prop_cells(i, st):
        r = rend(i, st)
        if r is None:
            return set()
        img, x0, y0 = r
        m = img[..., 3] >= 128
        ys, xs = np.nonzero(m)
        cells = set()
        for cy, cx in set(zip((ys + y0) // 8, (xs + x0) // 8)):
            if 0 <= cx < cw and 0 <= cy < ch:
                cells.add((int(cx), int(cy)))
        return cells

    def overlay_variants(cx, cy, over):
        """Lista [(profundidad, rgba 8x8)] para la celda."""
        lst = []
        for i in order:
            st = over.get(i, props[i]["_states"][0])
            r = rend(i, st)
            if r is None:
                continue
            img, x0, y0 = r
            # ¿tiene píxeles opacos en la celda?
            sx0, sy0 = cx * 8 - x0, cy * 8 - y0
            sub = img[max(0, sy0):max(0, sy0 + 8), max(0, sx0):max(0, sx0 + 8)]
            if sub.size == 0 or not (sub[..., 3] >= 128).any():
                continue
            lst.append(i)
        out = []
        prev_depth = None
        for k in range(len(lst)):
            d = int(props[lst[k]]["y"])
            if d == prev_depth:
                continue
            prev_depth = d
            tile = np.zeros((8, 8, 4), np.int32)
            for i in lst[k:]:
                st = over.get(i, props[i]["_states"][0])
                img, x0, y0 = rend(i, st)
                blit(tile, img, x0 - cx * 8, y0 - cy * 8)
            out.append((d, tile))
        return out

    print("  %-16s %4dx%-4d %3d objetos" % (mid, W, H, len(props)), flush=True)
    full = compose(0, 0, cw * 8, ch * 8, {})
    covered = set()
    for i in range(len(props)):
        covered |= prop_cells(i, props[i]["_states"][0])
    ov_default = {}
    for (cx, cy) in covered:
        v = overlay_variants(cx, cy, {})
        if v:
            ov_default[(cx, cy)] = v

    # ------------------------------------------------ parches
    patches = []
    for i, p in enumerate(props):
        sts = p["_states"]
        if len(sts) < 2:
            continue
        cells = set()
        for st in sts:
            cells |= prop_cells(i, st)
        if not cells:
            continue
        xs = [c[0] for c in cells]
        ys = [c[1] for c in cells]
        rx0, ry0, rx1, ry1 = min(xs), min(ys), max(xs) + 1, max(ys) + 1
        states = []
        for st in sts:
            over = {i: st}
            g = compose(rx0 * 8, ry0 * 8, (rx1 - rx0) * 8, (ry1 - ry0) * 8, over)
            ov = {}
            for cy in range(ry0, ry1):
                for cx in range(rx0, rx1):
                    v = overlay_variants(cx, cy, over)
                    if v:
                        ov[(cx, cy)] = v
            states.append((state_name(st), g, ov))
        kind = 2 if p.get("_added") else (1 if sts[1][0] == "frame" else 0)
        patches.append({"prop": i, "rect": (rx0, ry0, rx1 - rx0, ry1 - ry0), "states": states, "kind": kind})

    # ------------------------------------------------ paletas (suelo + estados de parches)
    def to_img(a):
        return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGBA")

    g_rgb, g_al, _, _ = img_cells(to_img(full))
    extra_rgb, extra_al = [], []
    for pt in patches:
        for (_, g, _) in pt["states"][1:]:
            r, a, _, _ = img_cells(to_img(g))
            extra_rgb.append(r)
            extra_al.append(a)
    all_rgb = np.concatenate([g_rgb] + extra_rgb) if extra_rgb else g_rgb
    all_al = np.concatenate([g_al] + extra_al) if extra_al else g_al
    pals = build_palettes(all_rgb, all_al, MAP_PALS, transparent=False)
    gbank = TileBank()

    def encode_ground(rgb, al):
        asg = assign_palettes(rgb, al, pals, transparent=False)
        enc = encode_cells(rgb, al, pals, asg, transparent=False)
        return [gbank.add(t) for t in enc], [int(a) for a in asg]

    gtiles, gpal = encode_ground(g_rgb, g_al)
    obank = TileBank(reserve_blank=True)
    olist = [0]

    def encode_ov(variants):
        """-> índice en olist."""
        rgbs = np.array([quant5(t[..., :3]).reshape(64, 3) for _, t in variants])
        als = np.array([t[..., 3].reshape(64) for _, t in variants])
        asg = assign_palettes(rgbs, als, pals, transparent=True)
        enc = encode_cells(rgbs, als, pals, asg, transparent=True)
        off = len(olist)
        olist.append(len(variants))
        for (d, _), t, a in zip(variants, enc, asg):
            olist.extend([d & 0xFFFF, obank.add(t), int(a)])
        return off

    ocell = [0] * (cw * ch)
    for (cx, cy), v in ov_default.items():
        ocell[cy * cw + cx] = encode_ov(v)
    pdata = []
    for pt in patches:
        rx0, ry0, rw, rh = pt["rect"]
        sts = []
        for (nm, g, ov) in pt["states"]:
            r, a, _, _ = img_cells(to_img(g))
            t, pl = encode_ground(r, a)
            oc = []
            for cy in range(ry0, ry0 + rh):
                for cx in range(rx0, rx0 + rw):
                    oc.append(encode_ov(ov[(cx, cy)]) if (cx, cy) in ov else 0)
            sts.append((nm, t, pl, oc))
        pdata.append((pt, sts))
    if len(olist) > 65535 or len(obank.tiles) > 65535 or len(gbank.tiles) > 65535:
        raise SystemExit("mapa %s demasiado grande" % mid)

    # ------------------------------------------------ salida
    ident = c_ident(mid)
    pal16 = [v for p in pals for v in pal555(p)]
    write_blob("mg_%s" % ident, gbank.blob())
    write_blob("mgc_%s" % ident, np.array(gtiles, np.uint16))
    write_blob("mgp_%s" % ident, np.array(gpal, np.uint8))
    write_blob("mo_%s" % ident, obank.blob())
    write_blob("moc_%s" % ident, np.array(ocell, np.uint16))
    write_blob("mol_%s" % ident, np.array(olist, np.uint16))
    c = ['#include "../../src/mapdata.h"', ""]
    for n in ("mg", "mo"):
        c.append("extern const u32 %s_%s[];" % (n, ident))
    for n in ("mgc", "moc", "mol"):
        c.append("extern const u16 %s_%s[];" % (n, ident))
    c.append("extern const u8 mgp_%s[];" % ident)
    c.append(c_array("pal_%s" % ident, "u16", pal16, 16))
    # parches
    pstructs = []
    for k, (pt, sts) in enumerate(pdata):
        rx0, ry0, rw, rh = pt["rect"]
        names = []
        sdefs = []
        for j, (nm, t, pl, oc) in enumerate(sts):
            write_blob("mpt_%s_%d_%d" % (ident, k, j), np.array(t, np.uint16))
            write_blob("mpp_%s_%d_%d" % (ident, k, j), np.array(pl, np.uint8))
            write_blob("mpo_%s_%d_%d" % (ident, k, j), np.array(oc, np.uint16))
            c.append("extern const u16 mpt_%s_%d_%d[]; extern const u8 mpp_%s_%d_%d[]; extern const u16 mpo_%s_%d_%d[];"
                     % (ident, k, j, ident, k, j, ident, k, j))
            sdefs.append("{mpt_%s_%d_%d, mpp_%s_%d_%d, mpo_%s_%d_%d, %s}" % (ident, k, j, ident, k, j, ident, k, j, c_string(nm)))
        c.append("static const PatchState ps_%s_%d[] = {%s};" % (ident, k, ", ".join(sdefs)))
        pstructs.append("{%d, %d, %d, %d, %d, %d, %d, ps_%s_%d}" % (pt["prop"], rx0, ry0, rw, rh, len(sts), pt["kind"], ident, k))
    c.append("static const MapPatch patches_%s[] = {%s};" % (ident, ", ".join(pstructs) or "{0}"))
    patch_of = {pt["prop"]: k for k, (pt, _) in enumerate(pdata)}
    # luces
    lights = []
    for l in data.get("lights", []):
        lights.append((int(l["x"]), int(l["y"]), l))
    prop_light = {}
    for i, p in enumerate(props):
        if "light" in p:
            l = p["light"]
            prop_light[i] = len(lights)
            lights.append((int(p["x"]), int(p["y"]) + int(l.get("oy", 0)), l))
    ldefs = []
    for (x, y, l) in lights:
        col = l.get("color", [1, 1, 1])
        ldefs.append("{%d, %d, %d, %d, %d, %d, %d, %d}" % (x, y, int(l.get("r", 64)), int(col[0] * 255), int(col[1] * 255),
                                                       int(col[2] * 255), int(float(l.get("e", 0.6)) * 100),
                                                       1 if l.get("flicker") else 0))
    c.append("static const MapLight lights_%s[] = {%s};" % (ident, ", ".join(ldefs) or "{0}"))
    pdefs = []
    for i, p in enumerate(props):
        col = p.get("col")
        pdefs.append("{%d, %d, %s, %s, {%d, %d, %d, %d}, %d, %d, %d, %d}" % (
            p["x"], p["y"], c_string(p.get("id")), c_string(p["sprite"]),
            *(col if col else [0, 0, 0, 0]), 1 if col else 0, patch_of.get(i, -1), prop_light.get(i, -1),
            int(p.get("frames", 1))))
    c.append("static const MapProp props_%s[] = {%s};" % (ident, ",\n\t".join(pdefs) or "{0}"))
    c.append("static const Rect16 solids_%s[] = {%s};" % (ident, ", ".join("{%d,%d,%d,%d}" % tuple(s) for s in data["solids"]) or "{0}"))
    c.append("static const MapMarker markers_%s[] = {%s};" % (ident, ", ".join(
        "{%s, %d, %d}" % (c_string(k), v[0], v[1]) for k, v in data["markers"].items()) or "{0}"))
    c.append("static const MapExit exits_%s[] = {%s};" % (ident, ", ".join(
        "{{%d,%d,%d,%d}, %s, %s, %s, %s}" % (*e["rect"], c_string(e["to"]), c_string(e["spawn"]),
                                            c_string(e.get("requires") or None), c_string(e.get("blocked") or None))
        for e in data["exits"]) or "{{0}}"))
    c.append("static const MapTrigger triggers_%s[] = {%s};" % (ident, ", ".join(
        "{{%d,%d,%d,%d}, %s}" % (*t["rect"], c_string(t["id"])) for t in data["triggers"]) or "{{0}}"))
    enc = data.get("encounters")
    forms = []
    if enc:
        for k, f in enumerate(enc["table"]):
            c.append("static const char* const form_%s_%d[] = {%s};" % (ident, k, ", ".join(c_string(e) for e in f)))
            forms.append("{%d, form_%s_%d}" % (len(f), ident, k))
    c.append("static const Formation enc_%s[] = {%s};" % (ident, ", ".join(forms) or "{0}"))
    mod = data.get("modulate")
    c.append("const MapData map_%s = {" % ident)
    c.append("\t%s, %s, %s, %s, %d," % (c_string(mid), c_string(data.get("display", "")), c_string(data.get("music", "")),
                                         c_string(data.get("battle_bg", "forest")), 1 if data.get("interior") else 0))
    c.append("\t%d, %d, %d, %d," % (W, H, cw, ch))
    c.append("\tmg_%s, %d, mgc_%s, mgp_%s, pal_%s," % (ident, len(gbank.tiles), ident, ident, ident))
    c.append("\tmo_%s, %d, moc_%s, mol_%s," % (ident, len(obank.tiles), ident, ident))
    c.append("\tsolids_%s, %d, props_%s, %d, patches_%s, %d," % (ident, len(data["solids"]), ident, len(props), ident, len(pdata)))
    c.append("\tmarkers_%s, %d, exits_%s, %d, triggers_%s, %d, lights_%s, %d," % (
        ident, len(data["markers"]), ident, len(data["exits"]), ident, len(data["triggers"]), ident, len(lights)))
    c.append("\t%d, %d, %d, %d," % ((1, int(mod[0] * 255), int(mod[1] * 255), int(mod[2] * 255)) if mod else (0, 255, 255, 255)))
    c.append("\t%d, enc_%s, %d" % (int(enc["rate"]) if enc else 0, ident, len(forms)))
    c.append("};")
    write_if_changed(os.path.join(OUT, "map_%s.c" % ident), "\n".join(c) + "\n")
    # vista previa (suelo GBA + oclusión de variante completa en rojo tenue)
    prev = np.zeros((ch * 8, cw * 8, 3), np.uint8)
    for i, t in enumerate(gtiles):
        cy, cx = divmod(i, cw)
        prev[cy * 8:cy * 8 + 8, cx * 8:cx * 8 + 8] = decode_tile(gbank.tiles[t], pal16[gpal[i] * 16:gpal[i] * 16 + 16])[..., :3]
    Image.fromarray(prev).save(os.path.join(PREVIEW, "map_%s.png" % ident))
    print("     suelo %5d tiles, oclusión %5d tiles (%d celdas), %d parches" % (
        len(gbank.tiles), len(obank.tiles), len(ov_default), len(pdata)), flush=True)
    return ident


def main(ids):
    allids = sorted(f[:-5] for f in os.listdir(os.path.join(GAME, "maps")) if f.endswith(".json"))
    todo = ids or allids
    for mid in todo:
        src = [os.path.join(GAME, "maps", mid + ".json"), os.path.join(GAME, "maps", mid + ".png"), __file__,
               os.path.join(HERE, "lib.py")]
        d = json.load(open(src[0]))
        for p in d["props"]:
            src.append(os.path.join(GAME, "sprites", p["sprite"] + ".png"))
        h = file_hash(src)
        stamp = os.path.join(CACHE, "map_%s.hash" % mid)
        if os.path.exists(stamp) and open(stamp).read() == h and os.path.exists(os.path.join(OUT, "map_%s.c" % c_ident(mid))):
            continue
        convert(mid)
        open(stamp, "w").write(h)
    # índice
    lines = ['#include "../../src/mapdata.h"']
    for mid in allids:
        lines.append("extern const MapData map_%s;" % c_ident(mid))
    lines.append("const MapData* const g_maps[] = {%s};" % ", ".join("&map_%s" % c_ident(m) for m in allids))
    lines.append("const int g_nmaps = %d;" % len(allids))
    write_if_changed(os.path.join(OUT, "maps_index.c"), "\n".join(lines) + "\n")
    blobs_asm()


if __name__ == "__main__":
    main(sys.argv[1:])
