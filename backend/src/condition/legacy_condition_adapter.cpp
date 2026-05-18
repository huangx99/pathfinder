#include "pathfinder/condition/legacy_condition_adapter.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <map>
#include <set>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

Result<std::string> LegacyConditionAdapter::canonicalizeKey(const std::string& legacy_key, bool test_only) const {
    if (legacy_key.empty()) return Result<std::string>::fail(makeError(ErrorCode::validation_failed, "legacy condition key empty"));
    if (containsConditionForbiddenKey(legacy_key)) return Result<std::string>::fail(makeError(ErrorCode::validation_failed, "legacy condition key forbidden"));
    if (legacy_key.rfind("condition:", 0) == 0) return Result<std::string>::ok(legacy_key);

    static const std::map<std::string, std::string> mappings = {
        {"fresh", "condition:object_state:eq:fresh"},
        {"state_fresh", "condition:object_state:eq:fresh"},
        {"decayed", "condition:object_state:eq:decayed"},
        {"state_decayed", "condition:object_state:eq:decayed"},
        {"cooked", "condition:object_state:eq:cooked"},
        {"raw", "condition:object_state:eq:raw"},
        {"wet", "condition:object_state:eq:wet"},
        {"dry", "condition:object_state:eq:dry"},
        {"has_tool", "condition:actor_requirement:has:tool"},
        {"taught", "condition:knowledge_source:eq:taught"},
        {"observed", "condition:knowledge_source:eq:observed"}
    };
    auto it = mappings.find(legacy_key);
    if (it != mappings.end()) return Result<std::string>::ok(it->second);
    if (test_only && legacy_key.rfind("test_", 0) == 0) return Result<std::string>::ok("condition:test:eq:" + legacy_key);
    return Result<std::string>::fail(makeError(ErrorCode::validation_failed, "unknown legacy condition key: " + legacy_key));
}

Result<std::vector<std::string>> LegacyConditionAdapter::collectCanonicalKeys(const LegacyConditionInput& input) const {
    std::vector<std::string> raw;
    if (!input.condition_key.empty()) raw.push_back(input.condition_key);
    raw.insert(raw.end(), input.condition_keys.begin(), input.condition_keys.end());
    raw.insert(raw.end(), input.object_state_keys.begin(), input.object_state_keys.end());
    raw.insert(raw.end(), input.actor_requirement_keys.begin(), input.actor_requirement_keys.end());
    raw.insert(raw.end(), input.expression_id_keys.begin(), input.expression_id_keys.end());

    std::set<std::string> seen;
    std::vector<std::string> canonical;
    for (const auto& key : raw) {
        auto normalized = canonicalizeKey(key, input.test_only);
        if (normalized.is_error()) return Result<std::vector<std::string>>::fail(normalized.errors());
        if (seen.insert(normalized.value()).second) canonical.push_back(normalized.value());
    }
    std::sort(canonical.begin(), canonical.end());
    return Result<std::vector<std::string>>::ok(std::move(canonical));
}

} // namespace pathfinder::condition
