#include "pathfinder/reaction/reaction_rule.h"
#include "pathfinder/foundation/error.h"
#include <cmath>
#include <set>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static Result<void> validateKey(const std::string& key, const std::string& field, bool required = true) {
    if (key.empty()) {
        if (required) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " empty"));
        return Result<void>::ok();
    }
    if (containsReactionForbiddenKey(key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " forbidden"));
    return Result<void>::ok();
}

static Result<void> validateId(const pathfinder::foundation::ObjectDefinitionId& id, const std::string& field, bool required) {
    if (id.empty()) {
        if (required) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
        return Result<void>::ok();
    }
    if (!isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

Result<void> ReactionExecutionPrecondition::validateBasic() const {
    auto domain_valid = validateKey(missing_domain, "ReactionExecutionPrecondition missing_domain");
    if (domain_valid.is_error()) return domain_valid;
    auto key_valid = validateKey(missing_key, "ReactionExecutionPrecondition missing_key");
    if (key_valid.is_error()) return key_valid;
    auto required_valid = validateKey(required_value, "ReactionExecutionPrecondition required_value");
    if (required_valid.is_error()) return required_valid;
    return Result<void>::ok();
}

Result<void> ReactionObjectPattern::validateBasic() const {
    if (role == ReactionObjectRole::Unknown || role == ReactionObjectRole::TestOnly || role == ReactionObjectRole::Product) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectPattern role invalid"));
    }
    if (definition_id && (definition_id->empty() || !isValidIdString(definition_id->value()))) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionObjectPattern definition_id invalid"));
    }
    if (category && *category == pathfinder::object::ObjectCategory::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectPattern category invalid"));
    }
    if (containsReactionForbiddenKey(required_tag_keys) || containsReactionForbiddenKey(forbidden_tag_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectPattern tag key forbidden"));
    }
    for (const auto& tag : required_tag_keys) {
        if (isDynamicStateLikeKey(tag)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectPattern required_tag_keys cannot contain dynamic state semantics"));
        }
    }
    for (const auto& tag : forbidden_tag_keys) {
        if (isDynamicStateLikeKey(tag)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectPattern forbidden_tag_keys cannot contain dynamic state semantics"));
        }
    }
    for (const auto& precondition : execution_preconditions) {
        auto valid = precondition.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> ReactionOutputTemplate::validateBasic() const {
    if (output_kind == ReactionOutputKind::Unknown || output_kind == ReactionOutputKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionOutputTemplate output_kind invalid"));
    }
    if (target_role == ReactionObjectRole::Unknown || target_role == ReactionObjectRole::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionOutputTemplate target_role invalid"));
    }
    if (target_role == ReactionObjectRole::Product && output_kind != ReactionOutputKind::ProduceObject) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionOutputTemplate Product role only valid for ProduceObject"));
    }
    if (target_role != ReactionObjectRole::Product && output_kind == ReactionOutputKind::ProduceObject) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionOutputTemplate ProduceObject requires Product role"));
    }
    if (!std::isfinite(resource_delta)) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionOutputTemplate resource_delta must be finite"));
    }
    auto state_valid = validateKey(state_key, "ReactionOutputTemplate state_key", false);
    if (state_valid.is_error()) return state_valid;
    auto resource_valid = validateKey(resource_key, "ReactionOutputTemplate resource_key", false);
    if (resource_valid.is_error()) return resource_valid;
    auto feedback_valid = validateKey(feedback_key, "ReactionOutputTemplate feedback_key", false);
    if (feedback_valid.is_error()) return feedback_valid;
    auto knowledge_valid = validateKey(knowledge_key, "ReactionOutputTemplate knowledge_key", false);
    if (knowledge_valid.is_error()) return knowledge_valid;
    if (output_kind == ReactionOutputKind::TransformObject || output_kind == ReactionOutputKind::ProduceObject) {
        auto id_valid = validateId(product_definition_id, "ReactionOutputTemplate product_definition_id", true);
        if (id_valid.is_error()) return id_valid;
    }
    if ((output_kind == ReactionOutputKind::AddState || output_kind == ReactionOutputKind::RemoveState) && state_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionOutputTemplate state_key required"));
    }
    if (output_kind == ReactionOutputKind::ResourceDelta && resource_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionOutputTemplate resource_key required"));
    }
    return Result<void>::ok();
}

Result<void> ObjectReactionRule::validateBasic() const {
    auto key_valid = validateKey(rule_key, "ObjectReactionRule rule_key");
    if (key_valid.is_error()) return key_valid;
    if (action_kind == ReactionActionKind::Unknown || action_kind == ReactionActionKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ObjectReactionRule action_kind invalid"));
    }
    if (object_patterns.empty() || object_patterns.size() > 3) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule object_patterns count must be 1..3"));
    }
    if (condition_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule condition_refs empty"));
    }
    if (output_templates.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule output_templates empty"));
    }
    if (conflict_policy == ReactionConflictPolicy::Unknown || conflict_policy == ReactionConflictPolicy::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ObjectReactionRule conflict_policy invalid"));
    }
    auto feedback_valid = validateKey(feedback_key, "ObjectReactionRule feedback_key", false);
    if (feedback_valid.is_error()) return feedback_valid;
    if (containsReactionForbiddenKey(feedback_text)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule feedback_text forbidden"));
    }
    auto knowledge_valid = validateKey(knowledge_effect_key, "ObjectReactionRule knowledge_effect_key", false);
    if (knowledge_valid.is_error()) return knowledge_valid;
    auto execution_valid = validateKey(execution_effect_key, "ObjectReactionRule execution_effect_key", false);
    if (execution_valid.is_error()) return execution_valid;
    for (const auto& precondition : execution_preconditions) {
        auto valid = precondition.validateBasic();
        if (valid.is_error()) return valid;
    }
    if (containsReactionForbiddenKey(safe_tags)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule safe_tags forbidden"));
    }
    std::set<ReactionObjectRole> roles;
    for (const auto& pattern : object_patterns) {
        auto valid = pattern.validateBasic();
        if (valid.is_error()) return valid;
        if (!roles.insert(pattern.role).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectReactionRule duplicate pattern role"));
        }
    }
    for (const auto& ref : condition_refs) {
        auto valid = ref.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& output : output_templates) {
        auto valid = output.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> ReactionObjectChangeDraft::validateBasic() const {
    if (change_kind == ReactionOutputKind::Unknown || change_kind == ReactionOutputKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectChangeDraft change_kind invalid"));
    }
    if (role == ReactionObjectRole::Unknown || role == ReactionObjectRole::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectChangeDraft role invalid"));
    }
    if (role == ReactionObjectRole::Product && change_kind != ReactionOutputKind::ProduceObject) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectChangeDraft Product role only valid for ProduceObject"));
    }
    if (change_kind == ReactionOutputKind::ProduceObject) {
        auto to_valid = validateId(to_definition_id, "ReactionObjectChangeDraft to_definition_id", true);
        if (to_valid.is_error()) return to_valid;
    } else if (object_id.empty() || !isValidIdString(object_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionObjectChangeDraft object_id invalid"));
    }
    auto from_valid = validateId(from_definition_id, "ReactionObjectChangeDraft from_definition_id", false);
    if (from_valid.is_error()) return from_valid;
    auto to_valid = validateId(to_definition_id, "ReactionObjectChangeDraft to_definition_id", false);
    if (to_valid.is_error()) return to_valid;
    auto state_valid = validateKey(state_key, "ReactionObjectChangeDraft state_key", false);
    if (state_valid.is_error()) return state_valid;
    auto resource_valid = validateKey(resource_key, "ReactionObjectChangeDraft resource_key", false);
    if (resource_valid.is_error()) return resource_valid;
    if (!std::isfinite(resource_delta)) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionObjectChangeDraft resource_delta must be finite"));
    if (containsReactionForbiddenKey(reason_keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectChangeDraft reason_keys forbidden"));
    return Result<void>::ok();
}

Result<void> ReactionFeedbackDraft::validateBasic() const {
    auto feedback_valid = validateKey(feedback_key, "ReactionFeedbackDraft feedback_key");
    if (feedback_valid.is_error()) return feedback_valid;
    if (containsReactionForbiddenKey(safe_text) || containsReactionForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionFeedbackDraft contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> ReactionKnowledgeDraft::validateBasic() const {
    auto knowledge_valid = validateKey(knowledge_key, "ReactionKnowledgeDraft knowledge_key");
    if (knowledge_valid.is_error()) return knowledge_valid;
    auto subject_valid = validateKey(subject_id, "ReactionKnowledgeDraft subject_id");
    if (subject_valid.is_error()) return subject_valid;
    auto action_valid = validateKey(action_key, "ReactionKnowledgeDraft action_key");
    if (action_valid.is_error()) return action_valid;
    auto effect_valid = validateKey(effect_key, "ReactionKnowledgeDraft effect_key");
    if (effect_valid.is_error()) return effect_valid;
    if (containsReactionForbiddenKey(reason_keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionKnowledgeDraft reason_keys forbidden"));
    for (const auto& condition : conditions) {
        auto valid = condition.validateBasic();
        if (valid.is_error()) return valid;
        if (condition.condition_ref.empty() || condition.canonical_condition_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionKnowledgeDraft condition must be normalized"));
        }
    }
    return Result<void>::ok();
}

Result<void> ReactionEventDraft::validateBasic() const {
    auto event_valid = validateKey(event_key, "ReactionEventDraft event_key");
    if (event_valid.is_error()) return event_valid;
    if (containsReactionForbiddenKey(reason_keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionEventDraft reason_keys forbidden"));
    return Result<void>::ok();
}

} // namespace pathfinder::reaction
