#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/pipeline/context.h"
#include "pathfinder/pipeline/result.h"

namespace pathfinder::pipeline {

// Rule pipeline (empty executor for P2)
class RulePipeline {
public:
    // Execute pipeline (P2 returns empty success result)
    pathfinder::foundation::Result<PipelineResult> execute(const PipelineContext& context);
};

} // namespace pathfinder::pipeline
