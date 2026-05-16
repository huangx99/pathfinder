#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/state/state_change.h"
#include <vector>
#include <string>

namespace pathfinder::state {

// A single validation issue found by StateChangeGate
struct StateChangeValidationIssue {
    pathfinder::foundation::ErrorCode code;
    std::string message;
    std::string field_path;
    std::string target_id;
};

// Report containing all validation issues
struct StateChangeValidationReport {
    std::vector<StateChangeValidationIssue> issues;

    bool hasIssues() const { return !issues.empty(); }
    size_t count() const { return issues.size(); }
};

// StateChangeGate - validates state changes without modifying GameState
class StateChangeGate {
public:
    // Validate GameState and StateChangeSet
    // Does NOT modify GameState, does NOT apply changes
    static pathfinder::foundation::Result<StateChangeValidationReport> validate(
        const GameState& state,
        const StateChangeSet& change_set
    );
};

} // namespace pathfinder::state
