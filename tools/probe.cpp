// probe <note> [vel] [key=value ...]  -> dry envelope (RMS per 25 ms) of one hit, room off
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "../src/engine/engine.h"
using namespace df;
int main(int argc, char** argv)
{
    int note = argc > 1 ? std::atoi(argv[1]) : 36;
    int vel = argc > 2 ? std::atoi(argv[2]) : 100;
    DrumEngine e; e.init(48000);
    const ParamTable& t = paramTable();
    e.setParam(t.find("room.mix"), 0); e.setParam(t.find("humanize"), 0); e.setParam(t.find("lim.on"), 0);
    for (int i = 3; i < argc; ++i) {
        std::string s = argv[i]; size_t eq = s.find('=');
        int idx = t.find(s.substr(0, eq)); if (idx < 0) { std::printf("unknown %s\n", s.c_str()); return 1; }
        e.setParam(idx, (float)std::atof(s.c_str() + eq + 1));
    }
    e.noteOn(note, vel);
    const int W = 1200; int total = 48000 * 2;
    std::vector<float> L(total), R(total);
    for (int i = 0; i < total; i += 128) e.process(&L[i], &R[i], nullptr, nullptr, 128);
    double pk = 0;
    for (int i = 0; i < total; ++i) pk = std::max(pk, (double)std::fabs(L[i]));
    std::printf("peak %.3f\n", pk);
    for (int w = 0; w * W < total && w < 48; ++w) {
        double s = 0; for (int i = 0; i < W; ++i) s += L[w * W + i] * L[w * W + i];
        std::printf("%4d ms  %7.1f dB\n", w * 25, 10 * std::log10(s / W + 1e-12));
    }
}
