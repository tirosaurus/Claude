"""Efectos de sonido v3: síntesis más realista (golpes, magia, interfaz)."""
import os
import wave
import numpy as np
from scipy import signal
from orch import inst_celesta, inst_harp, inst_bell, inst_brass, inst_strings, hz, reverb

SR = 32000


def write(path, sig, rev=0.0):
    sig = np.asarray(sig, dtype=float)
    if rev > 0:
        st = np.stack([sig, sig], axis=1)
        wet = reverb(st, 1.2)[:, 0][:len(sig) + int(0.8 * SR)]
        out = np.zeros(len(wet))
        out[:len(sig)] += sig
        out += wet * rev
        sig = out
    peak = np.max(np.abs(sig)) + 1e-9
    sig = sig / peak * 0.9
    fade = min(len(sig), int(0.01 * SR))
    sig[-fade:] *= np.linspace(1, 0, fade)
    data = (sig * 32000).astype(np.int16)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())


def t_(sec):
    return np.arange(int(sec * SR)) / SR


def nz(n, seed=0):
    return np.random.default_rng(seed).uniform(-1, 1, n)


def band(x, lo, hi, order=2):
    sos = signal.butter(order, [lo / (SR / 2), min(hi, SR / 2 - 100) / (SR / 2)], "band", output="sos")
    return signal.sosfilt(sos, x)


def lp(x, fc, order=2):
    return signal.sosfilt(signal.butter(order, fc / (SR / 2), "low", output="sos"), x)


def hp(x, fc, order=2):
    return signal.sosfilt(signal.butter(order, fc / (SR / 2), "high", output="sos"), x)


def sweep_noise(sec, f0, f1, q=0.3, seed=1):
    """Ruido filtrado con centro que barre de f0 a f1 (whoosh)."""
    n = int(sec * SR)
    x = nz(n, seed)
    out = np.zeros(n)
    blocks = 24
    for i in range(blocks):
        a, b = i * n // blocks, (i + 1) * n // blocks
        fc = f0 * (f1 / f0) ** (i / (blocks - 1))
        seg = band(x[max(0, a - 400):b], fc * (1 - q), fc * (1 + q))[-(b - a):]
        out[a:b] = seg
    return out


def env_ar(n, a, r):
    t = np.arange(n) / SR
    e = np.minimum(1, t / max(a, 1e-4)) * np.exp(-np.maximum(0, t - a) / max(r, 1e-4))
    return e


def mixn(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[:len(p)] += p
    return out


def at(sig, sec, total=None):
    pad = np.zeros(int(sec * SR))
    return np.concatenate([pad, sig])


# ------------------------------------------------------------------ Combate
def hit():
    w = sweep_noise(0.12, 600, 3500, 0.5, 2) * env_ar(int(0.12 * SR), 0.06, 0.03) * 0.5
    t = t_(0.35)
    thud = np.sin(2 * np.pi * (90 + 120 * np.exp(-t * 40)) * t) * np.exp(-t * 16)
    crack = band(nz(len(t), 3), 1500, 6000) * np.exp(-t * 45) * 1.2
    return mixn(w, at(thud * 0.9 + crack, 0.08))


def crit():
    t = t_(0.8)
    base = hit()
    ring = (np.sin(2 * np.pi * 1720 * t) + 0.6 * np.sin(2 * np.pi * 2610 * t)) * np.exp(-t * 6) * 0.35
    boom = np.sin(2 * np.pi * (55 + 80 * np.exp(-t * 20)) * t) * np.exp(-t * 7)
    return mixn(base, at(ring + boom * 0.8, 0.08))


def defend():
    t = t_(0.9)
    clang = sum(np.sin(2 * np.pi * f * t) * np.exp(-t * d) * a for f, d, a in
                ((620, 5, 1), (1480, 7, 0.6), (2310, 9, 0.4), (3900, 14, 0.25)))
    tick = band(nz(len(t), 5), 2000, 8000) * np.exp(-t * 80)
    return clang * 0.6 + tick


def heal():
    parts = []
    for i, m in enumerate((72, 76, 79, 84, 88)):
        parts.append(at(inst_celesta(m, 0.3, 0.8), i * 0.07))
    t = t_(1.0)
    shimmer = hp(nz(len(t), 7), 6000) * np.exp(-t * 4) * 0.12
    return mixn(*parts, shimmer)


def magic():
    parts = [at(inst_bell(m, 0.2, 0.6), i * 0.05) for i, m in enumerate((79, 86, 91))]
    sw = sweep_noise(0.5, 800, 6000, 0.25, 9) * env_ar(int(0.5 * SR), 0.3, 0.15) * 0.4
    return mixn(sw, *parts)


def fire():
    n = int(0.9 * SR)
    t = np.arange(n) / SR
    roar = lp(nz(n, 11), 1400) * env_ar(n, 0.08, 0.35) * 1.4
    crackle = np.zeros(n)
    rng = np.random.default_rng(12)
    for _ in range(30):
        k = rng.integers(0, n - 400)
        crackle[k:k + 200] += band(nz(200, int(k)), 2000, 7000) * np.exp(-np.arange(200) / 30) * rng.uniform(0.3, 1)
    whoosh = sweep_noise(0.5, 300, 2400, 0.4, 13) * env_ar(int(0.5 * SR), 0.2, 0.15)
    return mixn(roar, crackle * 0.6, whoosh * 0.6)


def ice():
    t = t_(0.8)
    crack = band(nz(len(t), 21), 1500, 9000) * np.exp(-t * 30)
    parts = [crack]
    rng = np.random.default_rng(22)
    for i in range(7):
        f = rng.uniform(2500, 5200)
        tt = t_(0.4)
        parts.append(at(np.sin(2 * np.pi * f * tt) * np.exp(-tt * 14) * 0.3, 0.05 + i * 0.05))
    return mixn(*parts)


def bolt():
    n = int(1.2 * SR)
    t = np.arange(n) / SR
    zap = np.sign(np.sin(2 * np.pi * (80 + 40 * nz(n, 31)) * t)) * band(nz(n, 32), 800, 8000) * np.exp(-t * 12)
    thunder = lp(nz(n, 33), 300) * env_ar(n, 0.05, 0.5) * 2.5
    snap = hp(nz(int(0.03 * SR), 34), 3000)
    return mixn(snap * 1.5, zap * 0.8, at(thunder, 0.05))


def dark():
    n = int(1.0 * SR)
    t = np.arange(n) / SR
    drone = sum(np.sin(2 * np.pi * f * t * (1 - 0.3 * t)) for f in (70, 104, 139)) * env_ar(n, 0.5, 0.3)
    hiss = band(nz(n, 41), 300, 1500) * env_ar(n, 0.6, 0.2) * 0.6
    return drone * 0.5 + hiss


def growl():
    n = int(0.9 * SR)
    t = np.arange(n) / SR
    f = 80 + 20 * np.sin(2 * np.pi * 7 * t)
    buzz = signal.sawtooth(2 * np.pi * np.cumsum(f) / SR)
    g = band(buzz * (1 + 0.5 * nz(n, 51)), 150, 1200) * env_ar(n, 0.12, 0.4)
    return g


def battle_start():
    riser = sweep_noise(0.55, 400, 7000, 0.3, 61) * np.linspace(0.2, 1, int(0.55 * SR)) ** 2
    t = t_(1.0)
    boom = np.sin(2 * np.pi * (48 + 90 * np.exp(-t * 25)) * t) * np.exp(-t * 4)
    stab = sum(inst_brass(m, 0.35, 0.9) for m in (52, 59, 64))
    return mixn(riser * 0.8, at(boom * 1.2, 0.55), at(stab * 0.35, 0.55))


def levelup():
    parts = []
    for i, m in enumerate((67, 72, 76, 79, 84)):
        parts.append(at(inst_harp(m, 0.3, 0.8), i * 0.06))
    parts.append(at(sum(inst_brass(m, 0.7, 0.8) for m in (72, 76, 79)) * 0.35, 0.32))
    parts.append(at(inst_bell(96, 0.5, 0.5), 0.32))
    return mixn(*parts)


def flee():
    parts = []
    for i in range(5):
        tt = t_(0.08)
        parts.append(at(lp(nz(len(tt), 70 + i), 900) * np.exp(-tt * 50), i * 0.09))
    parts.append(sweep_noise(0.5, 2000, 400, 0.4, 75) * env_ar(int(0.5 * SR), 0.1, 0.2) * 0.5)
    return mixn(*parts)


# ------------------------------------------------------------------ Interfaz / mundo
def select():
    t = t_(0.08)
    return np.sin(2 * np.pi * 1320 * t) * np.exp(-t * 60) * 0.6 + np.sin(2 * np.pi * 2640 * t) * np.exp(-t * 90) * 0.2


def confirm():
    return mixn(inst_celesta(84, 0.1, 0.7), at(inst_celesta(91, 0.1, 0.6), 0.05))


def cancel():
    return mixn(inst_celesta(79, 0.1, 0.6), at(inst_celesta(72, 0.1, 0.6), 0.05))


def buy():
    parts = []
    rng = np.random.default_rng(81)
    for i in range(4):
        tt = t_(0.25)
        f = rng.uniform(3200, 4800)
        parts.append(at((np.sin(2 * np.pi * f * tt) + 0.5 * np.sin(2 * np.pi * f * 1.5 * tt)) * np.exp(-tt * 18), i * 0.05))
    return mixn(*parts)


def chest():
    t = t_(0.3)
    creak = band(signal.sawtooth(2 * np.pi * np.cumsum(180 + 60 * np.sin(2 * np.pi * 3 * t)) / SR), 300, 2000) \
        * env_ar(len(t), 0.05, 0.2) * 0.5
    return mixn(creak, at(levelup() * 0.6, 0.2))


def save():
    return mixn(*[at(inst_bell(m, 0.4, 0.6), i * 0.12) for i, m in enumerate((76, 83, 88))])


def door():
    n = int(0.6 * SR)
    t = np.arange(n) / SR
    creak = band(signal.sawtooth(2 * np.pi * np.cumsum(140 + 90 * t) / SR), 250, 1800) * env_ar(n, 0.1, 0.25) * 0.4
    thud = np.sin(2 * np.pi * 70 * t) * np.exp(-t * 14)
    return mixn(creak, at(thud[:int(0.3 * SR)], 0.35))


def build_all(out):
    d = f"{out}/sfx"
    for name, fn, rev in (("hit", hit, 0.1), ("crit", crit, 0.2), ("defend", defend, 0.2), ("heal", heal, 0.35),
                          ("magic", magic, 0.35), ("fire", fire, 0.15), ("ice", ice, 0.3), ("bolt", bolt, 0.2),
                          ("dark", dark, 0.3), ("growl", growl, 0.2), ("battle_start", battle_start, 0.25),
                          ("levelup", levelup, 0.35), ("flee", flee, 0.1), ("select", select, 0.0),
                          ("confirm", confirm, 0.1), ("cancel", cancel, 0.1), ("buy", buy, 0.15),
                          ("chest", chest, 0.2), ("save", save, 0.4), ("door", door, 0.1)):
        write(f"{d}/{name}.wav", fn(), rev)
