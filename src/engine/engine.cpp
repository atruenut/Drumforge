#include "engine.h"

#include <cstring>

#include "presets.h"

namespace df {

static const float kHeadroomDb = -4.0f;   // fixed calibration: a full kit hit at 0 dB faders peaks around -3..+4 dBFS before the limiter

// ---------------------------------------------------------------- GM-style note map
namespace {
struct NoteTable {
    NoteInfo n[128];
    const char* label[128];
    NoteTable()
    {
        for (int i = 0; i < 128; ++i) label[i] = "";
        auto set = [&](int note, int piece, float semis, float decay, int artic, const char* lab) {
            n[note] = {piece, semis, decay, artic}; label[note] = lab;
        };
        set(35, Kick, -1.5f, 1.0f, 0, "Kick 2");  set(36, Kick, 0, 1.0f, 0, "Kick");
        set(37, Rim, 0, 1.0f, 0, "Sidestick");
        set(38, Snare, 0, 1.0f, 0, "Snare");      set(40, Snare, 1.5f, 0.8f, 1, "Snare (rimshot)");
        set(41, TomF, 0, 1.0f, 0, "Floor Tom");   set(43, TomF, 4.0f, 0.9f, 0, "Floor Tom Hi");
        set(45, TomM, 0, 1.0f, 0, "Mid Tom");     set(47, TomM, 3.0f, 0.9f, 0, "Mid Tom Hi");
        set(48, TomH, -2.0f, 1.0f, 0, "High Tom Lo"); set(50, TomH, 0, 1.0f, 0, "High Tom");
        set(42, Hat, 0, 1.0f, 0, "Closed Hat");   set(44, Hat, 0, 1.0f, 2, "Pedal Hat");
        set(46, Hat, 0, 1.0f, 1, "Open Hat");
        set(49, Crash, 0, 1.0f, 0, "Crash");      set(57, Crash, 3.5f, 0.75f, 0, "Crash 2");
        set(55, Crash, 8.0f, 0.3f, 4, "Splash");  set(52, Crash, -2.0f, 0.8f, 3, "China");
        set(51, Ride, 0, 1.0f, 0, "Ride");        set(59, Ride, 2.0f, 0.9f, 0, "Ride 2");
        set(53, Ride, 0, 0.8f, 2, "Ride Bell");
        set(39, Perc, 0, 1.0f, 0, "Clap");        set(54, Perc, 0, 1.0f, 1, "Tambourine");
        set(56, Perc, 0, 1.0f, 2, "Cowbell");     set(70, Perc, 0, 1.0f, 3, "Shaker");
        set(75, Perc, 0, 1.0f, 4, "Claves");      set(63, Perc, 3.0f, 1.0f, 5, "Conga Hi");
        set(64, Perc, -2.0f, 1.0f, 5, "Conga Lo"); set(81, Perc, 0, 1.0f, 6, "Triangle");
        set(80, Perc, 0, 0.3f, 6, "Triangle (mute)");
    }
};
const NoteTable& noteTable() { static const NoteTable t; return t; }
}  // namespace

const NoteInfo& noteInfo(int note) { return noteTable().n[note < 0 ? 0 : (note > 127 ? 127 : note)]; }
const char* noteLabel(int note) { return noteTable().label[note < 0 ? 0 : (note > 127 ? 127 : note)]; }

// ---------------------------------------------------------------- engine
DrumEngine::DrumEngine()
{
    tbl = &paramTable();
    vals.resize((size_t)tbl->size());
    for (int i = 0; i < tbl->size(); ++i) vals[(size_t)i] = tbl->defs[(size_t)i].defV;
    eff = vals; morphTarget = vals; effDirty = true;
    rng.seed(0xD2077E5u);
}

void DrumEngine::init(float sampleRate)
{
    fs = sampleRate;
    room.init(fs);
    eq.init(fs); comp.init(fs); lim.init(fs);
    const int lat = lim.latency();
    for (auto& s : stemDelay) for (auto& d : s) d.init(lat + 8);
    for (auto& s : strip) {
        s.gain.setTime(fs, 8); s.panS.setTime(fs, 8); s.send.setTime(fs, 8);
        s.gain.init(1); s.panS.init(0); s.send.init(0);
        s.toneDb = 1e9f;
    }
    {   // transient shaper detectors + snare sympathetic buzz filters
        auto k = [&](float ms) { return 1.0f - std::exp(-1.0f / std::max(1.0f, ms * 0.001f * fs)); };
        kEfA = k(0.5f); kEfR = k(18.0f); kEsA = k(22.0f); kEsR = k(140.0f);
        buzzLp.setLowpass(fs, 260.0f, 0.7f); buzzBp1.setBandpass(fs, 4200.0f, 0.6f); buzzBp2.setBandpass(fs, 7600.0f, 0.5f);
        buzzLp.reset(); buzzBp1.reset(); buzzBp2.reset(); buzzEnv = 0;
    }
    masterGain.setTime(fs, 8); masterGain.init(1);
    widthS.setTime(fs, 8); widthS.init(1);
    reset();
    updateBlockParams();
    for (auto& s : strip) { s.gain.v = s.gain.target; s.panS.v = s.panS.target; s.send.v = s.send.target; }
    masterGain.v = masterGain.target; widthS.v = widthS.target;
}

void DrumEngine::reset()
{
    for (auto& v : kick) v.active = false;
    for (auto& v : snare) v.active = false;
    for (auto& v : rim) v.active = false;
    for (auto& t : tom) for (auto& v : t) v.active = false;
    for (auto& v : hat) v.active = false;
    for (auto& v : crash) v.active = false;
    for (auto& v : ride) v.active = false;
    for (auto& v : perc) v.active = false;
    room.reset();
    for (auto& s : stemDelay) for (auto& d : s) d.clear();
    lim.init(fs);
    for (int p = 0; p < NumPieces; ++p) { act[p] = 0; strip[p].ef = strip[p].es = 0; for (auto& b : strip[p].lo) b.reset(); for (auto& b : strip[p].hi) b.reset(); }
    buzzEnv = 0; buzzLp.reset(); buzzBp1.reset(); buzzBp2.reset();
}

void DrumEngine::chokeCymbals(float ms)
{
    for (auto& v : crash) if (v.active) v.choke(ms);
    for (auto& v : ride) if (v.active) v.choke(ms);
}

// vals -> eff: the user's settings blended toward the Morph target preset (numbers in normalised space so log scales morph evenly;
// switches and choices flip at the halfway point). Groove / morph / limiter / note-shift / mute / solo are never morphed.
void DrumEngine::rebuildEffective()
{
    if (!effDirty) return;
    effDirty = false;
    eff = vals;
    const float amt = vals[(size_t)tbl->ext(E_MorphAmt)] * 0.01f;
    if (amt <= 0.0005f) return;
    const int to = std::max(0, std::min(presetCount() - 1, (int)std::lround(vals[(size_t)tbl->ext(E_MorphTo)])));
    if (to != morphIdx) { presetApply(to, morphTarget.data()); morphIdx = to; }
    for (int i = 0; i < tbl->size(); ++i) {
        if (tbl->isSession(i)) continue;
        const ParamDef& d = tbl->defs[(size_t)i];
        if (d.key == "lim.on" || d.key == "lim.ceil" || d.key == "noteoffset" || d.key == "master") continue;
        if (d.type == PT::Bool && (d.key.size() > 5 && (d.key.compare(d.key.size() - 5, 5, ".mute") == 0 || d.key.compare(d.key.size() - 5, 5, ".solo") == 0))) continue;
        const float a = vals[(size_t)i], b = morphTarget[(size_t)i];
        if (a == b) continue;
        if (d.type == PT::Bool || d.type == PT::Choice) { if (amt >= 0.5f) eff[(size_t)i] = b; continue; }
        const float na = tbl->toNorm(i, a), nb = tbl->toNorm(i, b);
        eff[(size_t)i] = tbl->toPlain(i, na + (nb - na) * amt);
    }
}

void DrumEngine::allSoundOff() { reset(); }

void DrumEngine::setParam(int idx, float plain)
{
    if (idx < 0 || idx >= (int)vals.size()) return;
    const ParamDef& d = tbl->defs[(size_t)idx];
    const float nv = std::min(d.maxV, std::max(d.minV, plain));
    if (nv != vals[(size_t)idx]) effDirty = true;
    vals[(size_t)idx] = nv;
}

void DrumEngine::setAll(const float* v)
{
    for (int i = 0; i < tbl->size(); ++i) setParam(i, v[i]);
}

template <class V, int N>
V* DrumEngine::alloc(V (&arr)[N])
{
    V* best = &arr[0];
    for (int i = 0; i < N; ++i) {
        if (!arr[i].active) return &arr[i];
        if (arr[i].age > best->age) best = &arr[i];
    }
    return best;     // steal the oldest
}

HitCtx DrumEngine::makeCtx(int piece, float v, const NoteInfo& ni)
{
    HitCtx h;
    const float human = gl(G_Humanize) * 0.01f;
    float curve = gl(G_VelCurve) * 0.01f;
    v = std::pow(std::min(1.0f, std::max(0.0f, v)), std::pow(2.0f, -curve));
    v *= 1.0f + 0.04f * human * rng.bi();
    v = std::min(1.0f, std::max(0.02f, v));
    float dyn = cm(piece, C_Dyn) * 0.01f;
    h.vel = 1.0f - dyn + dyn * v;                       // Dynamics: how strongly velocity shapes the sound
    h.vel = std::max(0.05f, h.vel);
    h.tune = semisToRatio(ni.semis + gl(G_Tune)) * (1.0f + 0.004f * human * rng.bi());
    h.decay = ni.decay * (1.0f + 0.05f * human * rng.bi());
    h.artic = ni.artic;
    h.fs = fs; h.rng = &rng;
    return h;
}

void DrumEngine::noteOn(int note, int velocity, bool applyShift)
{
    rebuildEffective();
    const int shifted = note + (applyShift ? (int)std::lround(gl(G_NoteOffset)) : 0);
    if (shifted < 0 || shifted > 127) return;
    const NoteInfo& ni = noteInfo(shifted);
    if (ni.piece < 0 || velocity <= 0) return;
    const int p = ni.piece;
    const float v = (float)velocity / 127.0f;
    HitCtx h = makeCtx(p, v, ni);
    const float* s = spPtr(p);
    // each hit gets its own noise seed so repeated hits never sound machine-gunned
    switch (p) {
        case Kick: { auto* x = alloc(kick); x->trigger(s, h); break; }
        case Snare: { auto* x = alloc(snare); x->trigger(s, h); break; }
        case Rim: { auto* x = alloc(rim); x->trigger(s, h); break; }
        case TomF: case TomM: case TomH: { auto* x = alloc(tom[p - TomF]); x->trigger(s, h); break; }
        case Hat: {
            int next = 1 - hatCur;
            if (hat[hatCur].active) hat[hatCur].choke();
            hat[next].trigger(s, h); hatCur = next; break; }
        case Crash: { auto* x = alloc(crash); x->trigger(s, h, ni.artic); x->age = 0; break; }
        case Ride: { auto* x = alloc(ride); x->trigger(s, h, ni.artic == 2 ? 2 : 1); x->age = 0; break; }
        case Perc: { auto* x = alloc(perc); x->trigger(s, h, ni.artic); break; }
    }
    room.hit();
    act[p] = std::max(act[p], h.vel);
}

void DrumEngine::updateBlockParams()
{
    rebuildEffective();
    anySolo = false;
    for (int p = 0; p < NumPieces; ++p) anySolo = anySolo || cm(p, C_Solo) > 0.5f;
    for (int p = 0; p < NumPieces; ++p) {
        Strip& s = strip[p];
        const bool muted = cm(p, C_Mute) > 0.5f || (anySolo && cm(p, C_Solo) < 0.5f);
        static const float kTrimDb[NumPieces] = {0.0f, 5.5f, 2.0f, -1.2f, -1.2f, -1.2f, 5.0f, -12.5f, -3.4f, 0.0f};   // loudness calibration: a 0 dB fader = a balanced kit
        s.gain.target = muted ? 0.0f : dbToGain(cm(p, C_Level) + kTrimDb[p] + kHeadroomDb);
        if (cm(p, C_Level) <= tbl->defs[(size_t)tbl->common(p, C_Level)].minV + 0.01f) s.gain.target = 0;
        s.panS.target = cm(p, C_Pan) * 0.01f;
        s.send.target = muted ? 0.0f : cm(p, C_Room) * 0.01f;
        float tone = cm(p, C_Tone);
        if (tone != s.toneDb) {
            s.toneDb = tone;
            for (int c = 0; c < 2; ++c) {
                s.lo[c].setLowShelf(fs, 300.0f, -tone * 0.5f, 0.8f);
                s.hi[c].setHighShelf(fs, 2800.0f, tone * 0.8f, 0.8f);
            }
        }
        s.tAtt = eff[(size_t)tbl->pieceFx(p, 0)] * 0.01f; s.tSus = eff[(size_t)tbl->pieceFx(p, 1)] * 0.01f;
        s.drive = cm(p, C_Drive) * 0.01f;
        s.dGain = 1.0f + 8.0f * s.drive * s.drive;
        s.dComp = 0.7f / softClip(0.7f * s.dGain);
    }
    buzzAmt = ex(E_Buzz) * 0.01f;
    masterGain.target = gl(G_Master) <= -59.99f ? 0.0f : dbToGain(gl(G_Master));
    widthS.target = gl(G_Width) * 0.01f;
    eq.set(gl(G_EqLo), gl(G_EqMid), gl(G_EqMidF), gl(G_EqHi));
    comp.set(gl(G_CompThr), gl(G_CompRatio), gl(G_CompAtt), gl(G_CompRel), gl(G_CompMakeup), gl(G_CompMix) * 0.01f);
    sat.set(gl(G_SatDrive) * 0.01f, gl(G_SatMix) * 0.01f);
    room.setParams((int)std::lround(gl(G_RoomType)), gl(G_RoomSize) * 0.01f, gl(G_RoomDecay), gl(G_RoomDamp),
                   gl(G_RoomPre), gl(G_RoomGate), gl(G_RoomWidth) * 0.01f, gl(G_RoomMix) * 0.01f);
    lim.set(gl(G_LimOn) > 0.5f, gl(G_LimCeil));
}

void DrumEngine::process(float* mainL, float* mainR, float* const* stemL, float* const* stemR, int n)
{
    updateBlockParams();
    static const int stemOf[NumPieces] = {0, 1, 1, 2, 2, 2, 3, 4, 4, 5};
    const float roomMix = room.mixAmount();
    const int lat = lim.latency();
    float peak[NumPieces] = {};

    for (int i = 0; i < n; ++i) {
        float pl[NumPieces] = {}, pr[NumPieces] = {};
        // ---- voices
        { float s = 0; for (auto& v : kick) s += v.process(); pl[Kick] = pr[Kick] = s; }
        { float s = 0; for (auto& v : snare) s += v.process(); pl[Snare] = pr[Snare] = s; }
        { float s = 0; for (auto& v : rim) s += v.process(); pl[Rim] = pr[Rim] = s; }
        for (int t = 0; t < 3; ++t) { float s = 0; for (auto& v : tom[t]) s += v.process(); pl[TomF + t] = pr[TomF + t] = s; }
        { float s = 0; for (auto& v : hat) s += v.process(); pl[Hat] = pr[Hat] = s; }
        { float l = 0, r = 0; for (auto& v : crash) { float a, b; v.process(a, b); l += a; r += b; } pl[Crash] = l; pr[Crash] = r; }
        { float l = 0, r = 0; for (auto& v : ride) { float a, b; v.process(a, b); l += a; r += b; } pl[Ride] = l; pr[Ride] = r; }
        { float s = 0; for (auto& v : perc) s += v.process(); pl[Perc] = pr[Perc] = s; }
        if (buzzAmt > 0.0005f) {   // sympathetic snare wires: low drums make the wires rattle
            const float lo = buzzLp.process(pl[Kick] + pl[TomF] + pl[TomM] + pl[TomH]);
            const float a = std::fabs(lo);
            buzzEnv += (a - buzzEnv) * (a > buzzEnv ? 0.02f : 1.0f - std::exp(-1.0f / (0.16f * fs)));
            const float buzz = (buzzBp1.process(rng.bi()) + 0.5f * buzzBp2.process(rng.bi())) * buzzEnv * buzzAmt * 0.1f;
            pl[Snare] += buzz; pr[Snare] += buzz;
        }

        // ---- strips
        float sumL = 0, sumR = 0, sendL = 0, sendR = 0;
        float stL[kStems] = {}, stR[kStems] = {};
        for (int p = 0; p < NumPieces; ++p) {
            Strip& s = strip[p];
            float l = pl[p], r = pr[p];
            const float g = s.gain.next(), pan = s.panS.next(), snd = s.send.next();
            if (s.tAtt != 0.0f || s.tSus != 0.0f) {   // transient shaper: attack / sustain
                const float x = 0.5f * (std::fabs(l) + std::fabs(r));
                s.ef += (x - s.ef) * (x > s.ef ? kEfA : kEfR);
                s.es += (x - s.es) * (x > s.es ? kEsA : kEsR);
                const float d = (s.ef - s.es) / (s.ef + s.es + 1e-5f);
                float tg = std::exp(0.8f * (d > 0 ? s.tAtt * d : -s.tSus * d));
                tg = std::min(2.8f, std::max(0.35f, tg));
                l *= tg; r *= tg;
            }
            if (s.toneDb != 0.0f) {
                l = s.hi[0].process(s.lo[0].process(l)); r = s.hi[1].process(s.lo[1].process(r));
            }
            if (s.drive > 0.001f) {
                float dl = softClip(l * s.dGain) * s.dComp, dr = softClip(r * s.dGain) * s.dComp;
                l += (dl - l) * s.drive; r += (dr - r) * s.drive;
            }
            // balance-style pan keeps stereo sources wide; mono sources get constant-power pan
            const bool stereoSrc = (p == Crash || p == Ride);
            float gl_, gr_;
            if (stereoSrc) { gl_ = pan > 0 ? 1.0f - pan : 1.0f; gr_ = pan < 0 ? 1.0f + pan : 1.0f; }
            else { float th = (pan + 1.0f) * 0.7853982f; gl_ = std::cos(th); gr_ = std::sin(th); }
            l *= g * gl_; r *= g * gr_;
            peak[p] = std::max(peak[p], std::max(std::fabs(l), std::fabs(r)));
            sumL += l; sumR += r;
            sendL += l * snd; sendR += r * snd;
            const int st = stemOf[p];
            stL[st] += l; stR[st] += r;
        }
        // ---- room
        float rl, rr;
        room.process(sendL, sendR, rl, rr);
        rl *= roomMix * 2.0f; rr *= roomMix * 2.0f;
        stL[6] = rl; stR[6] = rr;
        float L = sumL + rl, R = sumR + rr;

        // ---- master chain
        {
            float w = widthS.next();
            float m = 0.5f * (L + R), sd = 0.5f * (L - R) * w;
            L = m + sd; R = m - sd;
        }
        eq.process(L, R);
        sat.process(L, R);
        comp.process(L, R);
        const float mg = masterGain.next();
        L *= mg; R *= mg;
        L = dcL.process(L); R = dcR.process(R);
        lim.process(L, R);
        if (!(L == L)) L = 0;        // NaN guard
        if (!(R == R)) R = 0;
        if (mainL) mainL[i] = L;
        if (mainR) mainR[i] = R;
        if (stemL || stemR) {
            for (int k = 0; k < kStems; ++k) {
                stemDelay[k][0].write(stL[k]); stemDelay[k][1].write(stR[k]);
                if (stemL && stemL[k]) stemL[k][i] = stemDelay[k][0].read(lat);
                if (stemR && stemR[k]) stemR[k][i] = stemDelay[k][1].read(lat);
            }
        }
    }
    // activity meters (fast attack, slow release; one block granularity)
    const float rel = std::exp(-(float)n / (0.25f * fs));
    for (int p = 0; p < NumPieces; ++p) act[p] = std::max(peak[p] > 0.0f ? std::min(1.0f, peak[p] * 2.0f) : 0.0f, act[p] * rel);
}

}  // namespace df
