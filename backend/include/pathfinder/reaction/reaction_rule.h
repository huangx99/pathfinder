#pragma once

#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/reaction/reaction_types.h"
#include <optional>

namespace pathfinder::reaction {

struct ReactionObjectPattern {
    ReactionObjectRole role{ReactionObjectRole::Unknown};
    std::optional<pathfinder::foundation::ObjectDefinitionId> definition_id;
    std::optional<pathfinder::object::ObjectCategory> category;
    std::vector<std::string> required_tag_keys;
    std::vector<std::string> forbidden_tag_keys;
    bool quantity_required{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionOutputTemplate {
    ReactionOutputKind output_kind{ReactionOutputKind::Unknown};
    ReactionObjectRole target_role{ReactionObjectRole::Unknown};
    pathfinder::foundation::ObjectDefinitionId product_definition_id;
    std::string state_key;
    std::string resource_key;
    double resource_delta{0.0};
    int quantity_delta{0};
    std::string feedback_key;
    std::string knowledge_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ObjectReactionRule {
    std::string rule_key;
    ReactionActionKind action_kind{ReactionActionKind::Unknown};
    std::vector<ReactionObjectPattern> object_patterns;
    std::vector<pathfinder::condition::ConditionExpressionRef> condition_refs;
    std::vector<ReactionOutputTemplate> output_templates;
    int priority{0};
    ReactionConflictPolicy conflict_policy{ReactionConflictPolicy::HighestPriorityOnly};
    std::string feedback_key;
    std::string feedback_text;
    std::string knowledge_effect_key;
    std::vector<std::string> safe_tags;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionObjectChangeDraft {
    ReactionOutputKind change_kind{ReactionOutputKind::Unknown};
    pathfinder::foundation::ObjectId object_id;
    ReactionObjectRole role{ReactionObjectRole::Unknown};
    pathfinder::foundation::ObjectDefinitionId from_definition_id;
    pathfinder::foundation::ObjectDefinitionId to_definition_id;
    std::string state_key;
    std::string resource_key;
    double resource_delta{0.0};
    int quantity_delta{0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionFeedbackDraft {
    std::string feedback_key;
    std::string safe_text;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionKnowledgeDraft {
    std::string knowledge_key;
    std::string subject_id;
    std::string action_key;
    std::string effect_key;
    std::vector<pathfinder::knowledge::KnowledgeCondition> conditions;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionEventDraft {
    std::string event_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::reaction
