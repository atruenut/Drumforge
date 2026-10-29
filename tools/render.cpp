// Offline renderer / test harness for the drum engine.
//   render notes  out.wav [fs] [preset]           one hit of every mapped note, 2.5 s apart
//   render groove out.wav [fs] [preset] [pattern] [bars] [bpm]
//   render stats  [fs]                            render every preset and print health metrics
#include <chrono>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../src/engine/engine.h"
#include "../src/engine/grooves.h"
#include "../src/engine/presets.h"

using namespace df;

static void writeWav(const char* path, const std::vector<float>& L, const std::vector<float>& R, int fs)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror(path); return; }
    uint32_t n = (uint32_t)L.size(), dataBytes = n * 2 * 4;
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); w32(36 + dataBytes); std::fwrite("WAVEfmt ", 1, 8, f);
    w32(16); w16(3); w16(2); w32((uint32_t)fs); w32((uint32_t)fs * 8); w16(8); w16(32);
    std::fwrite("data", 1, 4, f); w32(dataBytes);
    for (uint32_t i = 0; i < n; ++i) { std::fwrite(&L[i], 4, 1, f); std::fwrite(&R[i], 4, 1, f); }
    std::fclose(f);
}

static void applyPresetTo(DrumEngine& e, int preset)
{
    std::vector<float> v((size_t)paramTable().size());
    presetApply(preset, v.data());
    e.setAll(v.data());
}

static void renderBlocks(DrumEngine& e, std::vector<float>& L, std::vector<float>& R, int total)
{
    L.assign((size_t)total, 0.f); R.assign((size_t)total, 0.f);
    const int B = 256;
    for (int i = 0; i < total; i += B) {
        int n = std::min(B, total - i);
        e.process(&L[(size_t)i], &R[(size_t)i], nullptr, nullptr, n);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: render notes|groove|stats ...\n"); return 1; }
    _mm_setcsr(_mm_getcsr() | 0x8040);   // flush-to-zero + denormals-are-zero, as the plugin does
    std::string mode = argv[1];

    if (mode == "notes") {
        const char* out = argc > 2 ? argv[2] : "notes.wav";
        int fs = argc > 3 ? std::atoi(argv[3]) : 48000;
        int preset = argc > 4 ? std::atoi(argv[4]) : -1;
        DrumEngine e; e.init((float)fs);
        if (preset >= 0) applyPresetTo(e, preset);
        if (std::getenv("NOLIM")) e.setParam(paramTable().find("lim.on"), 0);
        std::vector<int> notes;
        for (int n = 0; n < 128; ++n) if (noteInfo(n).piece >= 0) notes.push_back(n);
        const int gap = (int)(2.5 * fs);
        const int total = gap * (int)notes.size() + fs;
        std::vector<float> L((size_t)total), R((size_t)total);
        size_t idx = 0;
        const int B = 128;
        for (int i = 0; i < total; i += B) {
            if (idx < notes.size() && i >= (int)idx * gap) { e.noteOn(notes[idx], 100); ++idx; }
            e.process(&L[(size_t)i], &R[(size_t)i], nullptr, nullptr, std::min(B, total - i));
        }
        writeWav(out, L, R, fs);
        for (size_t k = 0; k < notes.size(); ++k) {
            double pk = 0, rms = 0; int seg0 = (int)k * gap, cnt = 0;
            int lastAbove = 0;
            for (int i = 0; i < gap && seg0 + i < total; ++i) {
                double a = std::max(std::fabs(L[(size_t)(seg0 + i)]), std::fabs(R[(size_t)(seg0 + i)]));
                pk = std::max(pk, a);
                if (i < fs / 4) { rms += a * a; ++cnt; }
                if (a > 0.001) lastAbove = i;     // -60 dBFS
            }
            std::printf("note %3d %-18s peak %6.3f  rms(250ms) %6.3f  len(-60dB) %5.0f ms\n", notes[k], noteLabel(notes[k]), pk,
                        std::sqrt(rms / std::max(1, cnt)), 1000.0 * lastAbove / fs);
        }
        return 0;
    }

    if (mode == "groove") {
        const char* out = argc > 2 ? argv[2] : "groove.wav";
        int fs = argc > 3 ? std::atoi(argv[3]) : 48000;
        int preset = argc > 4 ? std::atoi(argv[4]) : -1;
        int pat = argc > 5 ? std::atoi(argv[5]) : 0;
        int bars = argc > 6 ? std::atoi(argv[6]) : 4;
        double bpm = argc > 7 ? std::atof(argv[7]) : 120;
        DrumEngine e; e.init((float)fs);
        if (preset >= 0) applyPresetTo(e, preset);
        GrooveSettings gs; gs.pattern = pat; gs.intensity = 80; gs.human = 10;
        const double beats = bars * 4.0;
        const int total = (int)(beats * 60.0 / bpm * fs) + fs * 2;
        std::vector<GrooveEvent> ev((size_t)(beats * 32 + 64));
        int ne = grooveGenerate(gs, 0, beats, ev.data(), (int)ev.size());
        std::vector<float> L((size_t)total), R((size_t)total);
        size_t ei = 0; const int B = 64;
        for (int i = 0; i < total; i += B) {
            int n = std::min(B, total - i);
            for (; ei < (size_t)ne; ++ei) {
                int s = (int)(ev[ei].ppq * 60.0 / bpm * fs);
                if (s >= i + n) break;
                e.noteOn(ev[ei].note, ev[ei].velocity);
            }
            e.process(&L[(size_t)i], &R[(size_t)i], nullptr, nullptr, n);
        }
        writeWav(out, L, R, fs);
        std::printf("%d events\n", ne);
        return 0;
    }

    if (mode == "stats") {
        int fs = argc > 2 ? std::atoi(argv[2]) : 48000;
        const int np = presetCount();
        std::printf("%-3s %-22s %-11s %7s %7s %8s %6s\n", "#", "preset", "category", "peakdB", "rmsdB", "dc", "cpu%");
        int bad = 0;
        for (int p = 0; p < np; ++p) {
            DrumEngine e; e.init((float)fs);
            applyPresetTo(e, p);
            if (std::getenv("NOLIM")) e.setParam(paramTable().find("lim.on"), 0);
            GrooveSettings gs; gs.pattern = (int)e.getParam(paramTable().find("groove.pattern")); gs.intensity = 85;
            const double bpm = 120, beats = 16;
            const int total = (int)(beats * 60.0 / bpm * fs) + fs;
            std::vector<GrooveEvent> ev(2048);
            int ne = grooveGenerate(gs, 0, beats, ev.data(), (int)ev.size());
            std::vector<float> L((size_t)total), R((size_t)total);
            size_t ei = 0; const int B = 128;
            auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < total; i += B) {
                int n = std::min(B, total - i);
                for (; ei < (size_t)ne; ++ei) {
                    int s = (int)(ev[ei].ppq * 60.0 / bpm * fs);
                    if (s >= i + n) break;
                    e.noteOn(ev[ei].note, ev[ei].velocity);
                }
                e.process(&L[(size_t)i], &R[(size_t)i], nullptr, nullptr, n);
            }
            double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            double pk = 0, sq = 0, dc = 0; bool nan = false;
            for (int i = 0; i < total; ++i) {
                float a = L[(size_t)i], b = R[(size_t)i];
                if (!(a == a) || !(b == b) || std::fabs(a) > 4 || std::fabs(b) > 4) nan = true;
                pk = std::max(pk, (double)std::max(std::fabs(a), std::fabs(b)));
                sq += 0.5 * (a * a + b * b); dc += 0.5 * (a + b);
            }
            double rms = std::sqrt(sq / total);
            const Preset& pr = preset(p);
            std::vector<float> tmpv((size_t)paramTable().size());
            int unk = presetParse(pr.spec, tmpv.data());
            if (unk) std::printf("   !! %d unknown keys in preset %s\n", unk, pr.name);
            bool warn = unk || nan || pk > 1.0001 || pk < 0.25 || rms < 0.02 || std::fabs(dc / total) > 0.005;
            std::printf("%-3d %-22s %-11s %7.1f %7.1f %8.5f %6.2f %s\n", p, pr.name, pr.category, 20 * std::log10(pk + 1e-9),
                        20 * std::log10(rms + 1e-9), dc / total, 100.0 * sec / ((double)total / fs), warn ? "<-- CHECK" : "");
            bad += warn;
        }
        std::printf("%d presets, %d flagged\n", np, bad);
        return bad ? 2 : 0;
    }
    return 1;
}
