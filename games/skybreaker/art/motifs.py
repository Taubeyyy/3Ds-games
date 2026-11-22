"""SKYBREAKER – kurze Leitmotive je Gott (ertönen, wenn ein Gott dir Segen anbietet). data/motif_<gott>.wav"""
import math, os, wave
import numpy as np

SR = 22050
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
rng = np.random.default_rng(3)


def midi(n): return 440.0 * 2 ** ((n - 69) / 12)


def env(n, a, d):
    t = np.arange(n) / SR
    e = np.minimum(1, t / max(a, 1e-4)) * np.exp(-t * d)
    return e


def tone(f, dur, kind, vol, d=3.0):
    n = int(dur * SR)
    t = np.arange(n) / SR
    if kind == "brass":
        s = sum(np.sin(2 * np.pi * f * k * t) / k ** 1.1 for k in range(1, 7)) * (0.6 + 0.4 * np.minimum(1, t * 8))
        e = env(n, 0.04, d * 0.6)
    elif kind == "bell":
        s = np.sin(2 * np.pi * f * t + 2.2 * np.sin(2 * np.pi * f * 3.5 * t) * np.exp(-t * 3)); e = env(n, 0.004, d)
    elif kind == "harp":
        s = np.sin(2 * np.pi * f * t) + 0.4 * np.sin(4 * np.pi * f * t) * np.exp(-t * 5); e = env(n, 0.003, d)
    elif kind == "flute":
        s = np.sin(2 * np.pi * f * t + 0.15 * np.sin(2 * np.pi * 5.5 * t)) + 0.08 * rng.normal(0, 1, n); e = env(n, 0.06, d * 0.5)
    elif kind == "dark":
        s = np.sign(np.sin(2 * np.pi * f * t)) * 0.3 + np.sin(2 * np.pi * f * 0.5 * t); e = env(n, 0.02, d)
    elif kind == "anvil":
        s = sum(np.sin(2 * np.pi * f * r * t) * np.exp(-t * (6 + r * 2)) for r in (1, 2.76, 5.4, 8.9)) + rng.normal(0, 1, n) * np.exp(-t * 60) * 0.5; e = env(n, 0.001, d)
    elif kind == "zap":
        s = np.sin(2 * np.pi * f * t) * (np.sin(2 * np.pi * 40 * t) > 0) + 0.3 * rng.normal(0, 1, n) * np.exp(-t * 20); e = env(n, 0.002, d)
    else:
        s = np.sin(2 * np.pi * f * t); e = env(n, 0.01, d)
    return s * e * vol


def build(notes, total=1.8, rev=0.35):
    out = np.zeros(int(total * SR) + SR)
    for start, n, dur, kind, vol in notes:
        sig = tone(midi(n), dur, kind, vol)
        i = int(start * SR)
        out[i:i + len(sig)] += sig[:len(out) - i]
    ir_n = int(1.2 * SR)
    ti = np.arange(ir_n) / SR
    ir = rng.normal(0, 1, ir_n) * np.exp(-ti * 3.5)
    ir /= np.sqrt(np.sum(ir ** 2))
    L = 1 << int(math.ceil(math.log2(len(out) + ir_n)))
    wet = np.fft.irfft(np.fft.rfft(out, L) * np.fft.rfft(ir, L), L)[:len(out)]
    mixd = out + wet * rev
    mixd = mixd[:int(total * SR)]
    mixd *= np.minimum(1, np.linspace(total, 0, len(mixd)) / 0.3)
    return mixd / (np.max(np.abs(mixd)) * 1.15)


MOTIFS = {
    # Ignara: Feuer – stolze Blechbläser, D-Dur aufsteigend
    "ignara": [(0, 62, 0.4, "brass", 0.5), (0.18, 66, 0.4, "brass", 0.5), (0.36, 69, 0.4, "brass", 0.5), (0.54, 74, 1.0, "brass", 0.6), (0.54, 50, 1.0, "brass", 0.4)],
    # Thalor: Sturm – Donnerschlag und helles Arpeggio
    "thalor": [(0, 40, 0.8, "anvil", 0.6), (0.1, 71, 0.3, "zap", 0.3), (0.22, 76, 0.3, "zap", 0.3), (0.34, 79, 0.3, "zap", 0.3), (0.46, 83, 0.9, "bell", 0.4)],
    # Nerys: Gezeiten – Harfenglissando, Dorisch
    "nerys": [(i * 0.07, n, 1.2, "harp", 0.35) for i, n in enumerate([62, 64, 65, 67, 69, 71, 72, 74, 76])] + [(0.7, 74, 1.0, "bell", 0.2)],
    # Vexa: Schatten – tiefes, schiefes Moll
    "vexa": [(0, 45, 1.2, "dark", 0.4), (0.25, 60, 0.6, "bell", 0.3), (0.5, 63, 0.6, "bell", 0.3), (0.75, 66, 1.0, "bell", 0.35)],
    # Korrin: Erde/Schmiede – Ambossschläge + tiefes Horn
    "korrin": [(0, 55, 0.6, "anvil", 0.6), (0.3, 55, 0.6, "anvil", 0.5), (0.6, 62, 0.6, "anvil", 0.6), (0.6, 38, 1.2, "brass", 0.5)],
    # Lyra: Sternenlicht – hohe Glocken, Dur
    "lyra": [(0, 76, 1.2, "bell", 0.35), (0.15, 79, 1.2, "bell", 0.3), (0.3, 83, 1.2, "bell", 0.3), (0.45, 88, 1.2, "bell", 0.3), (0.6, 64, 1.2, "harp", 0.3)],
    # Sylv: Wildnis – Flöte, Pentatonik
    "sylv": [(0, 69, 0.35, "flute", 0.4), (0.22, 72, 0.35, "flute", 0.4), (0.44, 74, 0.35, "flute", 0.4), (0.66, 76, 0.8, "flute", 0.45), (0.0, 45, 1.4, "harp", 0.25)],
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for god, notes in MOTIFS.items():
        pcm = (build(notes) * 32767).astype(np.int16)
        with wave.open(os.path.join(OUT, "motif_%s.wav" % god), "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
            w.writeframes(pcm.tobytes())
    print("Motive:", len(MOTIFS))
