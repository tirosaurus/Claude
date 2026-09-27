"""Motor de música orquestal sintetizada (estéreo, OGG).

Cuerdas en ensemble, metales con brillo dinámico, coro con formantes,
arpa, flauta, celesta, órgano, timbales, taikos, platos y reverb de sala.
Las canciones se escriben con una pequeña notación (ver songs.py).
"""
import numpy as np
from scipy import signal
import soundfile as sf

SR = 32000
NOTE_IDX = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6,
            "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11, "Cb": 11, "E#": 5, "B#": 0}


def midi(name):
    if isinstance(name, (int, float)):
        return name
    n, o = name[:-1], int(name[-1])
    return 12 * (o + 1) + NOTE_IDX[n]


def hz(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


# ------------------------------------------------------------------ Osciladores
TABLE_N = 2048
_tables = {}


def _make_table(kind, harmonics):
    ph = np.arange(TABLE_N) / TABLE_N
    out = np.zeros(TABLE_N)
    for k in range(1, harmonics + 1):
        if kind == "saw":
            a = 1.0 / k
        elif kind == "square":
            a = (1.0 / k) if k % 2 else 0.0
        elif kind == "soft":      # diente de sierra con caída rápida (cuerda/metal suave)
            a = 1.0 / (k ** 1.6)
        elif kind == "reed":
            a = (1.0 / k) * (1.0 if k % 2 else 0.45)
        else:
            raise ValueError(kind)
        out += a * np.sin(2 * np.pi * k * ph)
    return out / np.max(np.abs(out))


def table(kind, fmax):
    harm = max(1, min(60, int((SR / 2 * 0.9) / max(fmax, 1))))
    # cuantizar para cachear (nunca por encima de Nyquist)
    harm = [x for x in (1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 45, 60) if x <= harm][-1]
    key = (kind, harm)
    if key not in _tables:
        _tables[key] = _make_table(kind, harm)
    return _tables[key]


def wt_osc(kind, f_arr, phase0=0.0):
    """Oscilador de tabla con frecuencia variable (array)."""
    tab = table(kind, float(np.max(f_arr)))
    ph = (phase0 + np.cumsum(f_arr) / SR) % 1.0
    idx = ph * TABLE_N
    i0 = idx.astype(np.int64)
    fr = idx - i0
    return tab[i0 % TABLE_N] * (1 - fr) + tab[(i0 + 1) % TABLE_N] * fr


def sine(f_arr, phase0=0.0):
    return np.sin(2 * np.pi * (phase0 + np.cumsum(f_arr) / SR))


def adsr(n, a, d, s, r, sustain_n=None):
    """Envolvente; la nota suena n muestras (incluye la liberación)."""
    e = np.zeros(n)
    na, nd, nr = int(a * SR), int(d * SR), int(r * SR)
    hold = n - nr if sustain_n is None else sustain_n
    hold = max(1, min(hold, n))
    t = 0
    k = min(na, hold)
    if k > 0:
        e[:k] = np.linspace(0, 1, k, endpoint=False) ** 1.2
    t = k
    k2 = min(nd, hold - t)
    if k2 > 0:
        e[t:t + k2] = np.linspace(1, s, k2)
    t += k2
    if hold > t:
        e[t:hold] = s if na + nd > 0 else 1.0
    last = e[hold - 1] if hold > 0 else 0
    rel = n - hold
    if rel > 0:
        e[hold:] = last * np.exp(-np.linspace(0, 6, rel))
    return e


_rng = np.random.default_rng(1234)


def noise(n, seed=None):
    r = np.random.default_rng(seed) if seed is not None else _rng
    return r.uniform(-1, 1, n)


def vibrato(n, f, depth_cents=12, rate=5.4, delay=0.25):
    t = np.arange(n) / SR
    ramp = np.clip((t - delay) / 0.4, 0, 1)
    return f * 2 ** (depth_cents * ramp * np.sin(2 * np.pi * rate * t + _rng.uniform(0, 6)) / 1200.0)


# ------------------------------------------------------------------ Instrumentos
# Cada instrumento: f(midi, dur_s, vel) -> señal mono (con cola)

def inst_strings(m, dur, vel):
    rel = 0.45
    n = int((dur + rel) * SR)
    f = hz(m)
    out = np.zeros(n)
    for det in (-9, -3, 4, 10):
        fv = vibrato(n, f * 2 ** (det / 1200), 10, 5.2 + det * 0.03, 0.2)
        out += wt_osc("saw", fv, _rng.uniform())
    out /= 4
    att = 0.18 if dur > 0.4 else 0.06
    return out * adsr(n, att, 0.2, 0.85, rel, int(dur * SR)) * vel


def inst_strings_short(m, dur, vel):
    """Spiccato / ostinato."""
    rel = 0.12
    n = int((min(dur, 0.35) + rel) * SR)
    f = hz(m)
    out = np.zeros(n)
    for det in (-7, 0, 7):
        out += wt_osc("saw", np.full(n, f * 2 ** (det / 1200)), _rng.uniform())
    out /= 3
    return out * adsr(n, 0.008, 0.09, 0.45, rel, int(min(dur, 0.35) * SR)) * vel


def inst_pizz(m, dur, vel):
    n = int(0.7 * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = sine(np.full(n, f)) * 0.7 + sine(np.full(n, f * 2)) * 0.25 * np.exp(-t * 12) + wt_osc("soft", np.full(n, f)) * 0.3
    return out * np.exp(-t * 7) * np.minimum(1, t * 400) * vel


def inst_violin_solo(m, dur, vel):
    rel = 0.3
    n = int((dur + rel) * SR)
    fv = vibrato(n, hz(m), 22, 5.6, 0.18)
    out = wt_osc("saw", fv) * 0.75 + wt_osc("reed", fv * 1.002) * 0.25
    return out * adsr(n, 0.09, 0.2, 0.9, rel, int(dur * SR)) * vel


def inst_brass(m, dur, vel, soft=False):
    rel = 0.25
    n = int((dur + rel) * SR)
    f = hz(m)
    t = np.arange(n) / SR
    scoop = f * 2 ** (-35 * np.exp(-t * 30) / 1200)
    fv = vibrato(n, 1, 8, 5.0, 0.35) * scoop
    bright_t = 0.12 if not soft else 0.25
    br = np.clip(t / bright_t, 0, 1) * (0.55 + 0.45 * vel)
    if soft:
        br *= 0.55
    a = wt_osc("soft", fv)
    b = wt_osc("saw", fv * 1.0015)
    out = a * (1 - br) + b * br
    return out * adsr(n, 0.04 if not soft else 0.1, 0.15, 0.8, rel, int(dur * SR)) * vel


def inst_horn(m, dur, vel):
    return inst_brass(m, dur, vel, soft=True)


def inst_brass_stab(m, dur, vel):
    return inst_brass(m, min(dur, 0.22), vel)


def inst_choir(m, dur, vel):
    rel = 0.6
    n = int((dur + rel) * SR)
    f = hz(m)
    out = np.zeros(n)
    for det in (-12, -4, 5, 13):
        fv = vibrato(n, f * 2 ** (det / 1200), 14, 4.8 + det * 0.02, 0.3)
        out += wt_osc("saw", fv, _rng.uniform())
    out /= 4
    return out * adsr(n, 0.35, 0.3, 0.85, rel, int(dur * SR)) * vel


def inst_flute(m, dur, vel):
    rel = 0.18
    n = int((dur + rel) * SR)
    fv = vibrato(n, hz(m), 14, 5.0, 0.22)
    out = sine(fv) + 0.18 * sine(fv * 2) + 0.06 * sine(fv * 3)
    breath = signal.lfilter([0.05], [1, -0.95], noise(n)) * 0.6
    return (out + breath) * adsr(n, 0.06, 0.1, 0.85, rel, int(dur * SR)) * vel


def inst_oboe(m, dur, vel):
    rel = 0.15
    n = int((dur + rel) * SR)
    fv = vibrato(n, hz(m), 12, 5.3, 0.2)
    return wt_osc("reed", fv) * adsr(n, 0.04, 0.1, 0.85, rel, int(dur * SR)) * vel


def inst_harp(m, dur, vel):
    n = int(2.2 * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for k in range(1, 9):
        if f * k > SR / 2 * 0.9:
            break
        out += np.sin(2 * np.pi * f * k * t + k) * np.exp(-t * (1.6 + k * 1.1)) / (k ** 1.3)
    return out * np.minimum(1, t * 600) * vel


def inst_celesta(m, dur, vel):
    n = int(2.0 * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = np.sin(2 * np.pi * f * t) * np.exp(-t * 2.4) + 0.35 * np.sin(2 * np.pi * f * 4.0 * t) * np.exp(-t * 7) \
        + 0.12 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t * 5)
    return out * np.minimum(1, t * 800) * vel


def inst_bell(m, dur, vel):
    n = int(4.0 * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for ratio, amp, dec in ((0.5, 0.5, 0.8), (1.0, 1.0, 1.1), (1.19, 0.4, 1.6), (1.56, 0.3, 2.0), (2.0, 0.35, 2.2),
                            (2.74, 0.2, 3.0), (3.76, 0.1, 4.0)):
        if f * ratio < SR / 2:
            out += amp * np.sin(2 * np.pi * f * ratio * t) * np.exp(-t * dec)
    return out * np.minimum(1, t * 800) * vel * 0.6


def inst_organ(m, dur, vel):
    rel = 0.35
    n = int((dur + rel) * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for k, a in ((0.5, 0.6), (1, 1.0), (2, 0.6), (3, 0.3), (4, 0.35), (6, 0.12), (8, 0.15)):
        if f * k < SR / 2 * 0.9:
            out += a * np.sin(2 * np.pi * f * k * t * (1 + 0.0008 * np.sin(2 * np.pi * 5.8 * t)))
    return out / 2.5 * adsr(n, 0.05, 0.1, 1.0, rel, int(dur * SR)) * vel


def inst_piano(m, dur, vel):
    n = int(min(dur + 1.2, 3.5) * SR)
    f = hz(m)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for k in range(1, 10):
        fk = f * k * (1 + 0.0004 * k * k)
        if fk > SR / 2 * 0.9:
            break
        out += np.sin(2 * np.pi * fk * t) * np.exp(-t * (0.9 + 0.5 * k)) / k ** (1.4 - 0.4 * vel)
    held = int(dur * SR)
    if held < n:
        out[held:] *= np.exp(-np.arange(n - held) / SR * 8)
    return out * np.minimum(1, t * 900) * vel


def inst_bass(m, dur, vel):
    """Contrabajos arco."""
    rel = 0.25
    n = int((dur + rel) * SR)
    f = hz(m)
    fv = vibrato(n, f, 6, 5, 0.3)
    out = wt_osc("soft", fv) * 0.6 + wt_osc("saw", fv * 1.003) * 0.4
    return out * adsr(n, 0.05, 0.2, 0.85, rel, int(dur * SR)) * vel


# ------------------------------------------------------------------ Percusión
def perc_timpani(m, dur, vel):
    n = int(2.2 * SR)
    f = hz(m)
    t = np.arange(n) / SR
    fv = f * (1 + 0.04 * np.exp(-t * 20))
    body = sine(fv) + 0.5 * sine(fv * 1.5) * np.exp(-t * 3) + 0.25 * sine(fv * 1.98) * np.exp(-t * 4)
    hit = signal.lfilter([0.15], [1, -0.85], noise(n)) * np.exp(-t * 40) * 2
    return (body * np.exp(-t * 2.2) + hit) * vel


def perc_timp_roll(m, dur, vel):
    n = int((dur + 1.5) * SR)
    t = np.arange(n) / SR
    out = np.zeros(n)
    step = int(0.055 * SR)
    for i in range(0, int(dur * SR), step):
        h = perc_timpani(m, 0.1, 1.0)
        amp = 0.25 + 0.75 * (i / max(1, dur * SR)) ** 1.5
        end = min(n, i + len(h))
        out[i:end] += h[:end - i] * amp * 0.4
    return out * vel


def perc_taiko(m, dur, vel):
    n = int(1.2 * SR)
    t = np.arange(n) / SR
    fv = 52 + 70 * np.exp(-t * 28)
    body = sine(fv) * np.exp(-t * 5.5)
    skin = signal.lfilter([0.2], [1, -0.8], noise(n, 3)) * np.exp(-t * 30)
    return (body * 0.9 + skin * 1.5) * vel


def perc_snare(m, dur, vel):
    n = int(0.35 * SR)
    t = np.arange(n) / SR
    nz = noise(n, 11)
    b, a = signal.butter(2, [1200 / (SR / 2), 7000 / (SR / 2)], "band")
    rattle = signal.lfilter(b, a, nz) * np.exp(-t * 18)
    tone = np.sin(2 * np.pi * 190 * t) * np.exp(-t * 30)
    return (rattle * 1.4 + tone * 0.6) * vel


def perc_cymbal(m, dur, vel):
    n = int(3.5 * SR)
    t = np.arange(n) / SR
    b, a = signal.butter(2, 4500 / (SR / 2), "high")
    nz = signal.lfilter(b, a, noise(n, 21))
    return nz * np.exp(-t * 1.3) * np.minimum(1, t * 500) * vel * 0.9


def perc_swell(m, dur, vel):
    """Plato en crescendo (dur = duración de la subida)."""
    n = int((dur + 0.6) * SR)
    t = np.arange(n) / SR
    b, a = signal.butter(2, 3500 / (SR / 2), "high")
    nz = signal.lfilter(b, a, noise(n, 22))
    rise = np.clip(t / dur, 0, 1) ** 2.5
    rise[int(dur * SR):] = np.exp(-np.arange(n - int(dur * SR)) / SR * 12)
    return nz * rise * vel * 0.7


def perc_shaker(m, dur, vel):
    n = int(0.12 * SR)
    t = np.arange(n) / SR
    b, a = signal.butter(2, 6000 / (SR / 2), "high")
    return signal.lfilter(b, a, noise(n, 31)) * np.exp(-t * 40) * np.minimum(1, t * 300) * vel


def perc_tambourine(m, dur, vel):
    n = int(0.3 * SR)
    t = np.arange(n) / SR
    b, a = signal.butter(2, 7000 / (SR / 2), "high")
    nz = signal.lfilter(b, a, noise(n, 41))
    return nz * np.exp(-t * 14) * (1 + 0.5 * np.sin(2 * np.pi * 30 * t)) * vel


def perc_anvil(m, dur, vel):
    n = int(1.2 * SR)
    t = np.arange(n) / SR
    out = sum(np.sin(2 * np.pi * f * t) * np.exp(-t * d) for f, d in ((1850, 6), (2640, 8), (4100, 10)))
    return out * vel * 0.4


def perc_heart(m, dur, vel):
    """Latido grave."""
    n = int(0.8 * SR)
    t = np.arange(n) / SR
    fv = 45 + 30 * np.exp(-t * 20)
    return sine(fv) * np.exp(-t * 9) * vel


INSTRUMENTS = {
    "strings": (inst_strings, "strings"),
    "spic": (inst_strings_short, "strings"),
    "pizz": (inst_pizz, "strings"),
    "violin": (inst_violin_solo, "solo"),
    "brass": (inst_brass, "brass"),
    "horn": (inst_horn, "brass"),
    "stab": (inst_brass_stab, "brass"),
    "choir": (inst_choir, "choir"),
    "flute": (inst_flute, "winds"),
    "oboe": (inst_oboe, "winds"),
    "harp": (inst_harp, "harp"),
    "celesta": (inst_celesta, "harp"),
    "bell": (inst_bell, "harp"),
    "organ": (inst_organ, "organ"),
    "piano": (inst_piano, "keys"),
    "bass": (inst_bass, "low"),
    "timp": (perc_timpani, "perc"),
    "roll": (perc_timp_roll, "perc"),
    "taiko": (perc_taiko, "perc"),
    "snare": (perc_snare, "perc"),
    "cymbal": (perc_cymbal, "perc"),
    "swell": (perc_swell, "perc"),
    "shaker": (perc_shaker, "perc"),
    "tamb": (perc_tambourine, "perc"),
    "anvil": (perc_anvil, "perc"),
    "heart": (perc_heart, "perc"),
}

# procesado por bus: (filtro, reverb_send, ganancia)
BUS_FX = {
    "strings": (("low", 6500), 0.38, 1.0),
    "solo": (("low", 7000), 0.4, 1.0),
    "brass": (("low", 5200), 0.32, 0.9),
    "choir": (("formant", None), 0.55, 1.2),
    "winds": (("low", 8000), 0.4, 0.9),
    "harp": (("low", 9000), 0.42, 0.9),
    "organ": (("low", 6000), 0.6, 0.8),
    "keys": (("low", 8000), 0.35, 0.9),
    "low": (("low", 1800), 0.18, 0.85),
    "perc": (("low", 12000), 0.25, 1.0),
}

CHORD_TYPES = {
    "": (0, 4, 7), "m": (0, 3, 7), "7": (0, 4, 7, 10), "m7": (0, 3, 7, 10), "maj7": (0, 4, 7, 11),
    "sus4": (0, 5, 7), "sus2": (0, 2, 7), "dim": (0, 3, 6), "aug": (0, 4, 8), "add9": (0, 4, 7, 14),
    "m9": (0, 3, 7, 14), "5": (0, 7),
}


def parse_chord(name):
    root = name[:2] if len(name) > 1 and name[1] in "#b" else name[:1]
    rest = name[len(root):]
    bass = None
    if "/" in rest:
        rest, bass = rest.split("/")
    ivs = CHORD_TYPES[rest]
    return NOTE_IDX[root], ivs, (NOTE_IDX[bass] if bass else None)


class Song:
    def __init__(self, bpm, bars, beats_per_bar=4, tail=3.5):
        self.bpm = bpm
        self.bpb = beats_per_bar
        self.bars = bars
        self.beat_s = 60.0 / bpm
        self.length_s = bars * beats_per_bar * self.beat_s
        self.tail = tail
        self.events = []          # (inst, midi, start_s, dur_s, vel, pan)
        self._last_voicing = {}

    def b(self, bar, beat=0.0):
        """Posición en beats a partir de (compás, tiempo)."""
        return bar * self.bpb + beat

    def note(self, inst, pitch, beat, dur, vel=0.7, pan=0.0):
        self.events.append((inst, midi(pitch), beat * self.beat_s, dur * self.beat_s, vel, pan))

    def melody(self, inst, text, beat, vel=0.7, pan=0.0, transpose=0, octave=0, legato=1.0):
        """'E5:1 D5:.5 -:.5 ...' Devuelve el beat final."""
        pos = beat
        for tok in text.split():
            nm, d = tok.split(":")
            d = float(eval(d)) if "/" in d else float(d)
            if nm not in ("-", "r"):
                v = vel
                if nm.endswith("!"):
                    nm = nm[:-1]
                    v = min(1.0, vel * 1.25)
                self.note(inst, midi(nm) + transpose + 12 * octave, pos, d * legato, v, pan)
            pos += d
        return pos

    def voicing(self, chord, low, high, key="default", count=4):
        root, ivs, _ = parse_chord(chord)
        pcs = [(root + i) % 12 for i in ivs]
        cands = [m for m in range(low, high + 1) if m % 12 in pcs]
        prev = self._last_voicing.get(key)
        best, best_score = None, 1e9
        # combinaciones simples: ventanas contiguas de 'count' notas
        for i in range(0, max(1, len(cands) - count + 1)):
            v = cands[i:i + count]
            if len(v) < min(count, len(cands)):
                continue
            covered = len({m % 12 for m in v})
            score = (len(pcs) - covered) * 10
            if prev:
                score += abs(sum(v) / len(v) - sum(prev) / len(prev))
            else:
                score += abs(sum(v) / len(v) - (low + high) / 2)
            if score < best_score:
                best, best_score = v, score
        self._last_voicing[key] = best
        return best

    def pad(self, inst, prog, beat, low=55, high=76, vel=0.45, pan=0.0, count=4, key=None):
        """prog: [(acorde, beats), ...] notas sostenidas."""
        pos = beat
        key = key or inst
        for ch, d in prog:
            if ch not in ("-", "r"):
                for m in self.voicing(ch, low, high, key, count):
                    self.note(inst, m, pos, d, vel, pan)
            pos += d
        return pos

    def bassline(self, inst, prog, beat, octave=2, vel=0.6, pattern=None, pan=0.0):
        """pattern: lista de (offset_beats, dur, intervalo) repetida cada acorde."""
        pos = beat
        for ch, d in prog:
            if ch not in ("-", "r"):
                root, ivs, bass = parse_chord(ch)
                r = 12 * (octave + 1) + (bass if bass is not None else root)
                if pattern is None:
                    self.note(inst, r, pos, d, vel, pan)
                else:
                    plen = max(o + du for o, du, _ in pattern)
                    rep = 0.0
                    while rep < d - 1e-6:
                        for o, du, iv in pattern:
                            if rep + o < d - 1e-6:
                                self.note(inst, r + iv, pos + rep + o, min(du, d - rep - o), vel, pan)
                        rep += plen
            pos += d
        return pos

    def arp(self, inst, prog, beat, low=55, high=84, step=0.5, order=(0, 1, 2, 3, 2, 1), vel=0.5, pan=0.0,
            dur=None, count=4):
        pos = beat
        for ch, d in prog:
            if ch not in ("-", "r"):
                v = self.voicing(ch, low, high, inst + "_arp", count)
                t = 0.0
                k = 0
                while t < d - 1e-6:
                    m = v[order[k % len(order)] % len(v)]
                    self.note(inst, m, pos + t, dur or step * 1.5, vel * (1.0 if k % 4 == 0 else 0.8), pan)
                    t += step
                    k += 1
            pos += d
        return pos

    def ostinato(self, inst, prog, beat, pattern, octave=3, vel=0.5, pan=0.0):
        """pattern: [(offset, dur, grado)] donde grado indexa las notas del acorde (0=raíz, 1=tercera, 2=quinta, 3=octava)."""
        pos = beat
        plen = max(o + d for o, d, _ in pattern)
        for ch, d in prog:
            if ch not in ("-", "r"):
                root, ivs, _ = parse_chord(ch)
                tones = list(ivs[:3]) + [12, ivs[1] + 12 if len(ivs) > 1 else 12, ivs[2] + 12 if len(ivs) > 2 else 19]
                base = 12 * (octave + 1) + root
                rep = 0.0
                while rep < d - 1e-6:
                    for o, du, g in pattern:
                        if rep + o < d - 1e-6:
                            iv = tones[g] if isinstance(g, int) else int(g[1:])
                            self.note(inst, base + iv, pos + rep + o, du, vel, pan)
                    rep += plen
            pos += d
        return pos

    def drums(self, pattern, beat, step=0.25, bars=1, vel=0.8, pitch="C3", pan=0.0):
        """pattern: 'T..t.T..' con mapa de letras (T taiko fuerte, t taiko suave, s caja, c plato, x shaker, b tambor/timbal)."""
        mapping = {"T": ("taiko", 1.0), "t": ("taiko", 0.55), "S": ("snare", 0.9), "s": ("snare", 0.45),
                   "c": ("cymbal", 0.7), "x": ("shaker", 0.5), "X": ("shaker", 0.9), "j": ("tamb", 0.5),
                   "h": ("heart", 1.0), "a": ("anvil", 0.6), "K": ("timp", 1.0), "k": ("timp", 0.6)}
        span = len(pattern) * step
        for r in range(bars):
            for i, ch in enumerate(pattern):
                if ch in mapping:
                    ins, v = mapping[ch]
                    self.note(ins, pitch, beat + r * span + i * step, step, v * vel, pan)
        return beat + bars * span

    # -------------------------------------------------------------- Render
    def render(self, path, master=0.9, reverb_time=2.4, loop=True, width=1.0):
        total_s = self.length_s + self.tail
        N = int(total_s * SR)
        buses = {}
        cache = {}
        for inst, m, st, du, vel, pan in self.events:
            fn, bus = INSTRUMENTS[inst]
            key = (inst, round(m, 2), round(du, 3), round(vel, 3))
            if key in cache and inst not in ("strings", "choir", "violin"):
                sig = cache[key]
            else:
                sig = fn(m, du, vel)
                cache[key] = sig
            a = int(st * SR)
            if a >= N:
                continue
            e = min(N, a + len(sig))
            if bus not in buses:
                buses[bus] = np.zeros((N, 2))
            # panorámica de potencia constante
            p = (pan + 1) * np.pi / 4
            buses[bus][a:e, 0] += sig[:e - a] * np.cos(p)
            buses[bus][a:e, 1] += sig[:e - a] * np.sin(p)
        dry = np.zeros((N, 2))
        send = np.zeros((N, 2))
        for bus, buf in buses.items():
            (ft, fc), rs, g = BUS_FX[bus]
            if ft == "low":
                sos = signal.butter(2, fc / (SR / 2), "low", output="sos")
                buf = signal.sosfilt(sos, buf, axis=0)
            elif ft == "formant":
                outb = np.zeros_like(buf)
                for (lo, hi, amp) in ((500, 1100, 1.0), (1000, 1500, 0.6), (2400, 3200, 0.25), (150, 400, 0.5)):
                    sos = signal.butter(2, [lo / (SR / 2), hi / (SR / 2)], "band", output="sos")
                    outb += signal.sosfilt(sos, buf, axis=0) * amp
                buf = outb * 1.6
            # recorte de graves innecesarios salvo bus grave/percusión
            if bus not in ("low", "perc", "organ"):
                sos = signal.butter(1, 70 / (SR / 2), "high", output="sos")
                buf = signal.sosfilt(sos, buf, axis=0)
            dry += buf * g
            send += buf * g * rs
        wet = reverb(send, reverb_time)
        mix = dry + wet[:N] * 0.9
        # limpieza de subgraves y leve realce de presencia
        mix = signal.sosfilt(signal.butter(2, 38 / (SR / 2), "high", output="sos"), mix, axis=0)
        pres = signal.sosfilt(signal.butter(1, [1800 / (SR / 2), 5000 / (SR / 2)], "band", output="sos"), mix, axis=0)
        mix = mix + pres * 0.35
        # ensanchado estéreo sutil
        mid = (mix[:, 0] + mix[:, 1]) / 2
        side = (mix[:, 0] - mix[:, 1]) / 2 * width
        mix = np.stack([mid + side, mid - side], axis=1)
        if loop:
            L = int(self.length_s * SR)
            tail = mix[L:]
            mix = mix[:L].copy()
            mix[:len(tail)] += tail[:L]
        # compresión suave + normalizado
        peak = np.max(np.abs(mix)) + 1e-9
        mix = mix / peak
        mix = np.tanh(mix * 1.6) / np.tanh(1.6)
        mix = mix / (np.max(np.abs(mix)) + 1e-9) * master
        data = mix.astype(np.float32)
        with sf.SoundFile(path, "w", SR, 2, format="OGG", subtype="VORBIS") as fh:
            for i in range(0, len(data), 16384):
                fh.write(data[i:i + 16384])
        return len(mix) / SR


_ir_cache = {}


def reverb(x, rt=2.4):
    key = rt
    if key not in _ir_cache:
        n = int(rt * SR)
        t = np.arange(n) / SR
        r = np.random.default_rng(77)
        irs = []
        for ch in range(2):
            nz = r.normal(0, 1, n)
            dec = np.exp(-t * 6.9 / rt)
            bright = signal.sosfilt(signal.butter(1, 5000 / (SR / 2), "low", output="sos"), nz)
            dark = signal.sosfilt(signal.butter(1, 1200 / (SR / 2), "low", output="sos"), nz)
            mixw = np.exp(-t * 3)
            ir = (bright * mixw + dark * (1 - mixw)) * dec
            pre = int(0.018 * SR)
            ir = np.concatenate([np.zeros(pre), ir])
            # reflexiones tempranas
            for d_ms, a in ((11, 0.5), (23, 0.35), (37, 0.3), (53, 0.2)):
                k = int(d_ms / 1000 * SR) + ch * 7
                if k < len(ir):
                    ir[k] += a * 3
            irs.append(ir / np.sqrt(np.sum(ir ** 2)))
        _ir_cache[key] = irs
    irs = _ir_cache[key]
    out = np.zeros((len(x) + len(irs[0]) - 1, 2))
    for ch in range(2):
        out[:, ch] = signal.fftconvolve(x[:, ch], irs[ch]) + signal.fftconvolve(x[:, 1 - ch], irs[ch]) * 0.3
    return out * 1.2
