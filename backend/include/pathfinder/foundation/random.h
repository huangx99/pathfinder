#pragma once

#include <cstdint>

namespace pathfinder::foundation {

// RandomSeed represents a deterministic random seed
class RandomSeed {
public:
    // Default construct with seed 0
    RandomSeed() : value_(0) {}

    // Construct from uint64
    explicit RandomSeed(uint64_t value) : value_(value) {}

    // Get the underlying value
    uint64_t value() const { return value_; }

    // Comparison operators
    bool operator==(const RandomSeed& other) const { return value_ == other.value_; }
    bool operator!=(const RandomSeed& other) const { return value_ != other.value_; }

private:
    uint64_t value_;
};

// RandomDrawRecord records a single random draw for replay
struct RandomDrawRecord {
    RandomSeed seed;
    uint64_t draw_index;
    uint64_t value;
};

} // namespace pathfinder::foundation
