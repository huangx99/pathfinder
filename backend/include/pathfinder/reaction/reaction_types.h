#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/object/types.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::reaction {

enum class ReactionActionKind {
    Unknown,
    Use,
    Combine,
    Touch,
    Apply,
    Cook,
    Extinguish,
    TestOnly
};

enum class ReactionObjectRole {
    Unknown,
    Source,
    Target,
    Material,
    Tool,
    Catalyst,
    Container,
    Product,
    TestOnly
};

enum class ReactionOutputKind {
    Unknown,
    TransformObject,
    ConsumeObject,
    AddState,
    RemoveState,
    ResourceDelta,
    ProduceObject,
    FeedbackOnly,
    KnowledgeOnly,
    TestOnly
};

enum class ReactionDecision {
    Unknown,
    Reacted,
    NoMatchingRule,
    ConditionBlocked,
    NoEffect,
    Rejected,
    Failed,
    TestOnly
};

enum class ReactionConflictPolicy {
    Unknown,
    HighestPriorityOnly,
    FirstStableOrder,
    TestOnly
};

std::string toString(ReactionActionKind kind);
std::string toString(ReactionObjectRole role);
std::string toString(ReactionOutputKind kind);
std::string toString(ReactionDecision decision);
std::string toString(ReactionConflictPolicy policy);

pathfinder::foundation::Result<ReactionActionKind> reactionActionKindFromString(const std::string& value);
pathfinder::foundation::Result<ReactionObjectRole> reactionObjectRoleFromString(const std::string& value);
pathfinder::foundation::Result<ReactionOutputKind> reactionOutputKindFromString(const std::string& value);
pathfinder::foundation::Result<ReactionDecision> reactionDecisionFromString(const std::string& value);
pathfinder::foundation::Result<ReactionConflictPolicy> reactionConflictPolicyFromString(const std::string& value);

bool containsReactionForbiddenKey(const std::string& key);
bool containsReactionForbiddenKey(const std::vector<std::string>& keys);
bool isDynamicStateLikeKey(const std::string& key);

struct ReactionObjectRef {
    ReactionObjectRole role{ReactionObjectRole::Unknown};
    pathfinder::foundation::ObjectId object_id;
    pathfinder::foundation::ObjectDefinitionId definition_id;
    std::string object_key;
    pathfinder::object::ObjectCategory category{pathfinder::object::ObjectCategory::Unknown};
    int quantity{1};
    std::vector<std::string> state_keys;
    std::vector<std::string> tag_keys;
    std::map<std::string, double> resource_values;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionInputSet {
    std::string input_key;
    ReactionActionKind action_kind{ReactionActionKind::Unknown};
    pathfinder::foundation::EntityId actor_id;
    std::vector<ReactionObjectRef> objects;
    pathfinder::foundation::Tick tick;
    std::vector<std::string> safe_context_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::reaction
