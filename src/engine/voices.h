// DrumForge - synthesized drum voices (header only).
// Every voice is allocation free, deterministic given its RNG, and works at any sample rate.
#pragma once
#include "../dsp/dsp.h"
#include "params.h"

namespace df {

// T60 style coefficient: amplitude falls by 60 dB after `ms`
inline float t60Coef(float ms, float fs) { return std::exp(-6.907755f / std::max(1.0f, ms * 0.001f * fs)); }

struct HitCtx {
    float vel = 1;        // 0..1 after the velocity curve
    float tune = 1;       // pitch ratio (note map * humanize)
    float decay = 1;      // decay multiplier (note map)
    int artic = 0;        // articulation selector (voice specific)
    float fs = 48000;
    Rng* rng = nullptr;
};

// A tiny helper: exponentially decaying value
struct Env {
    float v = 0, c = 0;
    void set(float start, float coef) { v = start; c = coef; }
    inline float tick() { float r = v; v *= c; return r; }
};

// ====================================================================== KICK
struct KickVoice {
    bool active = false;
    float fs = 48000, ph = 0, subPh = 0;
    float f0 = 50, sweepOct = 1.8f, vel = 1, punch = 0.4f, clickAmt = 0.4f, subAmt = 0.3f;
    float sweepEnv = 1, sweepC = 0.99f, ampEnv = 1, ampC = 0.9999f, subEnv = 1, subC = 0.9999f;
    float punchEnv = 1, punchC = 0.99f, attack = 0, attackInc = 0.01f;
    Env click, thump;
    Biquad clickBp, thumpLp, damp;
    Rng* rng = nullptr;
    int age = 0;

    void trigger(const float* sp, const HitCtx& h)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0;
        float v = vel;
        f0 = sp[KickP::Tune] * h.tune;
        sweepOct = sp[KickP::Sweep] * (0.65f + 0.35f * v);
        sweepEnv = 1.0f;
        sweepC = std::exp(-1.0f / std::max(1.0f, sp[KickP::SweepTime] * 0.001f * fs));
        float dec = sp[KickP::Decay] * h.decay;
        ampEnv = 1.0f; ampC = t60Coef(dec, fs);
        subEnv = 1.0f; subC = t60Coef(dec * 1.35f, fs);
        punch = sp[KickP::Punch] * 0.01f;
        punchEnv = 1.0f; punchC = std::exp(-1.0f / (0.012f * fs));
        clickAmt = sp[KickP::Click] * 0.01f * (0.3f + 0.7f * v * v);
        subAmt = sp[KickP::Sub] * 0.01f;
        ph = 0.25f; subPh = 0.0f;
        attack = 0; attackInc = 1.0f / (0.0006f * fs);
        clickBp.setBandpass(fs, sp[KickP::ClickTone] * (0.8f + 0.4f * v), 0.9f);
        click.set(1.0f, t60Coef(7.0f, fs));
        thumpLp.setLowpass(fs, 700.0f, 0.7f);
        thump.set(1.0f, t60Coef(18.0f, fs));
        damp.setLowpass(fs, sp[KickP::Damp] * (0.6f + 0.4f * v), 0.7f);
        clickBp.reset(); thumpLp.reset(); damp.reset();
        active = true;
    }
    float process()
    {
        if (!active) return 0;
        float se = sweepEnv; sweepEnv *= sweepC;
        float f = f0 * std::exp2(sweepOct * se);
        ph += f / fs; ph -= std::floor(ph);
        float body = fastSin(ph);
        float pe = punchEnv; punchEnv *= punchC;
        float g = 1.0f + 5.0f * punch * pe * (0.5f + 0.5f * vel);
        body = softClip(body * g) / softClip(g);              // harmonic bite right at the hit
        attack = std::min(1.0f, attack + attackInc);
        float a = ampEnv * attack; ampEnv *= ampC;
        float out = body * a * 0.9f;
        subPh += f0 * 0.5f / fs; subPh -= std::floor(subPh);
        float sb = fastSin(subPh);
        out += sb * subEnv * subAmt * 0.45f * attack; subEnv *= subC;
        float n = rng->bi();
        float ce = click.tick();
        out += clickBp.process(n) * ce * clickAmt * 1.6f;
        float te = thump.tick();
        out += thumpLp.process(n) * te * clickAmt * 0.9f;
        out = damp.process(out) * (0.55f + 0.45f * vel);
        if (ampEnv < 1e-4f && subEnv < 1e-4f) active = false;
        ++age;
        return out;
    }
};

// ====================================================================== SNARE
struct SnareVoice {
    static constexpr int M = 5;
    bool active = false;
    float fs = 48000;
    float ph[M] = {}, inc[M] = {}, amp[M] = {}, mc[M] = {}, menv[M] = {};
    float shellPh = 0, shellInc = 0, shellAmp = 0, shellEnv = 1, shellC = 0.999f;
    float dropEnv = 1, dropC = 0.99f, dropAmt = 0.15f;
    float wA = 1, wB = 1, wAc = 0.999f, wBc = 0.9995f, wAtt = 0, wAttInc = 0.01f, wireAmt = 0.6f;
    Biquad wireHp, wireBp, wireLp, snapBp, damp;
    Env snap, rim; float rimPh[3] = {}, rimInc[3] = {}, rimAmt = 0;
    float vel = 1;
    Rng* rng = nullptr;
    int age = 0;

    void trigger(const float* sp, const HitCtx& h)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0;
        const float v = vel;
        const float f = sp[SnareP::Tune] * h.tune;
        const float tension = sp[SnareP::Tension] * 0.01f;
        static const float ratios[M] = {1.0f, 1.593f, 2.136f, 2.296f, 2.653f};
        static const float amps[M] = {1.0f, 0.55f, 0.34f, 0.22f, 0.14f};
        const float bodyT60 = sp[SnareP::BodyDecay] * h.decay;
        const float body = sp[SnareP::Body] * 0.01f;
        const float shell = sp[SnareP::Shell] * 0.01f;
        for (int k = 0; k < M; ++k) {
            float r = 1.0f + (ratios[k] - 1.0f) * (0.85f + 0.35f * tension);
            inc[k] = f * r / fs;
            ph[k] = 0.0f;
            amp[k] = amps[k] * body * (k == 0 ? 1.0f : (0.5f + 0.7f * v) * (0.6f + 0.8f * shell));
            menv[k] = 1.0f;
            mc[k] = t60Coef(bodyT60 / (1.0f + 0.55f * (float)k), fs);
        }
        shellInc = f * 0.62f / fs; shellPh = 0; shellAmp = shell * 0.5f * body; shellEnv = 1;
        shellC = t60Coef(bodyT60 * 1.5f, fs);
        dropEnv = 1.0f; dropC = std::exp(-1.0f / (0.010f * fs)); dropAmt = 0.10f + 0.12f * v;

        const float wd = sp[SnareP::WireDecay] * h.decay;
        wA = 1; wB = 1; wAc = t60Coef(wd * 0.3f, fs); wBc = t60Coef(wd, fs);
        wAtt = 0; wAttInc = 1.0f / (0.0007f * fs);
        wireAmt = sp[SnareP::Wires] * 0.01f * (0.35f + 0.9f * v);
        const float wt = sp[SnareP::WireTone];
        wireHp.setHighpass(fs, 1400.0f, 0.7f);
        wireBp.setBandpass(fs, wt * (0.85f + 0.3f * v), 0.55f);
        wireLp.setLowpass(fs, std::min(16000.0f, wt * 3.2f), 0.7f);
        snapBp.setBandpass(fs, 3800.0f * (0.9f + 0.3f * v), 0.8f);
        snap.set(sp[SnareP::Snap] * 0.01f * (0.3f + 0.9f * v), t60Coef(5.0f, fs));
        rimAmt = sp[SnareP::RimShot] * 0.01f * v;
        static const float rr[3] = {1650.0f, 2250.0f, 3350.0f};
        for (int k = 0; k < 3; ++k) { rimInc[k] = rr[k] * h.tune / fs; rimPh[k] = 0; }
        rim.set(1.0f, t60Coef(30.0f, fs));
        damp.setLowpass(fs, sp[SnareP::Damp], 0.7f);
        wireHp.reset(); wireBp.reset(); wireLp.reset(); snapBp.reset(); damp.reset();
        active = true;
    }
    float process()
    {
        if (!active) return 0;
        float de = dropEnv; dropEnv *= dropC;
        float pm = 1.0f + dropAmt * de;
        float out = 0;
        for (int k = 0; k < M; ++k) {
            ph[k] += inc[k] * pm; ph[k] -= std::floor(ph[k]);
            out += fastSin(ph[k]) * menv[k] * amp[k];
            menv[k] *= mc[k];
        }
        shellPh += shellInc; shellPh -= std::floor(shellPh);
        out += fastSin(shellPh) * shellEnv * shellAmp; shellEnv *= shellC;
        out *= 0.42f;

        float n = rng->bi();
        wAtt = std::min(1.0f, wAtt + wAttInc);
        float we = (0.62f * wA + 0.38f * wB) * wAtt; wA *= wAc; wB *= wBc;
        float sym = 0.65f + 0.9f * menv[0];                       // wires buzz harder while the head rings
        float wires = (wireBp.process(n) * 0.9f + wireHp.process(n) * 0.35f);
        wires = wireLp.process(wires);
        out += wires * we * wireAmt * sym * 0.62f;

        float se = snap.tick();
        out += snapBp.process(rng->bi()) * se * 0.9f;
        if (rimAmt > 0.001f) {
            float re = rim.tick(), r = 0;
            for (int k = 0; k < 3; ++k) { rimPh[k] += rimInc[k]; rimPh[k] -= std::floor(rimPh[k]); r += fastSin(rimPh[k]) * (1.0f - 0.25f * (float)k); }
            out += r * re * rimAmt * 0.38f;
        }
        out = damp.process(out);
        out = softClip(out * 0.8f) * 1.25f;               // tames the crest factor of the rimshot/snap transient
        if (menv[0] < 1e-4f && wB < 1e-4f) active = false;
        ++age;
        return out;
    }
};

// ====================================================================== SIDESTICK
struct RimVoice {
    bool active = false;
    float fs = 48000, ph[3] = {}, inc[3] = {}, amp[3] = {}, env = 1, c = 0.99f, vel = 1;
    Env click; Biquad bp;
    Rng* rng = nullptr; int age = 0;
    void trigger(const float* sp, const HitCtx& h)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0;
        float f = sp[RimP::Tune] * h.tune, tone = sp[RimP::Tone] * 0.01f;
        inc[0] = f / fs; inc[1] = f * 1.54f / fs; inc[2] = f * 0.55f / fs;
        amp[0] = 0.8f; amp[1] = 0.25f + 0.7f * tone; amp[2] = 0.7f * (1.0f - tone) + 0.1f;
        ph[0] = ph[1] = ph[2] = 0; env = 1; c = t60Coef(sp[RimP::Decay] * h.decay, fs);
        bp.setBandpass(fs, 2400.0f * (0.8f + 0.4f * vel), 0.9f); bp.reset();
        click.set(sp[RimP::Click] * 0.01f, t60Coef(6.0f, fs));
        active = true;
    }
    float process()
    {
        if (!active) return 0;
        float o = 0;
        for (int k = 0; k < 3; ++k) { ph[k] += inc[k]; ph[k] -= std::floor(ph[k]); o += fastSin(ph[k]) * amp[k]; }
        o *= env * 0.5f; env *= c;
        o += bp.process(rng->bi()) * click.tick() * 0.8f;
        if (env < 1e-4f) active = false;
        ++age;
        return o * (0.4f + 0.6f * vel);
    }
};

// ====================================================================== TOM
struct TomVoice {
    static constexpr int M = 3;
    bool active = false;
    float fs = 48000, ph[M] = {}, inc[M] = {}, amp[M] = {}, env[M] = {}, c[M] = {};
    float resPh = 0, resInc = 0, resAmp = 0, resEnv = 1, resC = 0.999f;
    float dropEnv = 1, dropC = 0.99f, dropAmt = 0.3f, vel = 1;
    Env stick; Biquad stickBp, damp;
    Rng* rng = nullptr; int age = 0;
    void trigger(const float* sp, const HitCtx& h)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0;
        float f = sp[TomP::Tune] * h.tune, dec = sp[TomP::Decay] * h.decay, tone = sp[TomP::Tone] * 0.01f;
        static const float r[M] = {1.0f, 1.58f, 2.13f};
        static const float a[M] = {1.0f, 0.34f, 0.18f};
        for (int k = 0; k < M; ++k) {
            inc[k] = f * r[k] / fs; ph[k] = 0;
            amp[k] = a[k] * (k == 0 ? 1.0f : tone * (0.4f + 0.8f * vel) * 1.4f);
            env[k] = 1; c[k] = t60Coef(dec / (1.0f + 0.7f * (float)k), fs);
        }
        float reso = sp[TomP::Reso] * 0.01f;
        resInc = f * 0.93f / fs; resPh = 0.13f; resAmp = reso * 0.65f; resEnv = 1; resC = t60Coef(dec * 1.35f, fs);
        dropAmt = sp[TomP::Drop] * 0.01f * 0.6f * (0.5f + 0.5f * vel);
        dropEnv = 1; dropC = std::exp(-1.0f / std::max(1.0f, sp[TomP::DropTime] * 0.001f * fs));
        stickBp.setBandpass(fs, 2200.0f * (0.8f + 0.5f * vel), 0.8f); stickBp.reset();
        stick.set(sp[TomP::Attack] * 0.01f * (0.3f + 0.9f * vel), t60Coef(9.0f, fs));
        damp.setLowpass(fs, sp[TomP::Damp] * (0.55f + 0.45f * vel), 0.7f); damp.reset();
        active = true;
    }
    float process()
    {
        if (!active) return 0;
        float de = dropEnv; dropEnv *= dropC;
        float pm = 1.0f + dropAmt * de;
        float o = 0;
        for (int k = 0; k < M; ++k) {
            ph[k] += inc[k] * pm; ph[k] -= std::floor(ph[k]);
            o += fastSin(ph[k]) * amp[k] * env[k]; env[k] *= c[k];
        }
        resPh += resInc * (1.0f + dropAmt * 0.6f * de); resPh -= std::floor(resPh);
        o += fastSin(resPh) * resAmp * resEnv; resEnv *= resC;
        o *= 0.55f;
        o += stickBp.process(rng->bi()) * stick.tick() * 0.7f;
        o = damp.process(o);
        if (env[0] < 1e-4f && resEnv < 1e-4f) active = false;
        ++age;
        return o;
    }
};

// ====================================================================== HI-HAT
struct HatVoice {
    bool active = false;
    float fs = 48000, vel = 1;
    BlepSquare sq[6];
    Biquad clusterBp, nHp, nLp, tickBp, outLp, outHp;
    Resonator ring[3];
    float e1 = 1, e2 = 1, c1 = 0.99f, c2 = 0.999f, att = 0, attInc = 0.05f;
    float noiseAmt = 1, clusterAmt = 0.3f, ringAmt = 0.3f;
    Env tick, chick; float chickPh = 0, chickInc = 0.01f;
    float chokeG = 1, chokeC = 1.0f;
    int artic = 0;                  // 0 closed, 1 open, 2 pedal
    Rng* rng = nullptr; int age = 0;

    void trigger(const float* sp, const HitCtx& h)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0; artic = h.artic; chokeG = 1; chokeC = 1;
        const float tuneR = semisToRatio(sp[HatP::Tune]) * h.tune;
        static const float ratios[6] = {2.0f, 3.0f, 4.16f, 5.43f, 6.79f, 8.21f};
        const float base = 205.3f * tuneR;
        for (int k = 0; k < 6; ++k) {
            float jit = 1.0f + 0.004f * rng->bi();
            sq[k].set(fs, std::min(base * ratios[k] * jit, 0.45f * fs), rng->uni());
        }
        const float metal = sp[HatP::Metal] * 0.01f, chr = sp[HatP::Char] * 0.01f;
        const float bright = sp[HatP::Bright] * (0.75f + 0.35f * vel);
        float dec = artic == 1 ? sp[HatP::OpenDecay] : sp[HatP::ClosedDecay];
        if (artic == 2) dec = sp[HatP::ClosedDecay] * 0.75f;
        dec *= h.decay * (0.85f + 0.3f * vel);
        e1 = 1; e2 = 1; c1 = t60Coef(dec * 0.35f, fs); c2 = t60Coef(dec, fs);
        att = 0; attInc = 1.0f / (0.0004f * fs);
        noiseAmt = (1.0f - 0.5f * metal);
        clusterAmt = 0.18f * metal + 0.55f * chr;
        clusterBp.setBandpass(fs, std::min(bright * 0.72f, 0.45f * fs), 1.3f);
        nHp.setHighpass(fs, 5200.0f * tuneR * (0.8f + 0.4f * (1.0f - vel)), 0.8f);
        nLp.setLowpass(fs, std::min(bright * 1.15f, 0.45f * fs), 0.7f);
        outHp.setHighpass(fs, 3200.0f, 0.7f); outLp.setLowpass(fs, bright, 0.7f);
        tickBp.setBandpass(fs, 6500.0f, 0.9f);
        clusterBp.reset(); nHp.reset(); nLp.reset(); tickBp.reset(); outHp.reset(); outLp.reset();
        float ringAmtP = sp[HatP::Ring] * 0.01f;
        static const float rf[3] = {3420.0f, 6150.0f, 9380.0f};
        for (int k = 0; k < 3; ++k) ring[k].setN(fs, rf[k] * tuneR, dec * 0.000145f, 0.25f);
        ringAmt = ringAmtP * 0.9f;
        tick.set(sp[HatP::Tick] * 0.01f * (0.4f + 0.8f * vel), t60Coef(2.5f, fs));
        chick.set(artic == 2 ? 0.8f : 0.0f, t60Coef(14.0f, fs)); chickPh = 0; chickInc = 190.0f / fs;
        active = true;
    }
    void choke() { chokeC = std::exp(-1.0f / (0.004f * fs)); }
    float process()
    {
        if (!active) return 0;
        float n = rng->bi();
        float cl = 0;
        for (int k = 0; k < 6; ++k) cl += sq[k].next();
        cl = clusterBp.process(cl * (1.0f / 6.0f));
        float nz = nLp.process(nHp.process(n));
        float rg = 0;
        for (int k = 0; k < 3; ++k) rg += ring[k].process(n);
        att = std::min(1.0f, att + attInc);
        float env = (0.65f * e1 + 0.35f * e2) * att; e1 *= c1; e2 *= c2;
        float o = (nz * noiseAmt * 0.9f + cl * clusterAmt * 3.2f + rg * ringAmt) * env;
        o = outLp.process(outHp.process(o));
        o += tickBp.process(rng->bi()) * tick.tick() * 1.2f;
        if (artic == 2) {
            chickPh += chickInc; chickPh -= std::floor(chickPh);
            o += fastSin(chickPh) * chick.tick() * 0.5f;
        }
        chokeG *= chokeC;
        o *= chokeG;
        if (e2 < 1e-4f || chokeG < 1e-3f) active = false;
        ++age;
        static const float artG[3] = {1.5f, 0.7f, 1.3f};
        return o * (0.35f + 0.65f * vel) * 0.85f * artG[artic < 0 ? 0 : (artic > 2 ? 2 : artic)];
    }
};

// ====================================================================== CYMBALS (crash / ride / bell / china / splash)
// Noise-excited resonator bank (inharmonic, individually decaying, individually panned) plus a
// band-limited noise "shimmer" layer that stands in for the thousands of modes a real plate has.
struct CymbalVoice {
    static constexpr int NR = 32;
    bool active = false;
    float fs = 48000, vel = 1;
    Resonator rs[NR];
    float wl[NR], wr[NR];
    int endAt[NR] = {};                       // partial k is skipped once the voice is older than this (it has decayed away)
    float burst = 1, burstC = 0.99f, wash = 0, washEnv = 1, washC = 0.9999f, bloomV = 0, bloomC = 0.999f;
    float trash = 0, shimAmt = 0.1f;
    Biquad hpL, hpR, lpL, lpR, tickBp, shHp1, shHp2, shLp; Env tick;
    int life = 0, age = 0; float kindGain = 1;
    float fadeG = 1, fadeC = 1;                 // soft choke (transport jumps)
    Rng* rng = nullptr;
    void choke(float ms) { fadeC = std::exp(-1.0f / std::max(1.0f, ms * 0.001f * fs)); }

    // kind: 0 crash, 1 ride, 2 bell, 3 china, 4 splash
    void trigger(const float* sp, const HitCtx& h, int kind)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; age = 0; fadeG = 1; fadeC = 1;
        const bool ride = (kind == 1 || kind == 2);
        float tuneSt, decayMs, bright, washP, stereo, tickP, bellP = 0, pingP = 0, trashP = 0, bloomMs;
        if (!ride) {
            tuneSt = sp[CrashP::Tune]; decayMs = sp[CrashP::Decay]; bright = sp[CrashP::Bright];
            washP = sp[CrashP::Wash] * 0.01f; stereo = sp[CrashP::Stereo] * 0.01f; tickP = sp[CrashP::Tick] * 0.01f;
            trashP = sp[CrashP::Trash] * 0.01f; bloomMs = sp[CrashP::Bloom];
            if (kind == 3) { trashP = std::min(1.0f, trashP + 0.55f); washP *= 0.8f; }
            if (kind == 4) { bloomMs *= 0.2f; }
        } else {
            tuneSt = sp[RideP::Tune]; decayMs = sp[RideP::Decay]; bright = sp[RideP::Bright];
            washP = sp[RideP::Wash] * 0.01f; stereo = sp[RideP::Stereo] * 0.01f; tickP = 0.3f;
            bellP = sp[RideP::Bell] * 0.01f; pingP = sp[RideP::Ping] * 0.01f; trashP = sp[RideP::Dirt] * 0.01f;
            bloomMs = 4.0f;
            if (kind == 2) { bellP = 0.5f + 0.5f * bellP; pingP *= 0.3f; washP *= 0.3f; }
        }
        const float tr = semisToRatio(tuneSt) * h.tune;
        decayMs *= h.decay;
        Rng lay; lay.seed(kind == 0 ? 0x1234567u : kind == 1 ? 0xBEEF123u : kind == 2 ? 0xACE0F11u : kind == 3 ? 0x33C0FFEu : 0x5EED5u);
        float maxT60 = 0;
        for (int k = 0; k < NR; ++k) {
            float f, t60, a;
            const float u = (float)k / (float)(NR - 1);
            if (ride && k < 6) {            // bell / cup partials: tonal and long
                static const float bellR[6] = {1.0f, 2.37f, 2.76f, 5.4f, 6.58f, 8.93f};
                f = 590.0f * bellR[k] * (kind == 2 ? 1.12f : 1.0f);
                t60 = decayMs * (kind == 2 ? 1.0f : 0.5f) * (1.0f - 0.07f * (float)k);
                a = bellP * (1.0f - 0.12f * (float)k) * (kind == 2 ? 1.2f : 0.45f);
            } else if (ride && k < 12) {    // stick "ping": bright, short
                f = 2300.0f * std::pow(1.19f, (float)(k - 6)) * (1.0f + 0.04f * lay.bi());
                t60 = decayMs * 0.25f * (1.0f - 0.04f * (float)(k - 6));
                a = pingP * 0.9f;
            } else {                        // plate body
                f = 400.0f * std::pow(1.115f, (float)k * (kind == 4 ? 1.1f : 1.0f)) * (1.0f + 0.06f * lay.bi());
                float low = std::pow(1.0f - u, 1.3f);
                t60 = decayMs * (0.25f + 0.75f * low) * (1.0f + 0.2f * lay.bi());
                if (ride) t60 *= 0.8f;
                a = 0.12f + 0.88f * std::pow(std::min(1.0f, f / 5000.0f), 0.9f);
                if (ride) a *= 0.5f * (0.3f + washP);
            }
            f *= tr;
            if (f > 0.46f * fs) { a = 0; f = 0.46f * fs; }
            a /= 1.0f + std::pow(f / std::max(1000.0f, bright), 4.0f);
            t60 = std::max(t60, 20.0f);
            maxT60 = std::max(maxT60, t60);
            float pan = lay.bi() * stereo;
            wl[k] = std::sqrt(0.5f * (1.0f - pan)); wr[k] = std::sqrt(0.5f * (1.0f + pan));
            float tau = t60 * 0.001f / 6.907755f;
            rs[k].setN(fs, f, tau, a * 0.34f);
            endAt[k] = (int)((t60 * 1.15f + 40.0f) * 0.001f * fs);
        }
        burst = (0.5f + 0.9f * vel); burstC = t60Coef(ride ? 30.0f : 55.0f, fs);
        wash = washP * (0.3f + 0.9f * vel) * (ride ? 0.7f : 1.0f);
        washEnv = 1; washC = t60Coef(decayMs * 0.55f, fs);
        bloomC = std::exp(-1.0f / std::max(1.0f, bloomMs * 0.001f * fs)); bloomV = 1;
        kindGain = kind == 4 ? 1.02f : (kind == 2 ? 2.35f : 1.0f);
        trash = trashP;
        shimAmt = ride ? 0.025f * (0.4f + washP) : 0.10f * (0.4f + washP);
        shHp1.setHighpass(fs, 2600.0f * tr, 0.7f); shHp2 = shHp1; shLp.setLowpass(fs, std::min(bright * 1.2f, 0.45f * fs), 0.7f);
        shHp1.reset(); shHp2.reset(); shLp.reset();
        hpL.setHighpass(fs, 220.0f, 0.7f); hpR = hpL; lpL.setLowpass(fs, std::min(bright * 1.3f, 0.45f * fs), 0.7f); lpR = lpL;
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
        tickBp.setBandpass(fs, 7000.0f, 0.7f); tickBp.reset();
        tick.set(tickP * (0.3f + 0.9f * vel), t60Coef(3.5f, fs));
        life = (int)((maxT60 * 1.05f + 150.0f) * 0.001f * fs);
        active = true;
    }
    void process(float& L, float& R)
    {
        L = R = 0;
        if (!active) return;
        float b = burst; burst *= burstC;
        float we = washEnv * (1.0f - bloomV); washEnv *= washC; bloomV *= bloomC;
        float exc = rng->bi() * (b + wash * we * 2.0f);
        float l = 0, r = 0;
        for (int k = 0; k < NR; ++k) {
            if (age >= endAt[k]) continue;
            float y = rs[k].process(exc);
            l += y * wl[k]; r += y * wr[k];
        }
        // shimmer: decorrelated noise in each channel, shaped by strike burst + wash envelope
        float senv = (0.45f * b + wash * we * 1.4f) * shimAmt;
        const float sh = shLp.process(shHp2.process(shHp1.process(rng->bi()))) * senv;
        l += sh; r += sh;                // shimmer is centred; the width comes from the resonator pans
        float t = tickBp.process(rng->bi()) * tick.tick();
        l += t; r += t;
        if (trash > 0.001f) {
            float g = 1.0f + 5.0f * trash;
            l = softClip(l * g) / g * (1.0f + 1.5f * trash);
            r = softClip(r * g) / g * (1.0f + 1.5f * trash);
        }
        fadeG *= fadeC;
        L = lpL.process(hpL.process(l)) * kindGain * fadeG; R = lpR.process(hpR.process(r)) * kindGain * fadeG;
        if (++age > life || fadeG < 1e-3f) active = false;
    }
};

// ====================================================================== PERCUSSION
struct PercVoice {
    bool active = false;
    int kind = 0; float fs = 48000, vel = 1;
    float ph[4] = {}, inc[4] = {}, amp[4] = {};
    BlepSquare sq[2];
    Resonator rr[5];
    Biquad bp, bp2, hp, hpIn, lp;
    float e = 1, c = 0.99f, e2 = 1, c2 = 0.99f, att = 0, attInc = 0.01f;
    float burstT = 0; int burstIdx = 0, burstGap = 0, burstCnt = 0;
    float spread = 0.5f, tone = 0.5f;
    Rng* rng = nullptr; int age = 0, life = 0;

    // kind: 0 clap, 1 tambourine, 2 cowbell, 3 shaker, 4 claves, 5 conga, 6 triangle
    void trigger(const float* sp, const HitCtx& h, int k)
    {
        fs = h.fs; rng = h.rng; vel = h.vel; kind = k; age = 0;
        const float tr = semisToRatio(sp[PercP::Tune]) * h.tune;
        const float dec = sp[PercP::Decay] * h.decay;
        tone = sp[PercP::Tone] * 0.01f; spread = sp[PercP::Spread] * 0.01f;
        e = 1; e2 = 1; att = 0; attInc = 1.0f / (0.0005f * fs);
        hp.setHighpass(fs, 100.0f, 0.7f); lp.setLowpass(fs, 18000.0f, 0.7f); hp.reset(); hpIn.reset(); lp.reset(); bp.reset(); bp2.reset();
        float lifeMs = dec * 1.6f + 50.0f;
        switch (kind) {
            case 0:   // clap: a few staggered noise bursts + decaying tail
                bp.setBandpass(fs, 1150.0f * tr * (0.7f + 0.6f * tone), 1.6f);
                bp2.setBandpass(fs, 2600.0f * tr, 0.9f);
                burstCnt = 4; burstIdx = 0; burstT = 0;
                burstGap = (int)((0.006f + 0.010f * spread) * fs);
                c = t60Coef(9.0f, fs); c2 = t60Coef(dec, fs); e2 = 0; lifeMs = dec * 1.5f + 80.0f;
                break;
            case 1:   // tambourine
                hpIn.setHighpass(fs, 4500.0f * tr, 0.7f);
                for (int i = 0; i < 5; ++i) { static const float f[5] = {5100, 6400, 7900, 9300, 11200}; rr[i].setN(fs, f[i] * tr, dec * 0.00014f, 0.2f * (0.4f + tone)); }
                c = t60Coef(dec, fs); c2 = t60Coef(20.0f, fs); break;
            case 2:   // 808 style cowbell
                sq[0].set(fs, 540.0f * tr, 0); sq[1].set(fs, 800.0f * tr * (1.0f + 0.04f * spread), 0.3f);
                bp.setBandpass(fs, 1050.0f * tr, 1.2f); c = t60Coef(dec, fs); c2 = t60Coef(dec * 0.2f, fs); break;
            case 3:   // shaker: swell then fade
                bp.setBandpass(fs, 7200.0f * tr * (0.8f + 0.4f * tone), 0.9f);
                attInc = 1.0f / ((0.004f + 0.02f * spread) * fs); c = t60Coef(dec, fs); break;
            case 4:   // claves
                inc[0] = 2450.0f * tr / fs; inc[1] = 2450.0f * tr * 2.56f / fs; ph[0] = ph[1] = 0;
                c = t60Coef(std::min(dec, 120.0f) * 0.5f + 12.0f, fs); c2 = t60Coef(6.0f, fs); break;
            case 5:   // conga
                inc[0] = 330.0f * tr / fs; inc[1] = 330.0f * tr * 1.58f / fs; ph[0] = ph[1] = 0;
                c = t60Coef(dec, fs); c2 = t60Coef(12.0f, fs);
                bp.setBandpass(fs, 2000.0f, 0.8f); break;
            case 6: { // triangle
                static const float f[4] = {4300, 6700, 9600, 12900};
                for (int i = 0; i < 4; ++i) rr[i].setN(fs, f[i] * tr, dec * 0.000145f * (1.0f - 0.1f * (float)i), 0.3f);
                c = t60Coef(dec, fs); c2 = t60Coef(3.0f, fs); lifeMs = dec * 1.5f; break; }
        }
        life = (int)(lifeMs * 0.001f * fs);
        active = true;
    }
    float process()
    {
        if (!active) return 0;
        float o = 0;
        float n = rng->bi();
        switch (kind) {
            case 0: {
                if (burstIdx < burstCnt && burstT <= 0) { e = 1; ++burstIdx; burstT = (float)burstGap; if (burstIdx == burstCnt) e2 = 1; }
                burstT -= 1.0f;
                const float a = bp.process(n), b = bp2.process(n);
                o = (a * 1.4f + b * 0.5f) * e; e *= c;
                if (burstIdx == burstCnt) { o += a * e2 * 0.5f; e2 *= c2; }
                break; }
            case 1: {
                float jin = 0;
                for (int i = 0; i < 5; ++i) jin += rr[i].process(n);
                o = (hpIn.process(n) * 0.35f + jin) * e; e *= c; break; }
            case 2: {
                float s = sq[0].next() + sq[1].next();
                o = bp.process(s) * (0.7f * e + 0.5f * e2) * 1.2f; e *= c; e2 *= c2; break; }
            case 3: {
                att = std::min(1.0f, att + attInc);
                o = bp.process(n) * att * e * 1.1f; e *= c; break; }
            case 4: {
                ph[0] += inc[0]; ph[0] -= std::floor(ph[0]); ph[1] += inc[1]; ph[1] -= std::floor(ph[1]);
                o = (fastSin(ph[0]) * 0.8f + fastSin(ph[1]) * 0.3f) * e + n * e2 * 0.3f; e *= c; e2 *= c2; break; }
            case 5: {
                float dm = 1.0f + 0.2f * e2;
                ph[0] += inc[0] * dm; ph[0] -= std::floor(ph[0]); ph[1] += inc[1] * dm; ph[1] -= std::floor(ph[1]);
                o = (fastSin(ph[0]) + fastSin(ph[1]) * 0.25f * tone) * e * 0.8f + bp.process(n) * e2 * 0.5f * tone; e *= c; e2 *= c2; break; }
            case 6: {
                float s = 0;
                for (int i = 0; i < 4; ++i) s += rr[i].process(n * (e2 + 0.003f));
                o = s; e2 *= c2; break; }
        }
        o = lp.process(hp.process(o));
        if (++age > life) active = false;
        static const float kindG[7] = {2.19f, 0.97f, 0.40f, 0.65f, 0.58f, 0.75f, 2.5f};   // loudness calibration per sound
        return o * (0.35f + 0.65f * vel) * kindG[kind];
    }
};

}  // namespace df
