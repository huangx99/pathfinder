#pragma once

#include "pathfinder/condition/condition_expression_types.h"
#include <map>
#include <optional>

namespace pathfinder::condition {

class ConditionExpressionRegistry {
public:
    pathfinder::foundation::Result<void> registerDefinition(const ConditionExpressionDefinition& definition);
    const ConditionExpressionDefinition* findById(const ConditionExpressionId& id) const;
    const ConditionExpressionDefinition* findByCanonicalKey(const std::string& canonical_key) const;

private:
    std::map<std::string, ConditionExpressionDefinition> by_id_;
    std::map<std::string, std::string> canonical_to_id_;
};

} // namespace pathfinder::condition
