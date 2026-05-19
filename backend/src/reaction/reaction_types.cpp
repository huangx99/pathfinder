#include "pathfinder/reaction/reaction_types.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

std::string toString(ReactionActionKind kind) {
    switch (kind) {
        case ReactionActionKind::Unknown: return "unknown";
        case ReactionActionKind::Use: return "use";
        case ReactionActionKind::Combine: return "combine";
        case ReactionActionKind::Touch: return "touch";
        case ReactionActionKind::Apply: return "apply";
        case ReactionActionKind::Cook: return "cook";
        case ReactionActionKind::Extinguish: return "extinguish";
        case ReactionActionKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReactionObjectRole role) {
    switch (role) {
        case ReactionObjectRole::Unknown: return "unknown";
        case ReactionObjectRole::Source: return "source";
        case ReactionObjectRole::Target: return "target";
        case ReactionObjectRole::Material: return "material";
        case ReactionObjectRole::Tool: return "tool";
        case ReactionObjectRole::Catalyst: return "catalyst";
        case ReactionObjectRole::Container: return "container";
        case ReactionObjectRole::Product: return "product";
        case ReactionObjectRole::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReactionOutputKind kind) {
    switch (kind) {
        case ReactionOutputKind::Unknown: return "unknown";
        case ReactionOutputKind::TransformObject: return "transform_object";
        case ReactionOutputKind::ConsumeObject: return "consume_object";
        case ReactionOutputKind::AddState: return "add_state";
        case ReactionOutputKind::RemoveState: return "remove_state";
        case ReactionOutputKind::ResourceDelta: return "resource_delta";
        case ReactionOutputKind::ProduceObject: return "produce_object";
        case ReactionOutputKind::FeedbackOnly: return "feedback_only";
        case ReactionOutputKind::KnowledgeOnly: return "knowledge_only";
        case ReactionOutputKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReactionDecision decision) {
    switch (decision) {
        case ReactionDecision::Unknown: return "unknown";
        case ReactionDecision::Reacted: return "reacted";
        case ReactionDecision::NoMatchingRule: return "no_matching_rule";
        case ReactionDecision::ConditionBlocked: return "condition_blocked";
        case ReactionDecision::NoEffect: return "no_effect";
        case ReactionDecision::Rejected: return "rejected";
        case ReactionDecision::Failed: return "failed";
        case ReactionDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReactionConflictPolicy policy) {
    switch (policy) {
        case ReactionConflictPolicy::Unknown: return "unknown";
        case ReactionConflictPolicy::HighestPriorityOnly: return "highest_priority_only";
        case ReactionConflictPolicy::FirstStableOrder: return "first_stable_order";
        case ReactionConflictPolicy::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<ReactionActionKind> reactionActionKindFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionActionKind>::ok(ReactionActionKind::Unknown);
    if (value == "use") return Result<ReactionActionKind>::ok(ReactionActionKind::Use);
    if (value == "combine") return Result<ReactionActionKind>::ok(ReactionActionKind::Combine);
    if (value == "touch") return Result<ReactionActionKind>::ok(ReactionActionKind::Touch);
    if (value == "apply") return Result<ReactionActionKind>::ok(ReactionActionKind::Apply);
    if (value == "cook") return Result<ReactionActionKind>::ok(ReactionActionKind::Cook);
    if (value == "extinguish") return Result<ReactionActionKind>::ok(ReactionActionKind::Extinguish);
    if (value == "test_only") return Result<ReactionActionKind>::ok(ReactionActionKind::TestOnly);
    return Result<ReactionActionKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionActionKind: " + value));
}

Result<ReactionObjectRole> reactionObjectRoleFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Unknown);
    if (value == "source") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Source);
    if (value == "target") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Target);
    if (value == "material") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Material);
    if (value == "tool") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Tool);
    if (value == "catalyst") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Catalyst);
    if (value == "container") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Container);
    if (value == "product") return Result<ReactionObjectRole>::ok(ReactionObjectRole::Product);
    if (value == "test_only") return Result<ReactionObjectRole>::ok(ReactionObjectRole::TestOnly);
    return Result<ReactionObjectRole>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionObjectRole: " + value));
}

Result<ReactionOutputKind> reactionOutputKindFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionOutputKind>::ok(ReactionOutputKind::Unknown);
    if (value == "transform_object") return Result<ReactionOutputKind>::ok(ReactionOutputKind::TransformObject);
    if (value == "consume_object") return Result<ReactionOutputKind>::ok(ReactionOutputKind::ConsumeObject);
    if (value == "add_state") return Result<ReactionOutputKind>::ok(ReactionOutputKind::AddState);
    if (value == "remove_state") return Result<ReactionOutputKind>::ok(ReactionOutputKind::RemoveState);
    if (value == "resource_delta") return Result<ReactionOutputKind>::ok(ReactionOutputKind::ResourceDelta);
    if (value == "produce_object") return Result<ReactionOutputKind>::ok(ReactionOutputKind::ProduceObject);
    if (value == "feedback_only") return Result<ReactionOutputKind>::ok(ReactionOutputKind::FeedbackOnly);
    if (value == "knowledge_only") return Result<ReactionOutputKind>::ok(ReactionOutputKind::KnowledgeOnly);
    if (value == "test_only") return Result<ReactionOutputKind>::ok(ReactionOutputKind::TestOnly);
    return Result<ReactionOutputKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionOutputKind: " + value));
}

Result<ReactionDecision> reactionDecisionFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionDecision>::ok(ReactionDecision::Unknown);
    if (value == "reacted") return Result<ReactionDecision>::ok(ReactionDecision::Reacted);
    if (value == "no_matching_rule") return Result<ReactionDecision>::ok(ReactionDecision::NoMatchingRule);
    if (value == "condition_blocked") return Result<ReactionDecision>::ok(ReactionDecision::ConditionBlocked);
    if (value == "no_effect") return Result<ReactionDecision>::ok(ReactionDecision::NoEffect);
    if (value == "rejected") return Result<ReactionDecision>::ok(ReactionDecision::Rejected);
    if (value == "failed") return Result<ReactionDecision>::ok(ReactionDecision::Failed);
    if (value == "test_only") return Result<ReactionDecision>::ok(ReactionDecision::TestOnly);
    return Result<ReactionDecision>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionDecision: " + value));
}

Result<ReactionConflictPolicy> reactionConflictPolicyFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionConflictPolicy>::ok(ReactionConflictPolicy::Unknown);
    if (value == "highest_priority_only") return Result<ReactionConflictPolicy>::ok(ReactionConflictPolicy::HighestPriorityOnly);
    if (value == "first_stable_order") return Result<ReactionConflictPolicy>::ok(ReactionConflictPolicy::FirstStableOrder);
    if (value == "test_only") return Result<ReactionConflictPolicy>::ok(ReactionConflictPolicy::TestOnly);
    return Result<ReactionConflictPolicy>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionConflictPolicy: " + value));
}

bool containsReactionForbiddenKey(const std::string& key) {
    return pathfinder::condition::containsConditionForbiddenKey(key);
}

bool containsReactionForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) { return containsReactionForbiddenKey(key); });
}

bool isDynamicStateLikeKey(const std::string& key) {
    static const std::set<std::string> dynamic_keys = {
        "dry", "wet", "burning", "poisoned", "cooked", "raw", "decayed", "fresh", "extinguished", "depleted"
    };
    return dynamic_keys.find(key) != dynamic_keys.end();
}

static Result<void> validateFiniteResources(const std::map<std::string, double>& values, const std::string& owner) {
    for (const auto& [key, value] : values) {
        if (key.empty() || containsReactionForbiddenKey(key)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " resource key invalid"));
        }
        if (!std::isfinite(value)) {
            return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, owner + " resource value must be finite"));
        }
    }
    return Result<void>::ok();
}

Result<void> ReactionObjectRef::validateBasic() const {
    if (role == ReactionObjectRole::Unknown || role == ReactionObjectRole::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectRef role invalid"));
    }
    if (object_id.empty() || !isValidIdString(object_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionObjectRef object_id invalid"));
    }
    if (definition_id.empty() || !isValidIdString(definition_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionObjectRef definition_id invalid"));
    }
    if (object_key.empty() || containsReactionForbiddenKey(object_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectRef object_key invalid"));
    }
    if (category == pathfinder::object::ObjectCategory::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionObjectRef category invalid"));
    }
    if (quantity < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionObjectRef quantity must be >= 0"));
    }
    if (containsReactionForbiddenKey(state_keys) || containsReactionForbiddenKey(tag_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionObjectRef contains forbidden key"));
    }
    return validateFiniteResources(resource_values, "ReactionObjectRef");
}

Result<void> ReactionInputSet::validateBasic() const {
    if (input_key.empty() || containsReactionForbiddenKey(input_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionInputSet input_key invalid"));
    }
    if (action_kind == ReactionActionKind::Unknown || action_kind == ReactionActionKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionInputSet action_kind invalid"));
    }
    if (actor_id.empty() || !isValidIdString(actor_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionInputSet actor_id invalid"));
    }
    if (objects.empty() || objects.size() > 3) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionInputSet objects count must be 1..3"));
    }
    if (containsReactionForbiddenKey(safe_context_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionInputSet safe_context_keys forbidden"));
    }
    std::set<ReactionObjectRole> roles;
    for (const auto& object : objects) {
        auto valid = object.validateBasic();
        if (valid.is_error()) return valid;
        if (!roles.insert(object.role).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionInputSet duplicate object role"));
        }
    }
    return Result<void>::ok();
}

} // namespace pathfinder::reaction
