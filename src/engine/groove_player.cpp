#include "groove_player.h"

#include <algorithm>
#include <cmath>

namespace df {

int GroovePlayer::process(const Transport& t, double sr, int n, const std::vector<float>& v, const UserBank* bank, std::vector<GrooveOut>& out)
{
    const ParamTable& tb = paramTable();
    auto P = [&](int idx) { return v[(size_t)idx]; };
    jumped = false;
    const bool grooveOn = P(tb.groove(GR_On)) > 0.5f;
    const bool hostRun = t.playing && t.ppqValid;
    const bool run = grooveOn && (hostRun || (!t.playing && P(tb.groove(GR_Stopped)) > 0.5f));
    if (!run) { lastValid = false; phase = -1.f; if (!t.playing) selfPpq = 0; else selfPpq = 0; return 0; }

    const double tempo = (t.tempoValid && t.tempo > 1.0) ? t.tempo : 120.0;
    const double bps = tempo / (60.0 * sr);                  // beats per sample
    double from = hostRun ? t.ppq : selfPpq;
    if (lastValid) {
        // the host's position is only trusted when it is not simply the continuation of the previous block;
        // continuity is taken from our own running position so blocks never overlap or leave gaps (no doubled / dropped hits)
        if (std::fabs(from - lastEnd) <= std::max(1e-4, 8.0 * bps)) from = lastEnd;
        else jumped = true;
    } else jumped = true;

    double to = from + n * bps;
    if (hostRun && t.cycleActive && t.cycleEnd > t.cycleStart && from < t.cycleEnd - 1e-9 && to > t.cycleEnd) to = t.cycleEnd;   // never play past the loop end

    GrooveSettings gs;
    gs.bank = bank;
    gs.pattern = (int)std::lround(P(tb.groove(GR_Pattern)));
    gs.swing = P(tb.groove(GR_Swing)); gs.intensity = P(tb.groove(GR_Intensity));
    gs.rate = (int)std::lround(P(tb.groove(GR_Rate))); gs.human = P(tb.groove(GR_Human));
    const int fillSel = (int)std::lround(P(tb.groove(GR_Fill)));
    gs.fillEvery = fillSel == 0 ? 0 : (1 << (fillSel - 1));
    gs.laidBack = P(tb.ext(E_GrLaid));
    gs.crashEvery = 0;   // the groove never adds crash cymbals of its own (the old Crash Accent switch is retired)
    int mask = 0;
    for (int i = 0; i < 6; ++i) if (P(tb.ext(E_GrMuteKick + i)) > 0.5f) mask |= 1 << i;
    gs.laneMuteMask = mask;
    const double cyc = grooveCycleBeats(gs);
    phase = cyc > 0 ? (float)(std::fmod(std::max(0.0, from), cyc) / cyc) : 0.f;

    int added = 0;
    if (to > from) {
        const int ng = grooveGenerate(gs, from, to, buf.data(), (int)buf.size());
        for (int i = 0; i < ng; ++i) {
            int off = (int)((buf[(size_t)i].ppq - from) / bps);
            off = std::max(0, std::min(n - 1, off));
            out.push_back({off, buf[(size_t)i].note, buf[(size_t)i].velocity});
            ++added;
        }
    }
    lastEnd = to; lastValid = true;
    selfPpq = hostRun ? 0.0 : from + n * bps;
    return added;
}

}  // namespace df
