#pragma once

#include "pathfinder/condition/condition_expression_context.h"
#include "pathfinder/reaction/reaction_types.h"

namespace pathfinder::reaction {

class ReactionConditionContextBuilder {
public:
    pathfinder::foundation::Result<pathfinder::condition::ConditionEvaluationContext> build(const ReactionInputSet& input) const;
};

} // namespace pathfinder::reaction
