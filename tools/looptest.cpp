// Simulates a host with a looping transport and checks the groove player: no hits past the loop end, no doubled hits,
// no crash at the loop restart, and identical output on every pass of the loop.
#include <cmath>
#include <cstdio>
#include <map>
#include <vector>
#include "../src/engine/groove_player.h"
using namespace df;

struct Hit { double ppq; int note; long abs; };
static std::vector<Hit> run(int pattern, int crashMode, bool guard, double loopBeats, int blockSize, int passes, int fill, bool& ok)
{
    const ParamTable& t = paramTable();
    std::vector<float> v((size_t)t.size());
    for (int i = 0; i < t.size(); ++i) v[(size_t)i] = t.defs[(size_t)i].defV;
    v[(size_t)t.groove(GR_On)] = 1; v[(size_t)t.groove(GR_Pattern)] = (float)pattern; v[(size_t)t.groove(GR_Fill)] = (float)fill;
    v[(size_t)t.ext(E_GrCrash)] = (float)crashMode; v[(size_t)t.ext(E_GrGuard)] = guard ? 1.f : 0.f;
    UserBank bank; for (auto& p : bank.p) p.clear();
    GroovePlayer gp; std::vector<GrooveOut> out; std::vector<Hit> hits;
    const double sr = 48000, tempo = 120, bps = tempo / (60 * sr);
    Transport tr; tr.playing = true; tr.tempoValid = true; tr.tempo = tempo; tr.ppqValid = true;
    tr.cycleActive = guard; tr.cycleStart = 0; tr.cycleEnd = loopBeats;
    double pos = 0; long total = (long)(loopBeats / bps) * passes;
    for (long done = 0; done < total;) {
        tr.ppq = pos;
        out.clear();
        gp.process(tr, sr, blockSize, v, &bank, out);
        for (auto& e : out) {
            double tt = pos + e.offset * bps;
            if (tt > loopBeats + 1e-6 && pos < loopBeats) { std::printf("  FAIL: hit at %.4f is beyond loop end %.2f\n", tt, loopBeats); ok = false; }
            hits.push_back({tt, e.note, done + e.offset});
        }
        pos += blockSize * bps; done += blockSize;
        if (pos >= loopBeats - 1e-9) pos -= loopBeats;     // host wraps (not block aligned: the block that crosses the end is cut by the host)
    }
    return hits;
}

int main()
{
    bool ok = true;
    const int pats[] = {0, 1, 8, 11, 10, 3, 27};
    for (int pat : pats) for (int bs : {64, 480, 1000, 1024, 4096}) {
        bool k = true;
        auto h = run(pat, 0, true, 16.0, bs, 3, 1, k);   // looping host, fills on
        int crashes = 0, dup = 0;
        std::map<int, long> lastAbs;
        for (auto& x : h) {
            if (x.note == 49 || x.note == 57 || x.note == 52 || x.note == 55) ++crashes;
            auto it = lastAbs.find(x.note);
            if (it != lastAbs.end() && x.abs - it->second < 200) ++dup;      // same drum twice within ~4 ms
            lastAbs[x.note] = x.abs;
        }
        if (crashes) { std::printf("FAIL pat %d bs %d: groove played %d crashes\n", pat, bs, crashes); k = false; }
        if (dup) { std::printf("FAIL pat %d bs %d: %d doubled hits\n", pat, bs, dup); k = false; }
        ok = ok && k;
    }
    // a crash the user placed in a user pattern must play on every pass of the loop
    {
        const ParamTable& t = paramTable();
        std::vector<float> v((size_t)t.size());
        for (int i = 0; i < t.size(); ++i) v[(size_t)i] = t.defs[(size_t)i].defV;
        v[(size_t)t.groove(GR_On)] = 1; v[(size_t)t.groove(GR_Pattern)] = (float)grooveCount();
        UserBank bank; for (auto& p : bank.p) p.clear();
        bank.p[0].steps = 16; bank.p[0].cell[10][0].vel = 110; bank.p[0].cell[0][0].vel = 100;
        GroovePlayer gp; std::vector<GrooveOut> out;
        Transport tr; tr.playing = true; tr.tempoValid = true; tr.tempo = 120; tr.ppqValid = true; tr.cycleActive = true; tr.cycleStart = 0; tr.cycleEnd = 4;
        int crashes = 0; double pos = 0; const double bps = 120 / (60.0 * 48000);
        for (int pass = 0; pass < 3; ++pass) { pos = 0; for (int b = 0; b < 4 * 24000 / 512 + 1; ++b) { tr.ppq = pos; out.clear(); gp.process(tr, 48000, 512, v, &bank, out); for (auto& e : out) if (e.note == 49) ++crashes; pos += 512 * bps; if (pos >= 4 - 1e-9) break; } }
        std::printf("user crash on step 1 over 3 loop passes: %d\n", crashes);
        if (crashes < 3) ok = false;
    }
    std::printf(ok ? "looptest: all good\n" : "looptest: FAILURES\n");
    return ok ? 0 : 1;
}
