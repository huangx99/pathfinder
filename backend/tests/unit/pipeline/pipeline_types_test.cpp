#include "pathfinder/pipeline/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::pipeline;
using namespace pathfinder::foundation;

void run_pipeline_types_tests() {
    // Test PipelineStage toString
    assert(toString(PipelineStage::AcceptCommand) == "accept_command");
    assert(toString(PipelineStage::ValidateCommand) == "validate_command");
    assert(toString(PipelineStage::PrepareContext) == "prepare_context");
    assert(toString(PipelineStage::ResolveRules) == "resolve_rules");
    assert(toString(PipelineStage::ValidateStateChanges) == "validate_state_changes");
    assert(toString(PipelineStage::ApplyStateChanges) == "apply_state_changes");
    assert(toString(PipelineStage::BuildEvents) == "build_events");
    assert(toString(PipelineStage::EmitEvents) == "emit_events");
    assert(toString(PipelineStage::Finish) == "finish");

    // Test PipelineStage fromString
    {
        auto result = pipelineStageFromString("accept_command");
        assert(result.is_ok());
        assert(result.value() == PipelineStage::AcceptCommand);
    }
    {
        auto result = pipelineStageFromString("invalid");
        assert(result.is_error());
    }

    // Test PipelineStatus toString
    assert(toString(PipelineStatus::NotStarted) == "not_started");
    assert(toString(PipelineStatus::Running) == "running");
    assert(toString(PipelineStatus::Succeeded) == "succeeded");
    assert(toString(PipelineStatus::Failed) == "failed");

    // Test PipelineStatus fromString
    {
        auto result = pipelineStatusFromString("succeeded");
        assert(result.is_ok());
        assert(result.value() == PipelineStatus::Succeeded);
    }
    {
        auto result = pipelineStatusFromString("invalid");
        assert(result.is_error());
    }

    // Test defaultPipelineSteps
    {
        auto steps = defaultPipelineSteps();
        assert(!steps.empty());
        assert(steps.size() == 9);

        // Check order is strictly increasing
        for (size_t i = 1; i < steps.size(); ++i) {
            assert(steps[i].order > steps[i-1].order);
        }

        // Check contains Finish
        bool has_finish = false;
        for (const auto& step : steps) {
            if (step.stage == PipelineStage::Finish) {
                has_finish = true;
                break;
            }
        }
        assert(has_finish);
    }

    std::cout << "pipeline_types tests passed" << std::endl;
}
