#include "pathfinder/state/state_value.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;

void run_state_value_tests() {
    // Test StateValueType toString
    assert(toString(StateValueType::Empty) == "empty");
    assert(toString(StateValueType::Int) == "int");
    assert(toString(StateValueType::Bool) == "bool");
    assert(toString(StateValueType::String) == "string");

    // Test StateValueType fromString
    {
        auto result = stateValueTypeFromString("empty");
        assert(result.is_ok());
        assert(result.value() == StateValueType::Empty);
    }
    {
        auto result = stateValueTypeFromString("int");
        assert(result.is_ok());
        assert(result.value() == StateValueType::Int);
    }
    {
        auto result = stateValueTypeFromString("bool");
        assert(result.is_ok());
        assert(result.value() == StateValueType::Bool);
    }
    {
        auto result = stateValueTypeFromString("string");
        assert(result.is_ok());
        assert(result.value() == StateValueType::String);
    }
    {
        auto result = stateValueTypeFromString("invalid");
        assert(result.is_error());
    }

    // Test StateValue factory methods
    {
        auto val = StateValue::makeInt(42);
        assert(val.type == StateValueType::Int);
        assert(val.int_value == 42);
    }
    {
        auto val = StateValue::makeBool(true);
        assert(val.type == StateValueType::Bool);
        assert(val.bool_value == true);
    }
    {
        auto val = StateValue::makeString("hello");
        assert(val.type == StateValueType::String);
        assert(val.string_value == "hello");
    }

    // Test StateValue equality
    {
        auto val1 = StateValue::makeInt(10);
        auto val2 = StateValue::makeInt(10);
        auto val3 = StateValue::makeInt(20);
        assert(val1 == val2);
        assert(!(val1 == val3));
    }
    {
        auto val1 = StateValue::makeBool(false);
        auto val2 = StateValue::makeBool(false);
        assert(val1 == val2);
    }
    {
        auto val1 = StateValue::makeString("test");
        auto val2 = StateValue::makeString("test");
        auto val3 = StateValue::makeString("other");
        assert(val1 == val2);
        assert(!(val1 == val3));
    }

    // Test default StateValue
    {
        StateValue val;
        assert(val.type == StateValueType::Empty);
        assert(val.int_value == 0);
        assert(val.bool_value == false);
        assert(val.string_value.empty());
    }

    std::cout << "state_value tests passed" << std::endl;
}
