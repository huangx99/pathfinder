#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pathfinder::condition {

using ConditionExpressionId = pathfinder::foundation::ExpressionId;

enum class ConditionExpressionNodeKind {
    Unknown,
    LiteralBool,
    LiteralNumber,
    LiteralString,
    FieldRef,
    Compare,
    Contains,
    HasTag,
    HasState,
    HasCapability,
    KnowledgeHasClaim,
    And,
    Or,
    Not,
    TestOnly
};

enum class ConditionExpressionOperator {
    Unknown,
    Eq,
    NotEq,
    Gt,
    Gte,
    Lt,
    Lte,
    In,
    Contains,
    HasTag,
    HasState,
    HasCapability,
    TestOnly
};

struct ConditionExpressionValue {
    std::variant<std::monostate, bool, int64_t, double, std::string, std::vector<std::string>> value;

    bool isEmpty() const;
    std::string stableDebugString() const;
};

struct ConditionExpressionNode {
    ConditionExpressionNodeKind kind{ConditionExpressionNodeKind::Unknown};
    std::string field_path;
    ConditionExpressionOperator op{ConditionExpressionOperator::Unknown};
    ConditionExpressionValue value;
    std::vector<ConditionExpressionNode> children;
    std::string summary_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConditionExpressionRef {
    ConditionExpressionId expression_id;
    std::string inline_canonical_key;
    bool inline_legacy_condition{false};

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConditionExpressionDefinition {
    ConditionExpressionId expression_id;
    std::string context_type;
    ConditionExpressionNode root;
    std::string canonical_key;
    std::string summary_key;
    bool trace_enabled{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

std::string toString(ConditionExpressionNodeKind kind);
std::string toString(ConditionExpressionOperator op);
pathfinder::foundation::Result<ConditionExpressionOperator> conditionOperatorFromString(const std::string& value);

bool containsConditionForbiddenKey(const std::string& key);
bool containsConditionForbiddenKey(const std::vector<std::string>& keys);

} // namespace pathfinder::condition
