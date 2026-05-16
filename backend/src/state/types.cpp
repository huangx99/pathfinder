#include "pathfinder/state/types.h"

namespace pathfinder::state {

// StateChangeOperation
std::string toString(StateChangeOperation op) {
    switch (op) {
        case StateChangeOperation::Create: return "create";
        case StateChangeOperation::Update: return "update";
        case StateChangeOperation::Delete: return "delete";
        case StateChangeOperation::Append: return "append";
        case StateChangeOperation::Remove: return "remove";
        case StateChangeOperation::NoOp: return "noop";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<StateChangeOperation> stateChangeOperationFromString(const std::string& str) {
    if (str == "create") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::Create);
    if (str == "update") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::Update);
    if (str == "delete") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::Delete);
    if (str == "append") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::Append);
    if (str == "remove") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::Remove);
    if (str == "noop") return pathfinder::foundation::Result<StateChangeOperation>::ok(StateChangeOperation::NoOp);
    return pathfinder::foundation::Result<StateChangeOperation>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown StateChangeOperation: " + str
        )
    );
}

// StateChangeStatus
std::string toString(StateChangeStatus status) {
    switch (status) {
        case StateChangeStatus::Draft: return "draft";
        case StateChangeStatus::Validated: return "validated";
        case StateChangeStatus::Applied: return "applied";
        case StateChangeStatus::Rejected: return "rejected";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<StateChangeStatus> stateChangeStatusFromString(const std::string& str) {
    if (str == "draft") return pathfinder::foundation::Result<StateChangeStatus>::ok(StateChangeStatus::Draft);
    if (str == "validated") return pathfinder::foundation::Result<StateChangeStatus>::ok(StateChangeStatus::Validated);
    if (str == "applied") return pathfinder::foundation::Result<StateChangeStatus>::ok(StateChangeStatus::Applied);
    if (str == "rejected") return pathfinder::foundation::Result<StateChangeStatus>::ok(StateChangeStatus::Rejected);
    return pathfinder::foundation::Result<StateChangeStatus>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown StateChangeStatus: " + str
        )
    );
}

// StateDomain
std::string toString(StateDomain domain) {
    switch (domain) {
        case StateDomain::Unknown: return "unknown";
        case StateDomain::World: return "world";
        case StateDomain::Object: return "object";
        case StateDomain::Entity: return "entity";
        case StateDomain::Region: return "region";
        case StateDomain::Knowledge: return "knowledge";
        case StateDomain::Tribe: return "tribe";
        case StateDomain::Civilization: return "civilization";
        case StateDomain::System: return "system";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<StateDomain> stateDomainFromString(const std::string& str) {
    if (str == "world") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::World);
    if (str == "object") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Object);
    if (str == "entity") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Entity);
    if (str == "region") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Region);
    if (str == "knowledge") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Knowledge);
    if (str == "tribe") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Tribe);
    if (str == "civilization") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::Civilization);
    if (str == "system") return pathfinder::foundation::Result<StateDomain>::ok(StateDomain::System);
    // Unknown is not a valid input, only default
    return pathfinder::foundation::Result<StateDomain>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown StateDomain: " + str
        )
    );
}

} // namespace pathfinder::state
