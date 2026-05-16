#pragma once

#include <string>
#include <cstdint>

namespace pathfinder::foundation {

// HashValue represents a stable hash for state, config, or replay
class HashValue {
public:
    // Default construct an empty hash
    HashValue() : value_(0) {}

    // Construct from uint64
    explicit HashValue(uint64_t value) : value_(value) {}

    // Get the underlying value
    uint64_t value() const { return value_; }

    // Check if the hash is empty (zero)
    bool empty() const { return value_ == 0; }

    // Create a HashValue from a string
    static HashValue fromString(const std::string& str) {
        // Simple deterministic hash (djb2)
        uint64_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
        }
        return HashValue(hash);
    }

    // Combine two hashes
    static HashValue combineHash(const HashValue& a, const HashValue& b) {
        // Simple hash combination
        uint64_t combined = a.value() ^ (b.value() + 0x9e3779b9 + (a.value() << 6) + (a.value() >> 2));
        return HashValue(combined);
    }

    // Comparison operators
    bool operator==(const HashValue& other) const { return value_ == other.value_; }
    bool operator!=(const HashValue& other) const { return value_ != other.value_; }

private:
    uint64_t value_;
};

} // namespace pathfinder::foundation
