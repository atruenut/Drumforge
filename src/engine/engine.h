// DrumForge - the complete drum engine (no VST3 dependencies; usable from any host or the offline renderer)
#pragma once
#include <vector>

#include "fx.h"
#include "params.h"
#include "voices.h"

namespace df {

struct NoteInfo { int piece = -1; float semis = 0; float decay = 1; int artic = 0; };
const NoteInfo& noteInfo(int midiNote);
const char* noteLabel(int midiNote);   // e.g. "Kick", "Open Hat" or "" if unmapped

class DrumEngine {
public:
    static constexpr int kStems = 7;   // kick, snare, toms, hats, cymbals, perc, room
    DrumEngine();

    void init(float sampleRate);
    void reset();                         // silence everything, keep parameters

    // ---- parameters (plain values, see ParamTable)
    void setParam(int idx, float plain);
    float getParam(int idx) const { return vals[(size_t)idx]; }   // the stored (un-morphed) value
    void setAll(const float* plainValues);
    const std::vector<float>& values() const { return vals; }

    // ---- events
    void noteOn(int note, int velocity127, bool applyShift = true);   // applyShift=false for the groove player and the GUI test pads
    void chokeCymbals(float fadeMs = 35.0f);   // soft-fades ringing crashes / rides (used when the host transport jumps)
    void allSoundOff();

    // ---- audio. Any output pointer may be null. Stems are pre-master-FX, delayed to line up with `main`.
    void process(float* mainL, float* mainR, float* const* stemL, float* const* stemR, int n);

    int latencySamples() const { return lim.latency(); }
    float sampleRate() const { return fs; }
    // peak-ish activity of each piece's last block, 0..1 (for meters / GUI)
    float activity(int piece) const { return act[piece]; }

private:
    struct Strip {
        Biquad lo[2], hi[2];
        float toneDb = 1e9f;
        Smooth gain, panS;
        float ef = 0, es = 0, tAtt = 0, tSus = 0;   // transient shaper state / amounts
        float drive = 0, dGain = 1, dComp = 1;
        float sendTarget = 0;
        Smooth send;
        bool mute = false;
    };

    float fs = 48000;
    std::vector<float> vals;           // what the user set
    std::vector<float> eff, morphTarget;   // what the voices read: vals blended toward the morph target preset
    bool effDirty = true; int morphIdx = -1;
    const ParamTable* tbl = nullptr;
    Rng rng;

    KickVoice kick[3];
    SnareVoice snare[4];
    RimVoice rim[3];
    TomVoice tom[3][4];
    HatVoice hat[2]; int hatCur = 0;
    CymbalVoice crash[4], ride[4];
    PercVoice perc[6];
    uint32_t order = 0;

    Strip strip[NumPieces];
    Room room;
    MasterEq eq; BusComp comp; Saturator sat; Limiter lim;
    Smooth masterGain, widthS;
    DcBlock dcL, dcR;
    Biquad buzzLp, buzzBp1, buzzBp2; float buzzEnv = 0, buzzAmt = 0, kEfA = 0, kEfR = 0, kEsA = 0, kEsR = 0;
    DelayLine stemDelay[kStems][2];
    float act[NumPieces] = {};
    bool anySolo = false;

    float sp(int piece, int k) const { return eff[(size_t)tbl->specific(piece, k)]; }
    float cm(int piece, int c) const { return eff[(size_t)tbl->common(piece, c)]; }
    float gl(int g) const { return eff[(size_t)g]; }
    float ex(int e) const { return eff[(size_t)tbl->ext(e)]; }
    const float* spPtr(int piece) const { return &eff[(size_t)tbl->specific(piece, 0)]; }
    void rebuildEffective();

    void updateBlockParams();
    HitCtx makeCtx(int piece, float vel01, const NoteInfo& ni);
    template <class V, int N> V* alloc(V (&arr)[N]);
};

}  // namespace df
