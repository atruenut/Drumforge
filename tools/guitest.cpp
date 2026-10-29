// Renders the editor to PPM files and exercises its interaction logic against a mock host.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../src/engine/engine.h"
#include "../src/engine/presets.h"
#include "../src/gui/gui.h"
using namespace df;

struct Mock : gui::IHost {
    std::vector<float> v; int prog = 0; int begins = 0, ends = 0, sets = 0; float met[10] = {};
    Mock() { const auto& t = paramTable(); v.resize((size_t)t.size()); for (int i = 0; i < t.size(); ++i) v[(size_t)i] = t.defs[(size_t)i].defV; }
    float get(int i) override { return v[(size_t)i]; }
    void begin(int) override { ++begins; }
    void set(int i, float p) override { v[(size_t)i] = p; ++sets; }
    void end(int) override { ++ends; }
    int program() override { return prog; }
    void selectProgram(int i) override { prog = i; presetApply(i, v.data()); }
    float meter(int p) override { return met[p]; }
    int lastNote = -1, lastVel = 0, trigs = 0, slotEdits = 0; UserBank bank; float phase = -1;
    void trigger(int n, int v) override { lastNote = n; lastVel = v; ++trigs; }
    UserBank& userBank() override { return bank; }
    void userSlotChanged(int) override { ++slotEdits; }
    float groovePhase() override { return phase; }
    std::string kitFile; bool saveKitText(const std::string& t) override { kitFile = t; return true; }
    bool loadKitText(std::string& t) override { t = kitFile; return !kitFile.empty(); }
};

static void savePpm(const gui::Canvas& c, const char* path)
{
    FILE* f = std::fopen(path, "wb");
    std::fprintf(f, "P6\n%d %d\n255\n", c.width(), c.height());
    for (int i = 0; i < c.width() * c.height(); ++i) { uint32_t p = c.pixels()[i]; unsigned char rgb[3] = {(unsigned char)(p >> 16), (unsigned char)(p >> 8), (unsigned char)p}; std::fwrite(rgb, 1, 3, f); }
    std::fclose(f);
}

int main(int argc, char** argv)
{
    const char* dir = argc > 1 ? argv[1] : ".";
    float scale = argc > 2 ? (float)std::atof(argv[2]) : 1.0f;
    Mock m; gui::Editor ed(&m); ed.setScale(scale);
    char path[512];
    int fails = 0;
    auto check = [&](bool ok, const char* what) { if (!ok) { std::printf("FAIL: %s\n", what); ++fails; } };
    const ParamTable& t = paramTable();

    m.selectProgram(12); m.met[0] = 0.9f; m.met[1] = 0.5f;   // Death Metal Trigger, kick + snare flashing
    ed.tick(); ed.render(); std::snprintf(path, sizeof path, "%s/gui_kick.ppm", dir); savePpm(ed.surface(), path);

    // select the snare through the kit column, drag a knob, check the parameter moved
    const float s = ed.scale();
    ed.mouseDown(100 * s, (80 + 56 * 1 + 26) * s, 0, false, false); ed.mouseUp(0, 0);
    check(ed.selectedPiece() == 1, "piece button selects snare");
    ed.render();
    float before = m.v[(size_t)t.specific(Snare, SnareP::Tune)];
    // first knob of the main panel: x = 222+20+45, knob centre y = 80+58+34
    ed.mouseDown((222 + 20 + 45) * s, (80 + 58 + 34) * s, 0, false, false);
    ed.mouseMove((222 + 20 + 45) * s, (80 + 58 + 34 - 60) * s, false);
    ed.mouseUp(0, 0);
    float after = m.v[(size_t)t.specific(Snare, SnareP::Tune)];
    check(after > before, "dragging a knob up raises snare tune");
    check(m.begins == m.ends && m.begins > 0, "begin/end edits are balanced");
    // double click resets
    ed.mouseDown((222 + 20 + 45) * s, (80 + 58 + 34) * s, 0, false, true); ed.mouseUp(0, 0);
    check(std::fabs(m.v[(size_t)t.specific(Snare, SnareP::Tune)] - 195.0f) < 0.5f, "double click resets to default");
    // wheel
    ed.render();
    ed.wheel((222 + 20 + 45) * s, (80 + 58 + 34) * s, 3, false);
    check(m.v[(size_t)t.specific(Snare, SnareP::Tune)] > 195.0f, "wheel changes value");

    ed.render(); std::snprintf(path, sizeof path, "%s/gui_snare.ppm", dir); savePpm(ed.surface(), path);
    // tabs
    for (int tb = 1; tb < 4; ++tb) {
        ed.mouseDown((222 + 16 + tb * 108 + 50) * s, (500 + 12 + 14) * s, 0, false, false); ed.mouseUp(0, 0);
        check(ed.tabIndex() == tb, "tab switch");
        ed.render(); std::snprintf(path, sizeof path, "%s/gui_tab%d.ppm", dir, tb); savePpm(ed.surface(), path);
    }
    // preset popup
    ed.mouseDown(500 * s, 32 * s, 0, false, false); ed.mouseUp(0, 0);
    check(ed.popupOpen(), "preset popup opens");
    ed.render(); std::snprintf(path, sizeof path, "%s/gui_popup.ppm", dir); savePpm(ed.surface(), path);
    ed.mouseDown((200 + 16 + 140 * 2 + 60) * s, (96 + 56 + 3 * 52 * 0.9f) * s, 0, false, false); ed.mouseUp(0, 0);
    check(!ed.popupOpen(), "popup closes after picking");
    std::printf("selected preset now %d (%s)\n", m.prog, preset(m.prog).name);
    // toggle + selector
    ed.mouseDown(1060 * s, 30 * s, 0, false, false); ed.mouseUp(0, 0);
    check(m.v[(size_t)G_LimOn] < 0.5f, "limiter toggle in header");
    // ---- test pads: kick piece is selected? select it, hit the second pad near the top (hard) and near the bottom (soft)
    ed.mouseDown(100 * s, (80 + 26) * s, 0, false, false); ed.mouseUp(0, 0);   // kick button: selects + auditions
    check(ed.selectedPiece() == 0 && m.lastNote == 35 && m.trigs >= 1, "clicking a drum button auditions it");
    ed.render();
    int t0 = m.trigs;
    ed.mouseDown((222 + 120 + 104 + 6 + 40) * s, (80 + 356 + 3) * s, 0, false, false); ed.mouseUp(0, 0);
    check(m.trigs == t0 + 1 && m.lastNote == 36, "kick pad 2 triggers note 36");
    const int hardV = m.lastVel;
    ed.mouseDown((222 + 120 + 104 + 6 + 40) * s, (80 + 356 + 41) * s, 0, false, false); ed.mouseUp(0, 0);
    check(m.lastVel < hardV - 40, "pad velocity follows click height");
    ed.mouseDown(100 * s, (80 + 56 * 7 + 26) * s, 0, false, false); ed.mouseUp(0, 0);   // crash piece
    ed.render();
    check(ed.selectedPiece() == 7, "crash piece selected");
    ed.mouseDown((222 + 120 + 1 * (104 + 6) + 40) * s, (80 + 356 + 20) * s, 0, false, false); ed.mouseUp(0, 0);
    check(m.lastNote == 52, "crash pad 2 is the china (note 52)");
    std::snprintf(path, sizeof path, "%s/gui_pads.ppm", dir); ed.render(); savePpm(ed.surface(), path);

    // ---- mutate changes piece parameters
    std::vector<float> before2 = m.v;
    ed.mouseDown(780 * s, 33 * s, 0, false, false); ed.mouseUp(0, 0);
    int diff = 0; for (size_t i = 0; i < m.v.size(); ++i) if (m.v[i] != before2[i]) ++diff;
    check(diff > 20, "mutate perturbs many parameters");

    // ---- groove editor
    ed.mouseDown((222 + 16 + 2 * 108 + 50) * s, (500 + 12 + 14) * s, 0, false, false); ed.mouseUp(0, 0);   // GROOVE tab
    ed.render(); std::snprintf(path, sizeof path, "%s/gui_groove_tab.ppm", dir); savePpm(ed.surface(), path);
    ed.mouseDown((222 + 830) * s, (548 + 128 + 30) * s, 0, false, false); ed.mouseUp(0, 0);
    check(ed.grooveEditorOpen(), "groove editor opens from the Groove tab");
    ed.render();
    ed.mouseDown((48 + 3 * 92 + 40) * s, (84 + 17) * s, 0, false, false); ed.mouseUp(0, 0);   // USER 4
    check(ed.grooveSlot() == 3 && (int)std::lround(m.v[(size_t)t.groove(GR_Pattern)]) == grooveCount() + 3, "slot select also selects the pattern");
    ed.render();
    const float gx = 150, gy = 236;
    ed.mouseDown((gx + 5) * s, (gy + 5) * s, 0, false, false); ed.mouseUp(0, 0);   // kick, step 0
    check(m.bank.p[3].cell[0][0].vel == 100 && m.slotEdits >= 1, "click places a hit and notifies the processor");
    ed.render();
    // drag up on the hit raises its velocity
    ed.mouseDown((gx + 5) * s, (gy + 20) * s, 0, false, false);
    ed.mouseMove((gx + 5) * s, (gy + 2) * s, false);
    ed.mouseUp(0, 0);
    check(m.bank.p[3].cell[0][0].vel > 100, "dragging up raises velocity");
    // click without drag removes
    ed.mouseDown((gx + 5) * s, (gy + 20) * s, 0, false, false); ed.mouseUp(0, 0);
    check(m.bank.p[3].cell[0][0].vel == 0, "click on a hit removes it");
    // paint a row of hits by dragging across the snare lane, right-click ratchet, shift-click probability
    const float cw16 = 40.0f;
    ed.mouseDown((gx + 4 * cw16 + 6 + 5) * s, (gy + 32 + 5) * s, 0, false, false);
    ed.mouseMove((gx + 6 * cw16 + 6 + 5) * s, (gy + 32 + 5) * s, false);
    ed.mouseUp(0, 0);
    check(m.bank.p[3].cell[1][4].vel && m.bank.p[3].cell[1][5].vel && m.bank.p[3].cell[1][6].vel, "dragging paints several hits");
    ed.mouseDown((gx + 4 * cw16 + 6 + 5) * s, (gy + 32 + 5) * s, 1, false, false); ed.mouseUp(0, 0);
    check(m.bank.p[3].cell[1][4].ratchet == 2, "right-click cycles ratchet");
    ed.mouseDown((gx + 5 * cw16 + 6 + 5) * s, (gy + 32 + 5) * s, 0, true, false); ed.mouseUp(0, 0);
    check(m.bank.p[3].cell[1][5].prob == 75, "shift-click cycles probability");
    // copy a factory groove into the slot (Rock Basic), switch to 32 steps and 64 steps (grid must stay clickable)
    ed.mouseDown(561 * s, 195 * s, 0, false, false); ed.mouseUp(0, 0);   // COPY INTO SLOT
    check(m.bank.p[3].cell[0][0].vel > 0 && m.bank.p[3].cell[1][4].vel > 0 && m.bank.p[3].cell[1][4].ratchet == 1, "copy factory groove replaces the slot");
    m.phase = 0.25f;
    ed.render(); std::snprintf(path, sizeof path, "%s/gui_editor16.ppm", dir); savePpm(ed.surface(), path);
    ed.mouseDown((128 + 3 * 56 + 25) * s, (134 + 15) * s, 0, false, false); ed.mouseUp(0, 0);   // 32 steps
    check(m.bank.p[3].steps == 32, "length 32");
    ed.mouseDown((128 + 5 * 56 + 25) * s, (134 + 15) * s, 0, false, false); ed.mouseUp(0, 0);   // 64 steps
    check(m.bank.p[3].steps == 64, "length 64");
    ed.render(); std::snprintf(path, sizeof path, "%s/gui_editor64.ppm", dir); savePpm(ed.surface(), path);
    ed.mouseDown((882) * s, (560) * s, 0, false, false); ed.mouseUp(0, 0);
    check(ed.grooveEditorOpen(), "editor stays open when clicking the grid");
    ed.mouseDown((986 + 40) * s, (38 + 16) * s, 0, false, false); ed.mouseUp(0, 0);
    check(!ed.grooveEditorOpen(), "DONE closes the editor");
    // ---- save / load a kit file
    ed.mouseDown(254 * s, 33 * s, 0, false, false); ed.mouseUp(0, 0);   // SAVE
    check(m.kitFile.rfind("DrumForge kit v1", 0) == 0 && m.kitFile.find("kick.tune=") != std::string::npos && m.kitFile.find("groove.") == std::string::npos, "save writes a kit file without groove settings");
    const float savedTune = m.v[(size_t)t.specific(Kick, KickP::Tune)];
    m.v[(size_t)t.specific(Kick, KickP::Tune)] = savedTune + 20.0f; m.v[(size_t)G_Humanize] = 77;
    const float grooveBefore = m.v[(size_t)t.groove(GR_Swing)] = 33;
    ed.mouseDown(302 * s, 33 * s, 0, false, false); ed.mouseUp(0, 0);   // LOAD
    check(std::fabs(m.v[(size_t)t.specific(Kick, KickP::Tune)] - savedTune) < 0.01f, "load restores the saved kit");
    check(m.v[(size_t)t.groove(GR_Swing)] == grooveBefore, "load leaves groove settings alone");
    check(m.begins == m.ends, "kit load keeps begin/end edits balanced");
    // master tab: morph controls exist
    ed.mouseDown((222 + 16 + 3 * 108 + 50) * s, (500 + 12 + 14) * s, 0, false, false); ed.mouseUp(0, 0);
    ed.render(); std::snprintf(path, sizeof path, "%s/gui_master_tab.ppm", dir); savePpm(ed.surface(), path);
    std::printf(fails ? "GUI TEST FAILED (%d)\n" : "GUI TEST OK\n", fails);
    return fails ? 1 : 0;
}
