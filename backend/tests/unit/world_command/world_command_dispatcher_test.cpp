#include <cassert>
#include <iostream>
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"

using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

void run_dispatcher_lookup_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createInspectCommandHandler());

    WorldCommandDispatcher dispatcher(registry);

    // Dispatcher can find handler by kind
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_1";
        cmd.command_key = "noop";
        cmd.command_kind = WorldCommandKind::Noop;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Noop);
    }

    // Unknown command returns Failed / unknown_command, no crash
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_2";
        cmd.command_key = "move";
        cmd.command_kind = WorldCommandKind::Move;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Failed);
        assert(!exec.failure_reason_keys.empty());
        assert(exec.failure_reason_keys[0] == "unknown_command");
    }

    std::cout << "dispatcher_lookup_tests: all passed" << std::endl;
}

void run_dispatcher_no_playbook_rules_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createInspectCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());
    registry.registerHandler(createGenerateWorldCommandHandler());
    WorldCommandDispatcher dispatcher(registry);

    // Content keys that must never appear in handler results
    std::vector<std::string> forbidden_keys = {
        "red_berry", "wolf", "torch", "axe", "poison_mushroom",
        "wood", "tree", "fire", "camp_fire"
    };

    auto check_no_content_rules = [&](const WorldCommandExecutionResult& exec) {
        for (const auto& reason : exec.failure_reason_keys) {
            for (const auto& forbidden : forbidden_keys) {
                assert(reason.find(forbidden) == std::string::npos);
            }
        }
        for (const auto& warning : exec.warning_keys) {
            for (const auto& forbidden : forbidden_keys) {
                assert(warning.find(forbidden) == std::string::npos);
            }
        }
    };

    // Ensure all P43 handlers don't contain hardcoded content rules
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_noop";
        cmd.command_key = "noop";
        cmd.command_kind = WorldCommandKind::Noop;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Noop);
        check_no_content_rules(exec);
    }

    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_wait";
        cmd.command_key = "wait";
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Succeeded);
        check_no_content_rules(exec);
    }

    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_inspect";
        cmd.command_key = "inspect";
        cmd.command_kind = WorldCommandKind::Inspect;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        cmd.target.target_kind = WorldCommandTargetKind::Entity;
        cmd.target.target_entity_id = "bush_001";
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Succeeded);
        check_no_content_rules(exec);
    }

    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_tick";
        cmd.command_key = "system_tick";
        cmd.command_kind = WorldCommandKind::SystemTick;
        cmd.source = WorldCommandSource::SystemTick;
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Succeeded);
        check_no_content_rules(exec);
    }

    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_gen";
        cmd.command_key = "generate_world";
        cmd.command_kind = WorldCommandKind::GenerateWorld;
        cmd.source = WorldCommandSource::WorldGeneration;
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Succeeded);
        check_no_content_rules(exec);
    }

    // Unknown command must fail with only "unknown_command", not content reasons
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_unknown";
        cmd.command_key = "move";
        cmd.command_kind = WorldCommandKind::Move;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        WorldCommandContext ctx(cmd);
        auto result = dispatcher.dispatch(ctx, cmd);
        assert(result.is_ok());
        auto exec = result.value();
        assert(exec.result_kind == WorldCommandResultKind::Failed);
        assert(!exec.failure_reason_keys.empty());
        assert(exec.failure_reason_keys[0] == "unknown_command");
        check_no_content_rules(exec);
    }

    std::cout << "dispatcher_no_playbook_rules_tests: all passed" << std::endl;
}


