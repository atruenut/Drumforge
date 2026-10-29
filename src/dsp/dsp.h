// DrumForge - DSP primitives (header only, no dependencies beyond the STL)
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace df {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float dbToGain(float db) { return std::exp(db * 0.115129254649702f); }   // 10^(db/20)
inline float gainToDb(float g) { return 8.685889638065035f * std::log(std::max(g, 1e-9f)); }
inline float semisToRatio(float st) { return std::exp2(st * (1.0f / 12.0f)); }
inline float msToCoef(float ms, float fs)   // per-sample multiplier giving 1/e decay after `ms`
{
    return std::exp(-1.0f / std::max(1.0f, ms * 0.001f * fs));
}

// ---------------------------------------------------------------- random
struct Rng {
    uint32_t s = 0x9E3779B9u;
    void seed(uint32_t v) { s = v ? v : 0x9E3779B9u; }
    inline uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float uni() { return (next() >> 8) * (1.0f / 16777216.0f); }  // [0,1)
    inline float bi() { return uni() * 2.0f - 1.0f; }                     // [-1,1)
    inline float range(float a, float b) { return a + (b - a) * uni(); }
};

// ---------------------------------------------------------------- sine table
struct SineTable {
    static constexpr int N = 4096;
    float t[N + 2];
    SineTable()
    {
        for (int i = 0; i < N + 2; ++i) t[i] = std::sin(kTwoPi * (float)i / (float)N);
    }
};
inline const SineTable& sineTable()
{
    static const SineTable s;
    return s;
}
// phase in [0,1)
inline float fastSin(float ph)
{
    const SineTable& s = sineTable();
    float x = ph * (float)SineTable::N;
    int i = (int)x;
    float f = x - (float)i;
    return s.t[i] + (s.t[i + 1] - s.t[i]) * f;
}

// ---------------------------------------------------------------- saturation
inline float softClip(float x)   // smooth, bounded to +-1
{
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);   // Pade tanh approximation
}
inline float fastTanh(float x) { return softClip(x); }

// drive in 0..1.  Gain-compensated tanh-ish waveshaper, blends in smoothly from bypass.
inline float driveShape(float x, float drive, float gain, float comp)
{
    float shaped = softClip(x * gain) * comp;
    return x + (shaped - x) * drive;
}

// ---------------------------------------------------------------- filters
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline void reset() { z1 = z2 = 0; }
    inline float process(float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void setCoefs(float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        float inv = 1.0f / a0;
        b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv; a1 = na1 * inv; a2 = na2 * inv;
    }
    static float clampF(float f, float fs) { return clampf(f, 10.0f, 0.46f * fs); }
    void setLowpass(float fs, float f, float q = 0.7071f)
    {
        f = clampF(f, fs);
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2.0f * q);
        setCoefs((1 - c) * 0.5f, 1 - c, (1 - c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    void setHighpass(float fs, float f, float q = 0.7071f)
    {
        f = clampF(f, fs);
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2.0f * q);
        setCoefs((1 + c) * 0.5f, -(1 + c), (1 + c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    void setBandpass(float fs, float f, float q = 1.0f)   // constant 0 dB peak
    {
        f = clampF(f, fs);
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2.0f * q);
        setCoefs(al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void setPeak(float fs, float f, float q, float gainDb)
    {
        f = clampF(f, fs);
        float A = std::exp(gainDb * 0.057564627f);   // 10^(dB/40)
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2.0f * q);
        setCoefs(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void setLowShelf(float fs, float f, float gainDb, float slope = 0.8f)
    {
        f = clampF(f, fs);
        float A = std::exp(gainDb * 0.057564627f);
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w);
        float al = s * 0.5f * std::sqrt((A + 1 / A) * (1 / slope - 1) + 2);
        float sq = 2 * std::sqrt(A) * al;
        setCoefs(A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c),
                 A * ((A + 1) - (A - 1) * c - sq), (A + 1) + (A - 1) * c + sq,
                 -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void setHighShelf(float fs, float f, float gainDb, float slope = 0.8f)
    {
        f = clampF(f, fs);
        float A = std::exp(gainDb * 0.057564627f);
        float w = kTwoPi * f / fs, c = std::cos(w), s = std::sin(w);
        float al = s * 0.5f * std::sqrt((A + 1 / A) * (1 / slope - 1) + 2);
        float sq = 2 * std::sqrt(A) * al;
        setCoefs(A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c),
                 A * ((A + 1) + (A - 1) * c - sq), (A + 1) - (A - 1) * c + sq,
                 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
};

struct OnePole {
    float a = 0.1f, y = 0;
    void setCutoff(float fs, float f) { a = 1.0f - std::exp(-kTwoPi * std::min(f, 0.45f * fs) / fs); }
    inline float lp(float x) { y += a * (x - y); return y; }
};

struct DcBlock {
    float x1 = 0, y1 = 0, r = 0.995f;
    inline float process(float x) { float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
};

// smoothed parameter (one pole)
struct Smooth {
    float v = 0, target = 0, a = 0.01f;
    void init(float x) { v = target = x; }
    void setTime(float fs, float ms) { a = 1.0f - std::exp(-1.0f / std::max(1.0f, ms * 0.001f * fs)); }
    inline float next() { v += a * (target - v); return v; }
};

// ---------------------------------------------------------------- 2-pole resonator (modal filter)
// y[n] = g*x[n] + c1*y[n-1] - c2*y[n-2]
struct Resonator {
    float c1 = 0, c2 = 0, g = 1, y1 = 0, y2 = 0;
    void set(float fs, float freq, float tauSec, float gain)
    {
        float r = std::exp(-1.0f / std::max(1.0f, tauSec * fs));
        float w = kTwoPi * std::min(freq, 0.47f * fs) / fs;
        c1 = 2 * r * std::cos(w);
        c2 = r * r;
        g = gain;
        y1 = y2 = 0;
    }
    // normalised: with white-noise input the output level is independent of decay time AND frequency (G=1 -> ~0.58 rms for +-1 uniform noise)
    void setN(float fs, float freq, float tauSec, float G)
    {
        float r = std::exp(-1.0f / std::max(1.0f, tauSec * fs));
        float w = kTwoPi * std::min(freq, 0.47f * fs) / fs;
        set(fs, freq, tauSec, G * std::sqrt(1.0f - r) * 2.0f * std::sin(w));
    }
    inline float process(float x)
    {
        float y = g * x + c1 * y1 - c2 * y2;
        y2 = y1; y1 = y;
        return y;
    }
};

// ---------------------------------------------------------------- polyBLEP square
inline float polyBlep(float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}
struct BlepSquare {
    float ph = 0, inc = 0;
    void set(float fs, float freq, float phase0 = 0.0f) { inc = freq / fs; ph = phase0; }
    inline float next()
    {
        float v = ph < 0.5f ? 1.0f : -1.0f;
        v += polyBlep(ph, inc);
        float p2 = ph + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
        v -= polyBlep(p2, inc);
        ph += inc; if (ph >= 1.0f) ph -= 1.0f;
        return v;
    }
};

// ---------------------------------------------------------------- delay helpers
struct DelayLine {
    std::vector<float> buf;
    int mask = 0, wp = 0;
    void init(int maxSamples)
    {
        int n = 1; while (n < maxSamples + 4) n <<= 1;
        buf.assign((size_t)n, 0.0f); mask = n - 1; wp = 0;
    }
    inline void write(float x) { buf[(size_t)wp] = x; wp = (wp + 1) & mask; }
    inline float read(int d) const { return buf[(size_t)((wp - 1 - d) & mask)]; }
    inline float readFrac(float d) const
    {
        int i = (int)d; float f = d - (float)i;
        float a = buf[(size_t)((wp - 1 - i) & mask)];
        float b = buf[(size_t)((wp - 2 - i) & mask)];
        return a + (b - a) * f;
    }
    void clear() { std::fill(buf.begin(), buf.end(), 0.0f); wp = 0; }
};

struct Allpass {
    DelayLine d; int len = 1; float g = 0.5f;
    void init(int maxLen) { d.init(maxLen); len = std::min(len, maxLen); }
    void setLen(int l) { len = std::max(1, l); }
    inline float process(float x)
    {
        float del = d.read(len - 1);
        float w = x + g * del;
        d.write(w);
        return del - g * w;
    }
    void clear() { d.clear(); }
};

}  // namespace df
