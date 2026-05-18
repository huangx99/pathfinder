#include "pathfinder/condition/condition_expression_registry.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

Result<void> ConditionExpressionRegistry::registerDefinition(const ConditionExpressionDefinition& definition) {
    auto valid = definition.validateBasic();
    if (valid.is_error()) return valid;
    if (by_id_.find(definition.expression_id.value()) != by_id_.end()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "duplicate condition expression id"));
    }
    if (canonical_to_id_.find(definition.canonical_key) != canonical_to_id_.end()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "duplicate condition canonical key"));
    }
    by_id_[definition.expression_id.value()] = definition;
    canonical_to_id_[definition.canonical_key] = definition.expression_id.value();
    return Result<void>::ok();
}

const ConditionExpressionDefinition* ConditionExpressionRegistry::findById(const ConditionExpressionId& id) const {
    auto it = by_id_.find(id.value());
    if (it == by_id_.end()) return nullptr;
    return &it->second;
}

const ConditionExpressionDefinition* ConditionExpressionRegistry::findByCanonicalKey(const std::string& canonical_key) const {
    auto key_it = canonical_to_id_.find(canonical_key);
    if (key_it == canonical_to_id_.end()) return nullptr;
    return findById(ConditionExpressionId(key_it->second));
}

} // namespace pathfinder::condition
