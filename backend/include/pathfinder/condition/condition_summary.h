#pragma once

#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::condition {

struct ConditionSummary {
    std::string canonical_condition_key;
    std::string summary_key;
    std::string safe_summary_text;
};

class ConditionSummaryBuilder {
public:
    pathfinder::foundation::Result<ConditionSummary> summarizeCanonicalKey(const std::string& canonical_key) const;
};

} // namespace pathfinder::condition
