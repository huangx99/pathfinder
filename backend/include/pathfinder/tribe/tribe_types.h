#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::tribe {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::Tick;
using pathfinder::foundation::TribeId;

enum class TribeMemberRole {
    Unknown,
    Pioneer,
    Explorer,
    Gatherer,
    Guardian,
    Teacher,
    Learner,
    Healer,
    Crafter,
    Elder,
    Child,
    TestOnly
};

std::string toString(TribeMemberRole role);
pathfinder::foundation::Result<TribeMemberRole> tribeMemberRoleFromString(const std::string& str);

enum class TribeCohesionState {
    Unknown,
    Stable,
    Watchful,
    Tense,
    Fracturing,
    Splintered,
    TestOnly
};

std::string toString(TribeCohesionState state);
pathfinder::foundation::Result<TribeCohesionState> tribeCohesionStateFromString(const std::string& str);

enum class TribeStateChangeKind {
    Unknown,
    MemberAdded,
    MemberRemoved,
    MemberRoleChanged,
    MoraleChanged,
    TrustChanged,
    SplitRiskChanged,
    CohesionChanged,
    KnowledgeLinked,
    TestOnly
};

std::string toString(TribeStateChangeKind kind);
pathfinder::foundation::Result<TribeStateChangeKind> tribeStateChangeKindFromString(const std::string& str);

bool containsTribeForbiddenKey(const std::string& key);
bool containsTribeForbiddenKey(const std::vector<std::string>& keys);
bool isValidRatio(double value);
double clampRatio(double value);
TribeCohesionState cohesionStateForRisk(double risk);

struct TribeMember {
    EntityId member_id;
    TribeMemberRole role{TribeMemberRole::Unknown};
    bool active{true};
    double trust{0.5};
    double morale{0.5};
    std::vector<KnowledgeId> known_knowledge_ids;
    Tick joined_tick;
    Tick updated_tick;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeMorale {
    double overall{0.5};
    double food_pressure{0.0};
    double safety_pressure{0.0};
    double recent_success{0.0};
    double recent_loss{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeTrust {
    double leader_trust{0.5};
    double member_trust{0.5};
    double teaching_fairness{0.5};
    double knowledge_conflict_pressure{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeSplitRisk {
    double risk{0.0};
    TribeCohesionState cohesion_state{TribeCohesionState::Stable};
    double resource_pressure{0.0};
    double trust_pressure{0.0};
    double knowledge_conflict_pressure{0.0};
    double casualty_pressure{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::tribe
