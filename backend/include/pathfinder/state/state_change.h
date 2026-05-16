#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/state/types.h"
#include "pathfinder/state/state_value.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::state {

// StatePath - describes what to change
struct StatePath {
    StateDomain domain;
    pathfinder::foundation::TargetId target_id;
    std::string field_path;
};

// StateChange - a single change draft
struct StateChange {
    pathfinder::foundation::StateChangeId change_id;
    StateChangeOperation operation;
    StateDomain domain;
    pathfinder::foundation::TargetId target_id;
    std::string field_path;
    pathfinder::foundation::HashValue before_hash;
    pathfinder::foundation::HashValue after_hash;
    StateChangeStatus status;
    std::string reason;

    // P3: optional typed values for testing
    std::optional<StateValue> before_value;
    std::optional<StateValue> after_value;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

// StateChangeSet - collection of changes
class StateChangeSet {
public:
    // Add a change to the set
    pathfinder::foundation::Result<void> addChange(StateChange change);

    // Check if empty
    bool empty() const { return changes_.empty(); }

    // Get size
    size_t size() const { return changes_.empty() ? 0 : changes_.size(); }

    // Get all changes
    const std::vector<StateChange>& changes() const { return changes_; }

    // Validate all changes
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<StateChange> changes_;
};

} // namespace pathfinder::state
