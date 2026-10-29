// DrumForge - transport-aware groove scheduling (no VST3 dependency, so it can be unit-tested with a simulated looping host).
#pragma once
#include <vector>

#include "grooves.h"
#include "params.h"

namespace df {

struct Transport {
    bool playing = false;
    bool tempoValid = false; double tempo = 120.0;
    bool ppqValid = false; double ppq = 0.0;               // position of the first sample of the block, in quarter notes
    bool cycleActive = false; double cycleStart = 0, cycleEnd = 0;   // host loop region
};

struct GrooveOut { int offset; int note; int vel; };

class GroovePlayer {
public:
    void reset() { lastValid = false; selfPpq = 0; jumped = false; phase = -1.f; }
    // Appends this block's events to `out` (offsets in samples). `vals` is the engine's parameter array.
    int process(const Transport& t, double sampleRate, int n, const std::vector<float>& vals, const UserBank* bank, std::vector<GrooveOut>& out);

    bool jumped = false;     // set when this block is not the continuation of the previous one (loop restart, locate, play after stop)
    float phase = -1.f;      // 0..1 position inside the pattern cycle, -1 when idle
private:
    bool lastValid = false;
    double lastEnd = 0, selfPpq = 0;
    std::vector<GrooveEvent> buf = std::vector<GrooveEvent>(1024);
};

}  // namespace df
