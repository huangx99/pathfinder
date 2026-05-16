#pragma once

#include <cstdint>

namespace pathfinder::foundation {

// StateVersion represents the version of the game state
class StateVersion {
public:
    // Default construct with version 0
    StateVersion() : value_(0) {}

    // Construct from uint64
    explicit StateVersion(uint64_t value) : value_(value) {}

    // Get the underlying value
    uint64_t value() const { return value_; }

    // Get the next version
    StateVersion next() const { return StateVersion(value_ + 1); }

    // Comparison operators
    bool operator==(const StateVersion& other) const { return value_ == other.value_; }
    bool operator!=(const StateVersion& other) const { return value_ != other.value_; }
    bool operator<(const StateVersion& other) const { return value_ < other.value_; }
    bool operator<=(const StateVersion& other) const { return value_ <= other.value_; }
    bool operator>(const StateVersion& other) const { return value_ > other.value_; }
    bool operator>=(const StateVersion& other) const { return value_ >= other.value_; }

private:
    uint64_t value_;
};

} // namespace pathfinder::foundation
