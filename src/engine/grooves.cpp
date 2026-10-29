#include "grooves.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace df {

#define N nullptr
//        name                  category      K                        S                        R                        TF                       TM                       TH                       Hat                      Crash                    Ride                     Perc
static const GroovePattern kPatterns[] = {
    {"Rock Basic",        "Rock",       {"9... .... 8... ..8.", ".... 9... .... 9...", N, N, N, N, "8.6. 8.6. 8.6. 8.6.", "9... .... .... ....", N, N}},
    {"Rock Driving",      "Rock",       {"9... 8.8. 9... .8..", ".... 9... .... 9...", N, N, N, N, N, "9... .... .... ....", "8.6. 8.6. 8.6. 8.6.", N}},
    {"Pop Four-on-Floor", "Pop",        {"9... 9... 9... 9...", ".... 9... .... 9...", N, N, N, N, "7.o. 7.o. 7.o. 7.o.", N, N, N}},
    {"Half-Time Heavy",   "Rock",       {"9... ..8. .... ..8.", ".... .... 9... ....", N, N, N, N, "7.7. 7.7. 7.7. 7.7.", "9... .... .... ....", N, N}},
    {"Motown Backbeat",   "Soul",       {"9... ..8. 9... ....", ".... 9... .... 9...", N, N, N, N, N, N, N, "t.t. t.t. t.t. t.t."}},
    {"Funk Pocket",       "Funk",       {"9.8. ..8. .8.. ....", ".... 9..3 .3.. 9.3.", N, N, N, N, "7.6. 7.6. 7.6. 7.6.", N, N, N}},
    {"Funk Sixteenths",   "Funk",       {"9..8 .9.. ..8. 9...", ".... 9... .3.3 9..3", N, N, N, N, "8545 8545 8545 8545", N, N, N}},
    {"Soul Ballad",       "Soul",       {"9... .... ..8. ....", ".... 9... .... 9...", N, N, N, N, "6.4. 6.4. 6.4. 6.4.", N, N, N}},
    {"Metal Gallop",      "Metal",      {"9.99 9.99 9.99 9.99", ".... 9... .... 9...", N, N, N, N, N, "9... .... .... ....", "8.7. 8.7. 8.7. 8.7.", N}},
    {"D-Beat Thrash",     "Metal",      {"9.9. ..9. 9.9. ..9.", ".... 9... .... 9...", N, N, N, N, "8.8. 8.8. 8.8. 8.8.", "9... .... .... ....", N, N}},
    {"Blast Beat",        "Metal",      {"9.9. 9.9. 9.9. 9.9.", ".9.9 .9.9 .9.9 .9.9", N, N, N, N, N, N, "8.8. 8.8. 8.8. 8.8.", N}},
    {"Death Double-Bass", "Metal",      {"9999 9999 9999 9999", ".... .... 9... ....", N, N, N, N, N, "9... .... .... ....", "8... 8... 8... 8...", N}},
    {"Doom Crawl",        "Metal",      {"9... .... .... 8...", ".... .... 9... ....", N, N, N, N, N, "9... .... .... ....", "7... .... 7... ....", N}},
    {"Breakdown Chug",    "Metal",      {"9..9 ..9. 9..9 ..9.", ".... .... 9... ....", N, N, N, N, "7... .... 7... ....", "9... .... .... ....", N, N}},
    {"House Four-on-Floor","Electronic",{"9... 9... 9... 9...", N, N, N, N, N, "5.o. 5.o. 5.o. 5.o.", N, N, ".... 9... .... 9..."}},
    {"Techno Driver",     "Electronic", {"9... 9... 9... 9...", N, N, N, N, N, "4.9. 4.9. 4.9. 4.9.", N, N, ".... 8... .... 8..."}},
    {"Trap Half-Time",    "Hip-Hop",    {"9..9 .... ..9. ..9.", ".... .... 9... ....", N, N, N, N, "7.7. 7.7. 7.7. 6666", N, N, ".... .... 8... ...."}},
    {"DnB Two-Step",      "Electronic", {"9... .... .9.. ....", ".... 9..3 .3.. 9.3.", N, N, N, N, "7.7. 7.7. 7.7. 7.7.", N, N, N}},
    {"Boom Bap",          "Hip-Hop",    {"9..9 .... ..9. ....", ".... 9... .... 9...", N, N, N, N, "7.5. 7.5. 7.5. 7.5.", N, N, N}},
    {"Disco",             "Dance",      {"9... 9... 9... 9...", ".... 9... .... 9...", N, N, N, N, "5.o5 5.o5 5.o5 5.o5", N, N, N}},
    {"Jazz Swing Ride",   "Jazz",       {"5... .... 5... ....", ".... .... .... ..4.", N, N, N, N, ".... p... .... p...", N, "8... 8.6. 8... 8.6.", N}},
    {"Bossa Nova",        "Latin",      {"9..9 ..9. 9..9 ..9.", N, "9..9 ..9. ..9. 9...", N, N, N, "7.7. 7.7. 7.7. 7.7.", N, N, N}},
    {"Reggae One-Drop",   "Reggae",     {".... .... 9... ....", N, ".... .... 9... ....", N, N, N, "7.7. 7.7. 7.7. 7.7.", N, N, N}},
    {"Tribal Toms",       "World",      {"9... .... 9... ....", N, N, "9... ..8. .... 8...", ".... 8... ..8. ....", ".8.. .... 9... ...8", N, N, N, N}},
    {"Taiko Pulse",       "Cinematic",  {"9... .... .... ....", N, N, "9... .... 8... 8...", ".... 9... .... ....", N, N, "9... .... .... ....", N, N}},
    {"Industrial Stomp",  "Industrial", {"9... 9... 9... 9...", ".... 9... .... 9...", N, N, N, N, "6.6. 6.6. 6.6. 6.6.", N, N, "..c. ..c. ..c. ..c."}},
    {"Lo-Fi Shuffle",     "Hip-Hop",    {"9... ..8. .9.. ....", ".... 9... .... 9...", N, N, N, N, "6.4. 6.4. 6.4. 6.5.", N, N, N}},
    {"Indie Dance",       "Pop",        {"9... .... 9... ..8.", ".... 9... .... 9...", N, N, N, N, "7.7. 7.7. 7.7. 7.7.", N, N, "..s. ..s. ..s. ..s."}},
};
#undef N

int grooveCount() { return (int)(sizeof(kPatterns) / sizeof(kPatterns[0])); }
const GroovePattern& groovePattern(int i)
{
    int n = grooveCount();
    return kPatterns[i < 0 ? 0 : (i >= n ? n - 1 : i)];
}

// ---------------------------------------------------------------- user patterns
const int kUserLaneNote[kUserLanes] = {36, 38, 37, 39, 42, 46, 44, 41, 45, 50, 49, 51, 53, 54};
const char* const kUserLaneName[kUserLanes] = {"KICK", "SNARE", "SIDE", "CLAP", "HAT", "OPEN", "PEDAL", "FLOOR", "MID", "HIGH", "CRASH", "RIDE", "BELL", "TAMB"};

void UserBank::serializeSlot(int slot, std::vector<uint8_t>& out) const
{
    const UserPattern& u = p[slot];
    out.clear(); out.push_back(u.steps);
    for (int l = 0; l < kUserLanes; ++l)
        for (int k = 0; k < kMaxSteps; ++k) { out.push_back(u.cell[l][k].vel); out.push_back(u.cell[l][k].ratchet); out.push_back(u.cell[l][k].prob); }
}
bool UserBank::deserializeSlot(int slot, const uint8_t* d, size_t n)
{
    if (slot < 0 || slot >= 8 || n < kBlobBytes) return false;
    UserPattern& u = p[slot];
    u.steps = (uint8_t)std::min<int>(kMaxSteps, std::max<int>(4, d[0]));
    size_t i = 1;
    for (int l = 0; l < kUserLanes; ++l)
        for (int k = 0; k < kMaxSteps; ++k) {
            StepCell& c = u.cell[l][k];
            c.vel = std::min<uint8_t>(127, d[i]); c.ratchet = std::min<uint8_t>(4, std::max<uint8_t>(1, d[i + 1])); c.prob = std::min<uint8_t>(100, d[i + 2]);
            i += 3;
        }
    return true;
}

static int userLaneOfNote(int note)
{
    for (int l = 0; l < kUserLanes; ++l) if (kUserLaneNote[l] == note) return l;
    return -1;
}
// stepChar/decode are defined below; forward declare for userFromFactory
static float hashf(uint32_t a, uint32_t b);
static char stepChar(const char* lane, int step);
static int laneLen(const char* lane);
static bool decode(int lane, char c, int& note, int& vel, bool& ghost);

void userFromFactory(int fi, UserPattern& out)
{
    out.clear();
    const GroovePattern& pat = groovePattern(fi);
    int maxLen = 16;
    for (int l = 0; l < NumLanes; ++l) maxLen = std::max(maxLen, laneLen(pat.lanes[l]));
    out.steps = (uint8_t)(((maxLen + 15) / 16) * 16);
    for (int l = 0; l < NumLanes; ++l)
        for (int st = 0; st < out.steps; ++st) {
            int note = 0, vel = 0; bool ghost = false;
            if (!decode(l, stepChar(pat.lanes[l], st), note, vel, ghost)) continue;
            int ul = userLaneOfNote(note);
            if (ul < 0) {   // e.g. cowbell / shaker / claves: closest lane is the tambourine
                ul = 13;
            }
            out.cell[ul][st].vel = (uint8_t)vel;
        }
}

void userRandomize(UserPattern& u, uint32_t seed)
{
    u.clear();
    const int n = u.steps;
    auto rnd = [&](uint32_t k) { return hashf(seed, k) * 0.5f + 0.5f; };
    uint32_t k = 1;
    for (int st = 0; st < n; ++st) {
        const bool beat = st % 4 == 0, off = st % 4 == 2;
        const bool back = (st % 8) == 4;
        if (back) u.cell[1][st].vel = 112;
        else if (rnd(k++) < 0.08f) u.cell[1][st].vel = 40;
        if (st % 8 == 0) u.cell[0][st].vel = 120;
        else if (rnd(k++) < (beat ? 0.15f : 0.2f)) u.cell[0][st].vel = (uint8_t)(80 + 40 * rnd(k++));
        if (beat || off) u.cell[4][st].vel = (uint8_t)(70 + 40 * rnd(k++));
        else if (rnd(k++) < 0.35f) u.cell[4][st].vel = (uint8_t)(35 + 30 * rnd(k++));
        if (rnd(k++) < 0.04f) { const int ln = 7 + (int)(rnd(k++) * 2.99f); u.cell[ln][st].vel = (uint8_t)(90 + 30 * rnd(k++)); }
        if (rnd(k++) < 0.06f && !back) { u.cell[1][st].vel = 50; u.cell[1][st].ratchet = 2; }
    }
}

// fills replace the last four steps of a pattern cycle. 4 chars per lane, nullptr = rest
struct Fill { const char* lanes[NumLanes]; };
static const Fill kFills[4] = {
    {{nullptr, "5679", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},   // snare roll
    {{nullptr, nullptr, nullptr, "...9", "..9.", "97..", nullptr, nullptr, nullptr, nullptr}},       // toms down
    {{"..9.", "9.99", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},     // snare + kick
    {{nullptr, nullptr, nullptr, "9...", ".9..", "..99", nullptr, nullptr, nullptr, nullptr}},       // toms up
};

// character at logical step index (spaces and '|' skipped); '.' if beyond the end
static char stepChar(const char* lane, int step)
{
    if (!lane) return '.';
    int k = 0;
    for (const char* p = lane; *p; ++p) {
        if (*p == ' ' || *p == '|') continue;
        if (k == step) return *p;
        ++k;
    }
    return '.';
}
static int laneLen(const char* lane)
{
    if (!lane) return 0;
    int k = 0;
    for (const char* p = lane; *p; ++p)
        if (*p != ' ' && *p != '|') ++k;
    return k;
}

static inline float hashf(uint32_t a, uint32_t b)   // deterministic [-1,1)
{
    uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u) * 40503u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return (float)(h >> 8) * (2.0f / 16777216.0f) - 1.0f;
}

static bool decode(int lane, char c, int& note, int& vel, bool& ghost)
{
    ghost = false;
    if (c == '.' || c == ' ') return false;
    int d = (c >= '1' && c <= '9') ? c - '0' : 0;
    int dv = d ? std::min(127, 14 * d + 2) : 0;
    if (d && d <= 3) ghost = true;
    switch (lane) {
        case L_Kick: if (!d) return false; note = 36; vel = dv; return true;
        case L_Snare: if (!d) return false; note = 38; vel = dv; return true;
        case L_Rim: if (!d) return false; note = 37; vel = dv; return true;
        case L_TomF: if (!d) return false; note = 41; vel = dv; return true;
        case L_TomM: if (!d) return false; note = 45; vel = dv; return true;
        case L_TomH: if (!d) return false; note = 50; vel = dv; return true;
        case L_Hat:
            if (d) { note = 42; vel = dv; return true; }
            if (c == 'o') { note = 46; vel = 100; return true; }
            if (c == 'p') { note = 44; vel = 80; return true; }
            return false;
        case L_Crash: if (!d) return false; note = 49; vel = dv; return true;
        case L_Ride:
            if (d) { note = 51; vel = dv; return true; }
            if (c == 'b') { note = 53; vel = 95; return true; }
            return false;
        case L_Perc:
            if (d) { note = 39; vel = dv; return true; }
            if (c == 't') { note = 54; vel = 80; return true; }
            if (c == 'T') { note = 54; vel = 108; return true; }
            if (c == 'c') { note = 56; vel = 90; return true; }
            if (c == 'C') { note = 56; vel = 112; return true; }
            if (c == 's') { note = 70; vel = 72; return true; }
            if (c == 'S') { note = 70; vel = 100; return true; }
            return false;
    }
    return false;
}

static int laneGroup(int note)   // 0 kick 1 snare 2 hat 3 tom 4 cymbal 5 perc
{
    switch (note) {
        case 35: case 36: return 0;
        case 37: case 38: case 40: case 39: return 1;
        case 42: case 44: case 46: return 2;
        case 41: case 43: case 45: case 47: case 48: case 50: return 3;
        case 49: case 57: case 55: case 52: case 51: case 59: case 53: return 4;
        default: return 5;
    }
}
static bool isCrashNote(int note) { return note == 49 || note == 57 || note == 55 || note == 52; }

static int cycleStepsOf(const GrooveSettings& s, bool& user, const UserPattern*& up)
{
    user = s.pattern >= grooveCount() && s.bank;
    up = nullptr;
    if (user) { up = &s.bank->p[std::min(7, s.pattern - grooveCount())]; return std::max(4, (int)up->steps); }
    const GroovePattern& pat = groovePattern(s.pattern);
    int maxLen = 16;
    for (int l = 0; l < NumLanes; ++l) maxLen = std::max(maxLen, laneLen(pat.lanes[l]));
    return ((maxLen + 15) / 16) * 16;
}

double grooveCycleBeats(const GrooveSettings& s)
{
    static const double kStepBeats[3] = {0.25, 0.5, 0.125};
    bool user; const UserPattern* up;
    const int cs = cycleStepsOf(s, user, up);
    return cs * kStepBeats[s.rate < 0 ? 0 : (s.rate > 2 ? 2 : s.rate)];
}

int grooveGenerate(const GrooveSettings& s, double from, double to, GrooveEvent* out, int cap)
{
    if (to <= from || cap <= 0) return 0;
    static const double kStepBeats[3] = {0.25, 0.5, 0.125};
    const double stepBeats = kStepBeats[s.rate < 0 ? 0 : (s.rate > 2 ? 2 : s.rate)];
    bool user; const UserPattern* up;
    const int cycleSteps = cycleStepsOf(s, user, up);
    const GroovePattern& pat = groovePattern(user ? 0 : s.pattern);

    const double swingDelay = (double)s.swing * 0.01 * (1.0 / 3.0) * stepBeats;
    const double humanSpan = (double)s.human * 0.01 * 0.12 * stepBeats;
    const double laid = (double)s.laidBack * 0.01 * stepBeats;
    const double slack = swingDelay + humanSpan + std::fabs(laid) + stepBeats * 0.5 + 1e-6;   // + ratchet sub-steps never exceed a step
    const float scale = 0.55f + 0.6f * s.intensity * 0.01f;
    const bool ghostsOn = s.intensity >= 35.0f;

    long long s0 = (long long)std::floor((from - slack) / stepBeats);
    long long s1 = (long long)std::ceil(to / stepBeats);
    if (s0 < 0) s0 = 0;

    int n = 0;
    auto emit = [&](double t, int note, int vel, int lane) {
        if (t < from || t >= to) return;
        const int grp = laneGroup(note);
        if (s.laneMuteMask & (1 << grp)) return;
        if (isCrashNote(note) && ((s.noCrashTo >= s.noCrashFrom && t >= s.noCrashFrom && t <= s.noCrashTo) ||
                                  (s.noCrashTo2 >= s.noCrashFrom2 && t >= s.noCrashFrom2 && t <= s.noCrashTo2))) return;
        (void)lane;
        if (n < cap) {
            int k = n++;
            while (k > 0 && out[k - 1].ppq > t) { out[k] = out[k - 1]; --k; }
            out[k] = {t, note, std::min(127, std::max(1, vel))};
        }
    };

    for (long long st = s0; st <= s1; ++st) {
        const long long cycle = st / cycleSteps;
        const int sc = (int)(st % cycleSteps);
        const bool crashCycle = s.crashEvery > 0 && (cycle % s.crashEvery) == 0;
        const bool fillOn = s.fillEvery > 0 && cycleSteps >= 8 && (cycle % s.fillEvery) == (s.fillEvery - 1);
        const bool inFill = fillOn && sc >= cycleSteps - 4;
        const Fill& fill = kFills[(uint32_t)(cycle * 7 + 3) & 3];
        const bool afterFill = s.fillEvery > 0 && cycle > 0 && (cycle % s.fillEvery) == 0 && sc == 0;
        double base = (double)st * stepBeats;
        if (st & 1) base += swingDelay;

        if (user) {
            for (int lane = 0; lane < kUserLanes; ++lane) {
                int note = kUserLaneNote[lane];
                const StepCell& c = up->cell[lane][sc];
                if (inFill && lane < NumLanes + 4) {
                    // fills are written in the factory lane vocabulary: map the 4 fill steps onto the user lanes
                    continue;
                }
                if (c.vel == 0) continue;
                if (c.prob < 100 && (hashf((uint32_t)st, (uint32_t)lane + 77u) * 0.5f + 0.5f) * 100.0f >= (float)c.prob) continue;
                const bool ghost = c.vel < 50;
                if (ghost && !ghostsOn) continue;
                double t = base;
                if (lane == 1 || lane == 2 || lane == 3) t += laid;
                const int r = std::max(1, (int)c.ratchet);
                for (int k = 0; k < r; ++k) {
                    double tk = t + (double)k * stepBeats / (double)r;
                    if (humanSpan > 0) tk += humanSpan * hashf((uint32_t)st * 4u + (uint32_t)k, (uint32_t)lane + 11u);
                    float v = (float)c.vel * scale * (1.0f - 0.14f * (float)k);
                    emit(tk, note, (int)std::lround(v), lane);
                }
            }
            if (inFill) {
                // generic fill for user patterns: snare roll / tom run, chosen like the factory fills
                const int fi = (int)((uint32_t)(cycle * 7 + 3) & 3);
                const int k4 = sc - (cycleSteps - 4);
                for (int lane = 0; lane < NumLanes; ++lane) {
                    char c = fill.lanes[lane] ? fill.lanes[lane][k4] : '.';
                    int note = 0, vel = 0; bool ghost = false;
                    if (!decode(lane, c, note, vel, ghost)) continue;
                    double t = base;
                    if (humanSpan > 0) t += humanSpan * hashf((uint32_t)st, (uint32_t)lane + 11u);
                    emit(t, note, (int)std::lround(vel * scale), lane);
                }
                (void)fi;
            }
            continue;
        }

        for (int lane = 0; lane <= NumLanes; ++lane) {
            char c;
            int note = 0, vel = 0; bool ghost = false;
            if (lane == NumLanes) {   // synthetic crash after a fill
                if (!afterFill || s.crashEvery == 0 || stepChar(pat.lanes[L_Crash], 0) != '.') continue;
                note = 49; vel = 112;
            } else {
                if (inFill) c = fill.lanes[lane] ? fill.lanes[lane][sc - (cycleSteps - 4)] : '.';
                else c = stepChar(pat.lanes[lane], sc);
                if (!decode(lane, c, note, vel, ghost)) continue;
                if (ghost && !ghostsOn) continue;
                if (lane == L_Crash && !inFill && !crashCycle) continue;      // crash accent only every N cycles
                vel = std::min(127, std::max(1, (int)std::lround(vel * scale)));
            }
            double t = base;
            if (lane == L_Snare || lane == L_Rim || (lane == L_Perc && note == 39)) t += laid;
            if (humanSpan > 0) t += humanSpan * hashf((uint32_t)st, (uint32_t)lane + 11u);
            emit(t, note, vel, lane);
        }
    }
    return n;
}

}  // namespace df
