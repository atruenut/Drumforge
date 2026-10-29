#include "params.h"
#include "grooves.h"
#include "presets.h"

#include <cctype>
#include <mutex>

namespace df {

namespace {

struct Builder {
    ParamTable& t;
    explicit Builder(ParamTable& tt) : t(tt) {}

    int add(const std::string& key, const std::string& title, const std::string& group, const std::string& units,
            float mn, float mx, float df_, PT type, int steps = 0, int piece = -1,
            std::vector<std::string> choices = {})
    {
        ParamDef d;
        d.key = key;
        for (auto& ch : d.key) ch = (char)std::tolower((unsigned char)ch); d.title = title; d.label = title; d.group = group; d.units = units;
        d.minV = mn; d.maxV = mx; d.defV = df_; d.type = type; d.steps = steps;
        d.piece = piece; d.choices = std::move(choices);
        if (type == PT::Bool) { d.steps = 1; d.choices = {"Off", "On"}; }
        if (type == PT::Choice) d.steps = (int)d.choices.size() - 1;
        int idx = (int)t.defs.size();
        t.byKey[d.key] = idx;
        t.defs.push_back(std::move(d));
        return idx;
    }
};

void build(ParamTable& t)
{
    Builder b(t);

    // ---- globals (indices must match GlobalIdx)
    b.add("master", "Master Volume", "Master", "dB", -60, 6, 0, PT::Db);
    b.add("humanize", "Humanize", "Master", "%", 0, 100, 25, PT::Lin);
    b.add("velcurve", "Vel Curve", "Master", "%", -100, 100, 0, PT::Lin);
    b.add("width", "Width", "Master", "%", 0, 150, 100, PT::Lin);
    b.add("tune", "Master Tune", "Master", "st", -12, 12, 0, PT::Lin);
    b.add("noteoffset", "Note Shift", "Master", "int", -12, 12, 0, PT::Lin, 24);

    b.add("eq.lo", "Low", "EQ", "dB", -12, 12, 0, PT::Lin);
    b.add("eq.mid", "Mid", "EQ", "dB", -12, 12, 0, PT::Lin);
    b.add("eq.midf", "Mid Freq", "EQ", "Hz", 200, 6000, 1000, PT::Log);
    b.add("eq.hi", "High", "EQ", "dB", -12, 12, 0, PT::Lin);

    b.add("comp.thr", "Threshold", "Bus Comp", "dB", -40, 0, -12, PT::Lin);
    b.add("comp.ratio", "Ratio", "Bus Comp", ":1", 1, 20, 3, PT::Log);
    b.add("comp.att", "Attack", "Bus Comp", "ms", 0.1f, 80, 12, PT::Log);
    b.add("comp.rel", "Release", "Bus Comp", "ms", 20, 800, 150, PT::Log);
    b.add("comp.makeup", "Makeup", "Bus Comp", "dB", 0, 18, 0, PT::Lin);
    b.add("comp.mix", "Mix", "Bus Comp", "%", 0, 100, 0, PT::Lin);

    b.add("sat.drive", "Drive", "Saturator", "%", 0, 100, 0, PT::Lin);
    b.add("sat.mix", "Mix", "Saturator", "%", 0, 100, 100, PT::Lin);

    b.add("room.mix", "Room Mix", "Room", "%", 0, 100, 18, PT::Lin);
    b.add("room.type", "Room Type", "Room", "", 0, 5, 0, PT::Choice, 0, -1,
          {"Studio", "Tight Booth", "Live Hall", "Cathedral", "Gated", "Plate"});
    b.add("room.size", "Size", "Room", "%", 0, 100, 50, PT::Lin);
    b.add("room.decay", "Decay", "Room", "ms", 100, 8000, 1200, PT::Log);
    b.add("room.damp", "Damping", "Room", "Hz", 1000, 20000, 7000, PT::Log);
    b.add("room.pre", "Pre-Delay", "Room", "ms", 0, 120, 8, PT::Lin);
    b.add("room.gate", "Gate Time", "Room", "ms", 0, 600, 0, PT::Lin);
    b.add("room.width", "Room Width", "Room", "%", 0, 100, 100, PT::Lin);

    b.add("lim.on", "Limiter", "Master", "", 0, 1, 1, PT::Bool);
    b.add("lim.ceil", "Ceiling", "Master", "dB", -12, 0, -0.3f, PT::Lin);

    // ---- pieces
    static const char* commonLabel[NumCommon] = {"Level", "Pan", "Tone", "Drive", "Dynamics", "Room Send", "Mute", "Solo"};
    static const float defPan[NumPieces] = {0, 0, -4, 28, 4, -26, -14, -34, 36, 18};
    static const float defLevel[NumPieces] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int p = 0; p < NumPieces; ++p) {
        t.pieceBase[p] = t.size();
        std::string k = pieceKey(p), g = pieceName(p);
        b.add(k + ".level", "Level", g, "dB", -60, 12, defLevel[p], PT::Db, 0, p);
        b.add(k + ".pan", "Pan", g, "pan", -100, 100, defPan[p], PT::Lin, 200, p);
        b.add(k + ".tilt", "Tilt", g, "dB", -12, 12, 0, PT::Lin, 0, p);
        b.add(k + ".drive", "Drive", g, "%", 0, 100, 0, PT::Lin, 0, p);
        b.add(k + ".dyn", "Dynamics", g, "%", 0, 100, 70, PT::Lin, 0, p);
        b.add(k + ".room", "Room Send", g, "%", 0, 100, (p == Kick) ? 12.f : 30.f, PT::Lin, 0, p);
        b.add(k + ".mute", "Mute", g, "", 0, 1, 0, PT::Bool, 0, p);
        b.add(k + ".solo", "Solo", g, "", 0, 1, 0, PT::Bool, 0, p);
        (void)commonLabel;
        int n = NumCommon;
#define DF_ADD(id, label, units, mn, mx, dv, ty)                                                  \
    b.add(k + "." #id, label, g, units, (float)(mn), (float)(mx), (float)(dv), PT::ty, 0, p);     \
    ++n;
        switch (p) {
            case Kick: KICK_P(DF_ADD) break;
            case Snare: SNARE_P(DF_ADD) break;
            case Rim: RIM_P(DF_ADD) break;
            case TomF: TOMF_P(DF_ADD) break;
            case TomM: TOMM_P(DF_ADD) break;
            case TomH: TOMH_P(DF_ADD) break;
            case Hat: HAT_P(DF_ADD) break;
            case Crash: CRASH_P(DF_ADD) break;
            case Ride: RIDE_P(DF_ADD) break;
            case Perc: PERC_P(DF_ADD) break;
        }
#undef DF_ADD
        t.pieceCount[p] = n;
    }

    // ---- groove
    t.grooveBase = t.size();
    b.add("groove.on", "Groove Player", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.pattern", "Pattern", "Groove", "int", 0, (float)(grooveCount() + kUserSlots - 1), 0, PT::Lin, grooveCount() + kUserSlots - 1);
    b.add("groove.swing", "Swing", "Groove", "%", 0, 100, 0, PT::Lin);
    b.add("groove.intensity", "Intensity", "Groove", "%", 0, 100, 70, PT::Lin);
    b.add("groove.rate", "Rate", "Groove", "", 0, 2, 0, PT::Choice, 0, -1, {"1/16", "1/8", "1/32"});
    b.add("groove.human", "Groove Human", "Groove", "%", 0, 100, 10, PT::Lin);
    b.add("groove.fill", "Fill Every", "Groove", "", 0, 4, 0, PT::Choice, 0, -1,
          {"Off", "Every cycle", "2 cycles", "4 cycles", "8 cycles"});
    b.add("groove.stopped", "Play When Stopped", "Groove", "", 0, 1, 0, PT::Bool);

    // ---- v1.1 extras (appended)
    t.extBase = t.size();
    b.add("groove.laid", "Laid-Back", "Groove", "%", -40, 40, 0, PT::Lin);
    b.add("groove.crash", "Crash Accent (retired)", "Groove", "", 0, 4, 0, PT::Choice, 0, -1, {"Off", "Every 8 bars", "Every 4 bars", "Every 2 bars", "Every bar"});
    b.add("groove.guard", "Loop Guard (always on)", "Groove", "", 0, 1, 1, PT::Bool);
    b.add("groove.mute.kick", "Mute Kick Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.mute.snare", "Mute Snare Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.mute.hat", "Mute Hat Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.mute.tom", "Mute Tom Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.mute.cym", "Mute Cymbal Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("groove.mute.perc", "Mute Perc Lane", "Groove", "", 0, 1, 0, PT::Bool);
    b.add("snare.buzz", "Snare Buzz", "Master", "%", 0, 100, 0, PT::Lin);
    b.add("morph.amount", "Morph", "Morph", "%", 0, 100, 0, PT::Lin);
    {
        std::vector<std::string> names;
        for (int i = 0; i < presetCount(); ++i) names.push_back(preset(i).name);
        b.add("morph.to", "Morph To", "Morph", "", 0, (float)(names.size() - 1), 0, PT::Choice, 0, -1, names);
    }
    for (int p = 0; p < NumPieces; ++p) {
        std::string k = pieceKey(p), g = pieceName(p);
        b.add(k + ".tattack", "Trans Attack", g, "%", -100, 100, 0, PT::Lin, 0, p);
        b.add(k + ".tsustain", "Trans Sustain", g, "%", -100, 100, 0, PT::Lin, 0, p);
    }
    t.session.assign((size_t)t.size(), 0);
    for (int i = 0; i < t.size(); ++i) {
        const std::string& key = t.defs[(size_t)i].key;
        if (key.rfind("groove.", 0) == 0 || key.rfind("morph.", 0) == 0) t.session[(size_t)i] = 1;
    }
}

}  // namespace

const ParamTable& paramTable()
{
    static ParamTable t;
    static std::once_flag once;
    std::call_once(once, [] { build(t); });
    return t;
}

}  // namespace df
