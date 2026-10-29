#include "controller.h"
#include "ids.h"
#include "processor.h"
#include "public.sdk/source/main/pluginfactory.h"

bool InitModule() { return true; }
bool DeinitModule() { return true; }

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(DF_COMPANY, DF_WEB, DF_EMAIL)

DEF_CLASS2(INLINE_UID_FROM_FUID(df::kProcessorUID), PClassInfo::kManyInstances, kVstAudioEffectClass, "DrumForge", Vst::kDistributable,
           PlugType::kInstrumentDrum, DF_VERSION_STR, kVstVersionString, df::Processor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(df::kControllerUID), PClassInfo::kManyInstances, kVstComponentControllerClass, "DrumForge Controller", 0, "",
           DF_VERSION_STR, kVstVersionString, df::Controller::createInstance)

END_FACTORY
