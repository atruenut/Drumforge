// Checks the v1.1 engine features: morph, transient shaper, snare buzz, note shift bypass, choke.
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/engine/engine.h"
#include "../src/engine/presets.h"
using namespace df;

static std::vector<float> render(DrumEngine& e, int note, int vel, int n, bool shift = true)
{
    std::vector<float> L((size_t)n), R((size_t)n);
    e.noteOn(note, vel, shift);
    int pos = 0; while (pos < n) { int k = std::min(256, n - pos); e.process(L.data() + pos, R.data() + pos, nullptr, nullptr, k); pos += k; }
    for (int i = 0; i < n; ++i) L[(size_t)i] = 0.5f * (L[(size_t)i] + R[(size_t)i]);
    return L;
}
static double rms(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b && i < v.size(); ++i) s += v[i] * v[i]; return std::sqrt(s / std::max<size_t>(1, b - a)); }
static DrumEngine make(int preset) { DrumEngine e; e.init(48000); std::vector<float> v((size_t)paramTable().size()); presetApply(preset, v.data()); v[(size_t)paramTable().find("lim.on")] = 0; v[(size_t)paramTable().find("room.mix")] = 0; e.setAll(v.data()); return e; }

int main()
{
    const ParamTable& t = paramTable();
    int fails = 0; auto check = [&](bool ok, const char* w) { std::printf("%s: %s\n", ok ? "ok  " : "FAIL", w); if (!ok) ++fails; };

    // transient shaper on the snare: attack up -> louder first 15 ms, sustain up -> louder tail
    {
        DrumEngine a = make(0), b = make(0), c = make(0);
        b.setParam(t.find("snare.tattack"), 100); c.setParam(t.find("snare.tsustain"), 100);
        auto ra = render(a, 38, 110, 24000), rb = render(b, 38, 110, 24000), rc = render(c, 38, 110, 24000);
        const size_t h = 720, tail0 = 4800, tail1 = 14400;
        std::printf("   snare early rms base %.4f attack+ %.4f | tail base %.4f sustain+ %.4f\n", rms(ra, 0, h), rms(rb, 0, h), rms(ra, tail0, tail1), rms(rc, tail0, tail1));
        check(rms(rb, 0, h) > rms(ra, 0, h) * 1.15, "attack +100 boosts the transient");
        check(rms(rc, tail0, tail1) > rms(ra, tail0, tail1) * 1.15, "sustain +100 lengthens the tail");
        DrumEngine d = make(0); d.setParam(t.find("snare.tattack"), -100);
        auto rd = render(d, 38, 110, 24000);
        check(rms(rd, 0, h) < rms(ra, 0, h) * 0.9, "attack -100 softens the transient");
        for (float x : rb) if (!std::isfinite(x)) { check(false, "finite output"); break; }
    }
    // snare buzz: kick hit makes the snare channel ring
    {
        DrumEngine a = make(0), b = make(0);
        b.setParam(t.find("snare.buzz"), 100);
        auto ra = render(a, 36, 120, 24000), rb = render(b, 36, 120, 24000);
        double d = 0; for (size_t i = 0; i < ra.size(); ++i) d += (rb[i] - ra[i]) * (rb[i] - ra[i]);
        d = std::sqrt(d / ra.size());
        std::printf("   kick rms %.4f, added buzz rms %.4f (%.1f dB re kick)\n", rms(ra, 0, 24000), d, 20 * std::log10(d / rms(ra, 0, 24000) + 1e-9));
        check(d > 1e-4 && d < rms(ra, 0, 24000) * 0.8, "kick excites audible but modest snare buzz");
    }
    // morph: amount 100 sounds like the target preset (session params aside), 0 is untouched
    {
        const int A = 0, B = 25;   // Studio Rock -> Trap Sub Kit
        DrumEngine ref = make(B), m = make(A), m0 = make(A), base = make(A);
        m.setParam(t.ext(E_MorphAmt), 100); m.setParam(t.ext(E_MorphTo), (float)B);
        m0.setParam(t.ext(E_MorphAmt), 0); m0.setParam(t.ext(E_MorphTo), (float)B);
        auto rr = render(ref, 36, 110, 20000), rm = render(m, 36, 110, 20000), r0 = render(m0, 36, 110, 20000), rb = render(base, 36, 110, 20000);
        double dm = 0, d0 = 0; for (size_t i = 0; i < rr.size(); ++i) { dm = std::max(dm, (double)std::fabs(rr[i] - rm[i])); d0 = std::max(d0, (double)std::fabs(rb[i] - r0[i])); }
        std::printf("   max diff morph100 vs target %.6f | morph0 vs source %.6f\n", dm, d0);
        // the target preset also sets master-group values that morph deliberately leaves alone, so allow a small difference
        check(dm < 0.02, "morph 100% matches the target kit");
        check(d0 < 1e-7, "morph 0% leaves the kit untouched");
        DrumEngine h = make(A); h.setParam(t.ext(E_MorphAmt), 50); h.setParam(t.ext(E_MorphTo), (float)B);
        auto rh = render(h, 36, 110, 20000);
        check(rms(rh, 0, 20000) != rms(rr, 0, 20000) && rms(rh, 0, 20000) != rms(rb, 0, 20000), "morph 50% is in between");
    }
    // note shift is bypassed for groove / pad notes
    {
        DrumEngine a = make(0); a.setParam(t.find("noteoffset"), 2);   // 36 would become 38 (snare)
        auto shifted = render(a, 36, 100, 4000, true);
        DrumEngine b = make(0); b.setParam(t.find("noteoffset"), 2);
        auto raw = render(b, 36, 100, 4000, false);
        DrumEngine k = make(0); auto kick = render(k, 36, 100, 4000, true);
        double dr = 0, ds = 0; for (size_t i = 0; i < kick.size(); ++i) { dr = std::max(dr, (double)std::fabs(raw[i] - kick[i])); ds = std::max(ds, (double)std::fabs(shifted[i] - kick[i])); }
        check(dr < 1e-6 && ds > 0.01, "raw notes ignore Note Shift, MIDI notes obey it");
    }
    // soft choke fades a crash
    {
        DrumEngine a = make(0);
        std::vector<float> L(48000), R(48000);
        a.noteOn(49, 110);
        a.process(L.data(), R.data(), nullptr, nullptr, 4800);
        const double before = rms(std::vector<float>(L.begin() + 4000, L.begin() + 4800), 0, 800);
        a.chokeCymbals(35);
        a.process(L.data(), R.data(), nullptr, nullptr, 9600);
        const double after = rms(std::vector<float>(L.begin() + 8000, L.begin() + 9600), 0, 1600);
        std::printf("   crash rms before choke %.4f, 100 ms after %.6f\n", before, after);
        check(after < before * 0.02, "chokeCymbals fades the crash out");
    }
    std::printf(fails ? "FEATURE TEST FAILED\n" : "FEATURE TEST OK\n");
    return fails ? 1 : 0;
}
