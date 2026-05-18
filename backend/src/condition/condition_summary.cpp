#include "pathfinder/condition/condition_summary.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/error.h"
#include <map>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

Result<ConditionSummary> ConditionSummaryBuilder::summarizeCanonicalKey(const std::string& canonical_key) const {
    if (canonical_key.empty()) return Result<ConditionSummary>::fail(makeError(ErrorCode::validation_failed, "canonical condition key empty"));
    if (containsConditionForbiddenKey(canonical_key)) return Result<ConditionSummary>::fail(makeError(ErrorCode::validation_failed, "canonical condition key forbidden"));

    static const std::map<std::string, std::pair<std::string, std::string>> summaries = {
        {"condition:object_state:eq:fresh", {"condition.summary.object_state.fresh", "新鲜状态"}},
        {"condition:object_state:eq:decayed", {"condition.summary.object_state.decayed", "腐烂状态"}},
        {"condition:object_state:eq:cooked", {"condition.summary.object_state.cooked", "熟制状态"}},
        {"condition:object_state:eq:raw", {"condition.summary.object_state.raw", "生的状态"}},
        {"condition:object_state:eq:wet", {"condition.summary.object_state.wet", "潮湿状态"}},
        {"condition:object_state:eq:dry", {"condition.summary.object_state.dry", "干燥状态"}},
        {"condition:actor_requirement:has:tool", {"condition.summary.actor_requirement.tool", "拥有工具"}},
        {"condition:knowledge_source:eq:taught", {"condition.summary.knowledge_source.taught", "由他人传授"}},
        {"condition:knowledge_source:eq:observed", {"condition.summary.knowledge_source.observed", "亲自观察"}}
    };
    auto it = summaries.find(canonical_key);
    ConditionSummary summary;
    summary.canonical_condition_key = canonical_key;
    if (it != summaries.end()) {
        summary.summary_key = it->second.first;
        summary.safe_summary_text = it->second.second;
    } else if (canonical_key.rfind("condition:civilization:", 0) == 0) {
        summary.summary_key = "condition.summary.civilization";
        summary.safe_summary_text = "文明能力条件";
    } else {
        summary.summary_key = "condition.summary.generic";
        summary.safe_summary_text = "条件";
    }
    return Result<ConditionSummary>::ok(std::move(summary));
}

} // namespace pathfinder::condition
