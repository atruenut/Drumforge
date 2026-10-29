#include "processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "../engine/presets.h"
#include "bankio.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>
struct ScopedFtz {
    unsigned old;
    ScopedFtz() : old(_mm_getcsr()) { _mm_setcsr(old | 0x8040u); }   // flush-to-zero + denormals-are-zero
    ~ScopedFtz() { _mm_setcsr(old); }
};
#elif defined(__aarch64__)
struct ScopedFtz {
    unsigned long old;
    ScopedFtz() { __asm__ volatile("mrs %0, fpcr" : "=r"(old)); unsigned long n = old | (1UL << 24); __asm__ volatile("msr fpcr, %0" ::"r"(n)); }
    ~ScopedFtz() { __asm__ volatile("msr fpcr, %0" ::"r"(old)); }
};
#else
struct ScopedFtz {};
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace df {

Processor::Processor()
{
    setControllerClass(kControllerUID);
    const ParamTable& t = paramTable();
    (void)t;
    events.reserve(4096);
    grooveOut.reserve(2048);
    for (int i = 0; i < kUserSlots; ++i) { bank.p[i].clear(); audioBank.p[i].clear(); }
}

tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
    tresult r = AudioEffect::initialize(context);
    if (r != kResultOk) return r;
    addEventInput(STR16("MIDI In"), 1);
    addAudioOutput(STR16("Main"), SpeakerArr::kStereo);
    static const char16_t* names[DrumEngine::kStems] = {u"Kick", u"Snare", u"Toms", u"Hats", u"Cymbals", u"Perc", u"Room"};
    for (int i = 0; i < DrumEngine::kStems; ++i)
        addAudioOutput((const TChar*)names[i], SpeakerArr::kStereo, kAux, 0);   // aux outs start inactive; the host enables them
    return kResultOk;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement*, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns != 0) return kResultFalse;
    for (int32 i = 0; i < numOuts; ++i)
        if (outputs[i] != SpeakerArr::kStereo) return kResultFalse;
    return AudioEffect::setBusArrangements(nullptr, 0, outputs, numOuts);
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 s) { return s == kSample32 ? kResultTrue : kResultFalse; }

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup)
{
    sampleRate = setup.sampleRate;
    engine.init((float)sampleRate);
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setActive(TBool state)
{
    if (state) { engine.reset(); groove.reset(); lastPhase = -2.f; for (float& m : lastMeter) m = -1.f; }
    return AudioEffect::setActive(state);
}

void Processor::applyProgram(int index)
{
    program = std::max(0, std::min(presetCount() - 1, index));
    std::vector<float> v((size_t)paramTable().size());
    presetApply(program, v.data());
    // presets never touch the groove-player or morph settings, those belong to the user's session
    const ParamTable& t = paramTable();
    for (int i = 0; i < t.size(); ++i)
        if (t.isSession(i)) v[(size_t)i] = engine.getParam(i);
    engine.setAll(v.data());
}

tresult PLUGIN_API Processor::process(ProcessData& data)
{
    ScopedFtz ftz;
    const int n = data.numSamples;
    const ParamTable& tbl = paramTable();
    events.clear();

    // ---- parameter changes
    if (data.inputParameterChanges) {
        const int32 nq = data.inputParameterChanges->getParameterCount();
        for (int32 q = 0; q < nq; ++q) {
            IParamValueQueue* queue = data.inputParameterChanges->getParameterData(q);
            if (!queue) continue;
            const ParamID id = queue->getParameterId();
            const int32 np = queue->getPointCount();
            for (int32 p = 0; p < np; ++p) {
                int32 offset; ParamValue v;
                if (queue->getPoint(p, offset, v) != kResultTrue) continue;
                if (id == kTriggerParamId) {
                    const int code = (int)std::lround(v * 262143.0);
                    const int note = (code >> 7) & 127, vel = code & 127;
                    if (vel > 0) events.push_back({offset, 3, note, (float)vel});
                } else if (id == kProgramParamId) {
                    int idx = (int)std::lround(v * (double)std::max(1, presetCount() - 1));
                    events.push_back({offset, 2, idx, 0.f});
                } else if (id < (ParamID)tbl.size()) {
                    events.push_back({offset, 0, (int)id, tbl.toPlain((int)id, (float)v)});
                }
            }
        }
    }
    // ---- MIDI
    if (data.inputEvents) {
        const int32 ne = data.inputEvents->getEventCount();
        for (int32 e = 0; e < ne; ++e) {
            Event ev;
            if (data.inputEvents->getEvent(e, ev) != kResultOk) continue;
            if (ev.type == Event::kNoteOnEvent) {
                float vel = ev.noteOn.velocity;
                int v127 = (int)std::lround(vel * 127.0f);
                events.push_back({ev.sampleOffset, 1, ev.noteOn.pitch, (float)std::max(1, v127)});
            }
        }
    }
    // ---- built-in groove player (follows the host transport; optionally free-runs when stopped)
    {
        if (audioVer != bankVer.load(std::memory_order_acquire) && bankMx.try_lock()) {
            audioBank = bank; audioVer = bankVer.load(std::memory_order_relaxed); bankMx.unlock();
        }
        Transport tr;
        if (const ProcessContext* ctx = data.processContext) {
            tr.playing = (ctx->state & ProcessContext::kPlaying) != 0;
            tr.tempoValid = (ctx->state & ProcessContext::kTempoValid) != 0; tr.tempo = ctx->tempo;
            tr.ppqValid = (ctx->state & ProcessContext::kProjectTimeMusicValid) != 0; tr.ppq = ctx->projectTimeMusic;
            tr.cycleActive = (ctx->state & ProcessContext::kCycleActive) && (ctx->state & ProcessContext::kCycleValid);
            tr.cycleStart = ctx->cycleStartMusic; tr.cycleEnd = ctx->cycleEndMusic;
        }
        grooveOut.clear();
        const bool wasRunning = groove.phase >= 0.f;
        groove.process(tr, sampleRate, n, engine.values(), &audioBank, grooveOut);
        // a loop restart / locate / play-after-stop: let ringing cymbals fade instead of piling up under the new bar
        if (groove.jumped && wasRunning) events.push_back({0, 4, 0, 0.f});
        for (const GrooveOut& g : grooveOut) events.push_back({g.offset, 3, g.note, (float)g.vel});
    }
    std::stable_sort(events.begin(), events.end(), [](const Ev& a, const Ev& b) { return a.offset < b.offset; });

    // ---- output buffers
    float* mainL = nullptr; float* mainR = nullptr;
    float* stemL[DrumEngine::kStems] = {}; float* stemR[DrumEngine::kStems] = {};
    bool anyStem = false;
    for (int32 b = 0; b < data.numOutputs; ++b) {
        AudioBusBuffers& bus = data.outputs[b];
        if (bus.numChannels < 2 || !bus.channelBuffers32) continue;
        if (b == 0) { mainL = bus.channelBuffers32[0]; mainR = bus.channelBuffers32[1]; bus.silenceFlags = 0; }
        else if (b - 1 < DrumEngine::kStems) { stemL[b - 1] = bus.channelBuffers32[0]; stemR[b - 1] = bus.channelBuffers32[1]; bus.silenceFlags = 0; anyStem = true; }
    }
    if (!mainL) { // no usable main bus: still keep state consistent
        for (const Ev& e : events) {
            if (e.kind == 0) engine.setParam(e.a, e.b);
            else if (e.kind == 2) applyProgram(e.a);
        }
        return kResultOk;
    }

    // ---- render in segments between events (sample accurate)
    int pos = 0; size_t ei = 0;
    float* sl[DrumEngine::kStems]; float* sr[DrumEngine::kStems];
    while (pos < n) {
        while (ei < events.size() && events[ei].offset <= pos) {
            const Ev& e = events[ei++];
            if (e.kind == 0) engine.setParam(e.a, e.b);
            else if (e.kind == 1) engine.noteOn(e.a, (int)e.b, true);
            else if (e.kind == 3) engine.noteOn(e.a, (int)e.b, false);
            else if (e.kind == 4) engine.chokeCymbals();
            else applyProgram(e.a);
        }
        int next = (ei < events.size()) ? std::min(n, std::max(pos + 1, events[ei].offset)) : n;
        const int len = next - pos;
        for (int k = 0; k < DrumEngine::kStems; ++k) { sl[k] = stemL[k] ? stemL[k] + pos : nullptr; sr[k] = stemR[k] ? stemR[k] + pos : nullptr; }
        engine.process(mainL + pos, mainR + pos, anyStem ? sl : nullptr, anyStem ? sr : nullptr, len);
        pos = next;
    }
    // inactive aux buses: keep the host's silence flags honest
    for (int32 b = 1; b < data.numOutputs; ++b)
        if (data.outputs[b].numChannels >= 2 && !data.outputs[b].channelBuffers32) data.outputs[b].silenceFlags = 3;

    // ---- hit meters for the GUI (read-only output parameters)
    if (data.outputParameterChanges) {
        for (int p = 0; p < NumPieces; ++p) {
            float m = std::floor(engine.activity(p) * 64.0f) / 64.0f;
            if (m != lastMeter[p]) {
                lastMeter[p] = m;
                int32 idx = 0;
                if (IParamValueQueue* q = data.outputParameterChanges->addParameterData(kMeterParamBase + (ParamID)p, idx))
                    q->addPoint(0, m, idx);
            }
        }
    }
    if (groove.phase >= 0.f || lastPhase >= 0.f) {
        const float ph = groove.phase < 0.f ? -1.f : std::floor(groove.phase * 255.0f) / 255.0f;
        if (ph != lastPhase && data.outputParameterChanges) {
            lastPhase = ph;
            int32 idx = 0;
            if (IParamValueQueue* q = data.outputParameterChanges->addParameterData(kPlayheadParamId, idx))
                q->addPoint(0, ph < 0.f ? 1.0 : ph, idx);     // 1.0 = idle
        }
    }
    return kResultOk;
}

tresult PLUGIN_API Processor::notify(IMessage* message)
{
    if (!message) return kResultFalse;
    if (std::strcmp(message->getMessageID(), "df.slot") == 0) {
        int64 slot = -1;
        const void* data = nullptr; uint32 size = 0;
        IAttributeList* a = message->getAttributes();
        if (a && a->getInt("slot", slot) == kResultOk && a->getBinary("data", data, size) == kResultOk && data) {
            std::lock_guard<std::mutex> lk(bankMx);
            bank.deserializeSlot((int)slot, (const uint8_t*)data, size);
            bankVer.fetch_add(1, std::memory_order_release);
        }
        return kResultOk;
    }
    return AudioEffect::notify(message);
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    const auto& v = engine.values();
    s.writeInt32(kStateMagic); s.writeInt32(kStateVersion); s.writeInt32((int32)v.size());
    for (float f : v) s.writeFloat(f);
    s.writeInt32(program);
    { std::lock_guard<std::mutex> lk(bankMx); writeBank(s, bank); }
    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    int32 magic = 0, ver = 0, count = 0;
    if (!s.readInt32(magic) || magic != kStateMagic || !s.readInt32(ver) || !s.readInt32(count) || count < 0 || count > 100000) return kResultFalse;
    const int total = paramTable().size();
    for (int32 i = 0; i < count; ++i) {
        float f = 0;
        if (!s.readFloat(f)) return kResultFalse;
        if (i < total && f == f) engine.setParam(i, f);     // extra values from newer versions are ignored; missing ones keep defaults
    }
    int32 prog = 0;
    if (s.readInt32(prog)) program = std::max(0, std::min(presetCount() - 1, (int)prog));
    {
        UserBank nb = bank;
        if (readBank(s, nb)) { std::lock_guard<std::mutex> lk(bankMx); bank = nb; bankVer.fetch_add(1, std::memory_order_release); }
    }
    return kResultOk;
}

}  // namespace df
