#include <cassert>
#include <iostream>
#include <string>
#include "pathfinder/command/envelope.h"

using namespace pathfinder::command;
using namespace pathfinder::foundation;

static CommandEnvelope makeValidEnvelope() {
    CommandEnvelope env;
    env.command_id = CommandId("cmd_000001");
    env.source = CommandSource::Player;
    env.issued_tick = Tick(100);
    env.idempotency_key = "key_001";
    env.correlation_id = "corr_001";

    ActionCommand cmd;
    cmd.action_id = ActionId("eat");
    cmd.actor_id = EntityId("ent_000001");
    cmd.targets.push_back(ActionTarget{
        ActionTargetType::Object,
        TargetId("obj_000001"),
        ActionTargetRole::Primary
    });
    cmd.intent = CommandIntent::Experiment;
    env.payload = cmd;

    return env;
}

void run_command_envelope_tests() {
    // Test 1: Valid envelope passes
    {
        auto env = makeValidEnvelope();
        assert(env.validateBasic().is_ok());
    }

    // Test 2: Empty command_id fails
    {
        auto env = makeValidEnvelope();
        env.command_id = CommandId("");
        assert(env.validateBasic().is_error());
    }

    // Test 3: Invalid command_id format fails
    {
        auto env = makeValidEnvelope();
        env.command_id = CommandId("cmd 001");
        assert(env.validateBasic().is_error());
    }

    // Test 4: Invalid payload fails (propagated)
    {
        auto env = makeValidEnvelope();
        env.payload.action_id = ActionId("");
        assert(env.validateBasic().is_error());
    }

    // Test 5: Empty idempotency_key is allowed
    {
        auto env = makeValidEnvelope();
        env.idempotency_key = std::nullopt;
        assert(env.validateBasic().is_ok());
    }

    // Test 6: Empty correlation_id is allowed
    {
        auto env = makeValidEnvelope();
        env.correlation_id = std::nullopt;
        assert(env.validateBasic().is_ok());
    }

    // Test 7: Too long idempotency_key fails
    {
        auto env = makeValidEnvelope();
        env.idempotency_key = std::string(300, 'x');
        assert(env.validateBasic().is_error());
    }

    // Test 8: Too long correlation_id fails
    {
        auto env = makeValidEnvelope();
        env.correlation_id = std::string(300, 'x');
        assert(env.validateBasic().is_error());
    }

    // Test 9: Different valid sources pass
    {
        CommandSource sources[] = {
            CommandSource::Player,
            CommandSource::Ai,
            CommandSource::System,
            CommandSource::Replay,
            CommandSource::Test
        };
        for (auto src : sources) {
            auto env = makeValidEnvelope();
            env.source = src;
            assert(env.validateBasic().is_ok());
        }
    }

    // Test 10: Key at max length passes
    {
        auto env = makeValidEnvelope();
        env.idempotency_key = std::string(256, 'k');
        assert(env.validateBasic().is_ok());
    }

    // Test 11: Unknown source fails (invalid source)
    {
        auto env = makeValidEnvelope();
        env.source = CommandSource::Unknown;
        assert(env.validateBasic().is_error());
    }

    std::cout << "command_envelope_tests: all passed" << std::endl;
}
