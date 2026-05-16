#pragma once

#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/random.h"
#include <string>

namespace pathfinder::foundation {

// Serialization contract for foundation types
// P0 only supports minimal stringification, no JSON library

inline std::string toStableString(const StateVersion& v) {
    return std::to_string(v.value());
}

inline std::string toStableString(const Tick& t) {
    return std::to_string(t.value());
}

inline std::string toStableString(const DurationTicks& d) {
    return std::to_string(d.value());
}

inline std::string toStableString(const HashValue& h) {
    return std::to_string(h.value());
}

inline std::string toStableString(const RandomSeed& s) {
    return std::to_string(s.value());
}

} // namespace pathfinder::foundation
