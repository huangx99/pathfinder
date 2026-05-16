#include "pathfinder/pipeline/context.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::pipeline;
using namespace pathfinder::foundation;

void run_pipeline_context_tests() {
    // Helper to create valid command
    auto makeValidCommand = [](const std::string& cmd_id) {
        pathfinder::command::CommandEnvelope cmd;
        cmd.command_id = CommandId(cmd_id);
        cmd.source = pathfinder::command::CommandSource::Player;
        cmd.payload.action_id = pathfinder::foundation::ActionId("action_test");
        cmd.payload.actor_id = pathfinder::foundation::EntityId("entity_001");
        pathfinder::command::ActionTarget target;
        target.target_id = pathfinder::foundation::TargetId("target_001");
        target.target_type = pathfinder::command::ActionTargetType::Object;
        target.role = pathfinder::command::ActionTargetRole::Primary;
        cmd.payload.targets.push_back(std::move(target));
        return cmd;
    };

    // Test valid context
    {
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_001");
        ctx.state_metadata.state_id = GameStateId("state_001");
        ctx.state_metadata.state_version = StateVersion(1);
        ctx.state_metadata.current_tick = Tick(100);
        ctx.random_seed = RandomSeed(12345);

        auto result = ctx.validateBasic();
        assert(result.is_ok());
    }

    // Test invalid command fails
    {
        PipelineContext ctx;
        ctx.command.source = pathfinder::command::CommandSource::Unknown;
        ctx.state_metadata.state_id = GameStateId("state_002");

        auto result = ctx.validateBasic();
        assert(result.is_error());
    }

    // Test empty state_id fails
    {
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_002");
        ctx.state_metadata.state_id = GameStateId("");

        auto result = ctx.validateBasic();
        assert(result.is_error());
    }

    // Test invalid state_id format fails
    {
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_invalid");
        ctx.state_metadata.state_id = GameStateId("invalid id!");

        auto result = ctx.validateBasic();
        assert(result.is_error());
    }

    // Test optional correlation_id
    {
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_003");
        ctx.state_metadata.state_id = GameStateId("state_003");
        ctx.correlation_id = "corr_001";

        auto result = ctx.validateBasic();
        assert(result.is_ok());
        assert(ctx.correlation_id.has_value());
        assert(ctx.correlation_id.value() == "corr_001");
    }

    std::cout << "pipeline_context tests passed" << std::endl;
}
