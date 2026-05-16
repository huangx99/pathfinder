#include "pathfinder/pipeline/rule_pipeline.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::pipeline;
using namespace pathfinder::foundation;

void run_rule_pipeline_tests() {
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

    // Test valid context executes successfully
    {
        RulePipeline pipeline;
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_001");
        ctx.state_metadata.state_id = GameStateId("state_001");
        ctx.state_metadata.state_version = StateVersion(1);
        ctx.state_metadata.current_tick = Tick(100);
        ctx.random_seed = RandomSeed(12345);

        auto result = pipeline.execute(ctx);
        assert(result.is_ok());
        assert(result.value().status == PipelineStatus::Succeeded);
        assert(result.value().state_changes.empty());
        assert(result.value().events.empty());
    }

    // Test invalid context fails
    {
        RulePipeline pipeline;
        PipelineContext ctx;
        ctx.command.source = pathfinder::command::CommandSource::Unknown;
        ctx.state_metadata.state_id = GameStateId("state_002");

        auto result = pipeline.execute(ctx);
        assert(result.is_error());
    }

    // Test input GameState not modified
    {
        RulePipeline pipeline;
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_003");
        GameStateId original_state_id("state_003");
        ctx.state_metadata.state_id = original_state_id;
        ctx.state_metadata.state_version = StateVersion(5);
        ctx.state_metadata.current_tick = Tick(500);
        ctx.random_seed = RandomSeed(99999);

        auto result = pipeline.execute(ctx);
        assert(result.is_ok());

        // Context should not be modified
        assert(ctx.state_metadata.state_id == original_state_id);
        assert(ctx.state_metadata.state_version == StateVersion(5));
        assert(ctx.state_metadata.current_tick == Tick(500));
    }

    // Test executed_steps order is correct
    {
        RulePipeline pipeline;
        PipelineContext ctx;
        ctx.command = makeValidCommand("cmd_004");
        ctx.state_metadata.state_id = GameStateId("state_004");
        ctx.random_seed = RandomSeed(11111);

        auto result = pipeline.execute(ctx);
        assert(result.is_ok());

        const auto& steps = result.value().executed_steps;
        assert(!steps.empty());
        for (size_t i = 1; i < steps.size(); ++i) {
            assert(steps[i].order > steps[i-1].order);
        }
    }

    std::cout << "rule_pipeline tests passed" << std::endl;
}
