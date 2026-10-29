#pragma once
#include <atomic>
#include <mutex>
#include <vector>

#include "../engine/engine.h"
#include "../engine/grooves.h"
#include "../engine/groove_player.h"
#include "ids.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

namespace df {

class Processor : public Steinberg::Vst::AudioEffect {
public:
    Processor();
    static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IAudioProcessor*)new Processor(); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API getLatencySamples() SMTG_OVERRIDE { return (Steinberg::uint32)engine.latencySamples(); }
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

private:
    struct Ev { int offset; int kind; int a; float b; };   // kind 0 = param(a=index,b=plain) 1 = note(a=note,b=vel127) 2 = program(a) 3 = note without Note Shift (groove, test pads) 4 = soft choke cymbals
    DrumEngine engine;
    std::vector<Ev> events;
    GroovePlayer groove;
    std::vector<GrooveOut> grooveOut;
    std::mutex bankMx; UserBank bank;                      // bank = truth (GUI edits arrive by message), audioBank = audio-thread copy
    std::atomic<uint32_t> bankVer{1}; UserBank audioBank; uint32_t audioVer = 0;
    float lastPhase = -2.f;
    int program = 0;
    double sampleRate = 48000, selfPpq = 0;
    float lastMeter[NumPieces] = {};
    int idxGrooveOn = 0, idxPattern = 0, idxSwing = 0, idxIntensity = 0, idxRate = 0, idxHuman = 0, idxFill = 0, idxStopped = 0;

    void applyProgram(int index);
};

}  // namespace df
