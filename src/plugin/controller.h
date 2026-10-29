#pragma once
#include <string>
#include <vector>

#include "../engine/grooves.h"
#include "../engine/params.h"
#include "ids.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace df {

class Controller : public Steinberg::Vst::EditControllerEx1, public Steinberg::Vst::IMidiMapping {
public:
    OBJ_METHODS(Controller, Steinberg::Vst::EditControllerEx1)
    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::Vst::IMidiMapping)
    END_DEFINE_INTERFACES(Steinberg::Vst::EditControllerEx1)
    REFCOUNT_METHODS(Steinberg::Vst::EditControllerEx1)
    Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32 bus, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber number, Steinberg::Vst::ParamID& id) SMTG_OVERRIDE;

    static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IEditController*)new Controller(); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

    // helpers for the editor
    float plain(int index) const;                       // current plain value of table parameter `index`
    void setPlain(int index, float plainValue);         // begin/perform/end edit in one go
    void beginEditIdx(int index) { beginEdit((Steinberg::Vst::ParamID)index); }
    void performEditPlain(int index, float plainValue);
    void endEditIdx(int index) { endEdit((Steinberg::Vst::ParamID)index); }
    int program() const;
    void selectProgram(int index);
    float meter(int piece) const;
    void trigger(int note, int velocity);               // audition a drum sound (test pads)
    float groovePhase() const;                          // 0..1 inside the groove cycle, -1 when the groove is idle
    UserBank& userBank() { return bank; }
    void userSlotChanged(int slot);                     // send an edited user pattern to the processor

private:
    UserBank bank;
    unsigned trigSeq = 0;
public:
};

// Defined by the platform specific editor (editor_win.cpp) or the "no editor" fallback.
Steinberg::IPlugView* createEditor(Controller* controller);

}  // namespace df
