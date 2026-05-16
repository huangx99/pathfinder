#include "pathfinder/pipeline/result.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::pipeline;
using namespace pathfinder::foundation;

void run_pipeline_result_tests() {
    // Test success empty result
    {
        PipelineResult result = PipelineResult::successEmpty();
        assert(result.status == PipelineStatus::Succeeded);
        assert(!result.executed_steps.empty());
        assert(result.state_changes.empty());
        assert(result.events.empty());
        assert(!result.hasErrors());
    }

    // Test validateBasic on success result
    {
        PipelineResult result = PipelineResult::successEmpty();
        auto v_result = result.validateBasic();
        assert(v_result.is_ok());
    }

    // Test result with errors
    {
        PipelineResult result = PipelineResult::successEmpty();
        result.errors.push_back(makeError(ErrorCode::common_unknown, "test error"));
        assert(result.hasErrors());
    }

    // Test invalid status fails
    {
        PipelineResult result;
        result.status = PipelineStatus::NotStarted;
        auto v_result = result.validateBasic();
        assert(v_result.is_error());
    }

    // Test invalid steps order fails
    {
        PipelineResult result = PipelineResult::successEmpty();
        // Manually mess up order
        if (result.executed_steps.size() >= 2) {
            result.executed_steps[1].order = result.executed_steps[0].order;
            auto v_result = result.validateBasic();
            assert(v_result.is_error());
        }
    }

    std::cout << "pipeline_result tests passed" << std::endl;
}
