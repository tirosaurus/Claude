"""Icono del juego: escudo con el Árbol del Pacto (pixel art 32x32)."""
from PIL import Image
from artlib import Canvas, OUT, mix


def build_icon(out_dir):
    c = Canvas(32, 32)
    RIM, RIM_L, RIM_D = (200, 206, 220), (244, 246, 252), (120, 126, 146)
    FIELD_T, FIELD_B = (44, 70, 140), (20, 28, 72)
    # escudo (forma de cometa redondeada)
    def inside(x, y):
        if y < 3 or y > 29:
            return False
        if y <= 18:
            return 4 <= x <= 27
        half = 12 - (y - 18) * 12 / 11.5
        return abs(x + 0.5 - 16) <= half
    for y in range(32):
        for x in range(32):
            if inside(x, y):
                edge = not (inside(x - 1, y) and inside(x + 1, y) and inside(x, y - 1) and inside(x, y + 1))
                edge2 = not (inside(x - 2, y) and inside(x + 2, y) and inside(x, y - 2) and inside(x, y + 2))
                if edge:
                    col = RIM_D if (x > 16 or y > 22) else RIM
                elif edge2:
                    col = RIM_L if (x < 16 and y < 16) else RIM
                else:
                    col = mix(FIELD_T, FIELD_B, (y - 3) / 26)
                c.px(x, y, col)
    # árbol: copa ancha y redonda, tronco corto con raíces
    GOLD, GOLD_L, GOLD_D = (232, 190, 84), (255, 232, 150), (170, 120, 46)
    for (cx, cy, r) in ((16, 11, 6.2), (11.5, 13, 4.2), (20.5, 13, 4.2), (16, 8, 4.5)):
        c.ellipse(cx, cy, r, r * 0.85, GOLD)
    c.ellipse(14, 9, 2.5, 2, GOLD_L)
    c.ellipse(19, 15, 3, 2, GOLD_D)
    c.rect(15, 16, 3, 6, GOLD_D)
    c.px(15, 16, GOLD)
    for (x, y) in ((13, 22), (14, 21), (18, 21), (19, 22), (12, 23), (20, 23), (16, 22)):
        c.px(x, y, GOLD_D)
    # la Semilla: una gema verde en el centro de la copa
    c.rect(15, 11, 2, 2, (110, 230, 150))
    c.px(15, 11, (220, 255, 230))
    c.outline(OUT)
    big = c.im.resize((256, 256), Image.NEAREST)
    big.save(f"{out_dir}/icon.png")
    c.im.resize((64, 64), Image.NEAREST).save(f"{out_dir}/icon_64.png")
    big.save(f"{out_dir}/icon.ico", sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    return big


if __name__ == "__main__":
    build_icon("../game")
