#include <cassert>
#include <iostream>
#include "pathfinder/command/action_command.h"

using namespace pathfinder::command;
using namespace pathfinder::foundation;

static ActionCommand makeValidCommand() {
    ActionCommand cmd;
    cmd.action_id = ActionId("eat");
    cmd.actor_id = EntityId("ent_000001");
    cmd.targets.push_back(ActionTarget{
        ActionTargetType::Object,
        TargetId("obj_000001"),
        ActionTargetRole::Primary
    });
    cmd.intent = CommandIntent::Experiment;
    return cmd;
}

void run_action_command_tests() {
    // Test 1: Valid ActionCommand passes
    {
        auto cmd = makeValidCommand();
        assert(cmd.validateBasic().is_ok());
    }

    // Test 2: Empty action_id fails
    {
        auto cmd = makeValidCommand();
        cmd.action_id = ActionId("");
        assert(cmd.validateBasic().is_error());
    }

    // Test 3: Invalid action_id format fails
    {
        auto cmd = makeValidCommand();
        cmd.action_id = ActionId("eat food");
        assert(cmd.validateBasic().is_error());
    }

    // Test 4: Empty actor_id fails
    {
        auto cmd = makeValidCommand();
        cmd.actor_id = EntityId("");
        assert(cmd.validateBasic().is_error());
    }

    // Test 5: Invalid actor_id format fails
    {
        auto cmd = makeValidCommand();
        cmd.actor_id = EntityId("ENT_001");
        assert(cmd.validateBasic().is_error());
    }

    // Test 6: Empty targets fails
    {
        auto cmd = makeValidCommand();
        cmd.targets.clear();
        assert(cmd.validateBasic().is_error());
    }

    // Test 7: Invalid target propagates failure
    {
        auto cmd = makeValidCommand();
        cmd.targets.push_back(ActionTarget{
            ActionTargetType::None,
            TargetId("obj_000002"),
            ActionTargetRole::Secondary
        });
        assert(cmd.validateBasic().is_error());
    }

    // Test 8: Intent Unknown is allowed
    {
        auto cmd = makeValidCommand();
        cmd.intent = CommandIntent::Unknown;
        assert(cmd.validateBasic().is_ok());
    }

    // Test 9: Multiple valid targets pass
    {
        auto cmd = makeValidCommand();
        cmd.targets.push_back(ActionTarget{
            ActionTargetType::Object,
            TargetId("obj_000002"),
            ActionTargetRole::Material
        });
        assert(cmd.validateBasic().is_ok());
    }

    std::cout << "action_command_tests: all passed" << std::endl;
}
