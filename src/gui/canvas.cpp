#include "canvas.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../third_party/stb/stb_truetype.h"
#include "font_data.h"

namespace df { namespace gui {

struct Glyph { int w = 0, h = 0, xoff = 0, yoff = 0; float adv = 0; std::vector<uint8_t> bmp; };

struct Canvas::Impl {
    stbtt_fontinfo fonts[2];
    bool ready = false;
    std::unordered_map<uint32_t, Glyph> cache;     // key: font | pixelSize<<1 | codepoint<<16
    Impl()
    {
        ready = stbtt_InitFont(&fonts[0], font_regular(), stbtt_GetFontOffsetForIndex(font_regular(), 0)) &&
                stbtt_InitFont(&fonts[1], font_bold(), stbtt_GetFontOffsetForIndex(font_bold(), 0));
    }
    const Glyph& glyph(int font, int px, int cp)
    {
        uint32_t key = (uint32_t)font | ((uint32_t)px << 1) | ((uint32_t)cp << 16);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        Glyph g;
        stbtt_fontinfo& f = fonts[font];
        float s = stbtt_ScaleForPixelHeight(&f, (float)px);
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&f, cp, &adv, &lsb);
        g.adv = adv * s;
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&f, cp, s, s, &x0, &y0, &x1, &y1);
        g.w = x1 - x0; g.h = y1 - y0; g.xoff = x0; g.yoff = y0;
        if (g.w > 0 && g.h > 0) {
            g.bmp.assign((size_t)g.w * (size_t)g.h, 0);
            stbtt_MakeCodepointBitmap(&f, g.bmp.data(), g.w, g.h, g.w, s, s, cp);
        }
        return cache.emplace(key, std::move(g)).first->second;
    }
};

Canvas::Canvas() : impl(new Impl) {}
Canvas::~Canvas() { delete impl; }

void Canvas::resize(int w, int h, float scale)
{
    pw = std::max(1, w); ph = std::max(1, h); sc = scale;
    buf.assign((size_t)pw * (size_t)ph, 0xFF000000u);
}

inline void Canvas::blend(int x, int y, Color c, float cover)
{
    if ((unsigned)x >= (unsigned)pw || (unsigned)y >= (unsigned)ph) return;
    float a = cover * (c.a * (1.0f / 255.0f));
    if (a <= 0.002f) return;
    uint32_t& d = buf[(size_t)y * (size_t)pw + (size_t)x];
    float ia = 1.0f - a;
    float dr = (float)((d >> 16) & 255), dg = (float)((d >> 8) & 255), db = (float)(d & 255);
    uint32_t r = (uint32_t)(c.r * a + dr * ia + 0.5f), g = (uint32_t)(c.g * a + dg * ia + 0.5f), b = (uint32_t)(c.b * a + db * ia + 0.5f);
    d = 0xFF000000u | (r << 16) | (g << 8) | b;
}

void Canvas::clear(Color c)
{
    uint32_t v = 0xFF000000u | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
    std::fill(buf.begin(), buf.end(), v);
}

// coverage of a pixel from a signed distance (negative inside), 1px soft edge
static inline float cov(float sd) { float v = 0.5f - sd; return v < 0 ? 0 : (v > 1 ? 1 : v); }

void Canvas::fillRect(Rect r, Color c) { fillRoundRect(r, 0, c); }

static inline float sdRoundBox(float px, float py, float cx, float cy, float hw, float hh, float rad)
{
    float qx = std::fabs(px - cx) - (hw - rad), qy = std::fabs(py - cy) - (hh - rad);
    float ox = std::max(qx, 0.0f), oy = std::max(qy, 0.0f);
    return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.0f) - rad;
}

void Canvas::fillRoundRectV(Rect r, float radius, Color top, Color bottom)
{
    float x = r.x * sc, y = r.y * sc, w = r.w * sc, h = r.h * sc, rad = std::min(radius * sc, std::min(w, h) * 0.5f);
    int x0 = (int)std::floor(x) - 1, y0 = (int)std::floor(y) - 1, x1 = (int)std::ceil(x + w) + 1, y1 = (int)std::ceil(y + h) + 1;
    for (int py = std::max(0, y0); py < std::min(ph, y1); ++py) {
        float t = (py + 0.5f - y) / std::max(1.0f, h);
        Color c = top.mix(bottom, t < 0 ? 0 : (t > 1 ? 1 : t));
        for (int px = std::max(0, x0); px < std::min(pw, x1); ++px)
            blend(px, py, c, cov(sdRoundBox(px + 0.5f, py + 0.5f, x + w * 0.5f, y + h * 0.5f, w * 0.5f, h * 0.5f, rad)));
    }
}
void Canvas::fillRoundRect(Rect r, float radius, Color c) { fillRoundRectV(r, radius, c, c); }

void Canvas::strokeRoundRect(Rect r, float radius, float th, Color c)
{
    float x = r.x * sc, y = r.y * sc, w = r.w * sc, h = r.h * sc, rad = std::min(radius * sc, std::min(w, h) * 0.5f), t = th * sc;
    int x0 = (int)std::floor(x) - 2, y0 = (int)std::floor(y) - 2, x1 = (int)std::ceil(x + w) + 2, y1 = (int)std::ceil(y + h) + 2;
    for (int py = std::max(0, y0); py < std::min(ph, y1); ++py)
        for (int px = std::max(0, x0); px < std::min(pw, x1); ++px) {
            float sd = sdRoundBox(px + 0.5f, py + 0.5f, x + w * 0.5f, y + h * 0.5f, w * 0.5f, h * 0.5f, rad);
            blend(px, py, c, cov(std::fabs(sd + t * 0.5f) - t * 0.5f));
        }
}

void Canvas::fillCircle(float cx, float cy, float radius, Color c)
{
    cx *= sc; cy *= sc; radius *= sc;
    for (int py = std::max(0, (int)(cy - radius) - 1); py < std::min(ph, (int)(cy + radius) + 2); ++py)
        for (int px = std::max(0, (int)(cx - radius) - 1); px < std::min(pw, (int)(cx + radius) + 2); ++px) {
            float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
            blend(px, py, c, cov(std::sqrt(dx * dx + dy * dy) - radius));
        }
}

void Canvas::ring(float cx, float cy, float radius, float th, Color c)
{
    cx *= sc; cy *= sc; radius *= sc; th *= sc;
    float ro = radius + th * 0.5f;
    for (int py = std::max(0, (int)(cy - ro) - 1); py < std::min(ph, (int)(cy + ro) + 2); ++py)
        for (int px = std::max(0, (int)(cx - ro) - 1); px < std::min(pw, (int)(cx + ro) + 2); ++px) {
            float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
            blend(px, py, c, cov(std::fabs(std::sqrt(dx * dx + dy * dy) - radius) - th * 0.5f));
        }
}

void Canvas::arc(float cx, float cy, float radius, float a0, float a1, float th, Color c)
{
    if (a1 < a0) std::swap(a0, a1);
    cx *= sc; cy *= sc; radius *= sc; th *= sc;
    float ro = radius + th * 0.5f + 1;
    const float kTwoPi = 6.2831853f;
    float mid = 0.5f * (a0 + a1), half = 0.5f * (a1 - a0);
    float capx0 = cx + std::cos(a0) * radius, capy0 = cy + std::sin(a0) * radius;
    float capx1 = cx + std::cos(a1) * radius, capy1 = cy + std::sin(a1) * radius;
    for (int py = std::max(0, (int)(cy - ro)); py < std::min(ph, (int)(cy + ro) + 1); ++py)
        for (int px = std::max(0, (int)(cx - ro)); px < std::min(pw, (int)(cx + ro) + 1); ++px) {
            float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            float sdRing = std::fabs(d - radius) - th * 0.5f;
            if (sdRing > 1.0f) continue;
            float ang = std::atan2(dy, dx);
            float da = ang - mid;
            while (da > kTwoPi * 0.5f) da -= kTwoPi;
            while (da < -kTwoPi * 0.5f) da += kTwoPi;
            float sd;
            if (std::fabs(da) <= half) sd = sdRing;
            else {   // round caps
                float e0 = std::sqrt((px + 0.5f - capx0) * (px + 0.5f - capx0) + (py + 0.5f - capy0) * (py + 0.5f - capy0));
                float e1 = std::sqrt((px + 0.5f - capx1) * (px + 0.5f - capx1) + (py + 0.5f - capy1) * (py + 0.5f - capy1));
                sd = std::min(e0, e1) - th * 0.5f;
            }
            blend(px, py, c, cov(sd));
        }
}

void Canvas::line(float x0, float y0, float x1, float y1, float th, Color c)
{
    x0 *= sc; y0 *= sc; x1 *= sc; y1 *= sc; th *= sc;
    float minx = std::min(x0, x1) - th - 1, maxx = std::max(x0, x1) + th + 1, miny = std::min(y0, y1) - th - 1, maxy = std::max(y0, y1) + th + 1;
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy;
    for (int py = std::max(0, (int)miny); py < std::min(ph, (int)maxy + 1); ++py)
        for (int px = std::max(0, (int)minx); px < std::min(pw, (int)maxx + 1); ++px) {
            float qx = px + 0.5f - x0, qy = py + 0.5f - y0;
            float t = len2 > 0 ? (qx * vx + qy * vy) / len2 : 0;
            t = t < 0 ? 0 : (t > 1 ? 1 : t);
            float dx = qx - vx * t, dy = qy - vy * t;
            blend(px, py, c, cov(std::sqrt(dx * dx + dy * dy) - th * 0.5f));
        }
}

void Canvas::glow(float cx, float cy, float radius, Color c)
{
    cx *= sc; cy *= sc; radius *= sc;
    for (int py = std::max(0, (int)(cy - radius)); py < std::min(ph, (int)(cy + radius) + 1); ++py)
        for (int px = std::max(0, (int)(cx - radius)); px < std::min(pw, (int)(cx + radius) + 1); ++px) {
            float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
            float t = 1.0f - std::sqrt(dx * dx + dy * dy) / radius;
            if (t > 0) blend(px, py, c, t * t);
        }
}

static int nextCp(const std::string& s, size_t& i)   // UTF-8 -> codepoint
{
    unsigned char c = (unsigned char)s[i++];
    if (c < 0x80) return c;
    int n = (c >> 5) == 6 ? 1 : ((c >> 4) == 14 ? 2 : 3);
    int cp = c & (0x3F >> n);
    while (n-- > 0 && i < s.size()) cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
    return cp;
}

float Canvas::textWidth(const std::string& s, float size, bool bold)
{
    if (!impl->ready) return 0;
    int px = std::max(6, (int)std::lround(size * sc));
    float w = 0;
    for (size_t i = 0; i < s.size();) w += impl->glyph(bold ? 1 : 0, px, nextCp(s, i)).adv;
    return w / sc;
}

void Canvas::text(float x, float y, const std::string& s, float size, Color c, Align al, bool bold)
{
    if (!impl->ready) return;
    int px = std::max(6, (int)std::lround(size * sc));
    float w = textWidth(s, size, bold) * sc;
    float pen = x * sc - (al == Align::Center ? w * 0.5f : (al == Align::Right ? w : 0.0f));
    // y is the vertical centre of the text line
    stbtt_fontinfo& f = impl->fonts[bold ? 1 : 0];
    float s2 = stbtt_ScaleForPixelHeight(&f, (float)px);
    int asc, desc, gap; stbtt_GetFontVMetrics(&f, &asc, &desc, &gap);
    float base = y * sc + (asc + desc) * s2 * 0.5f + 0.0f * gap;
    // snap the baseline to whole pixels: crisp, consistent text
    int by = (int)std::floor(base + 0.5f);
    for (size_t i = 0; i < s.size();) {
        const Glyph& g = impl->glyph(bold ? 1 : 0, px, nextCp(s, i));
        int gx = (int)std::floor(pen + 0.5f) + g.xoff, gy = by + g.yoff;
        for (int yy = 0; yy < g.h; ++yy)
            for (int xx = 0; xx < g.w; ++xx) {
                uint8_t a = g.bmp[(size_t)yy * (size_t)g.w + (size_t)xx];
                if (a) blend(gx + xx, gy + yy, c, a * (1.0f / 255.0f));
            }
        pen += g.adv;
    }
}

}}  // namespace df::gui
