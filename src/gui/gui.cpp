#include "gui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../engine/engine.h"
#include "../engine/grooves.h"
#include "../engine/params.h"
#include "../engine/presets.h"

namespace df { namespace gui {

namespace {
const Color kBgTop = Color::hex(0x0f1115), kBgBot = Color::hex(0x171a20);
const Color kPanel = Color::hex(0x1a1d24), kPanelHi = Color::hex(0x20242c), kBorder = Color::hex(0x2b303a);
const Color kText = Color::hex(0xe9ecf2), kDim = Color::hex(0x8a92a3), kFaint = Color::hex(0x5b6272);
const Color kEmber = Color::hex(0xff7a2f), kEmber2 = Color::hex(0xffb347);
const Color kKnob = Color::hex(0x272b34), kKnobHi = Color::hex(0x323744), kTrack = Color::hex(0x3a4050);
const Color kPieceCol[NumPieces] = {
    Color::hex(0xff6b3d), Color::hex(0xffb23d), Color::hex(0xe8cf86), Color::hex(0xf0566f), Color::hex(0xd96bd0),
    Color::hex(0xa384ff), Color::hex(0x5fd0e0), Color::hex(0x6fa8ff), Color::hex(0x6ee0b4), Color::hex(0x9be06f)};
const float kPi = 3.14159265f;

const char* kTabNames[4] = {"MIX FX", "ROOM", "GROOVE", "MASTER"};
const Rect kMain{222, 80, 882, 408}, kBottom{222, 500, 882, 268};
const int kLaneColor[kUserLanes] = {0, 1, 2, 9, 6, 6, 6, 3, 4, 5, 7, 8, 8, 9};
const int kLenChoices[6] = {8, 16, 24, 32, 48, 64};

std::vector<int> padNotes(int piece)
{
    std::vector<int> v;
    for (int n = 0; n < 128; ++n)
        if (noteInfo(n).piece == piece && *noteLabel(n)) v.push_back(n);
    return v;
}

std::string noteName(int n)
{
    static const char* nm[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return std::string(nm[n % 12]) + std::to_string(n / 12 - 1);
}
Color categoryColor(const std::string& c)
{
    static const char* cats[] = {"Rock", "Vintage", "Jazz", "Funk", "Pop", "Dance", "Metal", "Electronic", "Experimental", "Hip-Hop",
                                 "Cinematic", "Industrial", "World", "Latin", "Reggae", "Ambient", "Lo-Fi"};
    for (int i = 0; i < 17; ++i)
        if (c == cats[i]) return kPieceCol[i % NumPieces].mix(Color::hex(0xffffff), 0.1f);
    return kDim;
}
}  // namespace

Editor::Editor(IHost* h) : host(h)
{
    snap.assign((size_t)paramTable().size(), -1e30f);
    setScale(1.0f);
}

void Editor::setScale(float s)
{
    s = std::max(0.75f, std::min(3.0f, s));
    canvas.resize((int)std::lround(kW * s), (int)std::lround(kH * s), s);
    dirty = true;
}

float Editor::norm(int p) { return paramTable().toNorm(p, host->get(p)); }

std::string Editor::valueText(int p)
{
    const ParamTable& t = paramTable();
    if (t.defs[(size_t)p].key == "groove.pattern") {
        const int v = (int)std::lround(host->get(p));
        if (v >= grooveCount()) return "User " + std::to_string(v - grooveCount() + 1);
        return groovePattern(v).name;
    }
    return t.format(p, host->get(p));
}

void Editor::applyNorm(int p, float n, bool bracket)
{
    float plain = paramTable().toPlain(p, n);
    if (bracket) host->begin(p);
    host->set(p, plain);
    if (bracket) host->end(p);
    dirty = true;
}

bool Editor::tick()
{
    bool changed = dirty;
    const ParamTable& t = paramTable();
    for (int i = 0; i < t.size(); ++i) {
        float v = host->get(i);
        if (v != snap[(size_t)i]) { snap[(size_t)i] = v; changed = true; }
    }
    int pr = host->program();
    if (pr != snapProgram) { snapProgram = pr; changed = true; }
    for (int n = 0; n < 128; ++n)
        if (padFlash[n] > 0) { padFlash[n] = padFlash[n] * 0.8f; if (padFlash[n] < 0.03f) padFlash[n] = 0; changed = true; }
    if (gedit || tab == 2) {
        const float ph = std::floor(host->groovePhase() * 128.0f);
        if (ph != snapPhase) { snapPhase = ph; changed = true; }
    }
    for (int p = 0; p < NumPieces; ++p) {
        float m = host->meter(p);
        float nf = std::max(m, flash[p] * 0.86f);
        if (nf < 0.01f) nf = 0;
        if (std::fabs(nf - flash[p]) > 0.004f) changed = true;
        flash[p] = nf;
    }
    return changed;
}

// ------------------------------------------------------------------ layout
void Editor::knob(Rect cell, int param, const char* labelOverride, float size)
{
    const ParamDef& d = paramTable().defs[(size_t)param];
    Widget w;
    w.param = param;
    w.arg = (int)(size * 10);
    w.r = cell;
    if (d.type == PT::Bool) w.type = W::Toggle;
    else if (d.type == PT::Choice || d.key == "groove.pattern") w.type = W::Selector;
    else w.type = W::Knob;
    (void)labelOverride;
    widgets.push_back(w);
}

void Editor::layoutPieceKnobs(Rect panel)
{
    const ParamTable& t = paramTable();
    const int nSpec = t.pieceCount[piece] - NumCommon;
    const float cw = 90, ch = 98;
    for (int k = 0; k < nSpec; ++k) {
        int col = k % 6, row = k / 6;
        knob({panel.x + 20 + col * cw, panel.y + 58 + row * ch, cw, ch}, t.specific(piece, k));
    }
    // channel strip
    const float sx = panel.x + 20 + 6 * cw + 20;
    Widget f; f.type = W::Fader; f.param = t.common(piece, C_Level); f.r = {sx, panel.y + 58, 36, 188}; widgets.push_back(f);
    const float kx = sx + 48, kw = 84;
    knob({kx + 0 * kw, panel.y + 58, kw, ch}, t.common(piece, C_Pan));
    knob({kx + 1 * kw, panel.y + 58, kw, ch}, t.common(piece, C_Tone));
    knob({kx + 2 * kw, panel.y + 58, kw, ch}, t.common(piece, C_Drive));
    knob({kx + 0 * kw, panel.y + 58 + ch, kw, ch}, t.common(piece, C_Dyn));
    knob({kx + 1 * kw, panel.y + 58 + ch, kw, ch}, t.common(piece, C_Room));
    knob({kx + 0 * kw, panel.y + 58 + 2 * ch, kw, ch}, t.pieceFx(piece, 0));
    knob({kx + 1 * kw, panel.y + 58 + 2 * ch, kw, ch}, t.pieceFx(piece, 1));
    Widget m; m.type = W::PieceMute; m.param = t.common(piece, C_Mute); m.r = {kx + 2 * kw + 12, panel.y + 58 + ch + 14, 58, 26}; widgets.push_back(m);
    Widget s; s.type = W::PieceSolo; s.param = t.common(piece, C_Solo); s.r = {kx + 2 * kw + 12, panel.y + 58 + ch + 48, 58, 26}; widgets.push_back(s);
    layoutPads(panel);
}

void Editor::layoutPads(Rect panel)
{
    const std::vector<int> notes = padNotes(piece);
    const int n = (int)notes.size();
    if (!n) return;
    const float x0 = panel.x + 120, avail = panel.w - 140, gap = 6;
    const float pw = std::min(104.0f, (avail - (n - 1) * gap) / (float)n);
    for (int i = 0; i < n; ++i) {
        Widget w; w.type = W::Pad; w.arg = notes[(size_t)i]; w.r = {x0 + i * (pw + gap), panel.y + 356, pw, 44};
        widgets.push_back(w);
    }
}

void Editor::layoutTab(Rect p)
{
    const ParamTable& t = paramTable();
    const float cw = 96, ch = 96, top = p.y + 48;
    auto K = [&](int col, int row, int idx, float xoff = 0) { knob({p.x + 18 + xoff + col * cw, top + 14 + row * 104.0f, cw, ch}, idx); };
    switch (tab) {
        case 0:
            K(0, 0, G_EqLo); K(1, 0, G_EqMid); K(2, 0, G_EqMidF); K(3, 0, G_EqHi);
            K(0, 0, G_SatDrive, 4 * cw + 40); K(1, 0, G_SatMix, 4 * cw + 40);
            K(0, 1, G_CompThr); K(1, 1, G_CompRatio); K(2, 1, G_CompAtt); K(3, 1, G_CompRel); K(4, 1, G_CompMakeup); K(5, 1, G_CompMix);
            break;
        case 1:
            for (int i = 0; i < 8; ++i) K(i, 0, G_RoomMix + i);
            break;
        case 2: {
            auto g = [&](int i) { return t.groove(i); };
            K(0, 0, g(GR_On)); K(1, 0, g(GR_Pattern)); K(2, 0, g(GR_Swing), cw * 0.6f); K(3, 0, g(GR_Intensity), cw * 0.6f);
            K(4, 0, g(GR_Rate), cw * 0.6f); K(5, 0, g(GR_Human), cw * 0.6f); K(6, 0, g(GR_Fill), cw * 0.6f); K(7, 0, g(GR_Stopped), cw * 0.6f);
            K(0, 1, t.ext(E_GrLaid));
            const float px = p.x + 18 + 1 * cw + 16;
            for (int i = 0; i < 6; ++i) {
                Widget lp; lp.type = W::LanePill; lp.param = t.ext(E_GrMuteKick + i); lp.arg = i;
                lp.r = {px + (i % 3) * 64.0f, top + 128 + (i / 3) * 36.0f, 58, 26}; widgets.push_back(lp);
            }
            Widget eb; eb.type = W::Btn; eb.arg = CMD_OPENEDIT; eb.r = {p.x + 790, top + 128, 80, 62}; widgets.push_back(eb);
            break; }
        case 3:
            K(0, 0, G_Master); K(1, 0, G_Humanize); K(2, 0, G_VelCurve); K(3, 0, G_Width); K(4, 0, G_Tune); K(5, 0, G_NoteOffset);
            K(6, 0, G_LimOn); K(7, 0, G_LimCeil);
            K(0, 1, t.ext(E_Buzz)); K(2, 1, t.ext(E_MorphAmt)); K(3, 1, t.ext(E_MorphTo));
            break;
    }
}

void Editor::build()
{
    widgets.clear();
    const ParamTable& t = paramTable();
    // header
    Widget w;
    w.type = W::PresetPrev; w.r = {330, 16, 32, 32}; widgets.push_back(w);
    w.type = W::PresetOpen; w.r = {368, 16, 336, 32}; widgets.push_back(w);
    w.type = W::PresetNext; w.r = {710, 16, 32, 32}; widgets.push_back(w);
    { Widget mb; mb.type = W::Btn; mb.arg = CMD_MUTATE; mb.r = {752, 18, 74, 30}; widgets.push_back(mb); }
    { Widget sb; sb.type = W::Btn; sb.arg = CMD_SAVEKIT; sb.r = {232, 18, 44, 30}; widgets.push_back(sb); sb.arg = CMD_LOADKIT; sb.r = {280, 18, 44, 30}; widgets.push_back(sb); }
    knob({834, 4, 84, 60}, t.ext(E_MorphAmt));
    widgets.back().arg = -1;   // compact header knob
    knob({930, 4, 84, 60}, G_Master);  // drawn compact in header
    widgets.back().arg = -1;
    Widget lim; lim.type = W::Toggle; lim.param = G_LimOn; lim.r = {1020, 4, 84, 60}; lim.arg = 0; widgets.push_back(lim);
    // kit column
    for (int p = 0; p < NumPieces; ++p) {
        Widget b; b.type = W::PieceBtn; b.arg = p; b.r = {16, 80.0f + p * 56.0f, 190, 52}; widgets.push_back(b);
    }
    const Rect main = kMain, bottom = kBottom;
    layoutPieceKnobs(main);
    for (int i = 0; i < 4; ++i) {
        Widget tb; tb.type = W::Tab; tb.arg = i; tb.r = {bottom.x + 16 + i * 108.0f, bottom.y + 12, 100, 28}; widgets.push_back(tb);
    }
    layoutTab(bottom);
    if (popup) {
        // preset grid overlay
        const int n = presetCount(), cols = 5, rows = (n + cols - 1) / cols;
        const float gx = 200, gy = 96, cw = 140, ch = std::min(52.0f, 520.0f / rows);
        for (int i = 0; i < n; ++i) {
            Widget it; it.type = W::PresetItem; it.arg = i;
            it.r = {gx + 16 + (i / rows) * cw, gy + 56 + (i % rows) * ch, cw - 8, ch - 6};
            widgets.push_back(it);
        }
    }
    (void)t;
    if (gedit) layoutEditor();
}

int Editor::hit(float lx, float ly) const
{
    for (int i = (int)widgets.size() - 1; i >= 0; --i) {
        const Widget& w = widgets[(size_t)i];
        if (gedit != w.ov) continue;
        if (popup && w.type != W::PresetItem) continue;
        Rect r = w.r;
        if (w.type == W::Knob) r = {w.r.x, w.r.y, w.r.w, w.r.h};
        if (r.contains(lx, ly)) return i;
    }
    return -1;
}

// ------------------------------------------------------------------ drawing
void Editor::drawWidget(const Widget& w, size_t index)
{
    Canvas& c = canvas;
    const ParamTable& t = paramTable();
    const bool hov = (int)index == hover || (int)index == drag;
    switch (w.type) {
        case W::Knob: {
            const ParamDef& d = t.defs[(size_t)w.param];
            const float n = norm(w.param);
            const bool compact = w.arg < 0;
            const float R = compact ? 17.0f : (w.arg > 0 ? w.arg / 10.0f : 26.0f);
            const float cx = w.r.cx(), cy = w.r.y + (compact ? 24.0f : 34.0f);
            const float a0 = 0.75f * kPi, a1 = 2.25f * kPi;
            c.fillCircle(cx, cy, R + 4, Color::hex(0x0c0e12));
            c.fillCircle(cx, cy, R + 1, hov ? kKnobHi : kKnob);
            c.arc(cx, cy, R + 7, a0, a1, 3.0f, kTrack);
            const bool bi = d.minV < 0 && d.maxV > 0 && d.type != PT::Db;
            float from = a0, to = a0 + n * (a1 - a0);
            if (bi) { float mid = a0 + (a1 - a0) * (-d.minV / (d.maxV - d.minV)); from = std::min(mid, to); to = std::max(mid, to); }
            if (to - from > 0.01f) c.arc(cx, cy, R + 7, from, to, 3.0f, drag == (int)index ? kEmber2 : kEmber);
            const float ang = a0 + n * (a1 - a0);
            c.line(cx + std::cos(ang) * (R * 0.35f), cy + std::sin(ang) * (R * 0.35f), cx + std::cos(ang) * (R - 2), cy + std::sin(ang) * (R - 2), 2.4f, kText);
            if (compact) { c.text(cx, w.r.y + 54, d.key == "master" ? "MASTER" : "MORPH", 9.5f, kDim, Align::Center, true); break; }
            c.text(cx, w.r.y + 74, d.title, 10.5f, kDim, Align::Center);
            c.text(cx, w.r.y + 88, valueText(w.param), 11, drag == (int)index ? kEmber2 : kText, Align::Center, true);
            break; }
        case W::Fader: {
            const float n = norm(w.param);
            const Rect r = w.r;
            c.fillRoundRect({r.cx() - 3, r.y, 6, r.h}, 3, Color::hex(0x0c0e12));
            const float hy = r.y + r.h - 12 - n * (r.h - 24);
            c.fillRoundRect({r.cx() - 3, hy, 6, r.y + r.h - hy}, 3, kPieceCol[piece].mix(kEmber, 0.35f));
            c.fillRoundRect({r.x + 2, hy - 8, r.w - 4, 16}, 4, hov ? kText : Color::hex(0xcfd3dc));
            c.line(r.x + 8, hy, r.x + r.w - 8, hy, 1.5f, Color::hex(0x2a2e38));
            c.text(r.cx(), r.y + r.h + 14, valueText(w.param), 10.5f, kText, Align::Center, true);
            c.text(r.cx(), r.y + r.h + 28, "LEVEL", 9.5f, kDim, Align::Center);
            break; }
        case W::Toggle:
        case W::PieceMute:
        case W::PieceSolo: {
            const bool on = host->get(w.param) > 0.5f;
            Rect pill = w.r;
            std::string label = t.defs[(size_t)w.param].title;
            if (w.type == W::Toggle) {
                const bool header = w.arg == 0 && w.r.y < 20;
                pill = {w.r.cx() - 28, w.r.y + (header ? 14.0f : 21.0f), 56, 26};
                if (header) label = "LIMITER";
            } else label = (w.type == W::PieceMute) ? "MUTE" : "SOLO";
            Color base = (w.type == W::PieceSolo) ? kEmber2 : kEmber;
            if (w.type == W::PieceMute) base = Color::hex(0xe5546b);
            c.fillRoundRect(pill, 13, on ? base : (hov ? kKnobHi : kKnob));
            c.strokeRoundRect(pill, 13, 1.0f, on ? base.mix(kText, 0.2f) : kBorder);
            if (w.type == W::Toggle) {
                c.text(pill.cx(), pill.cy(), on ? "ON" : "OFF", 10.5f, on ? Color::hex(0x1a0f08) : kDim, Align::Center, true);
                if (!(w.arg == 0 && w.r.y < 20)) c.text(w.r.cx(), w.r.y + 74, t.defs[(size_t)w.param].title, 10.5f, kDim, Align::Center);
                else c.text(w.r.cx(), w.r.y + 54, label, 9.5f, kDim, Align::Center, true);
            } else c.text(pill.cx(), pill.cy(), label, 10.5f, on ? Color::hex(0x1a0f08) : kDim, Align::Center, true);
            break; }
        case W::Selector: {
            const ParamDef& d = t.defs[(size_t)w.param];
            const bool wide = d.key == "groove.pattern" || d.key == "morph.to";
            Rect box = {w.r.x + (wide ? 6 : 10), w.r.y + 21, w.r.w - (wide ? 12 : 20), 26};
            if (wide) box.w = w.r.w + 30;
            c.fillRoundRect(box, 8, hov ? kKnobHi : kKnob);
            c.strokeRoundRect(box, 8, 1.0f, kBorder);
            std::string v = valueText(w.param);
            while (v.size() > 3 && c.textWidth(v, 11, true) > box.w - 16) v.pop_back();
            c.text(box.cx(), box.cy(), v, 11, kText, Align::Center, true);
            c.text(box.cx(), w.r.y + 74, d.title, 10.5f, kDim, Align::Center);
            break; }
        case W::PieceBtn: {
            const int p = w.arg;
            const bool sel = p == piece;
            const Color pc = kPieceCol[p];
            c.fillRoundRect(w.r, 10, sel ? kPanelHi : kPanel);
            if (flash[p] > 0.01f) c.glow(w.r.x + 24, w.r.cy(), 60, pc.alpha(0.55f * flash[p]));
            c.strokeRoundRect(w.r, 10, sel ? 1.6f : 1.0f, sel ? pc : kBorder);
            c.fillCircle(w.r.x + 22, w.r.cy() - 5, 6 + 3 * flash[p], pc.alpha(0.35f + 0.65f * flash[p]));
            c.text(w.r.x + 40, w.r.cy() - 7, pieceName(p), 13, sel ? kText : Color::hex(0xc4c9d4), Align::Left, true);
            const Rect meter = {w.r.x + 40, w.r.y + w.r.h - 16, 104, 5};
            c.fillRoundRect(meter, 2.5f, Color::hex(0x0c0e12));
            if (flash[p] > 0.01f) c.fillRoundRect({meter.x, meter.y, std::max(5.0f, meter.w * std::min(1.0f, flash[p])), meter.h}, 2.5f, pc);
            const bool muted = host->get(t.common(p, C_Mute)) > 0.5f, solo = host->get(t.common(p, C_Solo)) > 0.5f;
            c.text(w.r.x + w.r.w - 38, w.r.cy() - 8, "M", 10.5f, muted ? Color::hex(0xe5546b) : kFaint, Align::Center, true);
            c.text(w.r.x + w.r.w - 20, w.r.cy() - 8, "S", 10.5f, solo ? kEmber2 : kFaint, Align::Center, true);
            break; }
        case W::Pad: {
            const int note = w.arg;
            const Color pc = kPieceCol[piece];
            const float fl = std::max(padFlash[note], flash[piece] * 0.5f);
            c.fillRoundRect(w.r, 9, hov ? kKnobHi : kKnob);
            if (fl > 0.02f) { c.glow(w.r.cx(), w.r.cy(), 70, pc.alpha(0.55f * fl)); c.fillRoundRect(w.r, 9, pc.alpha(0.35f * fl)); }
            c.strokeRoundRect(w.r, 9, hov ? 1.6f : 1.0f, hov ? pc : kBorder);
            std::string lab = noteLabel(note);
            while (lab.size() > 3 && c.textWidth(lab, 11, true) > w.r.w - 8) lab.pop_back();
            c.text(w.r.cx(), w.r.y + 16, lab, 11, kText, Align::Center, true);
            c.text(w.r.cx(), w.r.y + 32, std::to_string(note) + " " + noteName(note), 9, kDim, Align::Center);
            break; }
        case W::LanePill: {
            static const char* nm[6] = {"KICK", "SNARE", "HATS", "TOMS", "CYMB", "PERC"};
            const bool on = host->get(w.param) > 0.5f;   // on = lane muted
            c.fillRoundRect(w.r, 13, on ? Color::hex(0xe5546b) : (hov ? kKnobHi : kKnob));
            c.strokeRoundRect(w.r, 13, 1.0f, on ? Color::hex(0xff8fa0) : kBorder);
            c.text(w.r.cx(), w.r.cy(), nm[w.arg], 10, on ? Color::hex(0x20080c) : kDim, Align::Center, true);
            break; }
        case W::Btn: {
            std::string label; bool on = false; bool tall = false;
            const ParamTable& tt = paramTable();
            switch (w.arg) {
                case CMD_MUTATE: label = "MUTATE"; break;
                case CMD_SAVEKIT: label = "SAVE"; break;
                case CMD_LOADKIT: label = "LOAD"; break;
                case CMD_OPENEDIT: label = "GROOVE\nEDITOR"; tall = true; break;
                case CMD_DONE: label = "DONE"; break;
                case CMD_SLOT: label = "USER " + std::to_string(w.param + 1); on = w.param == gslot; break;
                case CMD_LEN: label = std::to_string(w.param); on = host->userBank().p[gslot].steps == w.param; break;
                case CMD_CLEAR: label = "CLEAR"; break;
                case CMD_RANDOM: label = "RANDOM"; break;
                case CMD_ROTL: label = "< SHIFT"; break;
                case CMD_ROTR: label = "SHIFT >"; break;
                case CMD_DOUBLE: label = "x2 LENGTH"; break;
                case CMD_COPY: label = "COPY INTO SLOT"; break;
                case CMD_SRCPREV: label = "<"; break;
                case CMD_SRCNEXT: label = (w.r.w > 100) ? std::string(groovePattern(gsrc).name) : ">"; break;
                case CMD_GON: label = "GROOVE ON"; on = host->get(tt.groove(GR_On)) > 0.5f; break;
                case CMD_GSTOP: label = "PLAY WHEN STOPPED"; on = host->get(tt.groove(GR_Stopped)) > 0.5f; break;
                case CMD_RATE: label = std::string("RATE ") + tt.format(tt.groove(GR_Rate), host->get(tt.groove(GR_Rate))); break;
                case CMD_LANE: label = kUserLaneName[w.param]; break;
            }
            const bool accent = w.arg == CMD_MUTATE || w.arg == CMD_OPENEDIT || w.arg == CMD_DONE || w.arg == CMD_COPY;
            Color fill = on ? kEmber : (accent ? (hov ? kEmber2 : kEmber.mix(kKnob, 0.55f)) : (hov ? kKnobHi : kKnob));
            if (w.arg == CMD_LANE) fill = hov ? kKnobHi : kPanel;
            c.fillRoundRect(w.r, std::min(10.0f, w.r.h * 0.5f), fill);
            if (w.arg != CMD_LANE) c.strokeRoundRect(w.r, std::min(10.0f, w.r.h * 0.5f), 1.0f, on ? kEmber2 : kBorder);
            const Color tc = on || (accent && hov) ? Color::hex(0x1a0f08) : (accent ? kText : (w.arg == CMD_LANE ? kPieceCol[kLaneColor[w.param]] : kText));
            const size_t nl = label.find('\n');
            if (nl != std::string::npos) {
                c.text(w.r.cx(), w.r.cy() - 8, label.substr(0, nl), 11, tc, Align::Center, true);
                c.text(w.r.cx(), w.r.cy() + 9, label.substr(nl + 1), 11, tc, Align::Center, true);
            } else c.text(w.arg == CMD_LANE ? w.r.x + 6 : w.r.cx(), w.r.cy(), label, 10.5f, tc, w.arg == CMD_LANE ? Align::Left : Align::Center, true);
            (void)tall;
            if (w.arg == CMD_LANE) { c.fillCircle(w.r.x + w.r.w - 10, w.r.cy(), 3.5f, kPieceCol[kLaneColor[w.param]].alpha(0.5f)); }
            break; }
        case W::Tab: {
            const bool sel = w.arg == tab;
            c.fillRoundRect(w.r, 8, sel ? kEmber : (hov ? kKnobHi : kKnob));
            c.text(w.r.cx(), w.r.cy(), kTabNames[w.arg], 11, sel ? Color::hex(0x1a0f08) : kDim, Align::Center, true);
            break; }
        case W::PresetPrev:
        case W::PresetNext: {
            c.fillRoundRect(w.r, 9, hov ? kKnobHi : kKnob);
            const float cx = w.r.cx(), cy = w.r.cy(), d = w.type == W::PresetPrev ? -1.0f : 1.0f;
            c.line(cx - d * 3, cy - 7, cx + d * 4, cy, 2.2f, kText);
            c.line(cx + d * 4, cy, cx - d * 3, cy + 7, 2.2f, kText);
            break; }
        case W::PresetOpen: {
            const Preset& p = preset(host->program());
            c.fillRoundRect(w.r, 9, hov ? kKnobHi : kKnob);
            c.strokeRoundRect(w.r, 9, 1.0f, popup ? kEmber : kBorder);
            c.fillCircle(w.r.x + 18, w.r.cy(), 5, categoryColor(p.category));
            c.text(w.r.x + 34, w.r.cy(), p.name, 14, kText, Align::Left, true);
            c.text(w.r.x + w.r.w - 40, w.r.cy(), p.category, 10.5f, kDim, Align::Right);
            c.line(w.r.x + w.r.w - 28, w.r.cy() - 2, w.r.x + w.r.w - 22, w.r.cy() + 3, 2, kDim);
            c.line(w.r.x + w.r.w - 22, w.r.cy() + 3, w.r.x + w.r.w - 16, w.r.cy() - 2, 2, kDim);
            break; }
        case W::PresetItem: {
            const Preset& p = preset(w.arg);
            const bool cur = w.arg == host->program();
            c.fillRoundRect(w.r, 8, cur ? Color::hex(0x2c2218) : (hov ? kKnobHi : kPanel));
            c.strokeRoundRect(w.r, 8, cur ? 1.5f : 1.0f, cur ? kEmber : kBorder);
            c.fillRoundRect({w.r.x + 6, w.r.y + 8, 4, w.r.h - 16}, 2, categoryColor(p.category));
            std::string nm = p.name;
            while (nm.size() > 3 && c.textWidth(nm, 11.5f, true) > w.r.w - 24) nm.pop_back();
            c.text(w.r.x + 18, w.r.y + w.r.h * 0.38f, nm, 11.5f, kText, Align::Left, true);
            c.text(w.r.x + 18, w.r.y + w.r.h * 0.72f, p.category, 9.5f, kDim, Align::Left);
            break; }
    }
}

void Editor::render()
{
    Canvas& c = canvas;
    const ParamTable& t = paramTable();
    build();
    // background
    c.fillRoundRectV({0, 0, (float)kW, (float)kH}, 0, kBgTop, kBgBot);
    // header
    c.fillRoundRectV({0, 0, (float)kW, 68}, 0, Color::hex(0x1d2027), Color::hex(0x15171c));
    c.line(0, 68, (float)kW, 68, 1.0f, kBorder);
    // logo: forged hexagon + wordmark
    for (int i = 0; i < 6; ++i) {
        float a0 = kPi / 3 * i - kPi / 6, a1 = kPi / 3 * (i + 1) - kPi / 6;
        c.line(44 + std::cos(a0) * 19, 34 + std::sin(a0) * 19, 44 + std::cos(a1) * 19, 34 + std::sin(a1) * 19, 2.6f, kEmber);
    }
    c.fillCircle(44, 34, 7, kEmber2);
    c.text(76, 27, "DRUMFORGE", 21, kText, Align::Left, true);
    c.text(77, 48, "SYNTHESIZED DRUM KIT  ·  40 PRESETS", 9.5f, kDim, Align::Left);
    // panels
    const Rect main = kMain, bottom = kBottom;
    c.fillRoundRect(main, 14, kPanel); c.strokeRoundRect(main, 14, 1.0f, kBorder);
    c.fillRoundRect(bottom, 14, kPanel); c.strokeRoundRect(bottom, 14, 1.0f, kBorder);
    // piece header
    const Color pc = kPieceCol[piece];
    c.fillCircle(main.x + 30, main.y + 30, 7, pc);
    c.glow(main.x + 30, main.y + 30, 26, pc.alpha(0.4f * (0.3f + flash[piece])));
    c.text(main.x + 48, main.y + 30, pieceName(piece), 21, kText, Align::Left, true);
    {
        std::string notes = "MIDI: ";
        bool first = true;
        for (int n = 0; n < 128; ++n)
            if (noteInfo(n).piece == piece) { notes += (first ? "" : "  ·  ") + std::to_string(n) + " " + noteName(n); first = false; if (notes.size() > 70) break; }
        c.text(main.x + main.w - 20, main.y + 30, notes, 10.5f, kDim, Align::Right);
    }
    c.line(main.x + 20, main.y + 50, main.x + main.w - 20, main.y + 50, 1.0f, kBorder);
    // strip separators
    c.line(main.x + 20 + 6 * 90 + 10, main.y + 62, main.x + 20 + 6 * 90 + 10, main.y + 346, 1.0f, kBorder);
    c.line(main.x + 20, main.y + 350, main.x + main.w - 20, main.y + 350, 1.0f, kBorder);
    c.text(main.x + 20, main.y + 372, "TEST PADS", 10.5f, kDim, Align::Left, true);
    c.text(main.x + 20, main.y + 388, "click = hit, higher = harder", 8.5f, kFaint, Align::Left);
    // tab content headings
    {
        const float top = bottom.y + 48;
        auto head = [&](float x, float y, const char* s) { c.text(x, top + y, s, 9.5f, kFaint, Align::Left, true); };
        if (tab == 0) { head(bottom.x + 22, 4, "EQUALIZER"); head(bottom.x + 22 + 4 * 96 + 40, 4, "SATURATOR"); head(bottom.x + 22, 111, "BUS COMPRESSOR"); }
        if (tab == 1) { head(bottom.x + 22, 4, "ROOM / REVERB"); c.text(bottom.x + 22, top + 130, "Gate Time: 0 = off (the Gated room type uses 220 ms when it is 0). Every drum has its own Room Send.", 10.5f, kDim, Align::Left); }
        if (tab == 2) {
            head(bottom.x + 22, 4, "BUILT-IN GROOVE PLAYER  -  follows the host transport, or free-runs with 'Play When Stopped'");
            head(bottom.x + 22, 111, "FEEL");
            head(bottom.x + 22 + 1 * 96 + 16, 111, "LANE MUTES (lit = muted)");
            const float px = bottom.x + 22 + 1 * 96 + 16 + 3 * 64 + 28;
            const int pat = (int)std::lround(host->get(t.groove(GR_Pattern)));
            std::string nm = pat >= grooveCount() ? "User " + std::to_string(pat - grooveCount() + 1) : std::string(groovePattern(pat).name);
            c.text(px, top + 119, nm, 11.5f, kText, Align::Left, true);
            drawPatternPreview(px, top + 130, 360, 88);
        }
        if (tab == 3) { head(bottom.x + 22, 4, "MASTER"); head(bottom.x + 22, 111, "SNARE WIRES"); head(bottom.x + 22 + 2 * 96, 111, "KIT MORPH  -  blend toward another preset  (mod wheel CC1 controls Morph)"); }
    }
    for (size_t i = 0; i < widgets.size(); ++i)
        if (widgets[i].type != W::PresetItem) drawWidget(widgets[i], i);
    if (popup) {
        c.fillRect({0, 0, (float)kW, (float)kH}, Color(8, 9, 12, 205));
        const Rect pn{200, 96, 740, 590};
        c.fillRoundRect(pn, 16, Color::hex(0x14161b)); c.strokeRoundRect(pn, 16, 1.2f, kBorder);
        c.text(pn.x + 20, pn.y + 28, "FACTORY PRESETS", 13, kText, Align::Left, true);
        c.text(pn.x + pn.w - 20, pn.y + 28, "click to load  ·  Esc to close", 10.5f, kDim, Align::Right);
        for (size_t i = 0; i < widgets.size(); ++i)
            if (widgets[i].type == W::PresetItem) drawWidget(widgets[i], i);
    }
    if (gedit) drawEditor();
    dirty = false;
}

void Editor::drawPatternPreview(float x, float y, float w, float h)
{
    Canvas& c = canvas;
    const ParamTable& t = paramTable();
    const int pat = (int)std::lround(host->get(t.groove(GR_Pattern)));
    int steps = 16, lanes = NumLanes;
    const bool user = pat >= grooveCount();
    const GroovePattern* gp = user ? nullptr : &groovePattern(pat);
    const UserPattern* up = user ? &host->userBank().p[std::min(7, pat - grooveCount())] : nullptr;
    if (user) { steps = std::max(4, (int)up->steps); lanes = kUserLanes; }
    else {
        for (int l = 0; l < NumLanes; ++l) {
            int n = 0;
            if (gp->lanes[l]) for (const char* ch = gp->lanes[l]; *ch; ++ch) if (*ch != ' ' && *ch != '|') ++n;
            steps = std::max(steps, n);
        }
        steps = ((steps + 15) / 16) * 16;
    }
    const float gapW = 4.0f * (float)(steps / 4);
    const float cw = std::min(17.0f, (w - gapW) / (float)steps);
    const float rh = h / (float)lanes;
    const float ph = host->groovePhase();
    for (int l = 0; l < lanes; ++l) {
        const float ly = y + l * rh;
        const Color col = kPieceCol[user ? kLaneColor[l] : l % NumPieces];
        for (int st = 0; st < steps; ++st) {
            const float cx = x + st * cw + (st / 4) * 4.0f;
            float v = -1;
            if (user) { if (up->cell[l][st].vel) v = up->cell[l][st].vel / 127.0f; }
            else if (gp->lanes[l]) {
                int k = 0;
                for (const char* ch = gp->lanes[l]; *ch; ++ch) {
                    if (*ch == ' ' || *ch == '|') continue;
                    if (k++ == st && *ch != '.') v = (*ch >= '1' && *ch <= '9') ? (*ch - '0') / 9.0f : 0.9f;
                }
            }
            c.fillRoundRect({cx, ly, std::max(1.5f, cw - 1.5f), std::max(1.5f, rh - 1.5f)}, 1.5f, v >= 0 ? col.alpha(0.3f + 0.7f * v) : Color::hex(0x23262e));
        }
    }
    if (ph >= 0) {
        const int st = std::min(steps - 1, (int)(ph * steps));
        c.fillRect({x + st * cw + (st / 4) * 4.0f - 1, y - 2, cw + 1, h + 4}, Color(255, 255, 255, 40));
    }
}

void Editor::layoutEditor()
{
    auto B = [&](int cmd, int arg, Rect r) { Widget w; w.type = W::Btn; w.arg = cmd; w.param = arg; w.r = r; w.ov = true; widgets.push_back(w); };
    B(CMD_DONE, 0, {986, 38, 86, 32});
    for (int i = 0; i < 8; ++i) B(CMD_SLOT, i, {48.0f + i * 92, 84, 86, 34});
    for (int i = 0; i < 6; ++i) B(CMD_LEN, kLenChoices[i], {128.0f + i * 56, 134, 50, 30});
    B(CMD_CLEAR, 0, {482, 134, 70, 30}); B(CMD_RANDOM, 0, {558, 134, 80, 30});
    B(CMD_ROTL, 0, {644, 134, 66, 30}); B(CMD_ROTR, 0, {716, 134, 66, 30}); B(CMD_DOUBLE, 0, {788, 134, 90, 30});
    B(CMD_SRCPREV, 0, {250, 180, 30, 30}); B(CMD_SRCNEXT, 0, {284, 180, 220, 30}); B(CMD_SRCNEXT, 1, {508, 180, 30, 30});
    B(CMD_COPY, 0, {546, 180, 150, 30});
    const ParamTable& t = paramTable();
    (void)t;
    B(CMD_GON, 0, {716, 180, 100, 30}); B(CMD_GSTOP, 0, {822, 180, 150, 30}); B(CMD_RATE, 0, {978, 180, 94, 30});
    for (int l = 0; l < kUserLanes; ++l) B(CMD_LANE, l, {48, 236.0f + l * 32, 92, 30});
}

void Editor::drawEditor()
{
    Canvas& c = canvas;
    const ParamTable& t = paramTable();
    c.fillRect({0, 0, (float)kW, (float)kH}, Color(8, 9, 12, 225));
    const Rect pn{24, 24, 1072, 732};
    c.fillRoundRect(pn, 16, Color::hex(0x14161b)); c.strokeRoundRect(pn, 16, 1.2f, kBorder);
    c.text(48, 56, "GROOVE EDITOR", 20, kText, Align::Left, true);
    c.text(250, 58, "8 user slots, saved with your project. Pick a slot, click the grid to place hits - the slot plays when Groove is ON.", 10.5f, kDim, Align::Left);
    c.text(48, 150, "LENGTH", 10, kFaint, Align::Left, true);
    c.text(48, 196, "COPY A FACTORY GROOVE:", 10, kFaint, Align::Left, true);
    UserPattern& u = host->userBank().p[gslot];
    const int steps = std::max(4, (int)u.steps);
    c.text(886, 150, std::to_string(steps) + " steps  =  " + std::to_string(steps / 16) + (steps % 16 ? "+" : "") + " bar" + (steps > 16 ? "s" : "") + " at 1/16", 10.5f, kDim, Align::Left);
    for (size_t i = 0; i < widgets.size(); ++i) if (widgets[i].ov) drawWidget(widgets[i], i);
    // grid geometry
    const float gx = 150, gy = 236, rh = 32;
    const float cw = std::min(40.0f, (920.0f - 6.0f * (float)(steps / 4)) / (float)steps);
    const int curPat = (int)std::lround(host->get(t.groove(GR_Pattern)));
    const bool playingThis = curPat == grooveCount() + gslot;
    const float ph = host->groovePhase();
    for (int st = 0; st < steps; ++st) {
        const float x = gx + st * cw + (st / 4) * 6.0f;
        if (st % 4 == 0 && cw >= 11) c.text(x + 2, gy - 9, std::to_string(st / 4 % 4 + 1), 8.5f, st % 16 == 0 ? kEmber2 : kFaint, Align::Left, true);
    }
    for (int l = 0; l < kUserLanes; ++l) {
        const Color col = kPieceCol[kLaneColor[l]];
        for (int st = 0; st < steps; ++st) {
            const float x = gx + st * cw + (st / 4) * 6.0f, y = gy + l * rh;
            const Rect cell{x, y, std::max(3.0f, cw - 1.5f), rh - 3.0f};
            const bool alt = (st / 4) % 2 == 0;
            c.fillRoundRect(cell, 3, alt ? Color::hex(0x21252d) : Color::hex(0x1b1e25));
            const StepCell& sc = u.cell[l][st];
            if (sc.vel) {
                const float v = sc.vel / 127.0f;
                const float bh = std::max(5.0f, cell.h * v);
                c.fillRoundRect({cell.x, cell.y + cell.h - bh, cell.w, bh}, 3, col.alpha(0.45f + 0.55f * v));
                c.strokeRoundRect(cell, 3, 1.0f, col);
                if (sc.ratchet > 1 && cell.w > 7) for (int k = 0; k < sc.ratchet; ++k) c.fillCircle(cell.x + 3.5f + k * 4.5f, cell.y + 4, 1.5f, kText);
                if (sc.prob < 100) c.fillRect({cell.x + 1, cell.y + cell.h - 3, (cell.w - 2) * sc.prob / 100.0f, 2}, Color::hex(0xffffff));
            }
        }
    }
    if (ph >= 0 && playingThis) {
        const int st = std::min(steps - 1, (int)(ph * steps));
        const float x = gx + st * cw + (st / 4) * 6.0f;
        c.fillRect({x - 1, gy - 4, cw + 1, rh * kUserLanes + 4}, Color(255, 255, 255, 38));
    }
    if (!playingThis) c.text(gx, gy + rh * kUserLanes + 20, "This slot is not the selected groove pattern - click its USER button above to hear it.", 10.5f, kEmber2, Align::Left);
    c.text(gx, gy + rh * kUserLanes + (playingThis ? 20 : 38), "click: place / remove    drag a hit up or down: velocity    right-click: ratchet 1-4    shift+click: probability    click a lane name: hear it", 10, kDim, Align::Left);
}

// ------------------------------------------------------------------ groove editor logic
bool Editor::cellAt(float lx, float ly, int& lane, int& step)
{
    const UserPattern& u = host->userBank().p[gslot];
    const int steps = std::max(4, (int)u.steps);
    const float gx = 150, gy = 236, rh = 32;
    const float cw = std::min(40.0f, (920.0f - 6.0f * (float)(steps / 4)) / (float)steps);
    if (ly < gy || ly >= gy + rh * kUserLanes) return false;
    lane = (int)((ly - gy) / rh);
    for (int st = 0; st < steps; ++st) {
        const float x = gx + st * cw + (st / 4) * 6.0f;
        if (lx >= x && lx < x + cw) { step = st; return true; }
    }
    return false;
}

void Editor::setCell(int lane, int step, int vel)
{
    StepCell& c = host->userBank().p[gslot].cell[lane][step];
    c.vel = (uint8_t)std::max(0, std::min(127, vel));
    if (!c.vel) { c.ratchet = 1; c.prob = 100; }
    gchanged = true; dirty = true;
}

void Editor::editorMouseDown(float lx, float ly, int button, bool shift)
{
    int lane, step;
    if (!cellAt(lx, ly, lane, step)) return;
    StepCell& c = host->userBank().p[gslot].cell[lane][step];
    glane = lane; gstep = step; gmoved = false; gchanged = false;
    if (button == 1) {                          // right click: ratchet
        if (!c.vel) c.vel = 100;
        c.ratchet = (uint8_t)(c.ratchet % 4 + 1);
        gchanged = true; dirty = true; host->userSlotChanged(gslot); gchanged = false; gmode = 0; return;
    }
    if (c.vel && shift) {                       // shift+click: probability 100 -> 75 -> 50 -> 25
        c.prob = (uint8_t)(c.prob > 75 ? 75 : (c.prob > 50 ? 50 : (c.prob > 25 ? 25 : 100)));
        gchanged = true; dirty = true; host->userSlotChanged(gslot); gchanged = false; gmode = 0; return;
    }
    if (!c.vel) {
        setCell(lane, step, 100); gmode = 1;
        host->trigger(kUserLaneNote[lane], 100);
    } else { gmode = 2; gstartVel = c.vel; gstartY = ly; }
}

void Editor::command(int cmd, int arg, int button, bool shift)
{
    const ParamTable& t = paramTable();
    UserPattern& u = host->userBank().p[gslot];
    auto setParam = [&](int idx, float v) { host->begin(idx); host->set(idx, v); host->end(idx); };
    bool changed = false;
    switch (cmd) {
        case CMD_OPENEDIT: {
            const int pat = (int)std::lround(host->get(t.groove(GR_Pattern)));
            if (pat >= grooveCount()) gslot = std::min(7, pat - grooveCount());
            gedit = true; popup = false; break; }
        case CMD_DONE: gedit = false; break;
        case CMD_MUTATE: mutate(shift || button == 1 ? 0.35f : 0.15f); break;
        case CMD_SAVEKIT: {   // save every kit setting (not groove / morph) to a file of the user's choice
            std::vector<float> v((size_t)t.size());
            for (int i = 0; i < t.size(); ++i) v[(size_t)i] = host->get(i);
            host->saveKitText(kitToText(v.data()));
            break; }
        case CMD_LOADKIT: {
            std::string text;
            if (!host->loadKitText(text)) break;
            std::vector<float> v((size_t)t.size());
            for (int i = 0; i < t.size(); ++i) v[(size_t)i] = t.defs[(size_t)i].defV;
            if (kitFromText(text, v.data()) < 0) break;
            for (int i = 0; i < t.size(); ++i) {
                if (t.isSession(i) || v[(size_t)i] == host->get(i)) continue;
                host->begin(i); host->set(i, v[(size_t)i]); host->end(i);
            }
            break; }
        case CMD_SLOT: gslot = arg; setParam(t.groove(GR_Pattern), (float)(grooveCount() + arg)); break;
        case CMD_LEN: u.steps = (uint8_t)arg; changed = true; break;
        case CMD_CLEAR: u.clear(); changed = true; break;
        case CMD_RANDOM: userRandomize(u, (uint32_t)(rnd01() * 16777216.0f) + 1u); changed = true; break;
        case CMD_ROTL: case CMD_ROTR: {
            const int n = u.steps;
            for (int l = 0; l < kUserLanes; ++l) {
                StepCell tmp[kMaxSteps];
                for (int k = 0; k < n; ++k) tmp[(k + (cmd == CMD_ROTL ? n - 1 : 1)) % n] = u.cell[l][k];
                for (int k = 0; k < n; ++k) u.cell[l][k] = tmp[k];
            }
            changed = true; break; }
        case CMD_DOUBLE:
            if (u.steps * 2 <= kMaxSteps) {
                for (int l = 0; l < kUserLanes; ++l) for (int k = 0; k < u.steps; ++k) u.cell[l][u.steps + k] = u.cell[l][k];
                u.steps = (uint8_t)(u.steps * 2); changed = true;
            }
            break;
        case CMD_COPY: userFromFactory(gsrc, u); changed = true; break;
        case CMD_SRCPREV: gsrc = (gsrc + grooveCount() - 1) % grooveCount(); break;
        case CMD_SRCNEXT: gsrc = (gsrc + 1) % grooveCount(); break;
        case CMD_GON: setParam(t.groove(GR_On), host->get(t.groove(GR_On)) > 0.5f ? 0.f : 1.f); break;
        case CMD_GSTOP: setParam(t.groove(GR_Stopped), host->get(t.groove(GR_Stopped)) > 0.5f ? 0.f : 1.f); break;
        case CMD_RATE: setParam(t.groove(GR_Rate), (float)(((int)std::lround(host->get(t.groove(GR_Rate))) + 1) % 3)); break;
        case CMD_LANE: host->trigger(kUserLaneNote[arg], 100); break;
    }
    if (changed) host->userSlotChanged(gslot);
    dirty = true;
}

void Editor::mutate(float amount)
{
    const ParamTable& t = paramTable();
    for (int p = 0; p < NumPieces; ++p) {
        const int n = t.pieceCount[p] - NumCommon;
        for (int k = -1; k < n; ++k) {
            const int idx = k < 0 ? t.common(p, C_Tone) : t.specific(p, k);
            const ParamDef& d = t.defs[(size_t)idx];
            if (d.type == PT::Bool || d.type == PT::Choice) continue;
            const float n0 = t.toNorm(idx, host->get(idx));
            const float n1 = std::max(0.0f, std::min(1.0f, n0 + (rnd01() * 2.0f - 1.0f) * amount));
            host->begin(idx); host->set(idx, t.toPlain(idx, n1)); host->end(idx);
        }
    }
    {   // demo: play the drum that is selected, so you hear what the mutation did to it
        static const int demo[NumPieces] = {36, 38, 37, 41, 45, 50, 42, 49, 51, 39};
        host->trigger(demo[piece], 105);
        padFlash[demo[piece]] = 1.0f;
    }
    dirty = true;
}

void Editor::hitPad(int note, float ly, const Rect& r, bool shift)
{
    const float k = std::max(0.0f, std::min(1.0f, (ly - r.y) / r.h));
    int vel = (int)std::lround(127.0f - k * 80.0f);
    if (shift) vel = 45;
    host->trigger(note, vel);
    padFlash[note] = 1.0f; dirty = true;
}

// ------------------------------------------------------------------ input
void Editor::mouseDown(float px, float py, int button, bool shift, bool dbl)
{
    const float s = canvas.scale();
    const float lx = px / s, ly = py / s;
    mx = lx; my = ly;
    if (widgets.empty()) build();
    int i = hit(lx, ly);
    if (gedit) {
        if (i >= 0 && widgets[(size_t)i].type == W::Btn) command(widgets[(size_t)i].arg, widgets[(size_t)i].param, button, shift);
        else if (i < 0) editorMouseDown(lx, ly, button, shift);
        return;
    }
    if (popup && (i < 0 || widgets[(size_t)i].type != W::PresetItem)) { popup = false; dirty = true; return; }
    if (i < 0) return;
    const Widget w = widgets[(size_t)i];
    const ParamTable& t = paramTable();
    switch (w.type) {
        case W::Pad: hitPad(w.arg, ly, w.r, shift); break;
        case W::Btn: command(w.arg, w.param, button, shift); break;
        case W::LanePill: applyNorm(w.param, host->get(w.param) > 0.5f ? 0.0f : 1.0f, true); break;
        case W::Knob:
            if (dbl || button == 1) { applyNorm(w.param, t.defaultNorm(w.param), true); break; }
            drag = i; dragStartY = ly; dragStartNorm = norm(w.param); host->begin(w.param); break;
        case W::Fader:
            if (dbl) { applyNorm(w.param, t.defaultNorm(w.param), true); break; }
            drag = i; host->begin(w.param);
            applyNorm(w.param, 1.0f - (ly - w.r.y - 12) / (w.r.h - 24), false); break;
        case W::Toggle: case W::PieceMute: case W::PieceSolo:
            applyNorm(w.param, host->get(w.param) > 0.5f ? 0.0f : 1.0f, true); break;
        case W::Selector: {
            const ParamDef& d = t.defs[(size_t)w.param];
            const int steps = std::max(1, d.steps);
            int cur = (int)std::lround(t.toNorm(w.param, host->get(w.param)) * steps);
            cur = (cur + (button == 1 ? steps : 1)) % (steps + 1);
            applyNorm(w.param, (float)cur / (float)steps, true); break; }
        case W::PieceBtn: {
            piece = w.arg;
            const float lx2 = lx - w.r.x;
            if (lx2 <= w.r.w - 48) { const std::vector<int> pn = padNotes(piece); if (!pn.empty()) { host->trigger(pn[0], 100); padFlash[pn[0]] = 1.0f; } }   // clicking a drum also plays it
            if (lx2 > w.r.w - 48) {   // M / S letters
                const int idx = lx2 > w.r.w - 29 ? t.common(w.arg, C_Solo) : t.common(w.arg, C_Mute);
                applyNorm(idx, host->get(idx) > 0.5f ? 0.0f : 1.0f, true);
            }
            dirty = true; break; }
        case W::Tab: tab = w.arg; dirty = true; break;
        case W::PresetPrev: host->selectProgram((host->program() + presetCount() - 1) % presetCount()); dirty = true; break;
        case W::PresetNext: host->selectProgram((host->program() + 1) % presetCount()); dirty = true; break;
        case W::PresetOpen: popup = !popup; dirty = true; break;
        case W::PresetItem: host->selectProgram(w.arg); popup = false; dirty = true; break;
    }
}

void Editor::mouseMove(float px, float py, bool shift)
{
    const float s = canvas.scale();
    const float lx = px / s, ly = py / s;
    mx = lx; my = ly;
    if (gedit) {
        if (gmode) {
            int lane, step;
            if (gmode == 1 && cellAt(lx, ly, lane, step) && lane == glane && step != gstep) {
                const int dir = step > gstep ? 1 : -1;
                for (int st = gstep + dir; st != step + dir; st += dir) {        // fill every step the mouse skipped over
                    StepCell& c = host->userBank().p[gslot].cell[lane][st];
                    if (!c.vel) setCell(lane, st, 100);
                }
                gstep = step;
            } else if (gmode == 2) {
                if (std::fabs(ly - gstartY) > 3) gmoved = true;
                if (gmoved) setCell(glane, gstep, gstartVel + (int)std::lround((gstartY - ly) * 1.6f) + 0);
                if (gmoved && host->userBank().p[gslot].cell[glane][gstep].vel == 0) setCell(glane, gstep, 1);
            }
        }
        int h = widgets.empty() ? -1 : hit(lx, ly);
        if (h != hover) { hover = h; dirty = true; }
        return;
    }
    if (drag >= 0 && (size_t)drag < widgets.size()) {
        const Widget& w = widgets[(size_t)drag];
        if (w.type == W::Knob) {
            const float range = shift ? 1100.0f : 170.0f;
            float n = dragStartNorm + (dragStartY - ly) / range;
            applyNorm(w.param, std::max(0.0f, std::min(1.0f, n)), false);
        } else if (w.type == W::Fader) {
            applyNorm(w.param, std::max(0.0f, std::min(1.0f, 1.0f - (ly - w.r.y - 12) / (w.r.h - 24))), false);
        }
        return;
    }
    int h = widgets.empty() ? -1 : hit(lx, ly);
    if (h != hover) { hover = h; dirty = true; }
}

void Editor::mouseUp(float, float)
{
    if (gedit && gmode) {
        if (gmode == 2 && !gmoved) { setCell(glane, gstep, 0); }   // click without drag on an existing hit removes it
        if (gchanged) host->userSlotChanged(gslot);
        gmode = 0; gchanged = false; dirty = true;
        return;
    }
    if (drag >= 0 && (size_t)drag < widgets.size()) {
        const Widget& w = widgets[(size_t)drag];
        if (w.type == W::Knob || w.type == W::Fader) host->end(w.param);
    }
    drag = -1; dirty = true;
}

void Editor::wheel(float px, float py, float notches, bool shift)
{
    const float s = canvas.scale();
    if (gedit) return;
    int i = hit(px / s, py / s);
    if (i < 0) return;
    const Widget& w = widgets[(size_t)i];
    const ParamTable& t = paramTable();
    if (w.type == W::Knob || w.type == W::Fader) {
        const ParamDef& d = t.defs[(size_t)w.param];
        float step = shift ? 0.004f : 0.02f;
        if (d.steps > 0) step = 1.0f / (float)d.steps;
        applyNorm(w.param, std::max(0.0f, std::min(1.0f, norm(w.param) + notches * step)), true);
    } else if (w.type == W::Selector) {
        const ParamDef& d = t.defs[(size_t)w.param];
        const int steps = std::max(1, d.steps);
        int cur = (int)std::lround(norm(w.param) * steps) + (notches > 0 ? 1 : -1);
        cur = std::max(0, std::min(steps, cur));
        applyNorm(w.param, (float)cur / (float)steps, true);
    } else if (w.type == W::PresetOpen) {
        host->selectProgram((host->program() + (notches > 0 ? -1 : 1) + presetCount()) % presetCount()); dirty = true;
    }
}

void Editor::keyDown(int key)
{
    if (key == 27 && gedit) { gedit = false; dirty = true; }
    else if (key == 27 && popup) { popup = false; dirty = true; }
    else if (gedit) {}
    else if (key == 37) { host->selectProgram((host->program() + presetCount() - 1) % presetCount()); dirty = true; }
    else if (key == 39) { host->selectProgram((host->program() + 1) % presetCount()); dirty = true; }
}

void Editor::mouseLeave() { if (hover != -1) { hover = -1; dirty = true; } }

}}  // namespace df::gui
