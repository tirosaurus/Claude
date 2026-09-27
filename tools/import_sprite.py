"""Convierte sprites generados con IA (PixelLab, Retro Diffusion, etc.) o dibujados a mano
al formato exacto de Vaelmoor y los deja en game/assets/custom/, donde tienen prioridad
sobre los sprites generados por código.

Ejemplos
--------
# Hoja de mapa: la imagen de origen tiene 4 filas (abajo, izquierda, derecha, arriba) y N columnas
python3 tools/import_sprite.py map kaelen origen.png --grid 4x4
# Si solo tienes la fila "derecha", la izquierda se crea volteándola:
python3 tools/import_sprite.py map kaelen origen.png --grid 4x3 --rows down,right,up

# Hoja de combate (una fila de poses; indica qué columna es cada pose)
python3 tools/import_sprite.py battle kaelen poses.png --grid 6x1 --poses idle1=0,idle2=1,windup=2,strike=3,cast=4,hurt=5

# Enemigo (2 frames de animación, altura máxima en píxeles)
python3 tools/import_sprite.py enemy brute bruto.png --grid 2x1 --height 84

# Retrato 48x48 y protagonista (por raza y sexo)
python3 tools/import_sprite.py portrait yara cara.png
python3 tools/import_sprite.py map player:human_m heroe.png --grid 4x4

Opciones comunes: --colors N (paleta, por defecto 24), --no-outline, --bg R,G,B (color de fondo a quitar).
Después abre el proyecto en Godot (importa los PNG nuevos) y vuelve a exportar.
"""
import argparse
import os
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
CUSTOM = os.path.join(HERE, "..", "game", "assets", "custom")
FW, FH = 20, 30          # frame de mapa
BW, BH = 32, 32          # frame de combate
POSES = ["idle1", "idle2", "windup", "strike", "cast", "hurt", "kneel", "ko", "victory"]
OUTLINE = (30, 24, 34, 255)


def remove_bg(im, bg=None, tol=24):
    im = im.convert("RGBA")
    px = im.load()
    if bg is None:
        c = px[0, 0]
        if c[3] < 10:
            return im
        bg = c[:3]
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if abs(r - bg[0]) + abs(g - bg[1]) + abs(b - bg[2]) <= tol:
                px[x, y] = (0, 0, 0, 0)
    return im


def cells(im, cols, rows):
    cw, ch = im.width // cols, im.height // rows
    return [[im.crop((c * cw, r * ch, (c + 1) * cw, (r + 1) * ch)) for c in range(cols)] for r in range(rows)]


def fit(frame, w, h, colors, outline, anchor="bottom", scale_ref=None):
    """Recorta al contenido, escala para caber en w x h (sin suavizar al final) y cuantiza."""
    bbox = frame.getbbox()
    if not bbox:
        return Image.new("RGBA", (w, h), (0, 0, 0, 0))
    art = frame.crop(bbox)
    pad = 1 if outline else 0
    s = scale_ref or min((w - 2 * pad) / art.width, (h - 2 * pad) / art.height)
    nw, nh = max(1, round(art.width * s)), max(1, round(art.height * s))
    # reducción con promedio (BOX) y alfa binario: aspecto de pixel art limpio
    small = art.resize((nw, nh), Image.BOX)
    a = small.getchannel("A").point(lambda v: 255 if v > 110 else 0)
    rgb = small.convert("RGB").quantize(colors=colors, method=Image.MEDIANCUT, dither=Image.NONE).convert("RGB")
    small = Image.merge("RGBA", (*rgb.split(), a))
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    x = (w - nw) // 2
    y = h - nh - pad if anchor == "bottom" else (h - nh) // 2
    out.alpha_composite(small, (x, y))
    if outline:
        out = add_outline(out)
    return out


def add_outline(im):
    px = im.load()
    src = im.copy().load()
    for y in range(im.height):
        for x in range(im.width):
            if src[x, y][3]:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < im.width and 0 <= ny < im.height and src[nx, ny][3]:
                    r, g, b, _ = src[nx, ny]
                    px[x, y] = (int(r * 0.35 + 20), int(g * 0.3 + 16), int(b * 0.35 + 22), 255)
                    break
    return im


def common_scale(frames, w, h, pad):
    boxes = [f.getbbox() for f in frames if f.getbbox()]
    mw = max(b[2] - b[0] for b in boxes)
    mh = max(b[3] - b[1] for b in boxes)
    return min((w - 2 * pad) / mw, (h - 2 * pad) / mh)


def target_path(kind, name):
    if name.startswith("player:"):
        rs = name.split(":")[1]
        return os.path.join(CUSTOM, "player", f"{kind}_{rs}.png")
    sub = {"map": "chars", "battle": "battle", "enemy": "enemies", "portrait": "portraits"}[kind]
    return os.path.join(CUSTOM, sub, f"{name}.png")


def build_map(src, args):
    cols, rows = args.grid
    grid = cells(src, cols, rows)
    order = args.rows.split(",")
    by_dir = {d: grid[i] for i, d in enumerate(order)}
    if "left" not in by_dir and "right" in by_dir:
        by_dir["left"] = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in by_dir["right"]]
    if "right" not in by_dir and "left" in by_dir:
        by_dir["right"] = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in by_dir["left"]]
    allf = [f for d in by_dir.values() for f in d]
    sc = common_scale(allf, FW, FH, 0 if args.no_outline else 1)
    sheet = Image.new("RGBA", (FW * 8, FH * 4), (0, 0, 0, 0))
    for r, d in enumerate(["down", "left", "right", "up"]):
        fr = [fit(f, FW, FH, args.colors, not args.no_outline, scale_ref=sc) for f in by_dir[d]]
        idle = fr[0]
        breath = Image.new("RGBA", (FW, FH), (0, 0, 0, 0))
        breath.alpha_composite(idle.crop((0, 0, FW, FH - 8)), (0, 1))
        breath.alpha_composite(idle.crop((0, FH - 8, FW, FH)), (0, FH - 8))
        walk_src = fr[1:] if len(fr) > 1 else fr
        walk = [walk_src[int(i * len(walk_src) / 6) % len(walk_src)] for i in range(6)]
        for c, f in enumerate([idle, breath] + walk):
            sheet.alpha_composite(f, (c * FW, r * FH))
    return sheet


def build_battle(src, args):
    cols, rows = args.grid
    frames = [f for row in cells(src, cols, rows) for f in row]
    mapping = dict(p.split("=") for p in args.poses.split(",")) if args.poses else {}
    sc = common_scale(frames, BW, BH, 0 if args.no_outline else 1)
    fitted = [fit(f, BW, BH, args.colors, not args.no_outline, scale_ref=sc) for f in frames]
    if args.face == "right":
        fitted = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in fitted]
    sheet = Image.new("RGBA", (BW * len(POSES), BH), (0, 0, 0, 0))
    idle = fitted[int(mapping.get("idle1", 0))]
    for i, pose in enumerate(POSES):
        if pose in mapping:
            f = fitted[int(mapping[pose])]
        elif pose == "ko":
            f = idle.rotate(90)
            f2 = Image.new("RGBA", (BW, BH), (0, 0, 0, 0))
            f2.paste(f.crop((0, 0, BW, BH - 5)), (0, 5))
            f = f2
        elif pose == "kneel":
            f = Image.new("RGBA", (BW, BH), (0, 0, 0, 0))
            f.alpha_composite(idle.crop((0, 0, BW, BH - 3)), (0, 3))
        else:
            f = idle
        sheet.alpha_composite(f, (i * BW, 0))
    return sheet


def build_enemy(src, args):
    cols, rows = args.grid
    frames = [f for row in cells(src, cols, rows) for f in row][:2]
    if len(frames) == 1:
        frames.append(frames[0])
    boxes = [f.getbbox() for f in frames]
    mw = max(b[2] - b[0] for b in boxes)
    mh = max(b[3] - b[1] for b in boxes)
    h = args.height
    w = max(8, round(mw * h / mh)) + 2
    sc = (h - 2) / mh
    fitted = [fit(f, w, h, args.colors, not args.no_outline, scale_ref=sc) for f in frames]
    if args.face == "left":
        fitted = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in fitted]
    sheet = Image.new("RGBA", (w * 2, h), (0, 0, 0, 0))
    for i, f in enumerate(fitted):
        sheet.alpha_composite(f, (i * w, 0))
    return sheet


def build_portrait(src, args):
    return fit(src, 48, 48, args.colors, not args.no_outline, anchor="center")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("kind", choices=["map", "battle", "enemy", "portrait"])
    ap.add_argument("name", help="id del personaje/enemigo (kaelen, yara, brute...) o player:human_m")
    ap.add_argument("source")
    ap.add_argument("--grid", default="1x1", help="COLUMNASxFILAS de la imagen de origen")
    ap.add_argument("--rows", default="down,left,right,up", help="orden de las filas (mapa)")
    ap.add_argument("--poses", default="", help="pose=columna,... (combate)")
    ap.add_argument("--face", default=None, help="hacia dónde mira el origen (por defecto: combate left, enemigo right)")
    ap.add_argument("--height", type=int, default=64, help="altura del enemigo en píxeles")
    ap.add_argument("--colors", type=int, default=24)
    ap.add_argument("--no-outline", action="store_true")
    ap.add_argument("--bg", default=None, help="R,G,B del fondo a eliminar (por defecto, el píxel de la esquina)")
    args = ap.parse_args()
    c, r = args.grid.lower().split("x")
    args.grid = (int(c), int(r))
    bg = tuple(int(v) for v in args.bg.split(",")) if args.bg else None
    src = remove_bg(Image.open(args.source), bg)
    if args.kind == "enemy" and args.face == "left":
        pass
    out = {"map": build_map, "battle": build_battle, "enemy": build_enemy, "portrait": build_portrait}[args.kind](src, args)
    path = target_path(args.kind, args.name)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    out.save(path)
    print("guardado:", os.path.relpath(path, os.path.join(HERE, "..")), out.size)


if __name__ == "__main__":
    main()
