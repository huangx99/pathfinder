#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include <string>

namespace pathfinder::state {

// State change operations
enum class StateChangeOperation {
    Create,
    Update,
    Delete,
    Append,
    Remove,
    NoOp
};

std::string toString(StateChangeOperation op);
pathfinder::foundation::Result<StateChangeOperation> stateChangeOperationFromString(const std::string& str);

// State change status
enum class StateChangeStatus {
    Draft,
    Validated,
    Applied,
    Rejected
};

std::string toString(StateChangeStatus status);
pathfinder::foundation::Result<StateChangeStatus> stateChangeStatusFromString(const std::string& str);

// State domain
enum class StateDomain {
    Unknown,
    World,
    Object,
    Entity,
    Region,
    Knowledge,
    Tribe,
    Civilization,
    System
};

std::string toString(StateDomain domain);
pathfinder::foundation::Result<StateDomain> stateDomainFromString(const std::string& str);

} // namespace pathfinder::state
