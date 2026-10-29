#include "controller.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "../engine/presets.h"
#include "bankio.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace df {

static std::u16string widen(const std::string& s)
{
    std::u16string r;
    for (unsigned char c : s) r.push_back((char16_t)c);
    return r;
}
static void copyToString128(const std::string& s, String128 out)
{
    size_t i = 0;
    for (; i < s.size() && i < 127; ++i) out[i] = (TChar)(unsigned char)s[i];
    out[i] = 0;
}

// A parameter with the engine's own value mapping (log scales, dB, choices, units).
class DfParam : public Parameter {
public:
    DfParam(const ParameterInfo& info, int idx) : Parameter(info), index(idx) {}
    void toString(ParamValue v, String128 string) const SMTG_OVERRIDE
    {
        const ParamTable& t = paramTable();
        copyToString128(t.format(index, t.toPlain(index, (float)v)), string);
    }
    bool fromString(const TChar* string, ParamValue& v) const SMTG_OVERRIDE
    {
        std::string s;
        for (const TChar* p = string; *p; ++p) s.push_back((char)(*p < 128 ? *p : '?'));
        const ParamTable& t = paramTable();
        float plain;
        if (!t.parse(index, s, plain)) return false;
        v = t.toNorm(index, plain);
        return true;
    }
    ParamValue toPlain(ParamValue v) const SMTG_OVERRIDE { return paramTable().toPlain(index, (float)v); }
    ParamValue toNormalized(ParamValue p) const SMTG_OVERRIDE { return paramTable().toNorm(index, (float)p); }
private:
    int index;
};

tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
    tresult r = EditControllerEx1::initialize(context);
    if (r != kResultOk) return r;
    const ParamTable& t = paramTable();

    // root unit owns the preset list; every parameter group becomes a unit so hosts show a tidy tree
    addUnit(new Unit((const TChar*)u"Root", kRootUnitId, kNoParentUnitId, 1));
    auto* prg = new ProgramList((const TChar*)u"Presets", 1, kRootUnitId);
    addProgramList(prg);
    for (int i = 0; i < presetCount(); ++i) {
        const Preset& p = preset(i);
        std::u16string nm = widen(std::string(p.category) + ": " + p.name);
        prg->addProgram((const TChar*)nm.c_str());
    }
    Parameter* pp = prg->getParameter();
    pp->getInfo().id = kProgramParamId;
    pp->getInfo().flags &= ~ParameterInfo::kCanAutomate;
    parameters.addParameter(pp);

    std::map<std::string, UnitID> units;
    UnitID nextUnit = 2;
    for (int i = 0; i < t.size(); ++i) {
        const ParamDef& d = t.defs[(size_t)i];
        auto it = units.find(d.group);
        if (it == units.end()) {
            UnitID id = nextUnit++;
            std::u16string nm = widen(d.group);
            addUnit(new Unit((const TChar*)nm.c_str(), id, kRootUnitId));
            it = units.emplace(d.group, id).first;
        }
        ParameterInfo info{};
        info.id = (ParamID)i;
        std::u16string title = widen(d.title), group = widen(d.group);
        const bool prefixed = d.title.rfind(d.group, 0) == 0;   // e.g. "Room Mix" already starts with its group
        std::u16string full = (prefixed ? std::u16string() : group + u" ") + title;
        UString(info.title, 128).assign((const TChar*)full.c_str());
        UString(info.shortTitle, 128).assign((const TChar*)title.c_str());
        UString(info.units, 128).fromAscii(d.type == PT::Choice || d.type == PT::Bool ? "" : d.units.c_str());
        info.stepCount = (d.type == PT::Choice || d.type == PT::Bool || d.steps > 0) ? d.steps : 0;
        info.defaultNormalizedValue = t.toNorm(i, d.defV);
        info.unitId = it->second;
        info.flags = ParameterInfo::kCanAutomate;
        if (d.key == "groove.guard" || d.key == "groove.crash") info.flags |= ParameterInfo::kIsHidden;   // retired switch, the guard is always on
        if (d.type == PT::Choice) info.flags |= ParameterInfo::kIsList;
        parameters.addParameter(new DfParam(info, i));
    }
    // hidden, read-only hit meters (processor -> GUI)
    for (int p = 0; p < NumPieces; ++p) {
        ParameterInfo info{};
        info.id = kMeterParamBase + (ParamID)p;
        std::u16string nm = widen(std::string(pieceName(p)) + " Meter");
        UString(info.title, 128).assign((const TChar*)nm.c_str());
        info.flags = ParameterInfo::kIsReadOnly | ParameterInfo::kIsHidden;
        info.unitId = kRootUnitId;
        parameters.addParameter(new Parameter(info));
    }
    // hidden helper parameters: GUI -> processor test pad trigger, processor -> GUI groove playhead
    {
        ParameterInfo info{};
        info.id = kTriggerParamId;
        UString(info.title, 128).assign((const TChar*)u"Test Pad");
        info.flags = ParameterInfo::kIsHidden;
        info.unitId = kRootUnitId;
        parameters.addParameter(new Parameter(info));
        ParameterInfo ph{};
        ph.id = kPlayheadParamId;
        UString(ph.title, 128).assign((const TChar*)u"Groove Position");
        ph.flags = ParameterInfo::kIsReadOnly | ParameterInfo::kIsHidden;
        ph.unitId = kRootUnitId;
        ph.defaultNormalizedValue = 1.0;
        parameters.addParameter(new Parameter(ph));
        setParamNormalized(kPlayheadParamId, 1.0);
    }
    for (int i = 0; i < kUserSlots; ++i) bank.p[i].clear();
    return kResultOk;
}

tresult PLUGIN_API Controller::getMidiControllerAssignment(int32 bus, int16 channel, CtrlNumber number, ParamID& id)
{
    (void)channel;
    if (bus != 0) return kResultFalse;
    if (number == ControllerNumbers::kCtrlModWheel) { id = (ParamID)paramTable().ext(E_MorphAmt); return kResultTrue; }   // mod wheel morphs the kit
    return kResultFalse;
}

void Controller::trigger(int note, int velocity)
{
    trigSeq = (trigSeq + 1) & 15u;
    const int code = (int)(trigSeq << 14) | ((note & 127) << 7) | (velocity & 127);
    const ParamValue n = (double)code / 262143.0;
    beginEdit(kTriggerParamId);
    performEdit(kTriggerParamId, n);
    endEdit(kTriggerParamId);
}
float Controller::groovePhase() const
{
    double v = const_cast<Controller*>(this)->getParamNormalized(kPlayheadParamId);
    return v >= 0.9999 ? -1.f : (float)v;
}
void Controller::userSlotChanged(int slot)
{
    if (slot < 0 || slot >= kUserSlots) return;
    if (IMessage* msg = allocateMessage()) {
        std::vector<uint8_t> blob;
        bank.serializeSlot(slot, blob);
        msg->setMessageID("df.slot");
        msg->getAttributes()->setInt("slot", slot);
        msg->getAttributes()->setBinary("data", blob.data(), (uint32)blob.size());
        sendMessage(msg);
        msg->release();
    }
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    int32 magic = 0, ver = 0, count = 0;
    if (!s.readInt32(magic) || magic != kStateMagic || !s.readInt32(ver) || !s.readInt32(count)) return kResultFalse;
    const ParamTable& t = paramTable();
    for (int32 i = 0; i < count; ++i) {
        float f = 0;
        if (!s.readFloat(f)) return kResultFalse;
        if (i < t.size()) EditControllerEx1::setParamNormalized((ParamID)i, t.toNorm(i, f));
    }
    int32 prog = 0;
    if (s.readInt32(prog) && presetCount() > 1)
        EditControllerEx1::setParamNormalized(kProgramParamId, (double)prog / (double)(presetCount() - 1));
    UserBank nb = bank;
    if (readBank(s, nb)) bank = nb;
    return kResultOk;
}

tresult PLUGIN_API Controller::setParamNormalized(ParamID tag, ParamValue value)
{
    tresult res = EditControllerEx1::setParamNormalized(tag, value);
    if (res == kResultOk && tag == kProgramParamId) {
        // a preset was chosen: mirror what the processor does so the GUI and the host's parameter list follow
        const int idx = std::max(0, std::min(presetCount() - 1, (int)std::lround(value * (double)std::max(1, presetCount() - 1))));
        const ParamTable& t = paramTable();
        std::vector<float> v((size_t)t.size());
        presetApply(idx, v.data());
        for (int i = 0; i < t.size(); ++i)
            if (t.isSession(i)) v[(size_t)i] = plain(i);
        for (int i = 0; i < t.size(); ++i) EditControllerEx1::setParamNormalized((ParamID)i, t.toNorm(i, v[(size_t)i]));
    }
    return res;
}

IPlugView* PLUGIN_API Controller::createView(FIDString name)
{
    if (name && std::strcmp(name, ViewType::kEditor) == 0) return createEditor(this);
    return nullptr;
}

float Controller::plain(int index) const { return paramTable().toPlain(index, (float)const_cast<Controller*>(this)->getParamNormalized((ParamID)index)); }
void Controller::performEditPlain(int index, float v)
{
    const ParamValue n = paramTable().toNorm(index, v);
    setParamNormalized((ParamID)index, n);
    performEdit((ParamID)index, n);
}
void Controller::setPlain(int index, float v) { beginEditIdx(index); performEditPlain(index, v); endEditIdx(index); }
int Controller::program() const
{
    double n = const_cast<Controller*>(this)->getParamNormalized(kProgramParamId);
    return (int)std::lround(n * (double)std::max(1, presetCount() - 1));
}
void Controller::selectProgram(int index)
{
    index = std::max(0, std::min(presetCount() - 1, index));
    const ParamValue n = (double)index / (double)std::max(1, presetCount() - 1);
    beginEdit(kProgramParamId);
    setParamNormalized(kProgramParamId, n);
    performEdit(kProgramParamId, n);
    endEdit(kProgramParamId);
}
float Controller::meter(int piece) const { return (float)const_cast<Controller*>(this)->getParamNormalized(kMeterParamBase + (ParamID)piece); }

}  // namespace df
