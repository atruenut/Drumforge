// DrumForge - parameter table shared by the DSP engine, the VST3 wrapper, the GUI and the presets.
// The table is built once, deterministically; a parameter's index is its stable ID.
// New parameters may only be appended at the END of the table (state compatibility).
#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace df {

enum Piece { Kick, Snare, Rim, TomF, TomM, TomH, Hat, Crash, Ride, Perc, NumPieces };
inline const char* pieceName(int p)
{
    static const char* n[] = {"Kick", "Snare", "Sidestick", "Floor Tom", "Mid Tom", "High Tom",
                              "Hi-Hat", "Crash", "Ride", "Perc"};
    return n[p];
}
inline const char* pieceKey(int p)
{
    static const char* n[] = {"kick", "snare", "rim", "tomf", "tomm", "tomh", "hat", "crash", "ride", "perc"};
    return n[p];
}

enum class PT : unsigned char { Lin, Log, Db, Choice, Bool };

struct ParamDef {
    std::string key, title, label, group, units;
    float minV = 0, maxV = 1, defV = 0;
    PT type = PT::Lin;
    int steps = 0;                    // stepCount for VST3 (0 = continuous)
    std::vector<std::string> choices;
    int piece = -1;                   // -1: global
};

// ---- common strip params present on every piece
enum Common { C_Level, C_Pan, C_Tone, C_Drive, C_Dyn, C_Room, C_Mute, C_Solo, NumCommon };

// ---- piece specific parameter lists (X-macros: id, label, units, min, max, default, type)
#define KICK_P(X)                                                                   \
    X(Tune, "Tune", "Hz", 30, 120, 52, Log)                                         \
    X(Sweep, "Sweep", "oct", 0, 4, 1.8, Lin)                                        \
    X(SweepTime, "Sweep Time", "ms", 2, 120, 28, Log)                               \
    X(Decay, "Decay", "ms", 60, 2000, 420, Log)                                     \
    X(Punch, "Punch", "%", 0, 100, 40, Lin)                                         \
    X(Click, "Click", "%", 0, 100, 45, Lin)                                         \
    X(ClickTone, "Click Tone", "Hz", 800, 9000, 3500, Log)                          \
    X(Sub, "Sub", "%", 0, 100, 35, Lin)                                             \
    X(Damp, "Damp", "Hz", 300, 20000, 14000, Log)

#define SNARE_P(X)                                                                  \
    X(Tune, "Tune", "Hz", 100, 400, 195, Log)                                       \
    X(Tension, "Tension", "%", 0, 100, 30, Lin)                                     \
    X(BodyDecay, "Body Decay", "ms", 40, 800, 160, Log)                             \
    X(Body, "Body", "%", 0, 100, 70, Lin)                                           \
    X(Wires, "Wires", "%", 0, 100, 65, Lin)                                         \
    X(WireDecay, "Wire Decay", "ms", 60, 900, 260, Log)                             \
    X(WireTone, "Wire Tone", "Hz", 1500, 9000, 4200, Log)                           \
    X(Snap, "Snap", "%", 0, 100, 45, Lin)                                           \
    X(Shell, "Shell", "%", 0, 100, 40, Lin)                                         \
    X(RimShot, "Rimshot", "%", 0, 100, 15, Lin)                                     \
    X(Damp, "Damp", "Hz", 500, 20000, 16000, Log)

#define RIM_P(X)                                                                    \
    X(Tune, "Tune", "Hz", 300, 3000, 1100, Log)                                     \
    X(Decay, "Decay", "ms", 10, 300, 70, Log)                                       \
    X(Tone, "Tone", "%", 0, 100, 50, Lin)                                           \
    X(Click, "Click", "%", 0, 100, 50, Lin)

#define TOM_P(X, TUNE, DECAY)                                                       \
    X(Tune, "Tune", "Hz", 50, 500, TUNE, Log)                                       \
    X(Decay, "Decay", "ms", 80, 2500, DECAY, Log)                                   \
    X(Drop, "Pitch Drop", "%", 0, 100, 35, Lin)                                     \
    X(DropTime, "Drop Time", "ms", 5, 150, 40, Log)                                 \
    X(Attack, "Attack", "%", 0, 100, 50, Lin)                                       \
    X(Tone, "Tone", "%", 0, 100, 50, Lin)                                           \
    X(Reso, "Reso Head", "%", 0, 100, 40, Lin)                                      \
    X(Damp, "Damp", "Hz", 400, 20000, 12000, Log)

#define TOMF_P(X) TOM_P(X, 92, 650)
#define TOMM_P(X) TOM_P(X, 128, 520)
#define TOMH_P(X) TOM_P(X, 175, 420)

#define HAT_P(X)                                                                    \
    X(Tune, "Tune", "st", -12, 12, 0, Lin)                                          \
    X(ClosedDecay, "Closed Decay", "ms", 15, 400, 55, Log)                          \
    X(OpenDecay, "Open Decay", "ms", 150, 3000, 700, Log)                           \
    X(Bright, "Bright", "Hz", 3000, 18000, 11000, Log)                              \
    X(Metal, "Metal", "%", 0, 100, 60, Lin)                                         \
    X(Char, "808 Blend", "%", 0, 100, 25, Lin)                                      \
    X(Tick, "Tick", "%", 0, 100, 40, Lin)                                           \
    X(Ring, "Ring", "%", 0, 100, 50, Lin)

#define CRASH_P(X)                                                                  \
    X(Tune, "Tune", "st", -12, 12, 0, Lin)                                          \
    X(Decay, "Decay", "ms", 300, 6000, 2200, Log)                                   \
    X(Bloom, "Bloom", "ms", 5, 300, 60, Log)                                        \
    X(Bright, "Bright", "Hz", 3000, 18000, 12000, Log)                              \
    X(Wash, "Wash", "%", 0, 100, 40, Lin)                                           \
    X(Tick, "Tick", "%", 0, 100, 35, Lin)                                           \
    X(Stereo, "Stereo", "%", 0, 100, 60, Lin)                                       \
    X(Trash, "Trash", "%", 0, 100, 15, Lin)

#define RIDE_P(X)                                                                   \
    X(Tune, "Tune", "st", -12, 12, 0, Lin)                                          \
    X(Decay, "Decay", "ms", 400, 6000, 2800, Log)                                   \
    X(Ping, "Ping", "%", 0, 100, 55, Lin)                                           \
    X(Bell, "Bell", "%", 0, 100, 35, Lin)                                           \
    X(Wash, "Wash", "%", 0, 100, 25, Lin)                                           \
    X(Bright, "Bright", "Hz", 3000, 18000, 10000, Log)                              \
    X(Stereo, "Stereo", "%", 0, 100, 40, Lin)                                       \
    X(Dirt, "Dirt", "%", 0, 100, 10, Lin)

#define PERC_P(X)                                                                   \
    X(Tune, "Tune", "st", -12, 12, 0, Lin)                                          \
    X(Decay, "Decay", "ms", 30, 1500, 150, Log)                                     \
    X(Tone, "Tone", "%", 0, 100, 50, Lin)                                           \
    X(Spread, "Spread", "%", 0, 100, 50, Lin)

#define DF_ENUM(id, label, units, mn, mx, df, ty) id,
namespace KickP  { enum { KICK_P(DF_ENUM) Count }; }
namespace SnareP { enum { SNARE_P(DF_ENUM) Count }; }
namespace RimP   { enum { RIM_P(DF_ENUM) Count }; }
namespace TomP   { enum { TOMF_P(DF_ENUM) Count }; }
namespace HatP   { enum { HAT_P(DF_ENUM) Count }; }
namespace CrashP { enum { CRASH_P(DF_ENUM) Count }; }
namespace RideP  { enum { RIDE_P(DF_ENUM) Count }; }
namespace PercP  { enum { PERC_P(DF_ENUM) Count }; }
#undef DF_ENUM

// ---- global parameters
enum GlobalIdx {
    G_Master, G_Humanize, G_VelCurve, G_Width, G_Tune, G_NoteOffset,
    G_EqLo, G_EqMid, G_EqMidF, G_EqHi,
    G_CompThr, G_CompRatio, G_CompAtt, G_CompRel, G_CompMakeup, G_CompMix,
    G_SatDrive, G_SatMix,
    G_RoomMix, G_RoomType, G_RoomSize, G_RoomDecay, G_RoomDamp, G_RoomPre, G_RoomGate, G_RoomWidth,
    G_LimOn, G_LimCeil,
    NumGlobal
};
// ---- groove (built-in pattern player) parameters, appended after the pieces
enum GrooveIdx { GR_On, GR_Pattern, GR_Swing, GR_Intensity, GR_Rate, GR_Human, GR_Fill, GR_Stopped, NumGroove };
// ---- v1.1 extras, appended after the groove block (never reorder)
enum ExtIdx {
    E_GrLaid, E_GrCrash, E_GrGuard,                                           // groove: laid-back backbeat, crash accent frequency, no crash on loop restart
    E_GrMuteKick, E_GrMuteSnare, E_GrMuteHat, E_GrMuteTom, E_GrMuteCym, E_GrMutePerc,   // groove lane mutes
    E_Buzz,                                                                   // snare sympathetic buzz
    E_MorphAmt, E_MorphTo,                                                    // kit morph
    E_PieceFx,                                                                // + piece*2 + {0 attack, 1 sustain}: transient shaper
    NumExt = E_PieceFx + NumPieces * 2
};
constexpr int kUserSlots = 8;     // user groove slots; groove.pattern values 28..35 select them

struct ParamTable {
    std::vector<ParamDef> defs;
    int pieceBase[NumPieces] = {};
    int pieceCount[NumPieces] = {};
    int grooveBase = 0, extBase = 0;
    std::vector<char> session;       // 1 = belongs to the user's session (groove, morph); presets never overwrite these
    std::unordered_map<std::string, int> byKey;

    int size() const { return (int)defs.size(); }
    int global(int g) const { return g; }
    int piece(int p, int k) const { return pieceBase[p] + k; }
    int common(int p, int c) const { return pieceBase[p] + c; }
    int specific(int p, int k) const { return pieceBase[p] + NumCommon + k; }
    int groove(int g) const { return grooveBase + g; }
    int ext(int e) const { return extBase + e; }
    int pieceFx(int p, int k) const { return extBase + E_PieceFx + p * 2 + k; }
    bool isSession(int i) const { return session[(size_t)i] != 0; }
    int find(const std::string& keyIn) const
    {
        std::string key = keyIn;
        for (auto& ch : key) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        auto it = byKey.find(key);
        return it == byKey.end() ? -1 : it->second;
    }

    // ---- value mapping
    float toNorm(int i, float v) const
    {
        const ParamDef& d = defs[(size_t)i];
        float r;
        switch (d.type) {
            case PT::Log: r = std::log(std::max(v, d.minV) / d.minV) / std::log(d.maxV / d.minV); break;
            case PT::Choice: case PT::Bool: r = d.steps > 0 ? v / (float)d.steps : 0.f; break;
            default: r = (v - d.minV) / (d.maxV - d.minV); break;
        }
        return r < 0.f ? 0.f : (r > 1.f ? 1.f : r);
    }
    float toPlain(int i, float n) const
    {
        const ParamDef& d = defs[(size_t)i];
        n = n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
        switch (d.type) {
            case PT::Log: return d.minV * std::pow(d.maxV / d.minV, n);
            case PT::Choice: case PT::Bool: return std::floor(n * (float)d.steps + 0.5f);
            default: {
                float v = d.minV + n * (d.maxV - d.minV);
                if (d.steps > 0) {
                    float st = (d.maxV - d.minV) / (float)d.steps;
                    v = d.minV + std::floor((v - d.minV) / st + 0.5f) * st;
                }
                return v;
            }
        }
    }
    float defaultNorm(int i) const { return toNorm(i, defs[(size_t)i].defV); }

    std::string format(int i, float plain) const
    {
        const ParamDef& d = defs[(size_t)i];
        char b[64];
        if (d.type == PT::Choice || d.type == PT::Bool) {
            int k = (int)(plain + 0.5f);
            if (k < 0) k = 0;
            if (k >= (int)d.choices.size()) k = (int)d.choices.size() - 1;
            return d.choices[(size_t)k];
        }
        const std::string& u = d.units;
        if (u == "Hz") {
            if (plain >= 1000.f) std::snprintf(b, sizeof b, "%.2f kHz", plain / 1000.f);
            else std::snprintf(b, sizeof b, "%.0f Hz", plain);
        } else if (u == "ms") {
            if (d.key == "room.gate" && plain < 1.f) std::snprintf(b, sizeof b, "Off");
            else if (plain >= 1000.f) std::snprintf(b, sizeof b, "%.2f s", plain / 1000.f);
            else if (plain < 10.f) std::snprintf(b, sizeof b, "%.1f ms", plain);
            else std::snprintf(b, sizeof b, "%.0f ms", plain);
        } else if (u == "s") std::snprintf(b, sizeof b, "%.2f s", plain);
        else if (u == "dB") {
            if (d.type == PT::Db && plain <= d.minV + 0.01f) std::snprintf(b, sizeof b, "-inf");
            else std::snprintf(b, sizeof b, "%+.1f dB", plain);
        } else if (u == "pan") {
            int p = (int)std::lround(plain);
            if (p == 0) std::snprintf(b, sizeof b, "C");
            else std::snprintf(b, sizeof b, p < 0 ? "L%d" : "R%d", p < 0 ? -p : p);
        } else if (u == "st") std::snprintf(b, sizeof b, "%+.1f st", plain);
        else if (u == "oct") std::snprintf(b, sizeof b, "%.2f oct", plain);
        else if (u == "%") std::snprintf(b, sizeof b, "%.0f %%", plain);
        else if (u == ":1") std::snprintf(b, sizeof b, "%.1f:1", plain);
        else if (u == "int") std::snprintf(b, sizeof b, "%+d", (int)std::lround(plain));
        else std::snprintf(b, sizeof b, "%.2f %s", plain, u.c_str());
        return b;
    }
    // parse user text ("120", "1.5k", "-3 dB") back to a plain value; returns false on failure
    bool parse(int i, const std::string& s, float& out) const
    {
        const ParamDef& d = defs[(size_t)i];
        if (d.type == PT::Choice || d.type == PT::Bool) {
            for (size_t k = 0; k < d.choices.size(); ++k)
                if (d.choices[k] == s) { out = (float)k; return true; }
            return false;
        }
        if (s == "Off" || s == "off" || s == "-inf") { out = d.minV; return true; }
        if (d.units == "pan") {
            if (s == "C" || s == "c") { out = 0; return true; }
            if ((s[0] == 'L' || s[0] == 'l' || s[0] == 'R' || s[0] == 'r') && s.size() > 1) {
                double m = std::strtod(s.c_str() + 1, nullptr);
                out = (float)((s[0] == 'L' || s[0] == 'l') ? -m : m);
                return true;
            }
        }
        char* end = nullptr;
        double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        while (*end == ' ') ++end;
        if (*end == 'k' || *end == 'K') v *= 1000.0;
        else if ((*end == 's' || *end == 'S') && d.units == "ms") v *= 1000.0;
        out = (float)std::min<double>(std::max<double>(v, d.minV), d.maxV);
        return true;
    }
};

const ParamTable& paramTable();   // defined in params.cpp

}  // namespace df
