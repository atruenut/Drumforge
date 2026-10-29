#!/usr/bin/env python3
"""bands.py notes.wav label1,label2,...  -> octave-band energy (dB rel. loudest band) + centroid for chosen hits"""
import sys, struct, numpy as np
d = open(sys.argv[1], 'rb').read(); fs = struct.unpack('<I', d[24:28])[0]
n = struct.unpack('<I', d[40:44])[0] // 8
a = np.frombuffer(d[44:44 + n * 8], dtype='<f4').reshape(-1, 2).mean(axis=1)
gap = int(2.5 * fs)
labels = open('/tmp/labels.txt').read().strip().split(',')
want = sys.argv[2].split(',')
edges = [30, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
print("%-16s" % "" + "".join("%7d" % e for e in edges[:-1]) + "  centroid")
for w in want:
    i = labels.index(w)
    seg = a[i * gap:(i + 1) * gap][:int(0.8 * fs)]
    S = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
    f = np.fft.rfftfreq(len(seg), 1 / fs)
    e = [S[(f >= lo) & (f < hi)].sum() for lo, hi in zip(edges[:-1], edges[1:])]
    m = max(e); cen = (S * f).sum() / S.sum()
    print("%-16s" % w + "".join("%7.0f" % (10 * np.log10(x / m + 1e-12)) for x in e) + "  %6.0f Hz" % cen)
