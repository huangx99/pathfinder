#pragma once

#include "pathfinder/tribe/tribe_types.h"
#include "pathfinder/foundation/version.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::tribe {

struct TribeState {
    TribeId tribe_id;
    std::string display_key;
    std::vector<TribeMember> members;
    int population_total{0};
    int active_population{0};
    TribeMorale morale;
    TribeTrust trust;
    TribeSplitRisk split_risk;
    std::vector<KnowledgeId> shared_knowledge_ids;
    Tick updated_tick;
    pathfinder::foundation::StateVersion version;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeMemberEvent {
    TribeStateChangeKind kind{TribeStateChangeKind::Unknown};
    EntityId member_id;
    std::optional<TribeMemberRole> role;
    std::optional<double> trust;
    std::optional<double> morale;
    std::vector<KnowledgeId> known_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeKnowledgeEvent {
    KnowledgeId knowledge_id;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeStateInput {
    TribeId tribe_id;
    Tick input_tick;
    std::vector<TribeMemberEvent> member_events;
    std::vector<TribeKnowledgeEvent> knowledge_events;
    double morale_delta{0.0};
    double trust_delta{0.0};
    double resource_pressure{0.0};
    double safety_pressure{0.0};
    double knowledge_conflict_pressure{0.0};
    double casualty_pressure{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeStateOptions {
    double morale_delta_weight{1.0};
    double trust_delta_weight{1.0};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeStateChangeDraft {
    TribeStateChangeKind kind{TribeStateChangeKind::Unknown};
    std::optional<EntityId> member_id;
    std::optional<TribeMemberRole> old_role;
    std::optional<TribeMemberRole> new_role;
    double old_value{0.0};
    double new_value{0.0};
    std::string summary_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeProjection {
    TribeId tribe_id;
    std::string display_key;
    std::string population_summary;
    std::vector<std::string> role_summary_lines;
    std::string morale_summary;
    std::string trust_summary;
    std::string split_risk_summary;
    int shared_knowledge_count{0};
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeTrace {
    std::string trace_key;
    Tick input_tick;
    std::vector<std::string> steps;
    std::vector<std::string> matched_policy_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeStateResolveResult {
    std::string decision;
    TribeState updated_state;
    std::vector<TribeStateChangeDraft> state_changes;
    TribeProjection projection;
    TribeTrace trace;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class TribeProjectionBuilder {
public:
    pathfinder::foundation::Result<TribeProjection> build(const TribeState& state) const;
};

class TribeStateResolver {
public:
    pathfinder::foundation::Result<TribeStateResolveResult> resolve(
        const TribeState& current,
        const TribeStateInput& input,
        const TribeStateOptions& options = {}) const;
};

} // namespace pathfinder::tribe
