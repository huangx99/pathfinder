#pragma once

#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/reaction/reaction_types.h"
#include <string>
#include <vector>

namespace pathfinder::reaction {

struct ReactionTrace {
    std::string input_key;
    std::string selected_rule_key;
    ReactionDecision decision{ReactionDecision::Unknown};
    std::vector<std::string> matched_rule_keys;
    std::vector<std::string> blocked_rule_keys;
    std::vector<std::string> no_match_reason_keys;
    std::vector<pathfinder::condition::ConditionEvaluationTrace> condition_traces;
    std::vector<std::string> output_summary_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::reaction
