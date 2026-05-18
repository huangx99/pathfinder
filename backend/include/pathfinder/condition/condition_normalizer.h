#pragma once

#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/condition/legacy_condition_adapter.h"
#include <string_view>

namespace pathfinder::condition {

struct NormalizedCondition {
    ConditionExpressionRef expression_ref;
    std::string canonical_condition_key;
    std::string summary_key;
    std::string safe_summary_text;
    std::vector<std::string> source_legacy_keys;
    std::vector<std::string> normalization_trace;
    bool from_legacy{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ConditionNormalizer {
public:
    pathfinder::foundation::Result<NormalizedCondition> normalizeKey(std::string_view condition_key) const;
    pathfinder::foundation::Result<NormalizedCondition> normalizeKeys(const std::vector<std::string>& condition_keys) const;
    pathfinder::foundation::Result<NormalizedCondition> normalizeLegacyInput(const LegacyConditionInput& input) const;
    pathfinder::foundation::Result<NormalizedCondition> normalizeCapabilityCondition(const std::string& condition_type, double required_score) const;
};

} // namespace pathfinder::condition
