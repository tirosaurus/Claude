"""Música (IMA ADPCM 4 bits) y efectos (PCM 8 bits) a 18157 Hz para el mezclador de la GBA."""
import os
import subprocess
import sys
from fractions import Fraction
import numpy as np
import soundfile as sf
from scipy.signal import resample_poly
from lib import *

RATE = 18157
ENC = os.path.join(GBA, "build", "adpcm")
TMP = os.path.join(CACHE, "audio")
os.makedirs(TMP, exist_ok=True)


def load_mono(path):
    d, sr = sf.read(path, dtype="float32", always_2d=True)
    d = d.mean(1)
    fr = Fraction(RATE, sr).limit_denominator(2000)
    return resample_poly(d, fr.numerator, fr.denominator).astype(np.float32)


def music(name):
    src = os.path.join(GAME, "music", name + ".ogg")
    out = os.path.join(BIN, "mus_%s.bin" % c_ident(name))
    meta = os.path.join(TMP, name + ".meta")
    h = file_hash([src, __file__])
    if os.path.exists(meta) and open(meta).read().split("|")[0] == h and os.path.exists(out):
        return open(meta).read().split("|")[1]
    d = load_mono(src)
    s = np.clip(np.round(d * 32767), -32768, 32767).astype(np.int16)
    raw = os.path.join(TMP, name + ".raw")
    s.tofile(raw)
    r = subprocess.run([ENC, raw, os.path.join(TMP, name + ".adp")], capture_output=True, text=True)
    pred, idx = r.stdout.split()
    write_blob("mus_%s" % c_ident(name), open(os.path.join(TMP, name + ".adp"), "rb").read())
    info = "%d %s %s" % (len(s), pred, idx)
    open(meta, "w").write(h + "|" + info)
    print("  música %-12s %6.1f s" % (name, len(s) / RATE), flush=True)
    return info


def sfx(name):
    d = load_mono(os.path.join(GAME, "sfx", name + ".wav"))
    # recortar la cola en silencio
    thr = 0.004
    nz = np.nonzero(np.abs(d) > thr)[0]
    end = (nz[-1] + 64) if len(nz) else len(d)
    d = d[:min(len(d), end)]
    rng = np.random.default_rng(3)
    dither = (rng.random(len(d)) - rng.random(len(d))) / 127.0
    s = np.clip(np.round((d + dither) * 127), -127, 127).astype(np.int8)
    write_blob("sfx_%s" % c_ident(name), s.tobytes())
    return len(s)


def main():
    if not os.path.exists(ENC):
        subprocess.check_call(["gcc", "-O2", "-o", ENC, os.path.join(HERE, "adpcm.c")])
    tracks = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "music")) if f.endswith(".ogg"))
    sfxs = sorted(f[:-4] for f in os.listdir(os.path.join(GAME, "sfx")) if f.endswith(".wav"))
    c = ['#include "../../src/audio.h"']
    rows = []
    for t in tracks:
        n, pred, idx = music(t).split()
        c.append("extern const u8 mus_%s[];" % c_ident(t))
        rows.append('{"%s", mus_%s, %s, %s, %s, %d}' % (t, c_ident(t), n, pred, idx, 0 if t == "victory" else 1))
    c.append("const MusicTrack g_music[] = {%s};" % ",\n\t".join(rows))
    c.append("const int g_nmusic = %d;" % len(tracks))
    rows = []
    for s in sfxs:
        n = sfx(s)
        c.append("extern const s8 sfx_%s[];" % c_ident(s))
        rows.append('{"%s", sfx_%s, %d}' % (s, c_ident(s), n))
    c.append("const SfxSample g_sfx[] = {%s};" % ",\n\t".join(rows))
    c.append("const int g_nsfx = %d;" % len(sfxs))
    write_if_changed(os.path.join(OUT, "audio_data.c"), "\n".join(c) + "\n")
    blobs_asm()


if __name__ == "__main__":
    main()
