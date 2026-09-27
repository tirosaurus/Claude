"""Banda sonora de Vaelmoor (composición original).

Tema principal (Re mayor) reaparece en el título, finales y, en menor,
en el combate final. Cada pista se renderiza en bucle perfecto a OGG.
"""
import os
import sys
import time
from orch import Song


def P(bars_text, bpb=4):
    """'Em C | D B' -> [(acorde, beats)]: compases separados por espacios;
    dentro de un compás, varios acordes unidos con '_' se reparten el compás."""
    out = []
    for bar in bars_text.split():
        chs = bar.split("_")
        for c in chs:
            out.append((c, bpb / len(chs)))
    return out


# ---------------------------------------------------------------------------------------------
# Tema principal
THEME_A = ("A4:1.5 D5:.5 E5:1 F#5:1 G5:1.5 F#5:.5 E5:1 D5:1 E5:1.5 F#5:.5 A5:1 G5:.5 F#5:.5 E5:3 -:1 "
           "A4:1.5 D5:.5 E5:1 F#5:1 G5:1.5 A5:.5 B5:1 A5:.5 G5:.5 F#5:1 E5:1 D5:1 E5:1 D5:4")
THEME_A_CH = P("D G A_Em A D G_Em A D")
THEME_B = ("F#5:2 B5:1 A5:1 G5:2 D5:1 E5:1 F#5:1.5 E5:.5 D5:1 C#5:1 E5:4 "
           "F#5:2 B5:1 C#6:1 D6:2 B5:1 G5:1 A5:1.5 G5:.5 F#5:1 E5:1 E5:2 C#5:1 A4:1")
THEME_B_CH = P("Bm G D A Bm G Em A")


def title():
    s = Song(84, 28)
    # Intro misteriosa: coro + redoble
    s.pad("choir", P("D D Bm A"), 0, 50, 69, 0.35)
    s.pad("strings", P("D D Bm A"), 0, 38, 57, 0.25, count=3, key="lowpad")
    s.note("roll", "D2", s.b(2), 8, 0.6)
    s.note("swell", "C4", s.b(2), 8, 0.6)
    s.melody("horn", "A4:2 D5:2 E5:4 F#5:2 E5:1 D5:1 E5:4", 0, 0.5, pan=-0.2)
    s.note("timp", "D2", s.b(4), 1, 0.9)
    s.note("cymbal", "C4", s.b(4), 1, 0.8)
    # Tema A con metales
    st = s.b(4)
    s.melody("brass", THEME_A, st, 0.75, pan=-0.1)
    s.melody("strings", THEME_A, st, 0.35, octave=1, pan=0.15)
    s.pad("strings", THEME_A_CH, st, 54, 74, 0.4)
    s.bassline("bass", THEME_A_CH, st, 2, 0.6)
    s.arp("harp", THEME_A_CH, st, 62, 86, 0.5, vel=0.35, pan=0.4)
    for k in range(8):
        s.note("timp", "D2" if k % 4 in (0, 3) else "A1", st + k * 4, 1, 0.55)
        s.drums("T.......t...t...", st + k * 4, 0.25, 1, 0.35)
    # Tema B con coro
    st = s.b(12)
    s.note("cymbal", "C4", st, 1, 0.7)
    s.melody("strings", THEME_B, st, 0.6, pan=0.1)
    s.melody("horn", THEME_B, st, 0.45, octave=-1, pan=-0.3)
    s.pad("choir", THEME_B_CH, st, 54, 72, 0.4)
    s.bassline("bass", THEME_B_CH, st, 2, 0.6)
    s.ostinato("spic", THEME_B_CH, st, [(0, .5, 0), (.5, .5, 2), (1, .5, 3), (1.5, .5, 2)], 3, 0.3, pan=0.3)
    for k in range(8):
        s.drums("T...t...T...t.t.", st + k * 4, 0.25, 1, 0.45)
    s.note("roll", "A1", s.b(19), 4, 0.6)
    s.note("swell", "C4", s.b(19), 4, 0.6)
    # Tema A final, tutti
    st = s.b(20)
    s.note("cymbal", "C4", st, 1, 0.9)
    s.note("timp", "D2", st, 1, 1.0)
    s.melody("brass", THEME_A, st, 0.85, pan=-0.1)
    s.melody("strings", THEME_A, st, 0.5, octave=1, pan=0.2)
    s.melody("choir", THEME_A, st, 0.3)
    s.pad("strings", THEME_A_CH, st, 50, 72, 0.4, key="p2")
    s.bassline("bass", THEME_A_CH, st, 2, 0.7)
    s.ostinato("spic", THEME_A_CH, st, [(0, .5, 0), (.5, .5, 1), (1, .5, 2), (1.5, .5, 3)], 3, 0.3, pan=-0.3)
    for k in range(8):
        s.drums("T.t.T...T.t.T.tt", st + k * 4, 0.25, 1, 0.5)
    return s


def battle():
    bpm = 152
    A = P("Em C D B Em C Am_D B")
    B = P("Am Em Am B C D B B")
    Cb = P("C D Bm Em C D B7 B7")
    s = Song(bpm, 2 + 8 + 8 + 8 + 8)
    ost = [(0, .5, 0), (.5, .5, 3), (1, .5, 2), (1.5, .5, 3), (2, .5, 0), (2.5, .5, 3), (3, .5, 1), (3.5, .5, 3)]
    low16 = [(0, .25, 0), (.25, .25, 0), (.5, .25, 3), (.75, .25, 0)]
    # intro
    s.ostinato("spic", P("Em Em"), 0, ost, 4, 0.45, pan=0.25)
    s.ostinato("spic", P("Em Em"), 0, low16, 2, 0.5, pan=-0.2)
    s.drums("T..T..T.T.T.TTTT", 0, 0.5, 1, 0.8)
    s.melody("stab", "E3:.5 -:1 E3:.5 -:1 G3:.5 F#3:.5 E4:.5 -:1.5 D4:.5 -:.5 B3:1", 0, 0.8)
    s.note("swell", "C4", s.b(1), 4, 0.6)

    melA = ("E5:1.5 B4:.5 E5:1 F#5:1 G5:1.5 F#5:.5 E5:1 G5:1 F#5:1.5 E5:.5 D5:1 A4:1 B4:2 D#5:1 F#5:1 "
            "E5:1.5 B4:.5 E5:1 F#5:1 G5:1 A5:1 B5:1 C6:1 B5:1.5 A5:.5 G5:1 F#5:1 F#5:2 D#5:1 B4:1")
    melB = ("C6:2 B5:1 A5:1 G5:2 E5:1 B4:1 C5:1 E5:1 A5:1 C6:1 B5:4 "
            "E5:1 G5:1 C6:1 E6:1 D6:2 C6:1 A5:1 B5:2 A5:1 F#5:1 D#5:2 F#5:1 B5:1")
    melC = ("E5:3 G5:1 F#5:3 A5:1 B5:2 A5:1 G5:1 E5:4 E5:1 F#5:1 G5:1 A5:1 B5:1 C6:1 D6:1 E6:1 D#6:4 B5:4")

    def section(st, prog, mel, lead_inst, big=False):
        s.note("cymbal", "C4", st, 1, 0.8 if big else 0.6)
        s.melody(lead_inst, mel, st, 0.8, pan=-0.1)
        if big:
            s.melody("strings", mel, st, 0.45, octave=1, pan=0.2)
        s.ostinato("spic", prog, st, ost, 4, 0.4, pan=0.3)
        s.ostinato("spic", prog, st, low16, 2, 0.45, pan=-0.3)
        s.bassline("bass", prog, st, 1, 0.55, pattern=[(0, 1.5, 0), (1.5, .5, 0), (2, 2, 0)])
        s.pad("strings", prog, st, 52, 71, 0.28, count=3)
        for k in range(8):
            s.drums("T..t..T.T..t.Tt." if k % 4 != 3 else "T..t..T.T.SsSSSS", st + k * 4, 0.25, 1, 0.75)
            s.drums("..s...s...s...s.", st + k * 4, 0.25, 1, 0.3)
        # golpes de timbal en raíces
        s.bassline("timp", prog, st, 2, 0.7, pattern=[(0, 1, 0), (2.5, 1, 7)])

    section(s.b(2), A, melA, "brass")
    # B: cuerdas cantan, metales contracanto
    st = s.b(10)
    section(st, B, melB, "strings")
    s.melody("horn", "E4:4 E4:4 E4:4 D#4:4 E4:4 F#4:4 F#4:4 D#4:4", st, 0.55, pan=-0.35)
    s.pad("choir", B, st, 52, 69, 0.3, count=3)
    # C: puente con coro
    st = s.b(18)
    section(st, Cb, melC, "horn")
    s.pad("choir", Cb, st, 55, 74, 0.4)
    s.note("roll", "B1", s.b(25), 4, 0.6)
    s.note("swell", "C4", s.b(25), 4, 0.7)
    # A' tutti
    section(s.b(26), A, melA, "brass", big=True)
    s.melody("stab", "E4:.5 -:3.5 C4:.5 -:3.5 D4:.5 -:3.5 B3:.5 -:1.5 B3:.5 -:1.5", s.b(26), 0.7, pan=0.35)
    return s


def boss():
    s = Song(140, 2 + 8 + 8 + 8)
    A = P("Dm Bb Gm A Dm Eb Dm A")
    B = P("Gm Dm Gm A Bb C A A")
    low16 = [(0, .25, 0), (.25, .25, 0), (.5, .25, 0), (.75, .25, 3)]
    s.ostinato("spic", P("Dm Dm"), 0, low16, 2, 0.55)
    s.drums("T...T...T.T.T.T.TTTTTTTT", 0, 1 / 3, 1, 0.9)
    s.pad("choir", P("Dm A"), 0, 50, 64, 0.45, count=3)
    s.melody("stab", "D3:.5 -:1.5 D3:.5 -:1.5 D3:.5 -:.5 D3:.5 -:.5 C#3:1 E3:1", 0, 0.9)
    melA = ("D5:2 F5:1 E5:1 D5:1.5 C5:.5 Bb4:2 G4:1 Bb4:1 D5:1 G5:1 E5:3 C#5:1 "
            "D5:2 F5:1 A5:1 G5:1.5 F5:.5 Eb5:2 F5:1 D5:1 A4:1 D5:1 C#5:4")
    melB = ("Bb5:2 A5:1 G5:1 F5:2 D5:2 G5:1 A5:1 Bb5:1 D6:1 C#6:4 D6:2 C6:1 Bb5:1 C6:2 G5:2 A5:2 E5:2 C#5:2 A4:2")

    def section(st, prog, mel, big):
        s.note("cymbal", "C4", st, 1, 0.8)
        s.melody("brass", mel, st, 0.85, pan=-0.1)
        s.melody("choir", mel, st, 0.35 if big else 0.2, octave=-1)
        if big:
            s.melody("strings", mel, st, 0.45, octave=1, pan=0.2)
        s.ostinato("spic", prog, st, low16, 2, 0.5, pan=-0.2)
        s.ostinato("spic", prog, st, [(0, .5, 3), (.5, .5, 2), (1, .5, 1), (1.5, .5, 2)], 4, 0.35, pan=0.35)
        s.pad("choir", prog, st, 52, 69, 0.42, count=4)
        s.bassline("bass", prog, st, 1, 0.65)
        s.bassline("timp", prog, st, 2, 0.8, pattern=[(0, 1, 0), (1.5, .5, 0), (3, 1, 7)])
        for k in range(8):
            s.drums("T..TT..tT..TT.TT", st + k * 4, 0.25, 1, 0.85)
            s.drums("....S.......S.s.", st + k * 4, 0.25, 1, 0.5)
            s.drums("a.......", st + k * 4, 0.5, 1, 0.4 if k % 2 == 0 else 0)
    section(s.b(2), A, melA, False)
    section(s.b(10), B, melB, True)
    section(s.b(18), A, melA, True)
    s.pad("organ", A, s.b(18), 50, 72, 0.35)
    return s


def final():
    s = Song(132, 4 + 8 + 8 + 8)
    A = P("Cm Ab Fm G Cm Db Bb G")
    B = P("Ab Bb Gm Cm Ab Bb G G")
    s.pad("organ", P("Cm Cm Db G"), 0, 43, 67, 0.5, count=5)
    s.pad("choir", P("Cm Cm Db G"), 0, 55, 72, 0.5)
    s.note("roll", "C2", 0, 12, 0.7)
    s.note("swell", "C4", s.b(3), 4, 0.8)
    s.drums("h...h...h...h...", 0, 0.25, 3, 0.9)
    # tema principal en menor (Do menor)
    melA = ("G4:1.5 C5:.5 D5:1 Eb5:1 F5:1.5 Eb5:.5 D5:1 C5:1 D5:1.5 Eb5:.5 G5:1 F5:.5 Eb5:.5 D5:3 -:1 "
            "G4:1.5 C5:.5 D5:1 Eb5:1 F5:1.5 G5:.5 Ab5:1 G5:.5 F5:.5 Eb5:1 D5:1 C5:1 D5:1 B4:4")
    melB = ("Eb5:2 Ab5:1 G5:1 F5:2 Bb4:1 D5:1 Eb5:1.5 D5:.5 C5:1 Bb4:1 C5:4 "
            "Eb5:2 Ab5:1 Bb5:1 C6:2 Bb5:1 F5:1 G5:1.5 F5:.5 Eb5:1 D5:1 D5:2 B4:1 G4:1")
    low16 = [(0, .25, 0), (.25, .25, 0), (.5, .25, 3), (.75, .25, 0)]

    def section(st, prog, mel, extra):
        s.note("cymbal", "C4", st, 1, 0.9)
        s.note("timp", "C2", st, 1, 1.0)
        s.melody("brass", mel, st, 0.9, pan=-0.1)
        s.melody("choir", mel, st, 0.45)
        s.melody("strings", mel, st, 0.5, octave=1, pan=0.25)
        s.pad("organ", prog, st, 43, 67, 0.3, count=5)
        s.ostinato("spic", prog, st, low16, 2, 0.5, pan=-0.3)
        s.ostinato("spic", prog, st, [(0, .25, 3), (.25, .25, 2), (.5, .25, 1), (.75, .25, 2)], 4, 0.3, pan=0.3)
        s.bassline("bass", prog, st, 1, 0.7)
        s.bassline("timp", prog, st, 2, 0.85, pattern=[(0, 1, 0), (.75, .25, 0), (2, 1, 7), (3, .5, 0), (3.5, .5, 0)])
        for k in range(8):
            s.drums("T.tTT.t.T.tTTTTT" if k % 2 else "T.tTT.t.T.tT.T.T", st + k * 4, 0.25, 1, 0.9)
            s.drums("....S.......S...", st + k * 4, 0.25, 1, 0.55)
        if extra:
            s.melody("bell", "C5:4 Ab4:4 F4:4 G4:4 C5:4 Db5:4 Bb4:4 G4:4", st, 0.4, pan=0.4)
    section(s.b(4), A, melA, False)
    section(s.b(12), B, melB, False)
    section(s.b(20), A, melA, True)
    return s


def victory():
    s = Song(120, 5, tail=3.0)
    s.note("cymbal", "C4", 0, 1, 0.8)
    s.note("timp", "C2", 0, 1, 1.0)
    fan = "G4:1/3 G4:1/3 G4:1/3 C5:1 G4:.5 C5:.5 E5:1.5 D5:.5 E5:1 G5:3 "
    s.melody("brass", fan, 0, 0.9, pan=-0.1)
    s.melody("brass", "E4:1/3 E4:1/3 E4:1/3 G4:1 E4:.5 G4:.5 C5:1.5 B4:.5 C5:1 E5:3", 0, 0.7, pan=0.2)
    s.melody("strings", "C5:2 E5:2 G5:4", 1, 0.4, octave=1)
    s.pad("strings", P("C C"), 1, 52, 72, 0.4)
    s.drums("S.SS", 0, 1 / 3, 1, 0.6)
    s.note("timp", "G1", 1, 1, 0.8)
    s.note("timp", "C2", 3, 1, 0.9)
    s.melody("brass", "F5:1 A5:1 G5:1 E5:1 C5:4", s.b(2, 1), 0.75, pan=-0.1)
    s.pad("strings", P("F G C"), s.b(2, 1), 52, 72, 0.45, key="v2")
    s.pad("horn", P("F G C"), s.b(2, 1), 48, 64, 0.4, count=3)
    s.bassline("bass", P("F G C"), s.b(2, 1), 2, 0.6)
    s.arp("harp", P("F G C"), s.b(2, 1), 60, 84, 1 / 3, vel=0.4, pan=0.4)
    s.note("timp", "C2", s.b(4, 1), 2, 0.9)
    s.note("cymbal", "C4", s.b(4, 1), 1, 0.7)
    return s


def village():
    s = Song(100, 32)
    A = P("G C D G Em C Am D")
    B = P("C D Bm Em C D G G")
    melA = ("G5:1 B5:.5 A5:.5 G5:1 D5:1 E5:1 G5:1 C6:1.5 B5:.5 A5:1 F#5:.5 G5:.5 A5:1 D5:1 G5:3 -:1 "
            "B5:1 A5:.5 G5:.5 E5:1 B4:1 C5:1 E5:1 G5:1.5 E5:.5 A5:1 G5:.5 E5:.5 C5:1 E5:1 D5:3 F#5:1")
    melB = ("E5:1.5 G5:.5 C6:2 D6:1 C6:.5 B5:.5 A5:2 B5:1 A5:.5 F#5:.5 D5:2 E5:1 G5:1 B5:2 "
            "C6:1 B5:.5 A5:.5 G5:1 E5:1 F#5:1 A5:1 D6:2 B5:1 A5:.5 G5:.5 D5:1 F#5:1 G5:4")
    for rep in range(2):
        st = s.b(rep * 16)
        s.melody("flute", melA, st, 0.6, pan=-0.15)
        if rep == 1:
            s.melody("oboe", melA, st, 0.28, octave=-1, pan=0.3)
        s.arp("harp", A, st, 55, 79, 0.5, (0, 1, 2, 3, 2, 1, 0, 2), 0.4, pan=0.35)
        s.bassline("pizz", A, st, 2, 0.6, pattern=[(0, 1, 0), (2, 1, 7)])
        s.pad("strings", A, st, 55, 72, 0.22, count=3)
        for k in range(8):
            s.drums("j.x.j.x.j.x.j.xx", st + k * 4, 0.25, 1, 0.35)
        st = s.b(rep * 16 + 8)
        s.melody("violin" if rep == 0 else "flute", melB, st, 0.5, pan=0.1)
        s.melody("horn", "E4:4 F#4:4 D4:4 E4:4 E4:4 F#4:4 D4:8", st, 0.3, pan=-0.3)
        s.arp("harp", B, st, 55, 79, 0.5, (0, 1, 2, 3, 2, 1, 0, 2), 0.4, pan=0.35)
        s.bassline("pizz", B, st, 2, 0.6, pattern=[(0, 1, 0), (2, 1, 7)])
        s.pad("strings", B, st, 55, 72, 0.25, count=3)
        for k in range(8):
            s.drums("j.x.j.x.j.x.j.xx", st + k * 4, 0.25, 1, 0.35)
    return s


def home():
    s = Song(80, 16)
    A = P("F Bb C F Dm Bb Gm C")
    mel = ("A5:1.5 G5:.5 F5:1 C5:1 D5:1.5 F5:.5 Bb5:2 G5:1 A5:.5 G5:.5 E5:1 C5:1 F5:4 "
           "D5:1 E5:.5 F5:.5 A5:1 F5:1 Bb4:1 D5:1 F5:2 G5:1 F5:.5 E5:.5 D5:1 Bb4:1 C5:3 -:1")
    s.melody("celesta", mel, 0, 0.5, pan=-0.2)
    s.arp("harp", A, 0, 53, 77, 0.5, (0, 2, 1, 3, 2, 1), 0.35, pan=0.3)
    s.pad("strings", A, 0, 53, 70, 0.22, count=3)
    s.bassline("bass", A, 0, 2, 0.35)
    st = s.b(8)
    s.melody("flute", mel, st, 0.45, pan=-0.2)
    s.melody("celesta", mel, st, 0.25, octave=1, pan=0.3)
    s.arp("harp", A, st, 53, 77, 0.5, (0, 2, 1, 3, 2, 1), 0.35, pan=0.3)
    s.pad("strings", A, st, 53, 72, 0.28, count=4)
    s.bassline("pizz", A, st, 2, 0.45, pattern=[(0, 1, 0), (2, 1, 7)])
    return s


def forest():
    s = Song(90, 32)
    A = P("Am G D Am F G Em Am")
    mel = ("E5:2 D5:1 C5:.5 D5:.5 B4:2 G4:2 A4:1 F#5:1.5 E5:.5 D5:1 E5:4 "
           "C5:1 E5:1 A5:1.5 G5:.5 D5:1 G5:1 B5:2 A5:1 G5:.5 E5:.5 B4:2 A4:4")
    B = P("F C G Am F C E E")
    melB = "A5:3 G5:1 E5:2 C5:2 D5:3 E5:1 C5:2 A4:2 A5:3 G5:1 E5:2 C6:2 B5:4 G#5:4"
    for rep in range(2):
        st = s.b(rep * 16)
        s.melody("flute", mel, st, 0.55, pan=-0.2)
        s.arp("harp", A, st, 57, 81, 0.5, (0, 1, 2, 3, 1, 2), 0.4, pan=0.35)
        s.pad("strings", A, st, 52, 69, 0.25)
        s.bassline("bass", A, st, 2, 0.35)
        for k in range(8):
            s.drums("x.x.X.x.x.x.X.xx", st + k * 4, 0.25, 1, 0.22)
        st = s.b(rep * 16 + 8)
        s.melody("oboe" if rep == 0 else "violin", melB, st, 0.45, pan=0.15)
        s.pad("choir", B, st, 55, 72, 0.25, count=3)
        s.arp("harp", B, st, 57, 81, 0.5, (0, 1, 2, 3, 1, 2), 0.35, pan=0.35)
        s.pad("strings", B, st, 48, 64, 0.25, count=3, key="b")
        s.bassline("bass", B, st, 2, 0.35)
        s.note("timp", "A1", st, 1, 0.4)
    return s


def night():
    s = Song(124, 24)
    A = P("Cm Cm Ab G Cm Db Ab G")
    B = P("Fm Cm Fm G Ab Bb G G")
    trem = [(k * 0.125, 0.125, 0) for k in range(8)]
    mel = ("C5:2 Eb5:1 D5:1 C5:2 G4:2 Ab4:1 C5:1 Eb5:1 F5:1 D5:4 "
           "C5:2 Eb5:1 G5:1 Ab5:2 G5:1 F5:1 Eb5:1 D5:1 C5:1 Eb5:1 D5:4")
    melB = "F5:2 Ab5:2 G5:2 Eb5:2 F5:1 Ab5:1 C6:1 Ab5:1 B5:4 C6:2 Bb5:2 D6:2 Bb5:2 B5:4 G5:4"
    for rep, (prog, m) in enumerate(((A, mel), (B, melB), (A, mel))):
        st = s.b(rep * 8)
        s.ostinato("spic", prog, st, trem, 3, 0.3, pan=-0.2)
        s.ostinato("spic", prog, st, [(0, .5, 0), (.5, .5, 1), (1, .5, 2), (1.5, .5, 1)], 4, 0.25, pan=0.3)
        s.melody("horn" if rep != 1 else "strings", m, st, 0.7)
        s.pad("choir", prog, st, 50, 67, 0.3, count=3)
        s.bassline("bass", prog, st, 1, 0.55)
        for k in range(8):
            s.drums("T.....t.T...t.t." if rep != 1 else "T..t..t.T..t.TTT", st + k * 4, 0.25, 1, 0.7)
        s.bassline("timp", prog, st, 2, 0.5, pattern=[(0, 1, 0)])
        s.note("swell", "C4", st + 28, 4, 0.5)
    return s


def cathedral():
    s = Song(66, 16)
    A = P("Dm C Bb C Gm Dm Bb A")
    mel = "D5:2 E5:1 F5:1 E5:2 C5:2 D5:1 F5:1 Bb5:2 A5:4 G5:2 F5:1 E5:1 F5:2 D5:2 D5:1 E5:1 F5:1 G5:1 E5:2 C#5:2"
    s.pad("organ", A, 0, 50, 69, 0.35)
    s.bassline("organ", A, 0, 2, 0.35)
    s.melody("choir", mel, 0, 0.45)
    st = s.b(8)
    s.pad("choir", A, st, 50, 69, 0.35)
    s.pad("strings", A, st, 57, 74, 0.25, key="s2")
    s.melody("violin", mel, st, 0.45, pan=0.2)
    s.melody("bell", "D5:8 C5:8 Bb4:8 A4:8", 0, 0.25, pan=-0.4)
    s.bassline("bass", A, st, 2, 0.4)
    return s


def deep():
    s = Song(70, 16)
    A = P("F#m D Bm C# F#m D G C#")
    s.pad("strings", A, 0, 42, 61, 0.35, count=3)
    s.bassline("bass", A, 0, 1, 0.45)
    s.pad("choir", A, s.b(8), 54, 69, 0.3, count=3)
    mel = "C#5:3 D5:1 C#5:2 A4:2 B4:3 A4:1 F#4:4 C#5:3 D5:1 E5:2 F#5:2 G5:4 E#5:4"
    s.melody("flute", mel, s.b(8), 0.4, pan=-0.3)
    for k in range(16):
        if k % 2 == 0:
            s.note("harp", ["F#5", "A5", "C#6", "D6", "B5", "C#6", "G5", "G#5"][k // 2], s.b(k, 1), 1, 0.3, pan=0.4)
        s.drums("h.......h.h.....", s.b(k), 0.25, 1, 0.35)
    s.note("bell", "F#4", 0, 4, 0.2, pan=0.3)
    s.note("bell", "C#5", s.b(8), 4, 0.2, pan=-0.3)
    return s


def elves():
    s = Song(88, 16)
    A = P("E F#/E C#m B A B E E")
    mel = "B5:1.5 G#5:.5 E5:1 F#5:1 A#5:2 G#5:1 F#5:1 E5:1.5 C#5:.5 G#5:2 F#5:4 E5:1 F#5:1 G#5:1 A5:1 B5:2 D#6:2 E6:4 B5:4"
    for rep in range(2):
        st = s.b(rep * 8)
        s.melody("flute" if rep == 0 else "choir", mel, st, 0.5 if rep == 0 else 0.4, pan=-0.1)
        s.arp("harp", A, st, 56, 88, 0.25, (0, 1, 2, 3, 4, 3, 2, 1), 0.3, pan=0.35, count=5)
        s.pad("strings", A, st, 56, 76, 0.22)
        s.bassline("bass", A, st, 2, 0.3)
        s.melody("celesta", "E6:4 D#6:4 C#6:4 B5:4 C#6:4 D#6:4 E6:8", st, 0.2, pan=0.4)
    return s


def heart():
    s = Song(76, 16)
    A = P("Bm G Em F# Bm G C F#")
    for k in range(16):
        s.drums("h..h............", s.b(k), 0.25, 1, 0.9)
    s.pad("choir", A, 0, 47, 64, 0.4, count=3)
    s.pad("organ", A, s.b(8), 38, 59, 0.3, count=4)
    s.bassline("bass", A, 0, 1, 0.5)
    mel = "B4:3 C#5:1 D5:2 B4:2 G4:3 A4:1 F#4:4 B4:3 C#5:1 D5:2 F#5:2 E5:3 C5:1 A#4:4"
    s.melody("horn", mel, s.b(8), 0.5, pan=-0.2)
    s.melody("violin", "F#5:8 E5:8", s.b(4), 0.3, pan=0.3)
    s.note("roll", "F#1", s.b(15), 4, 0.5)
    return s


def sad():
    s = Song(64, 16)
    A = P("Am F C G Am Dm E Am")
    mel = "E5:2 C5:1 B4:1 A4:2 C5:1 D5:1 E5:1.5 G5:.5 F5:1 E5:1 D5:4 E5:2 A5:1 G5:1 F5:2 D5:1 C5:1 B4:1 C5:1 D5:1 G#4:1 A4:4"
    s.melody("violin", mel, 0, 0.5)
    s.arp("piano", A, 0, 52, 72, 1, (0, 1, 2, 1), 0.3, pan=0.2, count=3)
    s.bassline("piano", A, 0, 2, 0.35)
    st = s.b(8)
    s.melody("violin", mel, st, 0.5, octave=1)
    s.pad("strings", A, st, 52, 72, 0.3)
    s.bassline("bass", A, st, 2, 0.4)
    s.arp("harp", A, st, 60, 79, 1, (0, 1, 2, 1), 0.25, pan=0.3, count=3)
    return s


def ending_good():
    s = Song(88, 24)
    s.note("cymbal", "C4", 0, 1, 0.8)
    s.note("timp", "D2", 0, 1, 1.0)
    s.melody("brass", THEME_A, 0, 0.8)
    s.melody("strings", THEME_A, 0, 0.5, octave=1, pan=0.2)
    s.pad("strings", THEME_A_CH, 0, 54, 74, 0.4)
    s.pad("choir", THEME_A_CH, 0, 54, 72, 0.35)
    s.bassline("bass", THEME_A_CH, 0, 2, 0.6)
    s.arp("harp", THEME_A_CH, 0, 62, 88, 0.5, vel=0.35, pan=0.4)
    for k in range(8):
        s.note("timp", "D2" if k % 2 == 0 else "A1", s.b(k), 1, 0.5)
    st = s.b(8)
    s.melody("violin", THEME_B, st, 0.5, pan=0.1)
    s.melody("flute", THEME_B, st, 0.3, octave=1, pan=-0.3)
    s.pad("strings", THEME_B_CH, st, 54, 74, 0.35)
    s.bassline("bass", THEME_B_CH, st, 2, 0.5)
    s.arp("harp", THEME_B_CH, st, 62, 86, 0.5, vel=0.3, pan=0.4)
    st = s.b(16)
    s.note("cymbal", "C4", st, 1, 0.9)
    s.melody("brass", THEME_A, st, 0.9, pan=-0.1)
    s.melody("choir", THEME_A, st, 0.4)
    s.melody("strings", THEME_A, st, 0.5, octave=1, pan=0.2)
    s.pad("strings", THEME_A_CH, st, 50, 72, 0.4, key="e2")
    s.bassline("bass", THEME_A_CH, st, 2, 0.7)
    s.bassline("timp", THEME_A_CH, st, 2, 0.6, pattern=[(0, 1, 0), (2, 1, 7)])
    for k in range(8):
        s.drums("T.......T...t...", st + k * 4, 0.25, 1, 0.45)
    return s


def ending_bad():
    s = Song(60, 12)
    ch = P("Dm Bb Gm A Dm Bb Gm_A Dm")
    mel = "A4:1.5 D5:.5 E5:1 F5:1 G5:1.5 F5:.5 E5:1 D5:1 E5:1.5 F5:.5 A5:1 G5:.5 F5:.5 E5:4 -:4 A4:2 D5:2 C#5:2 A4:2 D5:8"
    s.melody("violin", mel, 0, 0.45)
    s.pad("strings", ch, 0, 50, 67, 0.28, count=3)
    s.pad("choir", ch, s.b(4), 50, 64, 0.25, count=3, key="c")
    s.bassline("bass", ch, 0, 2, 0.35)
    for k in range(0, 12, 2):
        s.note("bell", "D4", s.b(k), 4, 0.18, pan=0.3)
    s.note("timp", "D2", s.b(8), 1, 0.4)
    return s


TRACKS = {
    "title": (title, True), "battle": (battle, True), "boss": (boss, True), "final": (final, True),
    "victory": (victory, False), "village": (village, True), "home": (home, True), "forest": (forest, True),
    "night": (night, True), "cathedral": (cathedral, True), "deep": (deep, True), "elves": (elves, True),
    "heart": (heart, True), "sad": (sad, True), "ending_good": (ending_good, True), "ending_bad": (ending_bad, True),
}


def build(out_dir, only=None):
    os.makedirs(out_dir, exist_ok=True)
    for name, (fn, loop) in TRACKS.items():
        if only and name not in only:
            continue
        t0 = time.time()
        s = fn()
        length = s.render(os.path.join(out_dir, name + ".ogg"), loop=loop,
                          reverb_time=3.2 if name in ("cathedral", "heart", "deep", "elves") else 2.3)
        print(f"  {name}: {length:.1f}s ({time.time() - t0:.1f}s)")


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else "../game/assets/music", sys.argv[2:] or None)
