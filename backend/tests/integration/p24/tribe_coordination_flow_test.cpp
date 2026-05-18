#include "pathfinder/tribe/tribe_coordination.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static TribeState makeTribeState(const TribeId& tribe_id,
                                  const std::vector<std::pair<std::string, TribeMemberRole>>& members) {
    TribeState state;
    state.tribe_id = tribe_id;
    state.display_key = tribe_id.value();
    for (const auto& [id, role] : members) {
        TribeMember member;
        member.member_id = EntityId(id);
        member.role = role;
        member.active = true;
        member.trust = 0.7;
        member.morale = 0.7;
        member.joined_tick = Tick(1);
        member.updated_tick = Tick(1);
        member.reason_keys = {"test"};
        state.members.push_back(std::move(member));
    }
    state.population_total = static_cast<int>(state.members.size());
    state.active_population = state.population_total;
    state.morale.overall = 0.7;
    state.trust.leader_trust = 0.7;
    state.trust.member_trust = 0.7;
    state.trust.teaching_fairness = 0.7;
    state.split_risk.risk = 0.2;
    state.split_risk.resource_pressure = 0.1;
    state.split_risk.trust_pressure = 0.3;
    state.split_risk.knowledge_conflict_pressure = 0.1;
    state.split_risk.casualty_pressure = 0.1;
    state.split_risk.cohesion_state = TribeCohesionState::Stable;
    return state;
}

static CombatCoordinationInput makeInput(const TribeState& state,
                                          const std::vector<std::pair<std::string, double>>& threats) {
    CombatCoordinationInput input;
    input.tribe_id = state.tribe_id;
    input.input_tick = Tick(20);
    input.tribe_state = state;
    for (const auto& [key, pressure] : threats) {
        ThreatPressureSummary t;
        t.threat_id = EntityId("threat_" + key);
        t.threat_key = key;
        t.threat_pressure = pressure;
        t.proximity_pressure = pressure * 0.5;
        t.target_pressure = pressure * 0.3;
        t.reason_keys = {"test_threat"};
        input.threats.push_back(std::move(t));
    }
    input.reason_keys = {"p24_flow_test"};
    return input;
}

static void test_guardian_defends_learner() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"guardian", TribeMemberRole::Guardian},
        {"learner", TribeMemberRole::Learner},
    });
    auto input = makeInput(state, {{"wolf_like_threat", 0.7}});

    CombatCoordinationResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.validateBasic().is_ok());

    // Should defend group or escort vulnerable
    assert(dto.intent.kind == GroupIntentKind::EscortVulnerable ||
           dto.intent.kind == GroupIntentKind::DefendGroup);
    assert(!dto.defend_actions.empty());

    // Guardian should have defend actions
    bool guardian_defends = std::any_of(dto.defend_actions.begin(), dto.defend_actions.end(),
        [](const auto& da) { return da.actor_member_id == EntityId("guardian"); });
    assert(guardian_defends);

    // No damage/death/kill language
    assert(dto.pack_tactic.summary_key.find("damage") == std::string::npos);
    assert(dto.pack_tactic.summary_key.find("kill") == std::string::npos);
    assert(dto.pack_tactic.summary_key.find("death") == std::string::npos);

    assert(dto.pack_tactic.quality == CoordinationQuality::Stable ||
           dto.pack_tactic.quality == CoordinationQuality::Strong);
    std::cout << "p24_guardian_defends_learner_flow passed" << std::endl;
}

static void test_low_morale_avoid_engagement() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"companion", TribeMemberRole::Learner},
    });
    // Crank morale down to near-zero so coordination falls into avoid/screen territory
    state.members[0].morale = 0.05;
    state.members[0].trust = 0.1;
    state.members[1].morale = 0.05;
    state.members[1].trust = 0.1;
    state.morale.overall = 0.05;
    state.trust.leader_trust = 0.1;
    state.trust.member_trust = 0.1;
    state.trust.teaching_fairness = 0.1;
    state.split_risk.risk = 0.85;
    state.split_risk.cohesion_state = cohesionStateForRisk(0.85);
    auto input = makeInput(state, {{"wolf_like_threat", 0.9}});

    CombatCoordinationResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();

    assert(dto.decision == "avoid_engagement" || dto.decision == "weak_coordination");
    assert(dto.intent.kind == GroupIntentKind::AvoidEngagement ||
           dto.intent.kind == GroupIntentKind::ScreenRetreat);
    // Trace explains why
    assert(!dto.trace.steps.empty());
    std::cout << "p24_low_morale_avoid_engagement_flow passed" << std::endl;
}

static void test_high_trust_focus_threat() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"guardian", TribeMemberRole::Guardian},
        {"explorer", TribeMemberRole::Explorer},
    });
    state.members[0].trust = 0.9;
    state.members[0].morale = 0.85;
    state.members[1].trust = 0.9;
    state.members[1].morale = 0.85;
    state.members[2].trust = 0.9;
    state.members[2].morale = 0.85;
    state.morale.overall = 0.85;
    state.trust.leader_trust = 0.9;
    state.trust.member_trust = 0.9;
    state.trust.teaching_fairness = 0.85;
    // Low threat so FocusThreat is selected (not preempted by DefendGroup)
    auto input = makeInput(state, {{"wolf_like_threat", 0.3}});

    CombatCoordinationResolver resolver;
    auto first = resolver.resolve(input);
    auto second = resolver.resolve(input);
    assert(first.is_ok());
    assert(second.is_ok());

    auto dto = first.value();
    assert(dto.validateBasic().is_ok());
    assert(dto.intent.kind == GroupIntentKind::FocusThreat);
    assert(dto.pack_tactic.kind == PackTacticKind::FocusPressure);
    assert(dto.pack_tactic.quality == CoordinationQuality::Strong ||
           dto.pack_tactic.quality == CoordinationQuality::Stable);

    // Deterministic
    assert(first.value().pack_tactic.coordination_score == second.value().pack_tactic.coordination_score);
    assert(first.value().intent.kind == second.value().intent.kind);
    std::cout << "p24_high_trust_focus_threat_flow passed" << std::endl;
}

static void test_split_risk_weakens_coordination() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"guardian", TribeMemberRole::Guardian},
    });
    state.split_risk.risk = 0.1;

    auto input_low = makeInput(state, {{"wolf_like_threat", 0.5}});
    CombatCoordinationResolver resolver;
    auto result_low = resolver.resolve(input_low);

    state.split_risk.risk = 0.7;
    state.split_risk.cohesion_state = TribeCohesionState::Fracturing;
    auto input_high = makeInput(state, {{"wolf_like_threat", 0.5}});
    auto result_high = resolver.resolve(input_high);

    assert(result_low.is_ok() && result_high.is_ok());
    assert(result_low.value().pack_tactic.coordination_score >
           result_high.value().pack_tactic.coordination_score);
    std::cout << "p24_split_risk_weakens_coordination_flow passed" << std::endl;
}

static void test_deterministic_coordination() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"guardian", TribeMemberRole::Guardian},
        {"explorer", TribeMemberRole::Explorer},
        {"learner", TribeMemberRole::Learner},
    });
    auto input = makeInput(state, {{"wolf_like_threat", 0.6}, {"snake_threat", 0.3}});

    CombatCoordinationResolver resolver;
    auto r1 = resolver.resolve(input);
    auto r2 = resolver.resolve(input);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().pack_tactic.coordination_score == r2.value().pack_tactic.coordination_score);
    assert(r1.value().pack_tactic.quality == r2.value().pack_tactic.quality);
    assert(r1.value().intent.kind == r2.value().intent.kind);
    std::cout << "p24_deterministic_coordination_flow passed" << std::endl;
}

static void test_state_draft_no_duplicate_member_effects() {
    auto state = makeTribeState(TribeId("tribe_p24"), {
        {"pioneer", TribeMemberRole::Pioneer},
        {"guardian", TribeMemberRole::Guardian},
    });
    auto input = makeInput(state, {{"wolf_like_threat", 0.5}});

    CombatCoordinationResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();

    // state_draft.member_summaries must have unique member_ids
    std::set<std::string> ids;
    for (const auto& ms : dto.state_draft.member_summaries) {
        assert(ids.insert(ms.member_id.value()).second);
    }
    // result.member_summaries must also have unique member_ids
    std::set<std::string> result_ids;
    for (const auto& ms : dto.member_summaries) {
        assert(result_ids.insert(ms.member_id.value()).second);
    }
    std::cout << "p24_state_draft_unique_member_effects_flow passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "guardian_defends_learner") test_guardian_defends_learner();
    else if (arg == "low_morale_avoid_engagement") test_low_morale_avoid_engagement();
    else if (arg == "high_trust_focus_threat") test_high_trust_focus_threat();
    else if (arg == "split_risk_weakens_coordination") test_split_risk_weakens_coordination();
    else if (arg == "deterministic_coordination") test_deterministic_coordination();
    else if (arg == "state_draft_unique_member_effects") test_state_draft_no_duplicate_member_effects();
    else {
        test_guardian_defends_learner();
        test_low_morale_avoid_engagement();
        test_high_trust_focus_threat();
        test_split_risk_weakens_coordination();
        test_deterministic_coordination();
        test_state_draft_no_duplicate_member_effects();
    }
    return 0;
}
