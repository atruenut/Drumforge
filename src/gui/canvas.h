// DrumForge GUI - tiny anti-aliased software canvas (BGRA, top-down). No dependencies besides stb_truetype.
// All coordinates are LOGICAL; the canvas multiplies by its scale so the same layout renders at any DPI.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace df { namespace gui {

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    constexpr Color() = default;
    constexpr Color(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
    static constexpr Color hex(uint32_t rgb, uint8_t a = 255) { return Color((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, a); }
    Color alpha(float k) const { return Color(r, g, b, (uint8_t)(a * (k < 0 ? 0 : (k > 1 ? 1 : k)))); }
    Color mix(const Color& o, float t) const
    {
        auto L = [&](int x, int y) { return (uint8_t)(x + (y - x) * t + 0.5f); };
        return Color(L(r, o.r), L(g, o.g), L(b, o.b), L(a, o.a));
    }
};

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    float cx() const { return x + w * 0.5f; }
    float cy() const { return y + h * 0.5f; }
};

enum class Align { Left, Center, Right };

class Canvas {
public:
    Canvas();
    ~Canvas();
    void resize(int pixelW, int pixelH, float scale);
    int width() const { return pw; }
    int height() const { return ph; }
    float scale() const { return sc; }
    const uint32_t* pixels() const { return buf.data(); }   // 0xAARRGGBB little endian = BGRA bytes (what GDI wants)

    void clear(Color c);
    void fillRect(Rect r, Color c);
    void fillRoundRect(Rect r, float radius, Color c);
    void fillRoundRectV(Rect r, float radius, Color top, Color bottom);   // vertical gradient
    void strokeRoundRect(Rect r, float radius, float thickness, Color c);
    void fillCircle(float cx, float cy, float radius, Color c);
    void ring(float cx, float cy, float radius, float thickness, Color c);
    // angles in radians, 0 = +x axis, increasing clockwise on screen (y down)
    void arc(float cx, float cy, float radius, float a0, float a1, float thickness, Color c);
    void line(float x0, float y0, float x1, float y1, float thickness, Color c);
    void glow(float cx, float cy, float radius, Color c);                  // soft radial falloff

    void text(float x, float y, const std::string& s, float size, Color c, Align al = Align::Left, bool bold = false);
    float textWidth(const std::string& s, float size, bool bold = false);

private:
    int pw = 0, ph = 0;
    float sc = 1.0f;
    std::vector<uint32_t> buf;
    struct Impl;
    Impl* impl;
    inline void blend(int x, int y, Color c, float cover);
};

}}  // namespace df::gui
