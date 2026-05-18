#include "pathfinder/condition/condition_expression_context.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/error.h"
#include <cmath>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static Result<void> validateRatio(double value, const std::string& field) {
    if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    }
    return Result<void>::ok();
}

Result<void> ConditionEvaluationContext::validateBasic() const {
    if (context_type.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionEvaluationContext context_type empty"));
    if (containsConditionForbiddenKey(context_type) || containsConditionForbiddenKey(safe_context_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionEvaluationContext contains forbidden key"));
    }
    if (actor) {
        if (containsConditionForbiddenKey(actor->requirement_keys) || containsConditionForbiddenKey(actor->capability_keys) || containsConditionForbiddenKey(actor->tag_keys)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ActorConditionView contains forbidden key"));
        }
        auto result = validateRatio(actor->trust, "ActorConditionView trust");
        if (result.is_error()) return result;
    }
    if (target && (containsConditionForbiddenKey(target->state_keys) || containsConditionForbiddenKey(target->tag_keys))) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectConditionView target contains forbidden key"));
    }
    if (source && (containsConditionForbiddenKey(source->state_keys) || containsConditionForbiddenKey(source->tag_keys))) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ObjectConditionView source contains forbidden key"));
    }
    if (region && containsConditionForbiddenKey(region->tag_keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "RegionConditionView contains forbidden key"));
    if (knowledge && (containsConditionForbiddenKey(knowledge->canonical_claim_keys) || containsConditionForbiddenKey(knowledge->source_keys) || containsConditionForbiddenKey(knowledge->condition_keys))) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConditionView contains forbidden key"));
    }
    if (tribe) {
        if (containsConditionForbiddenKey(tribe->capability_keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeConditionView contains forbidden key"));
        auto cohesion_result = validateRatio(tribe->cohesion, "TribeConditionView cohesion");
        if (cohesion_result.is_error()) return cohesion_result;
        auto trust_result = validateRatio(tribe->trust, "TribeConditionView trust");
        if (trust_result.is_error()) return trust_result;
    }
    if (civilization && containsConditionForbiddenKey(civilization->capability_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationConditionView contains forbidden key"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::condition
