// DrumForge - built-in groove (pattern) player.
// Patterns are written as text: one string per lane, one character per step.
//   '.'  = rest      '1'..'9' = hit with increasing velocity (1-3 are "ghost" notes)
//   hat lane   : digit = closed hat, 'o' = open hat, 'p' = pedal hat
//   ride lane  : digit = ride, 'b' = bell
//   perc lane  : digit = clap, 't'/'T' = tambourine, 'c'/'C' = cowbell, 's'/'S' = shaker
// Spaces and '|' are ignored so patterns can be laid out in groups of four.
#pragma once
#include <cstdint>
#include <vector>

namespace df {

enum Lane { L_Kick, L_Snare, L_Rim, L_TomF, L_TomM, L_TomH, L_Hat, L_Crash, L_Ride, L_Perc, NumLanes };

struct GroovePattern {
    const char* name;
    const char* category;
    const char* lanes[NumLanes];   // nullptr = empty lane
};

int grooveCount();
const GroovePattern& groovePattern(int i);

struct GrooveEvent {
    double ppq;     // absolute time in quarter notes
    int note;       // GM note
    int velocity;   // 1..127
};

// ---- user (step-editor) patterns. 14 lanes, up to 64 steps, per-step velocity / ratchet / probability.
constexpr int kUserLanes = 14, kMaxSteps = 64;
extern const int kUserLaneNote[kUserLanes];          // GM note of each lane
extern const char* const kUserLaneName[kUserLanes];
struct StepCell { uint8_t vel = 0; uint8_t ratchet = 1; uint8_t prob = 100; };   // vel 0 = no hit
struct UserPattern {
    uint8_t steps = 16;
    StepCell cell[kUserLanes][kMaxSteps];
    void clear() { for (auto& l : cell) for (auto& c : l) c = StepCell(); }
};
struct UserBank {
    UserPattern p[8];
    static constexpr size_t kBlobBytes = 1 + (size_t)kUserLanes * kMaxSteps * 3;
    void serializeSlot(int slot, std::vector<uint8_t>& out) const;
    bool deserializeSlot(int slot, const uint8_t* data, size_t size);
};
// copies a factory pattern into a user pattern (16/32 steps)
void userFromFactory(int factoryIndex, UserPattern& out);
void userRandomize(UserPattern& u, uint32_t seed);

struct GrooveSettings {
    const UserBank* bank = nullptr;   // needed when pattern >= grooveCount()
    int laneMuteMask = 0;             // bit0 kick 1 snare 2 hat 3 tom 4 cymbal 5 perc
    float laidBack = 0;               // -40..40 % of a step: delays (or pushes) snare / sidestick / clap
    int crashEvery = 8;               // 0 = no crash accents, else keep crash-lane hits only every N pattern cycles
    double noCrashFrom = 0, noCrashTo = -1;   // no crash/china/splash events with from <= ppq <= to (loop restart guard)
    double noCrashFrom2 = 0, noCrashTo2 = -1; // second window: the end of the loop (a humanised downbeat crash can land just before the wrap)
    int pattern = 0;
    float swing = 0;        // 0..100 %
    float intensity = 70;   // 0..100 %
    int rate = 0;           // 0 = 1/16, 1 = 1/8, 2 = 1/32
    float human = 0;        // 0..100 % timing humanize
    int fillEvery = 0;      // 0 = off, else number of pattern cycles between fills
};

// Appends all events with  from <= ppq < to  (sorted by time) to `out` (capacity `cap`);
// returns the number appended. Pure function of its arguments (deterministic).
// length of one cycle of the pattern, in quarter notes (for playhead display)
double grooveCycleBeats(const GrooveSettings& s);
int grooveGenerate(const GrooveSettings& s, double from, double to, GrooveEvent* out, int cap);

}  // namespace df
