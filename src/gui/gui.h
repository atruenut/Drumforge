// DrumForge GUI - platform independent editor logic. Renders into a Canvas; the OS shell only forwards events.
#pragma once
#include <string>
#include <vector>

#include "../engine/grooves.h"
#include "canvas.h"

namespace df { namespace gui {

// What the editor needs from its host (the VST3 controller in the plugin, a mock in tests).
struct IHost {
    virtual ~IHost() {}
    virtual float get(int paramIndex) = 0;                  // plain value
    virtual void begin(int paramIndex) = 0;
    virtual void set(int paramIndex, float plain) = 0;
    virtual void end(int paramIndex) = 0;
    virtual int program() = 0;
    virtual void selectProgram(int index) = 0;
    virtual float meter(int piece) = 0;                     // 0..1 hit activity
    virtual void trigger(int note, int velocity) = 0;       // audition a drum sound (the processor plays it, Note Shift is bypassed)
    virtual UserBank& userBank() = 0;                       // the 8 user groove slots
    virtual void userSlotChanged(int slot) = 0;             // call after editing a slot so it reaches the processor
    virtual float groovePhase() = 0;                        // 0..1 position inside the groove cycle, <0 when idle
    virtual bool saveKitText(const std::string& text) = 0;  // ask the user where to save a kit file (platform dialog) and write it; false if cancelled
    virtual bool loadKitText(std::string& text) = 0;        // ask the user for a kit file and read it; false if cancelled
};

class Editor {
public:
    static constexpr int kW = 1120, kH = 780;               // logical size

    explicit Editor(IHost* host);
    void setScale(float s);                                 // 1.0, 1.25, 1.5, 2.0 ...
    float scale() const { return canvas.scale(); }
    int pixelWidth() const { return canvas.width(); }
    int pixelHeight() const { return canvas.height(); }
    const Canvas& surface() const { return canvas; }

    // returns true when something changed and render() should be called
    bool tick();
    void render();

    // pixel coordinates
    void mouseDown(float x, float y, int button, bool shift, bool doubleClick);
    void mouseMove(float x, float y, bool shift);
    void mouseUp(float x, float y);
    void wheel(float x, float y, float notches, bool shift);
    void keyDown(int key);                                  // 27 = Escape, 37/39 = left/right
    void mouseLeave();

    // test hooks
    int selectedPiece() const { return piece; }
    bool popupOpen() const { return popup; }
    bool grooveEditorOpen() const { return gedit; }
    int grooveSlot() const { return gslot; }
    int tabIndex() const { return tab; }

private:
    enum class W { Knob, Fader, Toggle, Selector, PieceBtn, Tab, PresetPrev, PresetNext, PresetOpen, PresetItem, PieceMute, PieceSolo, Pad, Btn, LanePill };
    enum Cmd { CMD_DONE, CMD_SLOT, CMD_LEN, CMD_CLEAR, CMD_RANDOM, CMD_ROTL, CMD_ROTR, CMD_DOUBLE, CMD_COPY, CMD_SRCPREV, CMD_SRCNEXT,
               CMD_GON, CMD_GSTOP, CMD_RATE, CMD_MUTATE, CMD_OPENEDIT, CMD_LANE, CMD_SAVEKIT, CMD_LOADKIT };
    struct Widget { W type; Rect r; int param = -1; int arg = 0; bool ov = false; };

    IHost* host;
    Canvas canvas;
    std::vector<Widget> widgets;
    int piece = 0, tab = 0;
    bool popup = false;
    int hover = -1, drag = -1;
    float dragStartY = 0, dragStartNorm = 0;
    float flash[10] = {};
    std::vector<float> snap;
    int snapProgram = -1;
    bool dirty = true;
    float mx = 0, my = 0;
    float padFlash[128] = {};
    float snapPhase = -2.f;
    // groove editor
    bool gedit = false; int gslot = 0, gsrc = 0;
    int gmode = 0;                       // 0 idle, 1 painting hits, 2 editing velocity
    int glane = 0, gstep = 0, gstartVel = 0; float gstartY = 0; bool gmoved = false, gchanged = false;
    uint32_t rngState = 0x1234567u;

    void build();
    void knob(Rect cell, int param, const char* labelOverride = nullptr, float size = 26);
    void layoutPieceKnobs(Rect panel);
    void layoutTab(Rect panel);
    void drawWidget(const Widget& w, size_t index);
    int hit(float lx, float ly) const;
    void applyNorm(int param, float norm, bool bracket);
    void layoutPads(Rect panel);
    void layoutEditor();
    void drawEditor();
    void drawPatternPreview(float x, float y, float w, float h);
    void command(int cmd, int arg, int button, bool shift);
    void mutate(float amount);
    void hitPad(int note, float ly, const Rect& r, bool shift);
    bool cellAt(float lx, float ly, int& lane, int& step);
    void setCell(int lane, int step, int vel);
    void editorMouseDown(float lx, float ly, int button, bool shift);
    float rnd01() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return (float)(rngState & 0xFFFFFF) / 16777216.0f; }
    bool ovMode() const { return gedit; }
    float norm(int param);
    std::string valueText(int param);
};

}}  // namespace df::gui
