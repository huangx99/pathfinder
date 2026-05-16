#include "pathfinder/state/state_value.h"

namespace pathfinder::state {

std::string toString(StateValueType type) {
    switch (type) {
        case StateValueType::Empty: return "empty";
        case StateValueType::Int: return "int";
        case StateValueType::Bool: return "bool";
        case StateValueType::String: return "string";
    }
    return "empty";
}

pathfinder::foundation::Result<StateValueType> stateValueTypeFromString(const std::string& str) {
    if (str == "empty") return pathfinder::foundation::Result<StateValueType>::ok(StateValueType::Empty);
    if (str == "int") return pathfinder::foundation::Result<StateValueType>::ok(StateValueType::Int);
    if (str == "bool") return pathfinder::foundation::Result<StateValueType>::ok(StateValueType::Bool);
    if (str == "string") return pathfinder::foundation::Result<StateValueType>::ok(StateValueType::String);
    return pathfinder::foundation::Result<StateValueType>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "invalid StateValueType: " + str));
}

} // namespace pathfinder::state
