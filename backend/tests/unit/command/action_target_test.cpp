#include <cassert>
#include <iostream>
#include "pathfinder/command/target.h"

using namespace pathfinder::command;
using namespace pathfinder::foundation;

void run_action_target_tests() {
    // Test 1: Valid object target passes
    {
        ActionTarget target;
        target.target_type = ActionTargetType::Object;
        target.target_id = TargetId("obj_000001");
        target.role = ActionTargetRole::Primary;
        assert(target.validateBasic().is_ok());
    }

    // Test 2: None target_type fails
    {
        ActionTarget target;
        target.target_type = ActionTargetType::None;
        target.target_id = TargetId("obj_000001");
        target.role = ActionTargetRole::Primary;
        assert(target.validateBasic().is_error());
    }

    // Test 3: Empty target_id fails
    {
        ActionTarget target;
        target.target_type = ActionTargetType::Object;
        target.target_id = TargetId("");
        target.role = ActionTargetRole::Primary;
        assert(target.validateBasic().is_error());
    }

    // Test 4: Invalid target_id format fails (contains space)
    {
        ActionTarget target;
        target.target_type = ActionTargetType::Object;
        target.target_id = TargetId("obj 001");
        target.role = ActionTargetRole::Primary;
        assert(target.validateBasic().is_error());
    }

    // Test 5: Invalid target_id format fails (contains uppercase)
    {
        ActionTarget target;
        target.target_type = ActionTargetType::Object;
        target.target_id = TargetId("OBJ_001");
        target.role = ActionTargetRole::Primary;
        assert(target.validateBasic().is_error());
    }

    // Test 6: Different roles pass
    {
        ActionTargetRole roles[] = {
            ActionTargetRole::Primary,
            ActionTargetRole::Secondary,
            ActionTargetRole::Tool,
            ActionTargetRole::Material,
            ActionTargetRole::Receiver,
            ActionTargetRole::Destination
        };
        for (auto role : roles) {
            ActionTarget target;
            target.target_type = ActionTargetType::Object;
            target.target_id = TargetId("obj_000001");
            target.role = role;
            assert(target.validateBasic().is_ok());
        }
    }

    // Test 7: Different target types pass
    {
        ActionTargetType types[] = {
            ActionTargetType::Object,
            ActionTargetType::Entity,
            ActionTargetType::Location,
            ActionTargetType::Region,
            ActionTargetType::Knowledge,
            ActionTargetType::Group
        };
        for (auto type : types) {
            ActionTarget target;
            target.target_type = type;
            target.target_id = TargetId("test_id");
            target.role = ActionTargetRole::Primary;
            assert(target.validateBasic().is_ok());
        }
    }

    std::cout << "action_target_tests: all passed" << std::endl;
}
