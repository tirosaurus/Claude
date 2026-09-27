"""Música y efectos chiptune generados por síntesis (WAV 22050 Hz mono 16-bit)."""
import os
import wave
import numpy as np

SR = 22050
NOTES = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6,
         "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11}


def freq(name):
    if name in ("-", "r"):
        return 0.0
    n, o = name[:-1], int(name[-1])
    midi = 12 * (o + 1) + NOTES[n]
    return 440.0 * 2 ** ((midi - 69) / 12)


def osc(kind, f, n, duty=0.5):
    t = np.arange(n) / SR
    if f <= 0:
        return np.zeros(n)
    ph = (t * f) % 1.0
    if kind == "square":
        return np.where(ph < duty, 1.0, -1.0)
    if kind == "tri":
        return 4 * np.abs(ph - 0.5) - 1
    if kind == "saw":
        return 2 * ph - 1
    if kind == "sine":
        return np.sin(2 * np.pi * ph)
    if kind == "noise":
        return np.random.default_rng(int(f)).uniform(-1, 1, n)
    raise ValueError(kind)


def env(n, a=0.005, d=0.08, s=0.6, r=0.05):
    e = np.ones(n) * s
    na, nd, nr = int(a * SR), int(d * SR), int(r * SR)
    na = min(na, n)
    e[:na] = np.linspace(0, 1, na) if na else e[:na]
    nd = min(nd, n - na)
    e[na:na + nd] = np.linspace(1, s, nd) if nd else e[na:na + nd]
    nr = min(nr, n)
    if nr:
        e[-nr:] *= np.linspace(1, 0, nr)
    return e


def pluck_env(n, decay=6.0):
    t = np.arange(n) / SR
    e = np.exp(-t * decay)
    rel = min(n, int(0.01 * SR))
    e[-rel:] *= np.linspace(1, 0, rel)
    return e


def seq(pattern, bpm, voice, vol=0.3, total_beats=None):
    """pattern: 'D4:1 F4:.5 -:.5'  (nota:beats)"""
    beat = 60.0 / bpm
    out = []
    for tok in pattern.split():
        name, dur = tok.split(":")
        n = int(float(dur) * beat * SR)
        out.append(voice(freq(name), n) * vol)
    sig = np.concatenate(out) if out else np.zeros(1)
    if total_beats:
        need = int(total_beats * beat * SR)
        if len(sig) < need:
            sig = np.concatenate([sig, np.zeros(need - len(sig))])
        sig = sig[:need]
    return sig


# --- Voces ---------------------------------------------------------------------
def lute(f, n):
    return (osc("tri", f, n) * 0.7 + osc("square", f, n, 0.25) * 0.3) * pluck_env(n, 5.0)


def lead(f, n):
    vib = 1.0
    return osc("square", f * vib, n, 0.25) * env(n, 0.01, 0.1, 0.55, 0.04)


def soft_lead(f, n):
    return (osc("tri", f, n) * 0.8 + osc("square", f, n, 0.125) * 0.2) * env(n, 0.03, 0.2, 0.6, 0.08)


def bass(f, n):
    return osc("tri", f, n) * env(n, 0.005, 0.05, 0.8, 0.03)


def pluck_bass(f, n):
    return osc("tri", f, n) * pluck_env(n, 3.5)


def bell(f, n):
    return (osc("sine", f, n) * 0.7 + osc("sine", f * 2.01, n) * 0.3) * pluck_env(n, 2.2)


def pad(f, n):
    return (osc("tri", f, n) + osc("tri", f * 1.004, n)) * 0.5 * env(n, 0.3, 0.2, 0.8, 0.3)


def drum_kick(n):
    t = np.arange(n) / SR
    f = 120 * np.exp(-t * 30) + 40
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 18)


def drum_snare(n):
    t = np.arange(n) / SR
    return np.random.default_rng(5).uniform(-1, 1, n) * np.exp(-t * 28)


def drum_hat(n):
    t = np.arange(n) / SR
    return np.random.default_rng(7).uniform(-1, 1, n) * np.exp(-t * 90) * 0.5


def drums(pattern, bpm, vol=0.25):
    """pattern de 16avos: k=bombo s=caja h=charles .=silencio"""
    step = int(60.0 / bpm / 4 * SR)
    out = np.zeros(step * len(pattern))
    for i, ch in enumerate(pattern):
        if ch == ".":
            continue
        n = step * 2
        s = {"k": drum_kick, "s": drum_snare, "h": drum_hat}[ch](n)
        a = i * step
        b = min(len(out), a + n)
        out[a:b] += s[:b - a]
    return out * vol


def mix_tracks(*tracks):
    n = max(len(t) for t in tracks)
    out = np.zeros(n)
    for t in tracks:
        out[:len(t)] += t
    peak = np.max(np.abs(out)) or 1
    return out / peak * 0.85


def write(path, sig):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    data = (np.clip(sig, -1, 1) * 32000).astype(np.int16)
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())


def rep(s, k):
    return " ".join([s] * k)


# --- Composiciones -------------------------------------------------------------
def track_home():
    bpm = 84
    mel = ("D5:1 E5:.5 F5:.5 A5:1 G5:.5 F5:.5 E5:1.5 D5:.5 C5:1 D5:1 "
           "F5:1 E5:.5 D5:.5 C5:1 A4:1 D5:2 -:2 "
           "A5:1 G5:.5 F5:.5 G5:1 A5:.5 G5:.5 F5:1.5 E5:.5 D5:1 E5:1 "
           "F5:1 E5:.5 C5:.5 D5:1.5 E5:.5 D5:2 -:2")
    arp = rep("D4:.5 A4:.5 D5:.5 A4:.5", 4) + " " + rep("C4:.5 G4:.5 C5:.5 G4:.5", 2) + " " + \
        rep("D4:.5 A4:.5 D5:.5 A4:.5", 2) + " " + rep("Bb3:.5 F4:.5 Bb4:.5 F4:.5", 2) + " " + \
        rep("C4:.5 G4:.5 C5:.5 G4:.5", 2) + " " + rep("D4:.5 A4:.5 D5:.5 A4:.5", 4)
    bs = "D3:4 C3:2 D3:2 Bb2:2 C3:2 D3:4 " * 1 + "D3:4 C3:2 D3:2 Bb2:2 C3:2 D3:4"
    total = 32
    return mix_tracks(seq(mel, bpm, soft_lead, 0.32, total), seq(arp, bpm, lute, 0.22, total),
                      seq(bs, bpm, pluck_bass, 0.35, total))


def track_village():
    bpm = 112
    mel = ("G5:.5 A5:.5 B5:1 A5:.5 G5:.5 E5:1 D5:.5 E5:.5 G5:1 A5:1 B5:.5 A5:.5 "
           "G5:1 F5:.5 G5:.5 A5:1 D5:1 G5:.5 A5:.5 B5:.5 C6:.5 B5:1 A5:1 "
           "B5:.5 C6:.5 D6:1 C6:.5 B5:.5 A5:1 G5:.5 A5:.5 B5:1 G5:1 E5:.5 F5:.5 "
           "G5:1 A5:.5 F5:.5 G5:1.5 D5:.5 G5:2 -:1")
    arp = rep("G4:.5 B4:.5 D5:.5 B4:.5", 2) + " " + rep("C4:.5 E4:.5 G4:.5 E4:.5", 2) + " " + \
        rep("F4:.5 A4:.5 C5:.5 A4:.5", 2) + " " + rep("D4:.5 F#4:.5 A4:.5 F#4:.5", 2) + " " + \
        rep("G4:.5 B4:.5 D5:.5 B4:.5", 2) + " " + rep("C4:.5 E4:.5 G4:.5 E4:.5", 2) + " " + \
        rep("F4:.5 A4:.5 C5:.5 A4:.5", 1) + " " + rep("D4:.5 F#4:.5 A4:.5 F#4:.5", 1) + " " + \
        rep("G4:.5 B4:.5 D5:.5 B4:.5", 2)
    bs = "G2:1 D3:1 G2:1 D3:1 C3:1 G3:1 C3:1 G3:1 F2:1 C3:1 F2:1 C3:1 D3:1 A3:1 D3:1 A2:1 " \
         "G2:1 D3:1 G2:1 D3:1 C3:1 G3:1 C3:1 G3:1 F2:1 C3:1 D3:1 A2:1 G2:1 D3:1 G2:2"
    total = 32
    dr = np.concatenate([drums("k...h...s...h...", bpm, 0.18)] * 8)
    return mix_tracks(seq(mel, bpm, lead, 0.2, total), seq(arp, bpm, lute, 0.2, total),
                      seq(bs, bpm, pluck_bass, 0.34, total), dr)


def track_forest():
    bpm = 66
    bells = ("A5:1 E5:1 C6:1 B5:1 A5:2 E5:2 G5:1 D5:1 B5:1 A5:1 G5:2 -:2 "
             "F5:1 C5:1 A5:1 G5:1 F5:2 E5:2 E5:1 B4:1 G#5:1 E5:1 A5:4")
    padl = "A3:4 E3:4 G3:4 D3:4 F3:4 C3:4 E3:4 A3:4"
    total = 32
    return mix_tracks(seq(bells, bpm, bell, 0.3, total), seq(padl, bpm, pad, 0.22, total),
                      seq("A2:8 G2:8 F2:8 E2:8", bpm, bass, 0.18, total))


def track_battle():
    bpm = 150
    mel = ("E5:.5 E5:.5 G5:.5 E5:.5 A5:.5 G5:.5 E5:.5 D5:.5 E5:1 B4:1 D5:1 E5:1 "
           "G5:.5 G5:.5 A5:.5 G5:.5 B5:.5 A5:.5 G5:.5 F#5:.5 G5:1 D5:1 F#5:2 "
           "E5:.5 E5:.5 G5:.5 E5:.5 A5:.5 G5:.5 E5:.5 D5:.5 C5:1 D5:1 E5:1 G5:1 "
           "F#5:.5 G5:.5 F#5:.5 E5:.5 D5:.5 E5:.5 F#5:.5 D5:.5 E5:2 B4:2")
    bs = rep("E2:.5 E3:.5", 8) + " " + rep("G2:.5 G3:.5", 4) + " " + rep("D2:.5 D3:.5", 4) + " " + \
        rep("C2:.5 C3:.5", 4) + " " + rep("D2:.5 D3:.5", 4) + " " + rep("E2:.5 E3:.5", 4) + " " + \
        rep("B1:.5 B2:.5", 4)
    total = 32
    dr = np.concatenate([drums("k.h.s.h.k.k.s.hh", bpm, 0.22)] * 8)
    return mix_tracks(seq(mel, bpm, lead, 0.24, total), seq(bs, bpm, bass, 0.32, total), dr)


def track_victory():
    bpm = 132
    mel = "C5:.5 E5:.5 G5:.5 C6:1.5 A5:.5 B5:.5 C6:3"
    harm = "E4:.5 G4:.5 C5:.5 E5:1.5 F4:.5 G4:.5 E5:3"
    return mix_tracks(seq(mel, bpm, lead, 0.3), seq(harm, bpm, soft_lead, 0.22),
                      seq("C3:2 F3:1 G3:1 C3:3", bpm, bass, 0.3))


def track_title():
    bpm = 76
    mel = ("D5:2 F5:1 E5:1 D5:2 A4:2 Bb4:2 C5:1 D5:1 A4:4 "
           "D5:2 F5:1 G5:1 A5:2 G5:1 F5:1 E5:2 C5:1 E5:1 D5:4")
    padl = "D3:4 D3:4 Bb2:4 A2:4 D3:4 F3:4 C3:4 D3:4"
    arp = rep("D4:.5 F4:.5 A4:.5 F4:.5", 4) + " " + rep("Bb3:.5 D4:.5 F4:.5 D4:.5", 2) + " " + \
        rep("A3:.5 C#4:.5 E4:.5 C#4:.5", 2) + " " + rep("D4:.5 F4:.5 A4:.5 F4:.5", 2) + " " + \
        rep("F3:.5 A3:.5 C4:.5 A3:.5", 2) + " " + rep("C4:.5 E4:.5 G4:.5 E4:.5", 2) + " " + \
        rep("D4:.5 F4:.5 A4:.5 F4:.5", 2)
    total = 32
    return mix_tracks(seq(mel, bpm, soft_lead, 0.3, total), seq(padl, bpm, pad, 0.24, total),
                      seq(arp, bpm, lute, 0.16, total))


# --- Efectos -------------------------------------------------------------------
def sfx_blip():
    n = int(0.03 * SR)
    return osc("square", 880, n, 0.5) * env(n, 0.001, 0.01, 0.5, 0.01) * 0.35


def sfx_select():
    return np.concatenate([osc("square", 660, 900, 0.25) * 0.3, osc("square", 990, 1400, 0.25) * env(1400) * 0.3])


def sfx_confirm():
    a = osc("square", 784, 1200, 0.25) * 0.3
    b = osc("square", 1175, 3000, 0.25) * pluck_env(3000, 10) * 0.3
    return np.concatenate([a, b])


def sfx_door():
    n = int(0.35 * SR)
    t = np.arange(n) / SR
    thud = np.sin(2 * np.pi * np.cumsum(90 * np.exp(-t * 8) + 40) / SR) * np.exp(-t * 12)
    creak = osc("saw", 1, n) * 0
    creak_n = int(0.25 * SR)
    tt = np.arange(creak_n) / SR
    f = 300 + 80 * np.sin(tt * 40)
    cr = np.sign(np.sin(2 * np.pi * np.cumsum(f) / SR)) * 0.12 * np.exp(-tt * 6)
    out = thud * 0.8
    out[:creak_n] += cr
    return out


def sfx_hit():
    n = int(0.18 * SR)
    t = np.arange(n) / SR
    noise = np.random.default_rng(3).uniform(-1, 1, n) * np.exp(-t * 25)
    tone = np.sin(2 * np.pi * np.cumsum(400 * np.exp(-t * 20) + 60) / SR) * np.exp(-t * 15)
    return (noise * 0.6 + tone * 0.6) * 0.6


def sfx_crit():
    return np.concatenate([sfx_hit(), sfx_hit()[: int(0.1 * SR)] * 0.7])


def sfx_heal():
    out = []
    for f in (523, 659, 784, 1047):
        n = int(0.07 * SR)
        out.append((osc("tri", f, n) * 0.7 + osc("sine", f * 2, n) * 0.3) * env(n, 0.005, 0.03, 0.6, 0.02) * 0.4)
    tail = int(0.3 * SR)
    out.append(bell(1319, tail) * 0.25)
    return np.concatenate(out)


def sfx_defend():
    n = int(0.25 * SR)
    return (osc("square", 220, n, 0.5) * 0.3 + osc("square", 330, n, 0.5) * 0.2) * pluck_env(n, 12)


def sfx_levelup():
    out = []
    for f in (523, 659, 784, 1047, 784, 1047, 1319):
        n = int(0.08 * SR)
        out.append(osc("square", f, n, 0.25) * env(n, 0.002, 0.03, 0.7, 0.02) * 0.3)
    n = int(0.5 * SR)
    out.append(osc("square", 1568, n, 0.25) * pluck_env(n, 4) * 0.3)
    return np.concatenate(out)


def sfx_growl():
    n = int(0.9 * SR)
    t = np.arange(n) / SR
    base = 70 + 15 * np.sin(t * 23)
    tone = np.sign(np.sin(2 * np.pi * np.cumsum(base) / SR)) * 0.3
    rumble = np.random.default_rng(9).uniform(-1, 1, n)
    k = 60
    rumble = np.convolve(rumble, np.ones(k) / k, mode="same") * 3
    e = env(n, 0.1, 0.2, 0.8, 0.3) * (0.6 + 0.4 * np.sin(t * 17))
    return (tone + rumble) * e * 0.6


def sfx_battle_start():
    n = int(0.6 * SR)
    t = np.arange(n) / SR
    sweep = np.sin(2 * np.pi * np.cumsum(200 + 1400 * t) / SR) * 0.3
    noise = np.random.default_rng(1).uniform(-1, 1, n) * 0.25
    return (sweep + noise) * env(n, 0.01, 0.1, 0.8, 0.3)


def sfx_cancel():
    return np.concatenate([osc("square", 440, 900, 0.25) * 0.25, osc("square", 330, 1500, 0.25) * env(1500) * 0.25])


def sfx_step():
    n = int(0.04 * SR)
    t = np.arange(n) / SR
    return np.random.default_rng(2).uniform(-1, 1, n) * np.exp(-t * 80) * 0.15


def build_all(out):
    m = f"{out}/music"
    write(f"{m}/home.wav", track_home())
    write(f"{m}/village.wav", track_village())
    write(f"{m}/forest.wav", track_forest())
    write(f"{m}/battle.wav", track_battle())
    write(f"{m}/victory.wav", track_victory())
    write(f"{m}/title.wav", track_title())
    s = f"{out}/sfx"
    for name, fn in (("blip", sfx_blip), ("select", sfx_select), ("confirm", sfx_confirm), ("door", sfx_door),
                     ("hit", sfx_hit), ("crit", sfx_crit), ("heal", sfx_heal), ("defend", sfx_defend),
                     ("levelup", sfx_levelup), ("growl", sfx_growl), ("battle_start", sfx_battle_start),
                     ("cancel", sfx_cancel), ("step", sfx_step)):
        write(f"{s}/{name}.wav", fn())
