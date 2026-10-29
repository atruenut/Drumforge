// Serialisation of the user groove bank inside the plugin state (shared by processor and controller).
#pragma once
#include <vector>

#include "../engine/grooves.h"
#include "base/source/fstreamer.h"

namespace df {

constexpr Steinberg::int32 kBankMagic = 0x4B424644;   // 'DFBK'

inline void writeBank(Steinberg::IBStreamer& s, const UserBank& b)
{
    s.writeInt32(kBankMagic);
    std::vector<uint8_t> blob;
    for (int i = 0; i < kUserSlots; ++i) {
        b.serializeSlot(i, blob);
        s.writeInt32((Steinberg::int32)blob.size());
        s.writeRaw(blob.data(), (Steinberg::int32)blob.size());
    }
}
// returns false if the stream has no (valid) bank, e.g. state saved by version 1.0
inline bool readBank(Steinberg::IBStreamer& s, UserBank& b)
{
    Steinberg::int32 magic = 0;
    if (!s.readInt32(magic) || magic != kBankMagic) return false;
    std::vector<uint8_t> blob;
    for (int i = 0; i < kUserSlots; ++i) {
        Steinberg::int32 n = 0;
        if (!s.readInt32(n) || n != (Steinberg::int32)UserBank::kBlobBytes) return false;
        blob.resize((size_t)n);
        if (s.readRaw(blob.data(), n) != n) return false;
        b.deserializeSlot(i, blob.data(), blob.size());
    }
    return true;
}

}  // namespace df
