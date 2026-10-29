// DrumForge VST3 - class ids and parameter id layout
#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

#define DF_VERSION_STR "1.1.0"
#define DF_COMPANY "Tom"
#define DF_WEB "https://example.invalid/drumforge"
#define DF_EMAIL "mailto:none@example.invalid"

namespace df {

static const Steinberg::FUID kProcessorUID(0xD7A3F2B1, 0x4C1E4B87, 0x9A6D0E35, 0x71C2B8F4);
static const Steinberg::FUID kControllerUID(0x5E19C0A7, 0xB2634D1F, 0x8E7A44D2, 0x0C93F6B8);

// parameter ids: 0..N-1 are the ParamTable indices (stable!), special ids live far above
constexpr Steinberg::Vst::ParamID kProgramParamId = 9000;
constexpr Steinberg::Vst::ParamID kMeterParamBase = 9100;   // + piece index, read-only hit meters for the GUI
constexpr Steinberg::Vst::ParamID kTriggerParamId = 9200;   // GUI -> processor: test-pad hit, value = (seq<<14 | note<<7 | vel) / 262143
constexpr Steinberg::Vst::ParamID kPlayheadParamId = 9201;  // processor -> GUI: position inside the groove cycle (0..1), 1/256 resolution
constexpr Steinberg::int32 kStateMagic = 0x44464745;        // 'DFGE'
constexpr Steinberg::int32 kStateVersion = 2;

}  // namespace df
