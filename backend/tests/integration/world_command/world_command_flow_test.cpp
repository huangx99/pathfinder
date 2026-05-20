#include <cassert>
#include <iostream>
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"

using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

void run_flow_bootstrap_tests() {
    // Bootstrap should be possible by setting up pipeline with empty registry
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createInspectCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());
    registry.registerHandler(createGenerateWorldCommandHandler());

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Verify initial projection version
    assert(pipeline.currentProjectionVersion() == 1);

    std::cout << "flow_bootstrap_tests: all passed" << std::endl;
}

void run_flow_command_sequence_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createInspectCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());
    registry.registerHandler(createGenerateWorldCommandHandler());

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Sequence of commands should increment projection version
    uint64_t base_version = pipeline.currentProjectionVersion();

    for (int i = 0; i < 5; ++i) {
        WorldCommandDto cmd;
        cmd.command_id = "seq_" + std::to_string(i);
        cmd.command_key = "wait";
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_seq", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.session_id == "session_seq");
        assert(response.command_id == cmd.command_id);
        assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
        assert(response.projection_patch.base_projection_version == base_version + i);
        assert(response.projection_patch.new_projection_version == base_version + i + 1);
        assert(!response.event_feed.empty());
        assert(response.event_feed[0].event_kind == "TimePassed");
        assert(response.result.command_id == cmd.command_id);
        assert(response.result.command_key == cmd.command_key);
    }

    assert(pipeline.currentProjectionVersion() == base_version + 5);

    std::cout << "flow_command_sequence_tests: all passed" << std::endl;
}

void run_flow_response_completeness_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    WorldCommandDto cmd;
    cmd.command_id = "cmd_comp";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";

    auto result = pipeline.execute("session_comp", cmd);
    assert(result.is_ok());
    auto response = result.value();

    // Response must contain result
    assert(response.result.result_kind != WorldCommandResultKind::Unknown);

    // Response must contain projection_patch with version
    assert(response.projection_patch.new_projection_version > 0);

    // Response must contain event_feed (could be empty, but struct valid)
    // Wait produces TimePassed event
    assert(!response.event_feed.empty());

    // Response must contain available_commands (empty in P43 stub is OK)
    // available_commands vector is always present

    // Response must contain frontend_hints (empty in P43 stub is OK)

    auto vr = response.validateBasic();
    assert(vr.is_ok());

    std::cout << "flow_response_completeness_tests: all passed" << std::endl;
}

void run_flow_invalid_command_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createWaitCommandHandler());
    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Empty command_id should fail shape validation
    {
        WorldCommandDto cmd;
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_inv", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Failed);
        assert(!response.result.failure_reason_keys.empty());
    }

    // Unknown command kind should return failed, not crash
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_inv";
        cmd.command_key = "fly_to_moon";
        cmd.command_kind = WorldCommandKind::Unknown;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        auto result = pipeline.execute("session_inv", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Failed);
    }

    std::cout << "flow_invalid_command_tests: all passed" << std::endl;
}

void run_flow_trace_integrity_tests() {
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createWaitCommandHandler());
    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    WorldCommandDto cmd;
    cmd.command_id = "cmd_trace";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";

    auto result = pipeline.execute("session_trace", cmd);
    assert(result.is_ok());
    auto response = result.value();

    // Trace should contain pipeline stages
    assert(!response.debug_trace_keys.empty());
    bool found_receive = false;
    bool found_result = false;
    for (const auto& entry : response.debug_trace_keys) {
        if (entry.find("receive") != std::string::npos) found_receive = true;
        if (entry.find("result") != std::string::npos) found_result = true;
    }
    assert(found_receive);
    assert(found_result);

    std::cout << "flow_trace_integrity_tests: all passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: bootstrap, sequence, response_completeness, invalid_command, trace_integrity" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "bootstrap") {
        run_flow_bootstrap_tests();
    } else if (test_name == "sequence") {
        run_flow_command_sequence_tests();
    } else if (test_name == "response_completeness") {
        run_flow_response_completeness_tests();
    } else if (test_name == "invalid_command") {
        run_flow_invalid_command_tests();
    } else if (test_name == "trace_integrity") {
        run_flow_trace_integrity_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
