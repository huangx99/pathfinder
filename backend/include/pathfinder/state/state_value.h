#pragma once

#include "pathfinder/foundation/result.h"
#include <string>
#include <optional>

namespace pathfinder::state {

enum class StateValueType {
    Empty,
    Int,
    Bool,
    String
};

std::string toString(StateValueType type);
pathfinder::foundation::Result<StateValueType> stateValueTypeFromString(const std::string& str);

struct StateValue {
    StateValueType type = StateValueType::Empty;
    int int_value = 0;
    bool bool_value = false;
    std::string string_value;

    static StateValue makeInt(int v) {
        StateValue sv;
        sv.type = StateValueType::Int;
        sv.int_value = v;
        return sv;
    }

    static StateValue makeBool(bool v) {
        StateValue sv;
        sv.type = StateValueType::Bool;
        sv.bool_value = v;
        return sv;
    }

    static StateValue makeString(const std::string& v) {
        StateValue sv;
        sv.type = StateValueType::String;
        sv.string_value = v;
        return sv;
    }

    bool operator==(const StateValue& other) const = default;
};

} // namespace pathfinder::state
