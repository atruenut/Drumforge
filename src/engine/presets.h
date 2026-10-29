// DrumForge - factory presets
#pragma once
#include <string>

namespace df {

struct Preset {
    const char* name;
    const char* category;
    const char* spec;       // "key=value;key=value" overrides applied on top of the defaults
};

int presetCount();
const Preset& preset(int index);
// Fills `values` (paramTable().size() entries, plain units) with defaults + the preset's overrides.
void presetApply(int index, float* values);
// Parses a "key=value;..." string into `values`; returns the number of unknown keys (for tests).
// Save / load of a user "kit" file: plain text, one key=value per line. Groove and morph settings belong to the session and are not part of a kit.
std::string kitToText(const float* values);
int kitFromText(const std::string& text, float* values);   // values must hold the defaults on entry; returns the number of settings read, -1 if it is not a kit file
int presetParse(const char* spec, float* values);

}  // namespace df
