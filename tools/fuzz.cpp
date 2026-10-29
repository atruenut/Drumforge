// Fuzz / stress test: random parameters, random notes, odd sample rates and block sizes. Must never produce NaN/Inf or overshoot the limiter ceiling.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../src/engine/engine.h"
#include "../src/engine/presets.h"
using namespace df;
int main(int argc, char** argv)
{
    int iters = argc > 1 ? std::atoi(argv[1]) : 40;
    Rng r; r.seed(12345);
    const ParamTable& t = paramTable();
    static const float rates[] = {22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000};
    int bad = 0; double worst = 0;
    for (int it = 0; it < iters; ++it) {
        float fs = rates[r.next() % 8];
        DrumEngine e; e.init(fs);
        std::vector<float> v((size_t)t.size());
        if (it % 3 == 0) presetApply((int)(r.next() % (uint32_t)presetCount()), v.data());
        else for (int i = 0; i < t.size(); ++i) { const ParamDef& d = t.defs[(size_t)i]; v[(size_t)i] = r.range(d.minV, d.maxV); }
        // always keep the limiter on to test the ceiling guarantee
        v[(size_t)t.find("lim.on")] = 1; v[(size_t)t.find("lim.ceil")] = -0.3f;
        v[(size_t)t.find("master")] = r.range(-6, 6);
        e.setAll(v.data());
        const int blocks = (int)(fs * 3 / 64);
        std::vector<float> L(1024), R(1024);
        for (int b = 0; b < blocks; ++b) {
            int n = 1 + (int)(r.next() % 1024);
            if (r.uni() < 0.35f) for (int k = 0; k < (int)(r.next() % 6); ++k) e.noteOn((int)(r.next() % 128), 1 + (int)(r.next() % 127));
            if (r.uni() < 0.02f) { int idx = (int)(r.next() % (uint32_t)t.size()); const ParamDef& d = t.defs[(size_t)idx]; if (d.key != "lim.on" && d.key != "lim.ceil") e.setParam(idx, r.range(d.minV, d.maxV)); }
            e.process(L.data(), R.data(), nullptr, nullptr, n);
            for (int i = 0; i < n; ++i) {
                float a = L[(size_t)i], c = R[(size_t)i];
                if (!std::isfinite(a) || !std::isfinite(c)) { ++bad; std::printf("NaN/Inf it=%d fs=%.0f\n", it, fs); goto next; }
                worst = std::max(worst, (double)std::max(std::fabs(a), std::fabs(c)));
            }
        }
        next:;
    }
    std::printf("fuzz done: %d bad, worst output peak %.4f (ceiling 0.9660)\n", bad, worst);
    return bad || worst > 0.9661 ? 1 : 0;
}
