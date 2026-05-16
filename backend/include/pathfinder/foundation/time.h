#pragma once

#include <cstdint>

namespace pathfinder::foundation {

// Tick represents a logical time point in the game
class Tick {
public:
    // Default construct with tick 0
    Tick() : value_(0) {}

    // Construct from uint64
    explicit Tick(uint64_t value) : value_(value) {}

    // Get the underlying value
    uint64_t value() const { return value_; }

    // Comparison operators
    bool operator==(const Tick& other) const { return value_ == other.value_; }
    bool operator!=(const Tick& other) const { return value_ != other.value_; }
    bool operator<(const Tick& other) const { return value_ < other.value_; }
    bool operator<=(const Tick& other) const { return value_ <= other.value_; }
    bool operator>(const Tick& other) const { return value_ > other.value_; }
    bool operator>=(const Tick& other) const { return value_ >= other.value_; }

private:
    uint64_t value_;
};

// DurationTicks represents a duration in logical ticks
class DurationTicks {
public:
    // Default construct with duration 0
    DurationTicks() : value_(0) {}

    // Construct from uint64
    explicit DurationTicks(uint64_t value) : value_(value) {}

    // Get the underlying value
    uint64_t value() const { return value_; }

    // Comparison operators
    bool operator==(const DurationTicks& other) const { return value_ == other.value_; }
    bool operator!=(const DurationTicks& other) const { return value_ != other.value_; }
    bool operator<(const DurationTicks& other) const { return value_ < other.value_; }
    bool operator<=(const DurationTicks& other) const { return value_ <= other.value_; }
    bool operator>(const DurationTicks& other) const { return value_ > other.value_; }
    bool operator>=(const DurationTicks& other) const { return value_ >= other.value_; }

private:
    uint64_t value_;
};

// Add DurationTicks to Tick
inline Tick operator+(const Tick& tick, const DurationTicks& duration) {
    return Tick(tick.value() + duration.value());
}

} // namespace pathfinder::foundation
