#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/result.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::replay {

// Tag for RandomReplayEntry ID
struct RandomReplayEntryIdTag {};
using RandomReplayEntryId = pathfinder::foundation::StrongId<RandomReplayEntryIdTag>;

// RandomReplayEntry - records a single random draw for replay
struct RandomReplayEntry {
    RandomReplayEntryId entry_id;
    pathfinder::foundation::CommandId command_id;
    pathfinder::foundation::RandomSeed seed;
    int64_t draw_index = 0;
    int64_t min_value = 0;
    int64_t max_value = 0;
    int64_t result_value = 0;
    std::string reason;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

// RandomReplayLog - in-memory log of random replay entries
class RandomReplayLog {
public:
    // Append an entry (rejects duplicate entry_id)
    pathfinder::foundation::Result<void> append(RandomReplayEntry entry);

    // Get number of entries
    size_t size() const { return entries_.size(); }

    // Get all entries
    const std::vector<RandomReplayEntry>& entries() const { return entries_; }

    // Find entries by command_id
    std::vector<RandomReplayEntry> findByCommandId(
        const pathfinder::foundation::CommandId& command_id) const;

    // Validate all entries
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<RandomReplayEntry> entries_;
};

} // namespace pathfinder::replay
