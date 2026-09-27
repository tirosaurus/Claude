"""Personajes v3: rig por piezas dibujadas a mano (plantillas ASCII).

Hoja de mapa: 3 frames x 4 direcciones, frames de 20x30.
Hoja de combate: 8 poses mirando a la izquierda, frames de 32x32
(se escalan x2 en el combate): idle1, idle2, ataque1, ataque2, magia,
herido, agotado, KO.  Además, retratos de 48x48.

Las capas del creador se generan en grises (piel/pelo) y color marcador
(ropa) para teñirse en el juego.
"""
from artlib import Canvas, OUT, darken, lighten, mix

FW, FH = 20, 30
BW, BH = 32, 32

# ------------------------------------------------------------------ Paletas
G_SKIN_L, G_SKIN, G_BLUSH, G_SKIN_S = (228,) * 3, (200,) * 3, (184,) * 3, (160,) * 3
G_HAIR_L, G_HAIR, G_HAIR_D = (236,) * 3, (170,) * 3, (110,) * 3
M_TOP, M_TOP_D = (255, 0, 255), (170, 0, 170)

EYE_D = (36, 26, 40)
EYE_I = (78, 86, 132)
EYE_W = (250, 248, 240)
MOUTH = (150, 76, 76)


def base_pal():
    return {
        "S": G_SKIN_L, "s": G_SKIN, "r": G_BLUSH, "k": G_SKIN_S,
        "H": G_HAIR_L, "h": G_HAIR, "d": G_HAIR_D,
        "T": M_TOP, "t": M_TOP_D, "U": (255, 120, 255),   # U = luz de la ropa (se tiñe como T aclarado)
        "e": EYE_D, "i": EYE_I, "w": EYE_W, "m": MOUTH,
        "P": (92, 74, 62), "p": (66, 52, 46),             # pantalón
        "B": (84, 56, 40), "b": (58, 38, 30),             # botas
        "L": (112, 74, 48), "G": (226, 184, 82),          # cinturón, hebilla
        "C": (236, 226, 204), "c": (196, 184, 160),       # camisa/cuello claro
        "o": OUT,
    }


def P(text):
    rows = [r for r in text.strip("\n").split("\n")]
    rows = [r.strip() for r in rows]
    w = max(len(r) for r in rows)
    return [r.ljust(w, ".") for r in rows]


def stamp(c, tpl, ox, oy, pal, flip=False):
    w = len(tpl[0])
    for y, row in enumerate(tpl):
        for x, ch in enumerate(row):
            if ch == "." or ch == " ":
                continue
            if ch == "_":           # borrar
                xx = ox + (w - 1 - x if flip else x)
                if 0 <= xx < c.w and 0 <= oy + y < c.h:
                    c.p[xx, oy + y] = (0, 0, 0, 0)
                continue
            col = pal.get(ch)
            if col is None:
                continue
            xx = ox + (w - 1 - x if flip else x)
            c.px(xx, oy + y, col)


# =====================================================================================
# CABEZAS  (12 x 12)   origen relativo a la hoja: x=4, y=head_y
# =====================================================================================
HEAD = {}
HEAD["down"] = P("""
...ssssss...
..ssssssss..
.ssssssssss.
.ssssssssss.
ssssssssssss
ssssssssssss
sseesssseess
sswissssswis
sriissssiirs
.ssssskssss.
.kssssmsssk.
..kkkkkkkk..
""")
HEAD["down_f"] = P("""
...ssssss...
..ssssssss..
.ssssssssss.
.ssssssssss.
ssssssssssss
ssssssssssss
seeesssseees
sswissssswis
rriissssiirr
.ssssskssss.
..ksssmssk..
...kkkkkk...
""")
HEAD["side"] = P("""
...sssss....
..sssssssss.
.ssssssssss.
.ssssssssss.
sssssssssss.
sssssssssss.
ssssssssees.
sssssssswiss
ssssssrriiss
.sssssssssk.
..kssssssm..
...kkkkkk...
""")
HEAD["side_f"] = P("""
...sssss....
..sssssssss.
.ssssssssss.
.ssssssssss.
sssssssssss.
ssssssssseee
ssssssssees.
sssssssswiss
sssssrrriiss
.sssssssssk.
..kssssssm..
...kkkkkk...
""")
HEAD["up"] = P("""
...ssssss...
..ssssssss..
.ssssssssss.
.ssssssssss.
ssssssssssss
ssssssssssss
ssssssssssss
ssssssssssss
ssssssssssss
.ssssssssss.
.kssssssssk.
..kkkkkkkk..
""")

# orejas / nariz por raza  (se estampan sobre la cabeza)
EARS = {
    ("human", "down"): (P("""
s..........s
k..........k
"""), -1, 6),
    ("human", "side"): (P("""
ss
sk
"""), 3, 6),
    ("elf", "down"): (P("""
s..............s
ss............ss
.ss..........ss.
.ks..........sk.
"""), -3, 4),
    ("elf", "side"): (P("""
s...
ss..
.ss.
.ksk
"""), 0, 4),
    ("dwarf", "down"): (P("""
s..........s
k..........k
"""), -1, 6),
    ("dwarf", "side"): (P("""
ss
sk
"""), 3, 6),
}
# nariz de enano
DWARF_NOSE = {
    "down": (P("""
.ss.
skks
"""), 4, 8),
    "side": (P("""
ss
sk
"""), 11, 7),
}

# =====================================================================================
# PELO  cada estilo: capas 'back' (detrás de todo), 'front' (sobre la cabeza)
#   coordenadas relativas a la cabeza (0,0 = esquina sup. izq. de la cabeza 12x12)
# =====================================================================================
HAIR = {}


def hair(style, direction, front, back=None, fx=0, fy=0, bx=0, by=0):
    HAIR[(style, direction)] = {"front": (P(front), fx, fy), "back": (P(back), bx, by) if back else None}


# --- corto --------------------------------------------------------------------
hair("short", "down", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhdhhdhhhhdhhh
hd.dd.dhhd.dhh
h..........dh.
h...........h.
""", fx=-1, fy=-2)
hair("short", "side", """
...hhhhhh....
.hhHHHhhhhh..
hhHHhhhhhhhh.
hhhhhhhhhhhh.
hhhhhhhhhhhhh
hhhhhhhhdhdh.
hhhhhhhd.d...
hhhhhd.......
hhhhd........
.hhd.........
""", fx=-1, fy=-2)
hair("short", "up", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
.hhhhhhhhhhhh.
.dhdhhhhhhdhd.
..d.d.dd.d.d..
""", fx=-1, fy=-2)

# --- de punta ---------------------------------------------------------------------
hair("spiky", "down", """
..h...h..h...h..
..hh.hh.hhh.hh..
...hhHHhhhhhh...
..hhHHhhhhhhhh..
..hhhhhhhhhhhh..
.hhhhhhhhhhhhhh.
.hhdhhhdhhhdhhh.
.hd.hd.d.hd.dhh.
.h.........dh...
.h...........h..
""", fx=-2, fy=-4)
hair("spiky", "side", """
...h..h..h.....
...hh.hh.hh.h..
..hhhHHhhhhhh..
.hhhHHhhhhhhhh.
hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh
hhhhhhhhhdhdh..
.hhhhhhhd.d....
hhhhhhd........
.hhhhd.........
..hhd..........
""", fx=-2, fy=-4)
hair("spiky", "up", """
..h...h..h...h..
..hh.hh.hhh.hh..
...hhHHhhhhhh...
..hhHHhhhhhhhh..
..hhhhhhhhhhhh..
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
hhhhhhhhhhhhhhhh
.hhhhhhhhhhhhhh.
..hhhhhhhhhhhh..
..hhhhhhhhhhhh..
..hdhhdhhdhhdh..
...d..d..d..d...
""", fx=-2, fy=-4)

# --- largo ----------------------------------------------------------------------
hair("long", "down", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhdhhhhdhhhh
hhd.hd..dh.dhh
hh.........hhh
hh..........hh
hh..........hh
hd..........dh
hd..........dh
""", back="""
..hhhhhhhhhhhh..
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.hhhhhhhhhhhhhh.
.dhhhhhhhhhhhhd.
.dhhhhhhhhhhhhd.
.dhhhhhhhhhhhhd.
..dhhhhhhhhhhd..
..dhhhhhhhhhhd..
...dd......dd...
""", fx=-1, fy=-2, bx=-2, by=1)
hair("long", "side", """
...hhhhhh....
.hhHHHhhhhh..
hhHHhhhhhhhh.
hhhhhhhhhhhh.
hhhhhhhhhhhhh
hhhhhhhhdhdh.
hhhhhhhd.d...
hhhhhd.......
hhhhh........
hhhhh........
hhhhh........
hhhhd........
hhhhd........
hhhd.........
.hdd.........
""", fx=-1, fy=-2)
hair("long", "up", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
dhhhhhdhhhhhhd
.dhhhhdhhhhhd.
..ddd.d..ddd..
""", fx=-1, fy=-2)

# --- coleta ---------------------------------------------------------------------
hair("ponytail", "down", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhdhhhhhhdhhh
hd.hd....dh.dh
h...........dh
h............h
""", fx=-1, fy=-2)
hair("ponytail", "side", """
.....hhhhhh....
...hhHHHhhhhh..
..hhHHhhhhhhhh.
..hhhhhhhhhhhh.
.GGhhhhhhhhhhhh
hhGhhhhhhhdhdh.
hhhhhhhhhd.d...
hhh.hhhd.......
hhh.hhd........
hhd..d.........
hhd............
.hd............
.dd............
""", fx=-3, fy=-2)
hair("ponytail", "up", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhGGGhhhhhh
hhhhhhhhhhhhhh
.hhhhhhhhhhhh.
.dhhhhhhhhhhd.
..ddd.hhd.dd..
......hhd.....
......hhd.....
......hhd.....
......hdd.....
.......d......
""", fx=-1, fy=-2)

# --- moño -----------------------------------------------------------------------
hair("bun", "down", """
.....hhhh.....
....hHHhhh....
....hhhhdh....
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhdhhhhhhdhhh
hd.hd....dh.dh
h............h
h............h
""", fx=-1, fy=-5)
hair("bun", "side", """
.hhhh..........
hHHhhh.........
hhhhdh.hhhh....
hhhhhhhHHhhhh..
.hhhhHHhhhhhhh.
..hhhhhhhhhhhh.
..hhhhhhhhhhhhh
..hhhhhhhhdhdh.
..hhhhhhhd.d...
..hhhhhd.......
..hhhhd........
...hhd.........
""", fx=-3, fy=-5)
hair("bun", "up", """
.....hhhh.....
....hHHhhh....
....hhhhdh....
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
.hhhhhhhhhhhh.
.dhdhhhhhhdhd.
..d.d.dd.d.d..
""", fx=-1, fy=-5)

# --- trenzas ----------------------------------------------------------------------
hair("braids", "down", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhdhhhdhhdhhhh
hd.hd.....d.dh
hh..........hh
hh..........hh
dh..........hd
hd..........dh
dh..........hd
hd..........dh
GG..........GG
hd..........dh
.d..........d.
""", fx=-1, fy=-2)
hair("braids", "side", """
...hhhhhh....
.hhHHHhhhhh..
hhHHhhhhhhhh.
hhhhhhhhhhhh.
hhhhhhhhhhhhh
hhhhhhhhdhdh.
hhhhhhhd.d...
hhhhhd.......
hhhd.........
.hd..........
.dh..........
.hd..........
.dh..........
.GG..........
.hd..........
..d..........
""", fx=-1, fy=-2)
hair("braids", "up", """
....hhhhhh....
..hhHHhhhhhh..
.hhHHhhhhhhhh.
.hhhhhhhhhhhh.
hhhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
hhhhhhdhhhhhhh
.hhhhhdhhhhhh.
.dhhhd.dhhhhd.
.hd........dh.
.dh........hd.
.hd........dh.
.GG........GG.
.hd........dh.
..d........d..
""", fx=-1, fy=-2)

# --- rapado ---------------------------------------------------------------------
hair("shaved", "down", """
...dddddd...
..dhdhdhdd..
.dddhdddhdd.
.d.d.d.d.dd.
""", fx=0, fy=0)
hair("shaved", "side", """
...ddddd....
..dhdhdhdd..
.dddhdddhdd.
.dd.d.d.d...
dd.d........
d...........
""", fx=0, fy=0)
hair("shaved", "up", """
...dddddd...
..dhdhdhdd..
.dddhdddhdd.
.dhdhdhdhdd.
dddhdhdhdddd
.dhdddhdhdd.
..d.d.d.d.d.
""", fx=0, fy=0)

# --- estilos de PNJ -------------------------------------------------------------------
hair("swept", "down", """
....hhhhhhh...
..hhHHhhhhhhh.
.hhHHhhhhhhhhh
.hhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhdhhh
hhhhhhhhd..dhh
hhhhd.d.....dh
hhd.........h.
hd...........
""", fx=-1, fy=-2)
hair("swept", "side", """
...hhhhhhh...
.hhHHHhhhhhh.
hhHHhhhhhhhhh
hhhhhhhhhhhhh
hhhhhhhhhhhhhh
hhhhhhhhhhhhh.
hhhhhhhhhdd...
hhhhhd.hd.....
hhhhd.........
.hhd..........
""", fx=-1, fy=-2)
hair("swept", "up", HAIR[("short", "up")]["front"][0] and "\n".join(HAIR[("short", "up")]["front"][0]), fx=-1, fy=-2)
hair("bald", "down", """
............
............
............
............
h..........h
hh........hh
hd........dh
""", fx=0, fy=0)
hair("bald", "side", """
............
............
............
............
hh..........
hhh.........
hhd.........
.d..........
""", fx=0, fy=0)
hair("bald", "up", """
............
............
............
............
h..........h
hhhhhhhhhhhh
hhhhhhhhhhhh
dhhhhhhhhhhd
.dddhhhhddd.
""", fx=0, fy=0)
hair("scarf", "down", """
....FFFFFF....
..FFFFFFFFFF..
.FFFFFFFFFFFF.
.FFFFFFFFFFFF.
FFFFFFFFFFFFFF
ffffffffffffff
fhhd......dhhf
fh..........hf
f............f
f............f
.f..........f.
""", fx=-1, fy=-2)
hair("scarf", "side", """
...FFFFFF....
.FFFFFFFFFF..
FFFFFFFFFFFF.
FFFFFFFFFFFF.
FFFFFFFFFFFFF
fffffffffffff
ffffffhdd....
fffff........
ffff.........
.fff.........
..ff.........
""", fx=-1, fy=-2)
hair("scarf", "up", """
....FFFFFF....
..FFFFFFFFFF..
.FFFFFFFFFFFF.
.FFFFFFFFFFFF.
FFFFFFFFFFFFFF
FFFFFFFFFFFFFF
FFFFFFFFFFFFFF
FFFFFFFFFFFFFF
FFFFFFFFFFFFFF
ffffffffffffff
.ffffffffffff.
..ffffffffff..
....ffffff....
""", fx=-1, fy=-2)
hair("hood", "down", """
.....QQQQ.....
...QQQQQQQQ...
..QQQQQQQQQQ..
.QQQQQQQQQQQQ.
.QQQqqqqqqQQQ.
QQQq......qQQQ
QQq........qQQ
QQq........qQQ
QQq........qQQ
QQq........qQQ
QQQ........QQQ
QQQQ......QQQQ
qQQQQ....QQQQq
qqQQQ....QQQqq
""", fx=-1, fy=-2)
hair("hood", "side", """
..QQQQQQ.....
.QQQQQQQQQ...
QQQQQQQQQQQ..
QQQQQQQQQQQQ.
QQQQQQQQqqqq.
QQQQQQQq.....
QQQQQQQq.....
QQQQQQQq.....
QQQQQQQq.....
QQQQQQQq.....
QQQQQQQQ.....
QQQQQQQQQ....
qQQQQQQQQ....
qqQQQQQQQ....
""", fx=-1, fy=-2)
hair("hood", "up", """
.....QQQQ.....
...QQQQQQQQ...
..QQQQQQQQQQ..
.QQQQQQQQQQQQ.
.QQQQQQQQQQQQ.
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
QQQQQQQQQQQQQQ
qQQQQQQQQQQQQq
qqQQQQQQQQQQqq
""", fx=-1, fy=-2)

# barbas (relativas a la cabeza)
BEARD = {
    "down": (P("""
h..........h
hh........hh
hhhhdmmdhhhh
.hhhhhhhhhh.
.hhhhhhhhhh.
..hhhhhhhh..
...dhhhhd...
....dhhd....
"""), 0, 6),
    "side": (P("""
......hh
.....hhh
....hhmh
...hhhhh
...hhhhh
....hhhd
.....hd.
"""), 4, 6),
}
BEARD_DWARF = {
    "down": (P("""
h..........h
hh........hh
hhhhdmmdhhhh
hhhhhhhhhhhh
hhhhhhhhhhhh
.hhhhhhhhhh.
.hhhdhhdhhh.
..hhhhhhhh..
..dhhddhhd..
...dh..hd...
"""), 0, 6),
    "side": (P("""
......hhh
.....hhhh
....hhhmh
...hhhhhh
...hhhhhh
...hhhhhh
....hhhhd
....hhdd.
.....d...
"""), 3, 6),
}

# =====================================================================================
# CUERPOS  (torso + brazos) y PIERNAS
# =====================================================================================
# Torso "down" 14 ancho, origen x=3
TORSO = {}
TORSO[("m", "down")] = P("""
...CccccC.....
..TTUTTTTUTT..
.tTUTTTtTTTtt.
.tTUTTTtTTTtt.
.tTTTTTtTTTtt.
.tTTTTTTTTttt.
..LLLLGLLLLL..
..TTTTtTTTtt..
""")
TORSO[("f", "down")] = P("""
....CccC......
...TUTTTUT....
..tUTTtTTtt...
..tUTTtTTtt...
...TTTTTtt....
...LLLGLLL....
..TTTTtTTtt...
.tTTTTtTTTtt..
""")
TORSO[("m", "side")] = P("""
.....Ccc.....
....TTTTTT...
...tTTTTTTU..
...tTTTTTTU..
...tTTTTTTT..
...tTTTTTTT..
...LLLLLLLL..
...tTTTTTTT..
""")
TORSO[("f", "side")] = P("""
.....Cc......
.....TTTTT...
....tTTTTU...
....tTTTTU...
.....TTTT....
.....LLLL....
....tTTTTTT..
...tTTTTTTTT.
""")
TORSO[("m", "up")] = P("""
...CCCCCC.....
..TTTTTTTTTT..
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
..LLLLLLLLLL..
..tTTTTTTTTt..
""")
TORSO[("f", "up")] = P("""
....CCCC......
...TTTTTTT....
..tTTTTTTTt...
..tTTTTTTTt...
...TTTTTTT....
...LLLLLLL....
..tTTTTTTTt...
.tTTTTTTTTTt..
""")
# enano: más ancho y corto
TORSO[("dm", "down")] = P("""
...CccccccC....
.TTUTTTTTTTUTT.
tTUTTTTTTTTTTTt
tTUTTTTTTTTTTTt
tTTTTTTTTTTTTTt
.LLLLLLGLLLLLL.
""")
TORSO[("df", "down")] = P("""
....CccccC.....
..TUTTTTTUTT...
.tUTTTTTTTTTt..
.tTTTTTTTTTTt..
..LLLLGLLLLL...
.tTTTTTTTTTTTt.
""")
TORSO[("dm", "side")] = P("""
.....Cccc.....
...TTTTTTTT...
..tTTTTTTTTU..
..tTTTTTTTTU..
..tTTTTTTTTT..
..LLLLLLLLLL..
""")
TORSO[("df", "side")] = P("""
.....Ccc......
....TTTTTTT...
...tTTTTTTU...
...tTTTTTTT...
....LLLLLL....
...tTTTTTTTT..
""")
TORSO[("dm", "up")] = P("""
...CCCCCCCC....
.TTTTTTTTTTTTT.
tTTTTTTTTTTTTTt
tTTTTTTTTTTTTTt
tTTTTTTTTTTTTTt
.LLLLLLLLLLLLL.
""")
TORSO[("df", "up")] = P("""
....CCCCCC.....
..TTTTTTTTTT...
.tTTTTTTTTTTt..
.tTTTTTTTTTTt..
..LLLLLLLLLL...
.tTTTTTTTTTTTt.
""")

# brazos: (plantilla, dx relativo al torso)
ARM_DOWN = P("""
t
t
t
t
s
""")
ARM_SIDE = P("""
ot
ot
ot
os
.s
""")

# PIERNAS  (ancho 14, origen x=3), 3 frames: quieto, paso A, paso B
LEGS = {}
LEGS[("m", "down")] = [P("""
...PPPpPPP....
...PPp.pPP....
...PPp.PPp....
...PPp.PPp....
...BBb.BBb....
..BBBb.BBBb...
"""), P("""
...PPPpPPP....
...PPp.pPP....
...PPp.PPp....
...BBb.PPp....
..BBBb.BBb....
.......BBBb...
"""), P("""
...PPPpPPP....
...PPp.pPP....
...PPp.PPp....
...PPp.BBb....
...BBb.BBBb...
..BBBb........
""")]
LEGS[("m", "side")] = [P("""
....PPPPp.....
....PPPPp.....
....PPpPp.....
....PPpPp.....
....BBbBb.....
....BBBBBb....
"""), P("""
....PPPPp.....
...PPp.PPp....
..PPp...PPp...
..BBb...PPp...
.BBb.....BBb..
.........BBBb.
"""), P("""
....PPPPp.....
....PPpPp.....
...PPp.PP.....
...PBb.BB.....
...BBb.BBb....
..BBb..BBBb...
""")]
LEGS[("m", "up")] = [P("""
...pPPPPPp....
...PPp.pPP....
...PPp.PPp....
...PPp.PPp....
...BBb.BBb....
...BBb.BBb....
"""), P("""
...pPPPPPp....
...PPp.pPP....
...PPp.PPp....
...BBb.PPp....
...BBb.BBb....
.......BBb....
"""), P("""
...pPPPPPp....
...PPp.pPP....
...PPp.PPp....
...PPp.BBb....
...BBb.BBb....
...BBb........
""")]
# falda / vestido (femenino con túnica larga ya en torso; piernas más finas)
LEGS[("f", "down")] = [P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...PPp.PPp....
...BBb.BBb....
...BBb.BBb....
"""), P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...BBb.PPp....
...BBb.BBb....
.......BBb....
"""), P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...PPp.BBb....
...BBb.BBb....
...BBb........
""")]
LEGS[("f", "side")] = [P("""
...tTTTTTTT...
.....PPp......
.....PPp......
.....PPp......
.....BBb......
.....BBBb.....
"""), P("""
...tTTTTTTT...
....PPp.Pp....
...PPp..PPp...
...BBb...Pp...
..BBb....BBb..
.........BBBb.
"""), P("""
...tTTTTTTT...
.....PPp......
....PPpPp.....
....BBbBB.....
....BBbBBb....
...BBb.BBBb...
""")]
LEGS[("f", "up")] = [P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...PPp.PPp....
...BBb.BBb....
...BBb.BBb....
"""), P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...BBb.PPp....
...BBb.BBb....
.......BBb....
"""), P("""
..tTTTTTTTTt..
...PPp.PPp....
...PPp.PPp....
...PPp.BBb....
...BBb.BBb....
...BBb........
""")]
# enanos: piernas cortas y anchas (4 filas)
LEGS[("d", "down")] = [P("""
..PPPPpPPPP....
..PPPp.PPPp....
..BBBb.BBBb....
.BBBBb.BBBBb...
"""), P("""
..PPPPpPPPP....
..PPPp.PPPp....
..BBBb.PPPp....
.BBBBb.BBBBb...
"""), P("""
..PPPPpPPPP....
..PPPp.PPPp....
..PPPp.BBBb....
.BBBBb.BBBBb...
""")]
LEGS[("d", "side")] = [P("""
....PPPPPp....
....PPPpPp....
....BBBbBb....
....BBBBBBb...
"""), P("""
...PPPp.PPp...
..PPPp...PPp..
..BBBb...BBb..
.BBBb.....BBBb
"""), P("""
....PPPpPp....
....PPpPPp....
....BBbBBb....
...BBBbBBBb...
""")]
LEGS[("d", "up")] = [P("""
..pPPPPPPPp....
..PPPp.PPPp....
..BBBb.BBBb....
..BBBb.BBBb....
"""), P("""
..pPPPPPPPp....
..PPPp.PPPp....
..BBBb.PPPp....
..BBBb.BBBb....
"""), P("""
..pPPPPPPPp....
..PPPp.PPPp....
..PPPp.BBBb....
..BBBb.BBBb....
""")]

# vestido largo (PNJ con dress=True), sustituye piernas
DRESS = {}
DRESS["down"] = [P("""
..tTTTTTTTTt..
..tTTTTTTTTt..
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
tTTTTTTTTTTTTt
.tttttttttttt.
"""), P("""
..tTTTTTTTTt..
..tTTTTTTTTt..
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
tTTTTTTTTTTTTt
.tttttttttBBb.
"""), P("""
..tTTTTTTTTt..
..tTTTTTTTTt..
.tTTTTTTTTTTt.
.tTTTTTTTTTTt.
tTTTTTTTTTTTTt
.BBbttttttttt.
""")]
DRESS["side"] = [P("""
....tTTTTTT...
...tTTTTTTTT..
...tTTTTTTTT..
..tTTTTTTTTTT.
..tTTTTTTTTTT.
...ttttttttt..
"""), P("""
....tTTTTTT...
...tTTTTTTTT..
...tTTTTTTTT..
..tTTTTTTTTTT.
..tTTTTTTTTTTT
...tttttttBBb.
"""), P("""
....tTTTTTT...
...tTTTTTTTT..
...tTTTTTTTT..
..tTTTTTTTTTT.
.tTTTTTTTTTTT.
..BBbttttttt..
""")]
DRESS["up"] = DRESS["down"]


# =====================================================================================
# EQUIPO VISIBLE
# =====================================================================================
ARMOR_PAL = {
    "leather": {"mid": (126, 84, 52), "dark": (88, 56, 36), "light": (162, 112, 70), "trim": (60, 38, 26), "pat": "stitch"},
    "chain": {"mid": (150, 152, 164), "dark": (104, 106, 120), "light": (196, 198, 208), "trim": (84, 60, 44), "pat": "check"},
    "plate": {"mid": (178, 184, 198), "dark": (116, 122, 140), "light": (232, 236, 246), "trim": (214, 172, 70), "pat": "bands"},
    "mithril": {"mid": (170, 200, 222), "dark": (104, 136, 170), "light": (236, 248, 255), "trim": (240, 210, 110), "pat": "bands"},
    "robe": {"mid": (72, 70, 140), "dark": (46, 44, 96), "light": (108, 106, 180), "trim": (214, 172, 70), "pat": "robe"},
    "robe_sage": {"mid": (224, 220, 206), "dark": (170, 164, 146), "light": (250, 248, 238), "trim": (80, 120, 190), "pat": "robe"},
    "robe_dark": {"mid": (62, 38, 74), "dark": (38, 22, 48), "light": (96, 64, 112), "trim": (170, 60, 190), "pat": "robe"},
}
MARKERS_TOP = {M_TOP: "mid", M_TOP_D: "dark", (255, 120, 255): "light"}


def apply_armor(c, look, pal, legs_from_y=None):
    ap = ARMOR_PAL[look]
    cloth = {pal["C"]: "light", pal["c"]: "mid", pal["L"]: "trim", pal["G"]: "trim"}
    legs = {pal["P"]: "mid", pal["p"]: "dark"} if ap["pat"] == "robe" else {}
    for y in range(c.h):
        for x in range(c.w):
            p = c.get(x, y)
            if p[3] == 0:
                continue
            rgb = p[:3]
            role = MARKERS_TOP.get(rgb) or cloth.get(rgb)
            if role is None and legs and legs_from_y is not None and y >= legs_from_y:
                role = legs.get(rgb)
            if role is None:
                continue
            col = ap[role]
            if role == "mid":
                if ap["pat"] == "check" and (x + y) % 2 == 0:
                    col = ap["dark"]
                elif ap["pat"] == "bands" and y % 3 == 0:
                    col = ap["light"]
                elif ap["pat"] == "stitch" and (x * 3 + y) % 7 == 0:
                    col = ap["trim"]
            c.px(x, y, col)


HELM_PAL = {
    "cap": {"M": (162, 112, 70), "m": (126, 84, 52), "k": (80, 52, 34)},
    "helm": {"M": (206, 210, 222), "m": (150, 154, 168), "k": (96, 100, 116)},
    "helm_gold": {"M": (236, 240, 248), "m": (178, 184, 198), "k": (214, 172, 70)},
    "hood": {"M": (86, 70, 96), "m": (58, 46, 68), "k": (36, 28, 44)},
    "circlet": {"M": (236, 236, 246), "m": (184, 186, 200), "k": (90, 170, 230)},
    "circlet_moon": {"M": (250, 240, 200), "m": (214, 180, 90), "k": (200, 220, 255)},
}
HELMS = {}
HELMS[("cap", "down")] = (P("""
....mmmmmm....
..mmMMMmmmmm..
.mMMmmmmmmmmm.
.mmmmmmmmmmmm.
kkkkkkkkkkkkkk
"""), -1, -2)
HELMS[("cap", "side")] = (P("""
...mmmmmm....
.mmMMMmmmmm..
mMMmmmmmmmmm.
mmmmmmmmmmmmm
kkkkkkkkkkkkkk
"""), -1, -2)
HELMS[("cap", "up")] = (P("""
....mmmmmm....
..mmMMMmmmmm..
.mMMmmmmmmmmm.
.mmmmmmmmmmmm.
mmmmmmmmmmmmmm
kkkkkkkkkkkkkk
"""), -1, -2)
HELMS[("helm", "down")] = (P("""
.....mmmm.....
...mMMMmmmm...
..mMMmmmmmmm..
.mMmmmmmmmmmm.
.mmmmmmmmmmmm.
mmmmmmmmmmmmmm
kkkkkkkkkkkkkk
mk..........km
mk..........km
m............m
"""), -1, -3)
HELMS[("helm", "side")] = (P("""
....mmmm.....
..mMMMmmmm...
.mMMmmmmmmm..
mMmmmmmmmmmm.
mmmmmmmmmmmmm
mmmmmmmmmmmmm
kkkkkkkkkkkkk
mmmmmmk..km..
mmmmmk...km..
mmmmk........
"""), -1, -3)
HELMS[("helm", "up")] = (P("""
.....mmmm.....
...mMMMmmmm...
..mMMmmmmmmm..
.mMmmmmmmmmmm.
.mmmmmmmmmmmm.
mmmmmmmmmmmmmm
kkkkkkkkkkkkkk
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
.mmmmmmmmmmmm.
"""), -1, -3)
for d in ("down", "side", "up"):
    HELMS[("helm_gold", d)] = HELMS[("helm", d)]
HELMS[("hood", "down")] = (P("""
.....MMMM.....
...MMmmmmMM...
..MmmmmmmmmM..
.MmmmmmmmmmmM.
.mmmkkkkkkmmm.
mmmk......kmmm
mmk........kmm
mmk........kmm
mmk........kmm
mmk........kmm
mmm........mmm
mmmm......mmmm
kmmmm....mmmmk
kkmmm....mmmkk
"""), -1, -2)
HELMS[("hood", "side")] = (P("""
..MMMMMM.....
.MmmmmmmMM...
MmmmmmmmmmM..
mmmmmmmmmmmm.
mmmmmmmmkkkk.
mmmmmmmk.....
mmmmmmmk.....
mmmmmmmk.....
mmmmmmmk.....
mmmmmmmk.....
mmmmmmmm.....
mmmmmmmmm....
kmmmmmmmm....
kkmmmmmmm....
"""), -1, -2)
HELMS[("hood", "up")] = (P("""
.....MMMM.....
...MMmmmmMM...
..MmmmmmmmmM..
.MmmmmmmmmmmM.
.mmmmmmmmmmmm.
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
mmmmmmmmmmmmmm
kmmmmmmmmmmmmk
kkmmmmmmmmmmkk
"""), -1, -2)
HELMS[("circlet", "down")] = (P("""
mMMMMMkMMMMMMm
"""), -1, 3)
HELMS[("circlet", "side")] = (P("""
mMMMMMMMMMkM.
"""), -1, 3)
HELMS[("circlet", "up")] = (P("""
mmmmmmmmmmmmmm
"""), -1, 3)
for d in ("down", "side", "up"):
    HELMS[("circlet_moon", d)] = HELMS[("circlet", d)]


def apply_helm(c, look, direction, hx, hy):
    key = (look, direction)
    if key not in HELMS:
        return
    tpl, fx, fy = HELMS[key]
    stamp(c, tpl, hx + fx, hy + fy, HELM_PAL[look])


SHIELD_PAL = {"wood": ((140, 96, 58), (98, 66, 40), (214, 172, 70)),
              "iron": ((170, 174, 188), (110, 114, 130), (214, 172, 70)),
              "aegis": ((90, 150, 110), (56, 104, 76), (236, 220, 140))}


def apply_shield(c, look, x, y):
    mid, dark, trim = SHIELD_PAL[look]
    for yy in range(-4, 5):
        half = 3 if abs(yy) < 3 else 2
        for xx in range(-half, half + 1):
            col = dark if xx == -half or xx == half or yy in (-4, 4) else mid
            c.px(x + xx, y + yy, col)
    c.px(x, y - 1, trim)
    c.px(x, y, trim)
    c.px(x - 1, y, trim)
    c.px(x + 1, y, trim)


# =====================================================================================
# ACABADO: sombreado de volumen y contorno selectivo de color
# =====================================================================================
G_SKIN_O, G_HAIR_O, M_TOP_O = (120,) * 3, (70,) * 3, (110, 0, 110)


def _ramps(pal):
    down = {pal["S"]: pal["s"], pal["s"]: pal["k"], pal["r"]: pal["k"],
            pal["H"]: pal["h"], pal["h"]: pal["d"],
            pal["U"]: pal["T"], pal["T"]: pal["t"],
            pal["C"]: pal["c"], pal["P"]: pal["p"], pal["B"]: pal["b"]}
    up = {pal["s"]: pal["S"], pal["h"]: pal["H"], pal["T"]: pal["U"], pal["d"]: pal["h"]}
    return down, up


def finish(c, pal, light_top=True):
    """Sombra de 1 px en el borde derecho/inferior y luz en el borde superior izquierdo."""
    down, up = _ramps(pal)
    src = c.im.copy()
    sp = src.load()
    W, H = c.w, c.h

    def solid(x, y):
        return 0 <= x < W and 0 <= y < H and sp[x, y][3] > 0

    for y in range(H):
        for x in range(W):
            p = sp[x, y]
            if p[3] == 0:
                continue
            rgb = p[:3]
            if rgb in (EYE_D, EYE_I, EYE_W, MOUTH, OUT):
                continue
            if not solid(x + 1, y) and rgb in down:
                c.px(x, y, down[rgb])
            elif light_top and (not solid(x, y - 1) or not solid(x - 1, y)) and rgb in up and solid(x + 1, y):
                c.px(x, y, up[rgb])


def selout(c, pal):
    """Contorno coloreado según el material vecino (en grises marcadores para las capas teñibles)."""
    skin = {pal["S"], pal["s"], pal["r"], pal["k"]}
    hair = {pal["H"], pal["h"], pal["d"]}
    top = {pal["T"], pal["t"], pal["U"]}
    marker_skin = pal["s"] == G_SKIN
    marker_hair = pal["h"] == G_HAIR
    marker_top = pal["T"] == M_TOP
    src = c.im.copy()
    sp = src.load()
    W, H = c.w, c.h
    for y in range(H):
        for x in range(W):
            if sp[x, y][3] > 0:
                continue
            best = None
            for dx, dy in ((0, 1), (1, 0), (-1, 0), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < W and 0 <= ny < H and sp[nx, ny][3] > 0:
                    best = sp[nx, ny][:3]
                    break
            if best is None:
                continue
            if best in skin:
                col = G_SKIN_O if marker_skin else darken(pal["k"], 0.5)
            elif best in hair:
                col = G_HAIR_O if marker_hair else darken(pal["d"], 0.45)
            elif best in top:
                col = M_TOP_O if marker_top else darken(pal["t"], 0.5)
            elif best == OUT:
                col = OUT
            else:
                col = mix(darken(best, 0.55), OUT, 0.35)
            c.p[x, y] = col + (255,)


# =====================================================================================
# Ensamblado
# =====================================================================================
def layout(spec):
    race = spec.get("race", "human")
    sex = spec.get("sex", "m")
    dwarf = race == "dwarf"
    kid = spec.get("kid", False)
    if dwarf:
        return dict(head_y=9, torso_y=20, legs_y=26 if not spec.get("dress") else 24, build="d" + sex, legs="d")
    if kid:
        return dict(head_y=9, torso_y=20, legs_y=24, build=sex, legs=sex)
    return dict(head_y=5, torso_y=16, legs_y=24, build=sex, legs=sex)


def spec_pal(spec):
    pal = base_pal()
    for k, v in spec.get("pal", {}).items():
        pal[k] = v
    return pal


def draw_map_frame(spec, direction, frame):
    """direction: down / side / up (side = mirando a la derecha)."""
    c = Canvas(FW, FH)
    pal = spec_pal(spec)
    lay = layout(spec)
    race = spec.get("race", "human")
    sex = spec.get("sex", "m")
    style = spec.get("style", "short")
    hx, hy = 4, lay["head_y"]
    # frame: 0-1 reposo (respiración), 2-7 ciclo de caminar de 6 fases
    if frame < 2:
        leg_i, bob, swing, hbob = 0, frame, 0, frame
    else:
        ph = frame - 2
        leg_i = [1, 1, 0, 2, 2, 0][ph]
        bob = [0, 1, 0, 0, 1, 0][ph]
        swing = [1, 1, 0, -1, -1, 0][ph]
        hbob = bob     # el pelo va pegado a la cabeza
    fr3 = leg_i
    hy += 0

    hdef = HAIR.get((style, direction))
    # pelo trasero
    if hdef and hdef["back"] and direction == "down":
        tpl, bx, by = hdef["back"]
        stamp(c, tpl, hx + bx, hy + by + hbob, pal)

    # piernas / vestido
    ly = lay["legs_y"]
    if spec.get("dress"):
        tpl = DRESS[direction][fr3]
        stamp(c, tpl, 3, ly, pal)
    else:
        stamp(c, LEGS[(lay["legs"], direction)][fr3], 3, ly, pal)

    # torso
    ty = lay["torso_y"] + bob
    build = lay["build"]
    torso = TORSO[(build, direction)]
    stamp(c, torso, 3 if not build.startswith("d") else 2, ty, pal)
    tw = len(torso[0])
    # brazos
    if direction in ("down", "up"):
        # encontrar extremos del torso
        rowmask = torso[2]
        left = next(i for i, ch in enumerate(rowmask) if ch != ".")
        right = len(rowmask) - 1 - next(i for i, ch in enumerate(reversed(rowmask)) if ch != ".")
        ox = 3 if not build.startswith("d") else 2
        stamp(c, ARM_DOWN, ox + left - 1, ty + 1 + swing, pal)
        stamp(c, ARM_DOWN, ox + right + 1, ty + 1 - swing, pal)
    else:
        ax = 8 if not build.startswith("d") else 8
        stamp(c, ARM_SIDE, ax + swing, ty + 1, pal)

    # extras de ropa
    if "apron" in spec and direction != "up":
        apx = 6 if direction == "down" else 9
        apw = 8 if direction == "down" else 4
        for yy in range(ty + 2, ly + 4):
            for xx in range(apx, apx + apw):
                if c.get(xx, yy)[3]:
                    c.px(xx, yy, spec["apron"] if xx > apx else darken(spec["apron"], 0.15))
    if "scarf_neck" in spec:
        col, cold = spec["scarf_neck"]
        for xx in range(5, 15):
            if c.get(xx, ty)[3]:
                c.px(xx, ty, col)
        if direction == "down":
            c.rect(11, ty + 1, 2, 4, cold)
        elif direction == "side":
            c.rect(6, ty + 1, 2, 3 + (fr3 % 2), cold)
        else:
            c.rect(9, ty + 1, 2, 5, cold)
    if "sash" in spec:
        col = spec["sash"]
        for xx in range(0, FW):
            for yy in (ty + 5,):
                if c.get(xx, yy)[3] and c.get(xx, yy)[:3] in (pal["T"], pal["t"], pal["L"], pal["G"]):
                    c.px(xx, yy, col)

    # cabeza
    hkey = direction if direction != "side" else "side"
    htpl = HEAD[hkey + ("_f" if sex == "f" and direction != "up" else "")]
    if direction == "side":
        stamp(c, htpl, hx, hy + bob, pal)
    else:
        stamp(c, htpl, hx, hy + bob, pal)
    if direction != "up":
        ek = (race, "down" if direction == "down" else "side")
        if ek in EARS:
            tpl, ex, ey = EARS[ek]
            if direction == "side" and race == "elf":
                stamp(c, tpl, hx + ex, hy + ey + bob, pal)
            else:
                stamp(c, tpl, hx + ex, hy + ey + bob, pal)
        if race == "dwarf":
            tpl, nx, ny = DWARF_NOSE["down" if direction == "down" else "side"]
            stamp(c, tpl, hx + nx, hy + ny + bob, pal)
        if spec.get("eyes"):
            ecol = spec["eyes"]
            for yy in range(hy, hy + 12):
                for xx in range(hx, hx + 13):
                    if c.get(xx, yy + bob)[:3] == EYE_I:
                        c.px(xx, yy + bob, ecol)
    elif race == "elf":
        tpl, ex, ey = EARS[("elf", "down")]
        stamp(c, tpl, hx + ex, hy + ey + bob, pal)
    # barba
    if spec.get("beard"):
        bd = BEARD_DWARF if race == "dwarf" else BEARD
        if direction != "up":
            tpl, bx, by = bd["down" if direction == "down" else "side"]
            stamp(c, tpl, hx + bx, hy + by + bob, spec.get("beard_pal", pal))
    # pelo delantero
    if hdef:
        tpl, fx, fy = hdef["front"]
        stamp(c, tpl, hx + fx, hy + fy + hbob, pal)
        if hdef["back"] and direction == "side":
            tpl, bx, by = hdef["back"]
    if spec.get("eq_body"):
        apply_armor(c, spec["eq_body"], pal, legs_from_y=ly)
    if spec.get("eq_head"):
        apply_helm(c, spec["eq_head"], direction, hx, hy + hbob)
    finish(c, pal)
    selout(c, pal)
    return c


MAP_COLS = 8


def build_map_sheet(spec):
    sheet = Canvas(FW * MAP_COLS, FH * 4)
    for r, d in enumerate(["down", "left", "right", "up"]):
        for f in range(MAP_COLS):
            if d == "left":
                fr = draw_map_frame(spec, "side", f).flip_h()
            elif d == "right":
                fr = draw_map_frame(spec, "side", f)
            else:
                fr = draw_map_frame(spec, d, f)
            sheet.paste(fr, f * FW, r * FH)
    return sheet


# =====================================================================================
# COMBATE
# =====================================================================================
BATTLE_POSES = ["idle1", "idle2", "windup", "strike", "cast", "hurt", "kneel", "ko", "victory"]

LEGS_STANCE = P("""
....PPPPp.....
...PPp.PPp....
...PPp..PPp...
..PPp...PPp...
..BBb....BBb..
.BBBb....BBBb.
""")
LEGS_LUNGE = P("""
....PPPPp.....
...PPp..PPp...
..PPp....PPp..
.PPp......PPp.
.BBb......BBb.
BBb.......BBBb
""")
LEGS_KNEEL = P("""
....PPPPPPp...
...PPp..PPPp..
..PPp....PPp..
.BBBBBb..BBb..
""")
DLEGS_STANCE = P("""
...PPPPPp.....
..PPPp.PPPp...
..BBBb..BBBb..
.BBBBb..BBBBb.
""")
DLEGS_LUNGE = P("""
...PPPPPp.....
..PPPp..PPPp..
.BBBb....BBBb.
BBBBb....BBBBb
""")
DLEGS_KNEEL = P("""
...PPPPPPPp...
..PPPp..PPPp..
.BBBBBb.BBBb..
""")

STEEL = (196, 206, 218)
STEEL_L = (240, 244, 250)
STEEL_D = (120, 128, 146)
WOOD = (122, 84, 52)
WOOD_D = (84, 56, 36)
GOLDW = (226, 184, 82)


def line(c, x0, y0, x1, y1, col, width=1):
    n = max(abs(x1 - x0), abs(y1 - y0), 1)
    for i in range(n + 1):
        x = round(x0 + (x1 - x0) * i / n)
        y = round(y0 + (y1 - y0) * i / n)
        for w in range(width):
            c.px(x + (w if abs(y1 - y0) > abs(x1 - x0) else 0), y + (w if abs(y1 - y0) <= abs(x1 - x0) else 0), col)


def draw_weapon(c, kind, hx, hy, d, orb=(120, 230, 150)):
    """Arma sostenida en (hx,hy) apuntando en dirección d (dx,dy) en {-1,0,1}."""
    dx, dy = d
    px, py = -dy, dx     # perpendicular
    if kind == "sword":
        for i in range(1, 11):
            col = STEEL_L if i == 10 else STEEL
            c.px(hx + dx * i, hy + dy * i, col)
            c.px(hx + dx * i + px, hy + dy * i + py, STEEL_D if i < 10 else STEEL)
        for k in (-2, -1, 1, 2):
            c.px(hx + dx + px * k, hy + dy + py * k, GOLDW)
        c.px(hx - dx, hy - dy, WOOD_D)
        c.px(hx - 2 * dx, hy - 2 * dy, GOLDW)
    elif kind == "daggers":
        for i in range(1, 6):
            c.px(hx + dx * i, hy + dy * i, STEEL_L if i == 5 else STEEL)
        c.px(hx + dx + px, hy + dy + py, GOLDW)
        c.px(hx + dx - px, hy + dy - py, GOLDW)
        c.px(hx - dx, hy - dy, WOOD_D)
    elif kind == "axe":
        for i in range(-2, 9):
            c.px(hx + dx * i, hy + dy * i, WOOD if i % 3 else WOOD_D)
        ex, ey = hx + dx * 7, hy + dy * 7
        for a in range(-1, 3):
            for b in range(1, 5):
                col = STEEL_L if b == 4 else (STEEL if a < 2 else STEEL_D)
                c.px(ex + dx * a - px * b, ey + dy * a - py * b, col)
        c.px(ex + dx * 3 - px, ey + dy * 3 - py, STEEL_D)
    elif kind == "staff":
        for i in range(-4, 11):
            c.px(hx + dx * i, hy + dy * i, WOOD if i % 4 else WOOD_D)
        ox, oy = hx + dx * 12, hy + dy * 12
        c.rect(ox - 1, oy - 1, 3, 3, orb)
        c.px(ox - 1, oy - 1, lighten(orb, 0.6))
        c.px(ox, oy - 2, darken(orb, 0.3))
        c.px(hx + dx * 10 + px, hy + dy * 10 + py, GOLDW)
        c.px(hx + dx * 10 - px, hy + dy * 10 - py, GOLDW)
    elif kind == "bow":
        # arco vertical frente a la mano
        for k in range(-7, 8):
            off = 2 - (k * k) // 18
            c.px(hx + 1 + off, hy + k, WOOD if abs(k) < 6 else WOOD_D)
        for k in range(-6, 7):
            c.px(hx, hy + k, (230, 230, 220))
    elif kind == "shield":
        c.rect(hx - 2, hy - 3, 5, 7, STEEL_D)
        c.rect(hx - 1, hy - 3, 3, 6, STEEL)
        c.px(hx, hy, GOLDW)
        c.px(hx, hy - 3, STEEL_L)


def _arm(c, pal, sx, sy, hx, hy):
    line(c, sx, sy, hx, hy, pal["t"])
    line(c, sx + 1, sy, hx + (1 if hx >= sx else 0), hy, pal["T"])
    c.rect(hx, hy, 2, 2, pal["s"])


def draw_battle_frame(spec, pose, weapon=None, orb=(120, 230, 150), weapon_only=False, no_weapon=False):
    """Frame de combate mirando a la DERECHA (se voltea al montar la hoja)."""
    c = Canvas(BW, BH)
    fig = Canvas(BW, BH)
    pal = spec_pal(spec)
    lay = layout(spec)
    race = spec.get("race", "human")
    sex = spec.get("sex", "m")
    style = spec.get("style", "short")
    dwarf = race == "dwarf"
    OX, OY = 6, 2
    drop = {"idle2": 1, "kneel": 4, "hurt": 0}.get(pose, 0)
    lean = {"strike": 1, "hurt": -1, "windup": -1, "cast": 1}.get(pose, 0)
    hx, hy = OX + 4 + lean, OY + lay["head_y"] + drop
    ty = OY + lay["torso_y"] + drop
    ly = OY + lay["legs_y"]

    hdef = HAIR.get((style, "side"))
    # brazo trasero (detrás del cuerpo)
    sh_x, sh_y = OX + 9 + lean, ty + 1
    wtarget = fig if not weapon_only else c
    hand = {"idle1": (sh_x + 3, ty + 6), "idle2": (sh_x + 3, ty + 6), "windup": (sh_x - 3, ty - 4),
            "strike": (sh_x + 6, ty + 2), "cast": (sh_x + 8, ty + 1), "hurt": (sh_x - 1, ty + 6),
            "kneel": (sh_x + 3, ty + 5), "victory": (sh_x + 1, ty - 6), "ko": (sh_x + 3, ty + 6)}[pose]
    wdir = {"idle1": (1, -1), "idle2": (1, -1), "windup": (-1, -1), "strike": (1, 0), "cast": (0, -1),
            "hurt": (-1, -1), "kneel": (1, 0), "victory": (0, -1), "ko": (1, -1)}[pose]
    if weapon == "bow":
        wdir = (0, -1)
    if weapon and not no_weapon:
        wk = weapon
        if pose == "cast" and weapon not in ("staff",):
            pass
        draw_weapon(wtarget, wk, hand[0], hand[1], wdir, orb)
    if weapon_only:
        return c

    # piernas
    if spec.get("dress"):
        fr = {"strike": 1, "windup": 2}.get(pose, 0)
        stamp(fig, DRESS["side"][fr], OX + 3, ly - (2 if pose == "kneel" else 0) + (2 if pose == "kneel" else 0), pal)
    else:
        if dwarf:
            tpl = {"strike": DLEGS_LUNGE, "kneel": DLEGS_KNEEL}.get(pose, DLEGS_STANCE)
        else:
            tpl = {"strike": LEGS_LUNGE, "kneel": LEGS_KNEEL}.get(pose, LEGS_STANCE)
            if sex == "f":
                tpl = [LEGS[("f", "side")][0][0]] + tpl[1:]
        kly = ly + (2 if pose == "kneel" and not dwarf else (1 if pose == "kneel" else 0))
        stamp(fig, tpl, OX + 3, kly, pal)
    # torso
    build = lay["build"]
    stamp(fig, TORSO[(build, "side")], OX + (3 if not build.startswith("d") else 2) + lean, ty, pal)
    arm_behind = pose in ("windup", "victory")
    if arm_behind:
        _arm(fig, pal, sh_x, sh_y, hand[0], hand[1])
    # cabeza
    stamp(fig, HEAD["side" + ("_f" if sex == "f" else "")], hx, hy, pal)
    ek = (race, "side")
    tpl, ex, ey = EARS[ek]
    stamp(fig, tpl, hx + ex, hy + ey, pal)
    if dwarf:
        tpl, nx, ny = DWARF_NOSE["side"]
        stamp(fig, tpl, hx + nx, hy + ny, pal)
    if spec.get("eyes"):
        for yy in range(hy, hy + 12):
            for xx in range(hx, hx + 13):
                if fig.get(xx, yy)[:3] == EYE_I:
                    fig.px(xx, yy, spec["eyes"])
    if pose in ("hurt", "ko"):
        for yy in range(hy, hy + 12):
            for xx in range(hx, hx + 13):
                p = fig.get(xx, yy)[:3]
                if p in (EYE_I, EYE_W) or (p == EYE_D and fig.get(xx, yy + 1)[:3] in (EYE_I, EYE_W)):
                    fig.px(xx, yy, pal["k"])
                if p == EYE_D and yy > hy + 6:
                    fig.px(xx, yy, pal["k"])
    if spec.get("beard"):
        bd = BEARD_DWARF if dwarf else BEARD
        tpl, bx, by = bd["side"]
        stamp(fig, tpl, hx + bx, hy + by, pal)
    if hdef:
        tpl, fx, fy = hdef["front"]
        stamp(fig, tpl, hx + fx, hy + fy, pal)
    # extras
    if "scarf_neck" in spec:
        col, cold = spec["scarf_neck"]
        for xx in range(OX + 4, OX + 16):
            if fig.get(xx, ty)[3]:
                fig.px(xx, ty, col)
        fig.rect(OX + 6 + lean, ty + 1, 2, 3, cold)
    if "sash" in spec:
        for xx in range(0, BW):
            yy = ty + 5
            if fig.get(xx, yy)[3] and fig.get(xx, yy)[:3] in (pal["T"], pal["t"], pal["L"], pal["G"]):
                fig.px(xx, yy, spec["sash"])
    # brazo delantero
    if not arm_behind:
        _arm(fig, pal, sh_x, sh_y, hand[0], hand[1])
    if pose == "cast":
        # segunda mano adelantada
        _arm(fig, pal, sh_x - 1, sh_y + 2, sh_x + 6, ty + 4)
    if spec.get("eq_body"):
        apply_armor(fig, spec["eq_body"], pal, legs_from_y=ly)
    if spec.get("eq_head"):
        apply_helm(fig, spec["eq_head"], "side", hx, hy)
    if spec.get("eq_shield"):
        apply_shield(fig, spec["eq_shield"], OX + 13 + lean, ty + 4)
    finish(fig, pal)
    selout(fig, pal)
    c.paste(fig, 0, 0)
    return c


def _ko_frame(frame):
    """Tumbado: rotación fija (sin recortar) para que todas las capas encajen."""
    im = frame.im.rotate(90, expand=False)
    out = Canvas(BW, BH)
    out.im.alpha_composite(im, (0, 5)) if False else out.im.paste(im.crop((0, 0, BW, BH - 5)), (0, 5))
    out.p = out.im.load()
    return out


WEAPON_TIERS = {
    2: {STEEL: (214, 222, 234), STEEL_D: (140, 148, 168), STEEL_L: (255, 255, 255), WOOD: (96, 60, 40), WOOD_D: (214, 172, 70)},
    3: {STEEL: (140, 200, 240), STEEL_D: (70, 120, 190), STEEL_L: (220, 250, 255), WOOD: (60, 40, 70), WOOD_D: (240, 210, 110),
        GOLDW: (250, 230, 140)},
}


def tint_weapon(canvas, tier):
    if tier not in WEAPON_TIERS:
        return canvas
    m = WEAPON_TIERS[tier]
    for y in range(canvas.h):
        for x in range(canvas.w):
            p = canvas.p[x, y]
            if p[3] and p[:3] in m:
                canvas.p[x, y] = m[p[:3]] + (p[3],)
    return canvas


def build_battle_sheet(spec, weapon=None, orb=(120, 230, 150), weapon_only=False, no_weapon=False):
    sheet = Canvas(BW * len(BATTLE_POSES), BH)
    for i, pose in enumerate(BATTLE_POSES):
        if pose == "ko":
            fr = draw_battle_frame(spec, "idle1", weapon, orb, weapon_only, no_weapon=True).flip_h()
            fr = _ko_frame(fr) if not weapon_only else Canvas(BW, BH)
        else:
            fr = draw_battle_frame(spec, pose, weapon, orb, weapon_only, no_weapon).flip_h()
        sheet.paste(fr, i * BW, 0)
    return sheet


# =====================================================================================
# Especificaciones de PNJ (derivadas de chars.SPECS) y construcción
# =====================================================================================
def from_old(o):
    skin, hair, top = o["skin"], o["hair"], o["top"]
    pal = {"S": lighten(skin, 0.12), "s": skin, "r": mix(skin, (230, 120, 120), 0.45), "k": darken(skin, 0.2),
           "H": lighten(hair, 0.3), "h": hair, "d": darken(hair, 0.32),
           "T": top, "t": o.get("top_d", darken(top, 0.3)), "U": lighten(top, 0.2)}
    if "pants" in o:
        pal["P"], pal["p"] = o["pants"], o.get("pants_d", darken(o["pants"], 0.3))
    if "boots" in o:
        pal["B"], pal["b"] = o["boots"], darken(o["boots"], 0.3)
    if "belt" in o:
        pal["L"] = o["belt"]
    elif o.get("dress"):
        pal["L"] = o.get("top_d", darken(top, 0.3))
        pal["G"] = o.get("top_d", darken(top, 0.3))
    if "hood" in o:
        pal["Q"], pal["q"] = o["hood"], o["hood_d"]
    if o.get("style") == "scarf":
        pal["F"], pal["f"] = o["scarf"], o["scarf_d"]
    spec = dict(race=o.get("race", "human"), sex=o.get("sex", "m"), style=o.get("style", "short"), pal=pal)
    for k in ("dress", "kid", "apron", "sash", "eyes"):
        if k in o:
            spec[k] = o[k]
    if "scarf" in o and o.get("style") != "scarf":
        spec["scarf_neck"] = (o["scarf"], o["scarf_d"])
    if "beard" in o:
        bp = dict(pal)
        b = o["beard"]
        bp.update({"H": lighten(b, 0.3), "h": b, "d": darken(b, 0.3)})
        spec["beard"] = True
        spec["beard_pal"] = bp
    if spec["style"] == "none":
        spec["style"] = "short"
    return spec


WEAPONS = {"kaelen": ("sword", None), "kaelen_dark": ("sword", None), "yara": ("staff", (130, 230, 150)),
           "yara_dark": ("staff", (190, 90, 230)), "aelis": ("bow", None), "brom": ("axe", None),
           "thrall": (None, None)}
PLAYER_WEAPONS = {"sword": (150, 230, 150), "bow": None, "daggers": None, "staff": (140, 220, 255), "axe": None}


def layer_spec2(race, sex, style="short", beard=False):
    d = dict(race=race, sex=sex, style=style, pal={})
    if beard:
        d["beard"] = True
    return d


def diff_layer(full, base):
    out = Canvas(full.w, full.h)
    for y in range(full.h):
        for x in range(full.w):
            a, b = full.p[x, y], base.p[x, y]
            if a != b and a[3] > 0:
                out.p[x, y] = a
    return out


def build_all(out_dir, old_specs, hair_styles, races, sexes):
    import os
    os.makedirs(f"{out_dir}/battle", exist_ok=True)
    for name, o in old_specs.items():
        spec = from_old(o)
        build_map_sheet(spec).save(f"{out_dir}/chars/{name}.png")
        w, orb = WEAPONS.get(name, (None, None))
        if name in WEAPONS:
            build_battle_sheet(spec, w, orb or (120, 230, 150)).save(f"{out_dir}/battle/{name}.png")
            build_battle_sheet(spec, no_weapon=True).save(f"{out_dir}/battle/{name}_nw.png")
    d = f"{out_dir}/creator"
    for race in races:
        for sex in sexes:
            base_spec = layer_spec2(race, sex, style="__none")
            base = build_map_sheet(base_spec)
            base.save(f"{d}/body_{race}_{sex}.png")
            bbase = build_battle_sheet(base_spec, no_weapon=True)
            bbase.save(f"{d}/bbody_{race}_{sex}.png")
            for style in hair_styles:
                s = layer_spec2(race, sex, style)
                diff_layer(build_map_sheet(s), base).save(f"{d}/hair_{race}_{sex}_{style}.png")
                diff_layer(build_battle_sheet(s, no_weapon=True), bbase).save(f"{d}/bhair_{race}_{sex}_{style}.png")
            bs = layer_spec2(race, sex, style="__none", beard=True)
            diff_layer(build_map_sheet(bs), base).save(f"{d}/beard_{race}_{sex}.png")
            diff_layer(build_battle_sheet(bs, no_weapon=True), bbase).save(f"{d}/bbeard_{race}_{sex}.png")
    # armas (capas detrás del cuerpo) por raza, tipo y calidad
    orbs = {"": (120, 230, 150), "dark": (190, 90, 230), "light": (255, 240, 150)}
    for race in races:
        for w in PLAYER_WEAPONS:
            for tier in (1, 2, 3):
                for oname, ocol in (orbs.items() if w == "staff" else [("", None)]):
                    sheet = build_battle_sheet(layer_spec2(race, "m"), w, ocol or (120, 230, 150), weapon_only=True)
                    tint_weapon(sheet, tier)
                    suffix = f"_{oname}" if oname else ""
                    sheet.save(f"{d}/weapon_{race}_{w}_{tier}{suffix}.png")
            if w != "staff":
                import shutil
                shutil.copy(f"{d}/weapon_{race}_{w}_1.png", f"{d}/weapon_{race}_{w}.png")
            else:
                import shutil
                shutil.copy(f"{d}/weapon_{race}_{w}_1.png", f"{d}/weapon_{race}_{w}.png")
    # equipo visible: armaduras, cascos y escudos
    for race in races:
        for sex in sexes:
            bs = layer_spec2(race, sex, style="__none")
            base_m = build_map_sheet(bs)
            base_b = build_battle_sheet(bs, no_weapon=True)
            for look in ARMOR_PAL:
                sp = dict(bs, eq_body=look)
                diff_layer(build_map_sheet(sp), base_m).save(f"{d}/eqbody_{look}_{race}_{sex}.png")
                diff_layer(build_battle_sheet(sp, no_weapon=True), base_b).save(f"{d}/beqbody_{look}_{race}_{sex}.png")
            for look in HELM_PAL:
                sp = dict(bs, eq_head=look)
                diff_layer(build_map_sheet(sp), base_m).save(f"{d}/eqhead_{look}_{race}_{sex}.png")
                diff_layer(build_battle_sheet(sp, no_weapon=True), base_b).save(f"{d}/beqhead_{look}_{race}_{sex}.png")
            for look in SHIELD_PAL:
                sp = dict(bs, eq_shield=look)
                diff_layer(build_battle_sheet(sp, no_weapon=True), base_b).save(f"{d}/beqshield_{look}_{race}_{sex}.png")
