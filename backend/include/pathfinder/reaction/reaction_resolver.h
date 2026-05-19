#pragma once

#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/reaction/reaction_context.h"
#include "pathfinder/reaction/reaction_rule.h"
#include "pathfinder/reaction/reaction_trace.h"

namespace pathfinder::reaction {

struct ReactionResult {
    ReactionDecision decision{ReactionDecision::Unknown};
    std::string selected_rule_key;
    std::vector<ReactionObjectChangeDraft> object_changes;
    std::vector<ReactionFeedbackDraft> feedbacks;
    std::vector<ReactionKnowledgeDraft> knowledge_drafts;
    std::vector<ReactionEventDraft> events;
    ReactionTrace trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ObjectReactionResolver {
public:
    pathfinder::foundation::Result<ReactionResult> resolve(
        const ReactionInputSet& input,
        const std::vector<ObjectReactionRule>& rules) const;
};

} // namespace pathfinder::reaction
