"""SKYBREAKER – Titelthema (prozedural): Flächen, Harfen-Arpeggien, Glockenmelodie, Bass, Pauke, Hall.
Nahtlose Schleife: zwei Durchläufe rechnen, den zweiten nehmen (Ausklang des ersten läuft hinein)."""
import math, os, struct, wave
import numpy as np

SR = 22050
BPM = 66
BEAT = 60 / BPM
BARS = 8
LOOP = int(SR * BEAT * 4 * BARS)
rng = np.random.default_rng(5)

def midi(n): return 440.0 * 2 ** ((n - 69) / 12)

# d-Moll: Dm Bb F C | Dm Bb Gm A
CHORDS = [(50, [62, 65, 69]), (46, [62, 65, 70]), (41, [60, 65, 69]), (48, [60, 64, 67]),
          (50, [62, 65, 69]), (46, [62, 65, 70]), (43, [62, 67, 70]), (45, [61, 64, 69])]
# Melodie (Takt, Schlag, Ton, Dauer in Schlägen)
MEL = [(2, 0, 74, 1.5), (2, 1.5, 76, 0.5), (2, 2, 77, 2), (3, 0, 76, 1), (3, 1, 74, 1), (3, 2, 72, 2),
       (4, 0, 74, 3), (4, 3, 69, 1), (5, 0, 70, 1.5), (5, 1.5, 72, 0.5), (5, 2, 74, 2),
       (6, 0, 74, 1), (6, 1, 72, 1), (6, 2, 70, 2), (7, 0, 69, 2), (7, 2, 73, 2)]

N = LOOP * 2
out = np.zeros(N)
t_all = np.arange(N) / SR


def env(n, a, r, sus=1.0):
    e = np.ones(n) * sus
    na, nr = int(a * SR), int(r * SR)
    na = min(na, n); e[:na] = np.linspace(0, sus, na)
    if nr > 0:
        nr = min(nr, n); e[-nr:] *= np.linspace(1, 0, nr)
    return e


def lowpass(x, cut):
    """einpoliger Tiefpass, blockweise vektorisiert"""
    a = math.exp(-2 * math.pi * cut / SR)
    b = 1 - a
    B = 128
    k = np.arange(B)
    ak, ainv = a ** k, a ** (-k.astype(float))
    y = np.empty_like(x)
    prev = 0.0
    for i in range(0, len(x), B):
        seg = x[i:i + B]
        m = len(seg)
        y[i:i + m] = np.cumsum(seg * b * ainv[:m]) * ak[:m] + prev * a * ak[:m]
        prev = y[i + m - 1]
    return y


pad = np.zeros(N)


def add(start, sig, bus=None):
    bus = out if bus is None else bus
    s = int(start)
    e = min(N, s + len(sig))
    if s < N:
        bus[s:e] += sig[:e - s]


def pad_note(f, dur, vol):
    n = int(dur * SR)
    t = np.arange(n) / SR
    sig = np.zeros(n)
    for det in (-0.006, 0.0, 0.007):
        ph = 2 * np.pi * f * (1 + det) * t + rng.uniform(0, 6.28)
        sig += 2 * ((ph / (2 * np.pi)) % 1) - 1   # Sägezahn
    return sig * env(n, 1.2, 1.5) * vol


def pluck(f, vol, dur=2.2):
    n = int(dur * SR)
    t = np.arange(n) / SR
    sig = (np.sin(2 * np.pi * f * t) + 0.5 * np.sin(4 * np.pi * f * t) * np.exp(-t * 6) + 0.25 * np.sin(6 * np.pi * f * t) * np.exp(-t * 9))
    return sig * np.exp(-t * 2.6) * env(n, 0.004, 0.05) * vol


def bell(f, vol, dur):
    n = int((dur + 2.5) * SR)
    t = np.arange(n) / SR
    mod = np.sin(2 * np.pi * f * 3.5 * t) * 2.2 * np.exp(-t * 3)
    sig = np.sin(2 * np.pi * f * t + mod) * np.exp(-t * 1.4) + 0.3 * np.sin(2 * np.pi * f * 2 * t) * np.exp(-t * 2.5)
    return sig * env(n, 0.01, 0.3) * vol


def timpani(vol):
    n = int(1.6 * SR)
    t = np.arange(n) / SR
    f = 70 * (1 + 0.5 * np.exp(-t * 18))
    sig = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 3.2) + rng.normal(0, 1, n) * np.exp(-t * 30) * 0.3
    return sig * vol


for loop in range(2):
    base = loop * LOOP
    for bar, (root, tones) in enumerate(CHORDS):
        s = base + bar * 4 * BEAT * SR
        for tn in tones:
            add(s, pad_note(midi(tn - 12), BEAT * 4 + 1.2, 0.05), pad)
        add(s, pad_note(midi(root - 12), BEAT * 4 + 1.0, 0.07), pad)
        n = int(BEAT * 4 * SR)
        tb = np.arange(n) / SR
        add(s, (np.sin(2 * np.pi * midi(root - 12) * tb) * 0.22) * env(n, 0.05, 0.4))
        arp = [tones[0], tones[1], tones[2], tones[0] + 12, tones[2], tones[1], tones[0] + 12, tones[1] + 12]
        for k, nt in enumerate(arp):
            add(s + k * BEAT / 2 * SR, pluck(midi(nt), 0.11 if k % 2 == 0 else 0.08))
        if bar % 2 == 0:
            add(s, timpani(0.28))
        if bar in (3, 7):
            add(s + 3 * BEAT * SR, timpani(0.18))
    for bar, beat, nt, d in MEL:
        add(base + (bar * 4 + beat) * BEAT * SR, bell(midi(nt), 0.16, d * BEAT))

out += lowpass(lowpass(pad, 1300), 1800)
# Hall per FFT-Faltung (synthetische Impulsantwort)
ir_n = int(2.8 * SR)
ti = np.arange(ir_n) / SR
ir = rng.normal(0, 1, ir_n) * np.exp(-ti * 2.2)
ir[0] = 0
ir /= np.sqrt(np.sum(ir ** 2))
L = 1 << int(math.ceil(math.log2(N + ir_n)))
wet = np.fft.irfft(np.fft.rfft(out, L) * np.fft.rfft(ir, L), L)[:N]
mixd = out * 0.75 + wet * 0.55
loop_sig = mixd[LOOP:2 * LOOP] + mixd[:LOOP] * 0  # zweiter Durchlauf enthält den Ausklang des ersten
loop_sig /= np.max(np.abs(loop_sig)) * 1.08
pcm = (loop_sig * 32767).astype(np.int16)
os.makedirs(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data"), exist_ok=True)
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "theme.wav")
with wave.open(path, "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes(pcm.tobytes())
print("theme.wav", len(pcm) / SR, "s", os.path.getsize(path) // 1024, "KB")
