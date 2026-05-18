#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::condition {

struct ActorConditionView {
    std::vector<std::string> requirement_keys;
    std::vector<std::string> capability_keys;
    std::vector<std::string> tag_keys;
    double trust{0.0};
};

struct ObjectConditionView {
    std::vector<std::string> state_keys;
    std::vector<std::string> tag_keys;
};

struct RegionConditionView {
    std::vector<std::string> tag_keys;
};

struct KnowledgeConditionView {
    std::vector<std::string> canonical_claim_keys;
    std::vector<std::string> source_keys;
    std::vector<std::string> condition_keys;
};

struct TribeConditionView {
    double cohesion{0.0};
    double trust{0.0};
    std::vector<std::string> capability_keys;
};

struct CivilizationConditionView {
    std::map<std::string, double> numeric_fields;
    std::vector<std::string> capability_keys;
};

struct ConflictConditionView {
    std::map<std::string, double> numeric_fields;
    std::vector<std::string> summary_keys;
};

struct ConditionEvaluationContext {
    std::string context_type;
    std::optional<ActorConditionView> actor;
    std::optional<ObjectConditionView> target;
    std::optional<ObjectConditionView> source;
    std::optional<RegionConditionView> region;
    std::optional<KnowledgeConditionView> knowledge;
    std::optional<TribeConditionView> tribe;
    std::optional<CivilizationConditionView> civilization;
    std::optional<ConflictConditionView> conflict;
    pathfinder::foundation::Tick tick;
    std::vector<std::string> safe_context_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::condition
