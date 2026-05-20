#include <cassert>
#include <iostream>
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"

using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// Forward declarations from world_command_dispatcher_test.cpp
void run_dispatcher_lookup_tests();
void run_dispatcher_no_playbook_rules_tests();

void run_enum_tests() {
    // WorldCommandKind
    assert(toString(WorldCommandKind::Noop) == "noop");
    assert(toString(WorldCommandKind::Wait) == "wait");
    assert(toString(WorldCommandKind::Inspect) == "inspect");
    assert(toString(WorldCommandKind::SystemTick) == "system_tick");
    assert(toString(WorldCommandKind::Unknown) == "unknown");

    auto r = worldCommandKindFromString("wait");
    assert(r.is_ok() && r.value() == WorldCommandKind::Wait);
    r = worldCommandKindFromString("invalid");
    assert(r.is_error());

    // WorldCommandSource
    assert(toString(WorldCommandSource::PlayerInput) == "player_input");
    assert(toString(WorldCommandSource::SystemTick) == "system_tick");
    auto r2 = worldCommandSourceFromString("player_input");
    assert(r2.is_ok() && r2.value() == WorldCommandSource::PlayerInput);
    r2 = worldCommandSourceFromString("invalid");
    assert(r2.is_error());

    // WorldCommandResultKind
    assert(toString(WorldCommandResultKind::Succeeded) == "succeeded");
    assert(toString(WorldCommandResultKind::Failed) == "failed");
    auto rr = worldCommandResultKindFromString("blocked");
    assert(rr.is_ok() && rr.value() == WorldCommandResultKind::Blocked);
    rr = worldCommandResultKindFromString("invalid");
    assert(rr.is_error());

    // WorldCommandTargetKind
    assert(toString(WorldCommandTargetKind::Coordinate) == "coordinate");
    assert(toString(WorldCommandTargetKind::Entity) == "entity");

    // PatchOp
    assert(toString(PatchOp::Add) == "add");
    assert(toString(PatchOp::Update) == "update");

    // FrontendHintKind
    assert(toString(FrontendHintKind::FloatingText) == "floating_text");
    assert(toString(FrontendHintKind::PlaySound) == "play_sound");

    // AreaShapeKind
    assert(toString(AreaShapeKind::CircleRadius) == "circle_radius");
    assert(toString(AreaShapeKind::Rectangle) == "rectangle");

    std::cout << "world_command_enum_tests: all passed" << std::endl;
}

void run_dto_validation_tests() {
    // command_id cannot be empty
    {
        WorldCommandDto cmd;
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        auto r = cmd.validateBasic();
        assert(r.is_error());
    }

    // command_key cannot be empty
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_001";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        auto r = cmd.validateBasic();
        assert(r.is_error());
    }

    // source cannot be Unknown
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_001";
        cmd.command_key = "wait";
        cmd.actor_key = "player";
        auto r = cmd.validateBasic();
        assert(r.is_error());
    }

    // PlayerInput must have actor_key
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_001";
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        auto r = cmd.validateBasic();
        assert(r.is_error());
    }

    // Valid command
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_001";
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        auto r = cmd.validateBasic();
        assert(r.is_ok());
    }

    // target_coord must have layer_key
    {
        WorldCommandTargetDto target;
        target.target_kind = WorldCommandTargetKind::Coordinate;
        WorldCoordinateDto coord;
        coord.layer_key = ""; // empty
        target.target_coord = coord;
        auto r = target.validateBasic();
        assert(r.is_error());
    }

    // client_sequence not validated at basic level (just struct check)
    {
        WorldCommandContextDto ctx;
        ctx.client_sequence = 5;
        auto r = ctx.validateBasic();
        assert(r.is_ok());
    }

    std::cout << "world_command_dto_validation_tests: all passed" << std::endl;
}

void run_pipeline_tests() {
    WorldCommandHandlerRegistry registry;
    auto r1 = registry.registerHandler(createNoopCommandHandler());
    assert(r1.is_ok());
    auto r2 = registry.registerHandler(createWaitCommandHandler());
    assert(r2.is_ok());
    auto r3 = registry.registerHandler(createInspectCommandHandler());
    assert(r3.is_ok());
    auto r4 = registry.registerHandler(createSystemTickCommandHandler());
    assert(r4.is_ok());
    auto r5 = registry.registerHandler(createGenerateWorldCommandHandler());
    assert(r5.is_ok());

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Noop command succeeds
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_noop";
        cmd.command_key = "noop";
        cmd.command_kind = WorldCommandKind::Noop;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_1", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Noop);
        assert(!response.event_feed.empty());
        assert(response.event_feed[0].event_kind == "Noop");
        assert(response.projection_patch.new_projection_version > response.projection_patch.base_projection_version);
    }

    // Wait command produces TimePassed event
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_wait";
        cmd.command_key = "wait";
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_1", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
        assert(!response.event_feed.empty());
        assert(response.event_feed[0].event_kind == "TimePassed");
    }

    // Inspect command succeeds
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_inspect";
        cmd.command_key = "inspect";
        cmd.command_kind = WorldCommandKind::Inspect;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        cmd.target.target_kind = WorldCommandTargetKind::Entity;
        cmd.target.target_entity_id = "bush_001";

        auto result = pipeline.execute("session_1", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
        assert(!response.event_feed.empty());
        assert(response.event_feed[0].event_kind == "Inspect");
    }

    // Unknown command returns Failed, no crash
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_unknown";
        cmd.command_key = "unknown_action";
        cmd.command_kind = WorldCommandKind::Unknown;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_1", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Failed);
        assert(!response.result.failure_reason_keys.empty());
        assert(response.result.failure_reason_keys[0] == "unknown_command");
    }

    // Command response must contain result / patch / events / available_commands
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_wait2";
        cmd.command_key = "wait";
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_1", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind != WorldCommandResultKind::Unknown);
        assert(response.projection_patch.new_projection_version >= response.projection_patch.base_projection_version);
        // events may be empty or not, but available_commands is always present (even if empty)
        // We just verify the structure is valid
        auto vr = response.validateBasic();
        assert(vr.is_ok());
    }

    std::cout << "world_command_pipeline_tests: all passed" << std::endl;
}

void run_registry_tests() {
    WorldCommandHandlerRegistry registry;

    // Register handlers
    assert(registry.registerHandler(createNoopCommandHandler()).is_ok());
    assert(registry.registerHandler(createWaitCommandHandler()).is_ok());
    assert(registry.handlerCount() == 2);

    // Duplicate registration fails
    assert(registry.registerHandler(createNoopCommandHandler()).is_error());
    assert(registry.handlerCount() == 2);

    // Find by kind
    assert(registry.findByKind(WorldCommandKind::Noop) != nullptr);
    assert(registry.findByKind(WorldCommandKind::Move) == nullptr);

    // Bind action_key to registered handler kind
    assert(registry.bindActionKey("noop", WorldCommandKind::Noop).is_ok());
    assert(registry.bindActionKey("wait", WorldCommandKind::Wait).is_ok());

    // Find by action_key returns correct handler
    assert(registry.findByActionKey("noop") != nullptr);
    assert(registry.findByActionKey("wait") != nullptr);
    assert(registry.findByActionKey("unknown_action") == nullptr);

    // Duplicate action_key binding fails
    assert(registry.bindActionKey("noop", WorldCommandKind::Wait).is_error());

    // Binding action_key to unregistered kind fails
    assert(registry.bindActionKey("move", WorldCommandKind::Move).is_error());

    // Binding empty action_key fails
    assert(registry.bindActionKey("", WorldCommandKind::Noop).is_error());

    // Clear
    registry.clear();
    assert(registry.handlerCount() == 0);

    std::cout << "world_command_registry_tests: all passed" << std::endl;
}

void run_trace_tests() {
    CommandTraceRecorder recorder;

    WorldCommandDto cmd;
    cmd.command_id = "cmd_trace";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";

    recorder.recordCommandReceived(cmd);
    recorder.recordStage("NormalizeCommand");
    recorder.recordValidated(true, {});

    auto trace = recorder.getTrace();
    assert(!trace.empty());
    assert(trace[0].find("receive") != std::string::npos);

    recorder.clear();
    assert(recorder.getTrace().empty());

    std::cout << "world_command_trace_tests: all passed" << std::endl;
}

void run_source_distinguish_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());
    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // PlayerInput source
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_player";
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    // AgentDecision source
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_agent";
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::AgentDecision;
        cmd.actor_key = "npc_1";
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    // SystemTick source
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_sys";
        cmd.command_key = "system_tick";
        cmd.command_kind = WorldCommandKind::SystemTick;
        cmd.source = WorldCommandSource::SystemTick;
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    // AreaEffectTick source (even though handler is stub, pipeline should not crash)
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_area";
        cmd.command_key = "noop";
        cmd.source = WorldCommandSource::AreaEffectTick;
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    // Test source
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_test";
        cmd.command_key = "noop";
        cmd.source = WorldCommandSource::Test;
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    // Replay source
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_replay";
        cmd.command_key = "noop";
        cmd.source = WorldCommandSource::Replay;
        auto result = pipeline.execute("s1", cmd);
        assert(result.is_ok());
    }

    std::cout << "world_command_source_distinguish_tests: all passed" << std::endl;
}

void run_trace_isolation_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Execute first command
    WorldCommandDto cmd1;
    cmd1.command_id = "cmd_first";
    cmd1.command_key = "wait";
    cmd1.command_kind = WorldCommandKind::Wait;
    cmd1.source = WorldCommandSource::PlayerInput;
    cmd1.actor_key = "player";

    auto result1 = pipeline.execute("session_iso", cmd1);
    assert(result1.is_ok());
    auto response1 = result1.value();

    // Execute second command
    WorldCommandDto cmd2;
    cmd2.command_id = "cmd_second";
    cmd2.command_key = "noop";
    cmd2.command_kind = WorldCommandKind::Noop;
    cmd2.source = WorldCommandSource::PlayerInput;
    cmd2.actor_key = "player";

    auto result2 = pipeline.execute("session_iso", cmd2);
    assert(result2.is_ok());
    auto response2 = result2.value();

    // Second trace should NOT contain first command's command_key
    for (const auto& entry : response2.debug_trace_keys) {
        assert(entry.find("wait") == std::string::npos);
    }

    // Second trace should contain exactly one receive record
    int receive_count = 0;
    for (const auto& entry : response2.debug_trace_keys) {
        if (entry.find("receive") != std::string::npos) {
            ++receive_count;
        }
    }
    assert(receive_count == 1);

    // Second trace should contain second command's command_key (noop)
    bool found_second = false;
    for (const auto& entry : response2.debug_trace_keys) {
        if (entry.find("noop") != std::string::npos) {
            found_second = true;
        }
    }
    assert(found_second);

    std::cout << "world_command_trace_isolation_tests: all passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: enum, dto_validation, pipeline, registry, trace, trace_isolation, source_distinguish, lookup, no_playbook_rules" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "enum") {
        run_enum_tests();
    } else if (test_name == "dto_validation") {
        run_dto_validation_tests();
    } else if (test_name == "pipeline") {
        run_pipeline_tests();
    } else if (test_name == "registry") {
        run_registry_tests();
    } else if (test_name == "trace") {
        run_trace_tests();
    } else if (test_name == "source_distinguish") {
        run_source_distinguish_tests();
    } else if (test_name == "trace_isolation") {
        run_trace_isolation_tests();
    } else if (test_name == "lookup") {
        run_dispatcher_lookup_tests();
    } else if (test_name == "no_playbook_rules") {
        run_dispatcher_no_playbook_rules_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
