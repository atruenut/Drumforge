#!/usr/bin/env python3
"""Analyse the output of `render notes`: K-weighted loudness per hit (relative to kick) + spectrogram sheet."""
import sys, struct, numpy as np
from scipy import signal
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt

def readwav(p):
    d = open(p, 'rb').read()
    fs = struct.unpack('<I', d[24:28])[0]
    n = struct.unpack('<I', d[40:44])[0] // 8
    a = np.frombuffer(d[44:44 + n * 8], dtype='<f4').reshape(-1, 2)
    return fs, a

def kweight(x, fs):
    b, a = signal.butter(2, 60 / (fs / 2), 'high')
    x = signal.lfilter(b, a, x)
    # simple +4 dB high shelf around 2 kHz
    f0 = 2000; A = 10 ** (4 / 40); w = 2 * np.pi * f0 / fs; c, s = np.cos(w), np.sin(w)
    al = s / 2 * np.sqrt((A + 1 / A) * (1 / 0.8 - 1) + 2); sq = 2 * np.sqrt(A) * al
    b0 = A * ((A + 1) + (A - 1) * c + sq); b1 = -2 * A * ((A - 1) + (A + 1) * c); b2 = A * ((A + 1) + (A - 1) * c - sq)
    a0 = (A + 1) - (A - 1) * c + sq; a1 = 2 * ((A - 1) - (A + 1) * c); a2 = (A + 1) - (A - 1) * c - sq
    return signal.lfilter([b0 / a0, b1 / a0, b2 / a0], [1, a1 / a0, a2 / a0], x)

wav = sys.argv[1]
labels = sys.argv[2].split(',') if len(sys.argv) > 2 else None
fs, a = readwav(wav)
gap = int(2.5 * fs)
k = len(a) // gap
mono = a.mean(axis=1)
res = []
for i in range(k):
    seg = a[i * gap:(i + 1) * gap]
    kw = kweight(seg.mean(axis=1), fs)[:int(0.6 * fs)]
    e = 10 * np.log10(np.mean(kw ** 2) + 1e-12)
    res.append(e)
print("K-weighted energy of first 600 ms (dB, relative to first hit):")
for i, e in enumerate(res):
    print(f"  hit {i:2d}  {e - res[0]:+6.1f} dB   {labels[i] if labels and i < len(labels) else ''}")

if len(sys.argv) > 3:
    cols = 6; rows = int(np.ceil(k / cols))
    fig, axs = plt.subplots(rows, cols, figsize=(cols * 3.2, rows * 2.4))
    for i, ax in enumerate(axs.flat):
        if i >= k: ax.axis('off'); continue
        seg = mono[i * gap:(i + 1) * gap][:int(1.2 * fs)]
        f, t, S = signal.spectrogram(seg, fs, nperseg=1024, noverlap=768)
        ax.pcolormesh(t, f, 10 * np.log10(S + 1e-12), vmin=-120, vmax=-40, shading='auto', cmap='magma')
        ax.set_yscale('symlog', linthresh=500); ax.set_ylim(30, fs / 2)
        ax.set_title(labels[i] if labels else str(i), fontsize=8); ax.tick_params(labelsize=6)
    plt.tight_layout(); plt.savefig(sys.argv[3], dpi=70)

# ---- calibration targets (dB relative to kick) -- printed only when labels are given
TARGET = {"Kick":0,"Kick 2":0,"Sidestick":-9,"Snare":-3,"Snare (rimshot)":-3.5,"Clap":-8,"Floor Tom":-3,"Floor Tom Hi":-3,"Mid Tom":-3,"Mid Tom Hi":-3,
 "High Tom Lo":-3,"High Tom":-3,"Closed Hat":-11,"Pedal Hat":-12,"Open Hat":-7,"Crash":-7,"Crash 2":-8,"China":-7,"Splash":-10,"Ride":-9,"Ride 2":-9,"Ride Bell":-10,
 "Tambourine":-9,"Cowbell":-9,"Shaker":-13,"Claves":-10,"Conga Hi":-8,"Conga Lo":-8,"Triangle":-12,"Triangle (mute)":-15}
if labels:
    print("\nneeded trim to hit target (dB):")
    for i, e in enumerate(res):
        if labels[i] in TARGET:
            print(f"  {labels[i]:18s} {TARGET[labels[i]] - (e - res[0]):+6.1f}")
