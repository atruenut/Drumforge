// DrumForge - effects: room reverb (FDN), master EQ, bus compressor, saturator, look-ahead limiter
#pragma once
#include "../dsp/dsp.h"

namespace df {

// ====================================================================== ROOM
class Room {
public:
    void init(float sampleRate)
    {
        fs = sampleRate;
        const int maxD = (int)(0.4f * fs) + 16;
        for (int i = 0; i < N; ++i) line[i].init(maxD);
        pre[0].init((int)(0.2f * fs)); pre[1].init((int)(0.2f * fs));
        er[0].init((int)(0.15f * fs)); er[1].init((int)(0.15f * fs));
        for (int c = 0; c < 2; ++c)
            for (int a = 0; a < 4; ++a) ap[c][a].init((int)(0.05f * fs));
        reset();
        dirty = true;
    }
    void reset()
    {
        for (int i = 0; i < N; ++i) { line[i].clear(); lp[i] = 0; }
        pre[0].clear(); pre[1].clear(); er[0].clear(); er[1].clear();
        for (int c = 0; c < 2; ++c) for (int a = 0; a < 4; ++a) ap[c][a].clear();
        gateEnv = 0; gateCount = 1 << 30;
    }
    void setParams(int type_, float size01, float decayMs, float dampHz, float preMs, float gateMs, float width01, float mix01)
    {
        if (type_ != type || size01 != size || decayMs != decay || dampHz != damp || preMs != preT || gateMs != gateT || width01 != width)
            dirty = true;
        type = type_; size = size01; decay = decayMs; damp = dampHz; preT = preMs; gateT = gateMs; width = width01; mix = mix01;
    }
    void hit() { gateCount = 0; }    // a drum was struck: (re)open the gate
    float mixAmount() const { return mix; }

    // in: stereo send, out: stereo return (wet only)
    inline void process(float inL, float inR, float& outL, float& outR)
    {
        if (dirty) update();
        // pre-delay
        pre[0].write(inL); pre[1].write(inR);
        float pl = pre[0].read(preSamp), pr = pre[1].read(preSamp);
        // input diffusion
        float dl = pl, dr = pr;
        for (int a = 0; a < nAp; ++a) { dl = ap[0][a].process(dl); dr = ap[1][a].process(dr); }
        // early reflections
        er[0].write(pl); er[1].write(pr);
        float eL = 0, eR = 0;
        for (int t = 0; t < 6; ++t) {
            eL += er[0].read(erTap[0][t]) * erGain[0][t];
            eR += er[1].read(erTap[1][t]) * erGain[1][t];
        }
        // FDN
        float y[N];
        for (int i = 0; i < N; ++i) {
            float md = modDepth * fastSin(lfoPh[i]);
            lfoPh[i] += lfoInc[i]; if (lfoPh[i] >= 1.0f) lfoPh[i] -= 1.0f;
            y[i] = line[i].readFrac(std::max(1.0f, len[i] + md));
            lp[i] += dampA * (y[i] - lp[i]);
            y[i] = lp[i] * fb[i];
        }
        // fast Walsh-Hadamard (orthogonal)
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h << 1)
                for (int j = i; j < i + h; ++j) { float a = y[j], b = y[j + h]; y[j] = a + b; y[j + h] = a - b; }
        const float s = 0.35355339f;   // 1/sqrt(8)
        for (int i = 0; i < N; ++i) {
            float in = (i & 1) ? dr : dl;
            line[i].write(y[i] * s + in * 0.5f);
        }
        float oL = 0, oR = 0;
        for (int i = 0; i < N; i += 2) { oL += (i & 2 ? -1.0f : 1.0f) * lp[i]; oR += (i & 2 ? -1.0f : 1.0f) * lp[i + 1]; }
        oL *= 0.5f; oR *= 0.5f;
        oL += eL * erLevel; oR += eR * erLevel;
        // width
        float m = 0.5f * (oL + oR), sd = 0.5f * (oL - oR) * width;
        oL = m + sd; oR = m - sd;
        // gate
        if (gateSamp > 0) {
            if (gateCount < gateSamp) gateTarget = 1.0f; else gateTarget = 0.0f;
            ++gateCount;
            gateEnv += (gateTarget - gateEnv) * (gateTarget > gateEnv ? 0.02f : gateRel);
            oL *= gateEnv; oR *= gateEnv;
        }
        outL = oL * 0.9f; outR = oR * 0.9f;
    }

private:
    static constexpr int N = 8;
    float fs = 48000;
    DelayLine line[N], pre[2], er[2];
    Allpass ap[2][4];
    int type = 0; float size = 0.5f, decay = 1200, damp = 7000, preT = 8, gateT = 0, width = 1, mix = 0.2f;
    bool dirty = true;
    float len[N] = {}, fb[N] = {}, lp[N] = {}, dampA = 0.3f, modDepth = 0, lfoPh[N] = {0, .13f, .27f, .41f, .56f, .68f, .8f, .93f}, lfoInc[N] = {};
    int preSamp = 0, nAp = 3; int erTap[2][6] = {}; float erGain[2][6] = {}; float erLevel = 0.3f;
    int gateSamp = 0, gateCount = 1 << 30; float gateEnv = 0, gateTarget = 0, gateRel = 0.002f;

    void update()
    {
        dirty = false;
        static const float base[N] = {23.7f, 29.3f, 37.1f, 41.9f, 47.9f, 53.3f, 59.9f, 67.1f};
        static const float typeScale[6] = {1.0f, 0.55f, 1.7f, 2.6f, 0.85f, 0.7f};
        static const float typeEr[6] = {0.45f, 0.55f, 0.35f, 0.2f, 0.25f, 0.0f};
        float sc = typeScale[type] * (0.35f + 1.9f * size);
        float rt = std::max(0.05f, decay * 0.001f);
        if (type == 4) rt = std::min(rt, 1.6f);
        for (int i = 0; i < N; ++i) {
            len[i] = std::min(base[i] * sc * 0.001f * fs, 0.38f * fs);
            float d = len[i] / fs;
            fb[i] = std::pow(10.0f, -3.0f * d / rt);
            lfoInc[i] = (0.21f + 0.11f * (float)i) / fs;
        }
        modDepth = (type == 5) ? 0.0006f * fs : 0.0003f * fs;
        float dampHz = (type == 4 || type == 5) ? std::max(damp, 5000.0f) : damp;
        dampA = 1.0f - std::exp(-kTwoPi * std::min(dampHz, 0.45f * fs) / fs);
        preSamp = (int)(std::max(0.0f, preT) * 0.001f * fs);
        nAp = (type == 5 || type == 4) ? 4 : 3;
        static const int apLen[2][4] = {{142, 107, 379, 277}, {151, 113, 401, 263}};
        for (int c = 0; c < 2; ++c) for (int a = 0; a < 4; ++a) { ap[c][a].setLen((int)((float)apLen[c][a] * fs / 44100.0f * (type == 3 ? 1.6f : 1.0f))); ap[c][a].g = 0.55f; }
        static const float erT[2][6] = {{4.1f, 9.3f, 13.7f, 19.1f, 27.3f, 35.9f}, {6.7f, 11.1f, 16.9f, 22.7f, 31.3f, 41.3f}};
        static const float erG[6] = {0.8f, 0.65f, 0.55f, 0.42f, 0.32f, 0.22f};
        for (int c = 0; c < 2; ++c) for (int t = 0; t < 6; ++t) {
            erTap[c][t] = std::min((int)(erT[c][t] * (0.4f + 1.2f * size) * typeScale[type] * 0.001f * fs), (int)(0.14f * fs));
            erGain[c][t] = erG[t] * ((t & 1) ? -1.0f : 1.0f) * (c ? 0.9f : 1.0f);
        }
        erLevel = typeEr[type];
        float g = gateT; if (type == 4 && g < 1.0f) g = 220.0f;
        gateSamp = g > 0.5f ? (int)(g * 0.001f * fs) : 0;
        gateRel = 1.0f - std::exp(-1.0f / (0.012f * fs));
        if (gateSamp == 0) gateEnv = 1.0f;
    }
};

// ====================================================================== MASTER EQ
struct MasterEq {
    Biquad lo[2], mid[2], hi[2];
    float fs = 48000, gl = 1e9f, gm = 1e9f, gh = 1e9f, fm = 0;
    bool active = false;
    void init(float f) { fs = f; gl = gm = gh = 1e9f; fm = 0; }
    void set(float loDb, float midDb, float midHz, float hiDb)
    {
        active = std::fabs(loDb) > 0.05f || std::fabs(midDb) > 0.05f || std::fabs(hiDb) > 0.05f;
        if (loDb != gl) { gl = loDb; for (auto& b : lo) b.setLowShelf(fs, 110.0f, loDb, 0.8f); }
        if (midDb != gm || midHz != fm) { gm = midDb; fm = midHz; for (auto& b : mid) b.setPeak(fs, midHz, 0.9f, midDb); }
        if (hiDb != gh) { gh = hiDb; for (auto& b : hi) b.setHighShelf(fs, 7500.0f, hiDb, 0.8f); }
    }
    inline void process(float& l, float& r)
    {
        if (!active) return;
        l = hi[0].process(mid[0].process(lo[0].process(l)));
        r = hi[1].process(mid[1].process(lo[1].process(r)));
    }
};

// ====================================================================== BUS COMPRESSOR
struct BusComp {
    float fs = 48000, env = 0, gr = 0, thr = -12, ratio = 3, makeup = 1, mix = 0, atkC = 0.9f, relC = 0.999f;
    void init(float f) { fs = f; env = 0; gr = 0; }
    void set(float thrDb, float r, float attMs, float relMs, float makeupDb, float mix01)
    {
        thr = thrDb; ratio = r; mix = mix01; makeup = dbToGain(makeupDb);
        atkC = std::exp(-1.0f / std::max(1.0f, attMs * 0.001f * fs));
        relC = std::exp(-1.0f / std::max(1.0f, relMs * 0.001f * fs));
    }
    inline void process(float& l, float& r)
    {
        if (mix <= 0.0005f) return;
        float pk = std::max(std::fabs(l), std::fabs(r));
        env = pk > env ? atkC * env + (1 - atkC) * pk : relC * env + (1 - relC) * pk;
        float db = gainToDb(env);
        float over = db - thr, red;
        const float knee = 6.0f;
        if (over <= -knee * 0.5f) red = 0;
        else if (over >= knee * 0.5f) red = over * (1.0f - 1.0f / ratio);
        else { float x = over + knee * 0.5f; red = (1.0f - 1.0f / ratio) * x * x / (2.0f * knee); }
        gr += (red - gr) * 0.3f;
        float g = dbToGain(-gr) * makeup;
        l = l + (l * g - l) * mix; r = r + (r * g - r) * mix;
    }
};

// ====================================================================== SATURATOR
struct Saturator {
    float drive = 0, mix = 1, gain = 1, comp = 1;
    DcBlock dc[2];
    void set(float drive01, float mix01)
    {
        drive = drive01; mix = mix01;
        gain = 1.0f + 9.0f * drive01 * drive01;
        comp = 1.0f / softClip(gain * 0.7f) * 0.7f;
    }
    inline void process(float& l, float& r)
    {
        if (drive <= 0.001f) return;
        float wl = dc[0].process(softClip(l * gain) * comp), wr = dc[1].process(softClip(r * gain) * comp);
        l += (wl - l) * mix; r += (wr - r) * mix;
    }
};

// ====================================================================== LIMITER (look-ahead, guaranteed ceiling)
struct Limiter {
    static constexpr int MAXN = 512;
    float fs = 48000, ceil = 0.966f, relC = 0.999f;
    int N = 48, wp = 0;
    float bufL[MAXN] = {}, bufR[MAXN] = {}, need[MAXN] = {}, boxBuf[MAXN] = {};
    int dq[MAXN] = {}, dqHead = 0, dqCount = 0;     // monotonic deque of sample counters (running minimum in O(1))
    unsigned long long tick = 0;
    float boxSum = 0, relG = 1.0f; bool on = true;
    void init(float f)
    {
        fs = f; N = std::min(MAXN - 2, std::max(8, (int)(0.0015f * fs)));
        relC = std::exp(-1.0f / (0.09f * fs));
        std::fill(bufL, bufL + MAXN, 0.f); std::fill(bufR, bufR + MAXN, 0.f);
        std::fill(need, need + MAXN, 1.f); std::fill(boxBuf, boxBuf + MAXN, 1.f);
        boxSum = (float)N; wp = 0; relG = 1; dqHead = 0; dqCount = 0; tick = 0;
    }
    void set(bool enabled, float ceilDb) { on = enabled; ceil = dbToGain(ceilDb); }
    inline void process(float& l, float& r)
    {
        const int M = MAXN;
        bufL[wp] = l; bufR[wp] = r;
        float pk = std::max(std::fabs(l), std::fabs(r));
        float g = (on && pk > ceil) ? ceil / pk : 1.0f;
        need[wp] = g;
        // running minimum over the last N samples
        while (dqCount > 0 && need[dq[(dqHead + dqCount - 1) % M]] >= g) --dqCount;
        dq[(dqHead + dqCount) % M] = wp; ++dqCount;
        ++tick;
        // drop entries older than N samples (distance measured in ring positions)
        while (dqCount > 0 && ((wp - dq[dqHead] + M) % M) >= N) { dqHead = (dqHead + 1) % M; --dqCount; }
        float mn = need[dq[dqHead]];
        // release: gain may rise only slowly; falls instantly (the box filter below spreads the fall over N samples)
        relG = std::min(mn, relG + (1.0f - relG) * (1.0f - relC));
        float held = std::min(mn, relG);
        // moving average of the held gain => smooth attack ramp across the look-ahead
        int oldIdx = (wp - N + M) % M;
        boxSum += held - boxBuf[oldIdx];
        boxBuf[wp] = held;
        float sg = boxSum / (float)N;
        int dIdx = (wp - (N - 1) + M) % M;   // full look-ahead: every averaged gain value is <= need[] for this sample
        float ol = bufL[dIdx] * sg, orr = bufR[dIdx] * sg;
        wp = (wp + 1) % M;
        if (on) { l = std::max(-ceil, std::min(ceil, ol)); r = std::max(-ceil, std::min(ceil, orr)); }
        else { l = ol; r = orr; }
    }
    int latency() const { return N - 1; }
};

}  // namespace df
