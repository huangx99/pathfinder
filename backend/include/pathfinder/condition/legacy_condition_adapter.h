#pragma once

#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::condition {

struct LegacyConditionInput {
    std::string condition_key;
    std::vector<std::string> condition_keys;
    std::vector<std::string> object_state_keys;
    std::vector<std::string> actor_requirement_keys;
    std::vector<std::string> expression_id_keys;
    bool test_only{false};
};

// Migration adapter for pre-P27 condition keys. Despite the name, this is not
// dead code: ConditionNormalizer uses it to fold older content into the unified
// condition-expression system. Do not add new gameplay rules here; new content
// should use expression refs directly.
class LegacyConditionAdapter {
public:
    pathfinder::foundation::Result<std::string> canonicalizeKey(const std::string& legacy_key, bool test_only = false) const;
    pathfinder::foundation::Result<std::vector<std::string>> collectCanonicalKeys(const LegacyConditionInput& input) const;
};

} // namespace pathfinder::condition
