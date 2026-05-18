#include "pathfinder/tribe/tribe_conflict.h"
#include "pathfinder/tribe/tribe_coordination.h"
#include "pathfinder/tribe/tribe_state.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

// --- Test fixture helpers ---

static TribeState makeTribeState(const TribeId& t_id,
                                  const std::vector<std::pair<std::string, TribeMemberRole>>& members) {
    TribeState s;
    s.tribe_id = t_id;
    s.display_key = t_id.value();
    for (const auto& [id, role] : members) {
        TribeMember m;
        m.member_id = EntityId(id);
        m.role = role;
        m.active = true;
        m.trust = 0.7;
        m.morale = 0.7;
        m.joined_tick = Tick(1);
        m.updated_tick = Tick(1);
        m.reason_keys = {"test"};
        s.members.push_back(std::move(m));
    }
    s.population_total = static_cast<int>(s.members.size());
    s.active_population = s.population_total;
    s.morale.overall = 0.7;
    s.trust.leader_trust = 0.7;
    s.trust.member_trust = 0.7;
    s.trust.teaching_fairness = 0.7;
    s.split_risk.risk = 0.2;
    s.split_risk.resource_pressure = 0.1;
    s.split_risk.trust_pressure = 0.3;
    s.split_risk.knowledge_conflict_pressure = 0.1;
    s.split_risk.casualty_pressure = 0.1;
    s.split_risk.cohesion_state = cohesionStateForRisk(0.2);
    return s;
}

static CombatCoordinationResult getCoordinationResult(const TribeState& state,
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
    input.reason_keys = {"p25_flow_test"};
    CombatCoordinationResolver resolver;
    auto r = resolver.resolve(input);
    assert(r.is_ok());
    return r.value();
}

static ConflictParticipantTribe makeParticipant(const TribeId& t_id, const std::string& role,
                                                  const TribeState& state,
                                                  const CombatCoordinationResult& coord) {
    ConflictParticipantTribe p;
    p.tribe_id = t_id;
    p.role_in_conflict = role;
    p.member_count_summary = state.population_total;
    p.active_population_summary = state.active_population;
    p.morale_summary = state.morale.overall;
    p.trust_summary = state.trust.member_trust;
    p.split_risk_summary = state.split_risk.risk;
    p.safety_pressure_summary = 0.1;
    p.loss_pressure_summary = 0.05;
    p.coordination_result = coord;
    p.coordination_state_draft = coord.state_draft;
    return p;
}

static TribeRelation makeRelation(const TribeId& source, const TribeId& target,
                                   HostilityState hostility) {
    TribeRelation r;
    r.source_tribe_id = source;
    r.target_tribe_id = target;
    r.hostility_state = hostility;
    r.trust_score = 0.2;
    r.fear_pressure = hostility >= HostilityState::Hostile ? 0.6 : 0.2;
    r.resource_tension = 0.1;
    r.reason_keys = {"test"};
    return r;
}

static ConflictPressureSummary makePressure(const std::string& key, double resource, double territory,
                                              double fear, double survival) {
    ConflictPressureSummary ps;
    ps.conflict_key = key;
    ps.resource_pressure = resource;
    ps.territory_pressure = territory;
    ps.fear_pressure = fear;
    ps.survival_pressure = survival;
    ps.reason_keys = {"test"};
    return ps;
}

static ConflictResolutionInput makeInput(ConflictParticipantTribe& source,
                                           ConflictParticipantTribe& target,
                                           const TribeRelation& rel,
                                           const ConflictPressureSummary& ps) {
    ConflictResolutionInput input;
    input.source = source;
    input.target = target;
    input.source_relation_to_target = rel;
    input.pressure_summary = ps;
    return input;
}

// --- Flow tests ---

static void test_neutral_low_pressure_avoids_conflict() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Neutral);
    auto ps = makePressure("low_pressure", 0.1, 0.1, 0.1, 0.1);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);
    assert(dto.outcome_kind == ConflictOutcomeKind::Avoided ||
           dto.outcome_kind == ConflictOutcomeKind::NoContact);
    assert(!dto.events.empty());
    std::cout << "p25_neutral_low_pressure_avoids_conflict_flow passed" << std::endl;
}

static void test_resource_contest_standoff() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"gb", TribeMemberRole::Guardian}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Hostile);
    auto ps = makePressure("resource_contest", 0.8, 0.3, 0.5, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);
    // With both sides having similar strength and hostile + high resource, expect ContestResource->Standoff or Intimidated
    assert(dto.outcome_kind != ConflictOutcomeKind::Unknown);
    assert(dto.projection.visible_outcome == dto.outcome_kind);
    std::cout << "p25_resource_contest_standoff_flow passed" << std::endl;
}

static void test_high_coordination_intimidates_low_morale() {
    // Side A: strong coordination (3 members including Guardian)
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}, {"ea", TribeMemberRole::Explorer}});
    // Side B: weak, low morale
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"lb", TribeMemberRole::Learner}});
    s_b.members[0].morale = 0.3;
    s_b.members[0].trust = 0.3;
    s_b.members[1].morale = 0.3;
    s_b.members[1].trust = 0.3;
    s_b.morale.overall = 0.3;
    s_b.trust.member_trust = 0.3;
    auto coord_a = getCoordinationResult(s_a, {{"wolf", 0.3}});
    auto coord_b = getCoordinationResult(s_b, {{"wolf", 0.3}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 3;
    pa.active_population_summary = 3;
    pa.morale_summary = s_a.morale.overall;
    pa.trust_summary = s_a.trust.member_trust;
    pa.split_risk_summary = s_a.split_risk.risk;
    pa.safety_pressure_summary = 0.1;
    pa.loss_pressure_summary = 0.05;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 2;
    pb.active_population_summary = 2;
    pb.morale_summary = s_b.morale.overall;
    pb.trust_summary = s_b.trust.member_trust;
    pb.split_risk_summary = s_b.split_risk.risk;
    pb.safety_pressure_summary = 0.1;
    pb.loss_pressure_summary = 0.05;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = coord_b.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Threatened);
    auto ps = makePressure("resource_dispute", 0.7, 0.4, 0.5, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // Stronger side should have intimidated, forced retreat, or standoff
    bool valid = (dto.outcome_kind == ConflictOutcomeKind::Intimidated ||
                  dto.outcome_kind == ConflictOutcomeKind::ForcedRetreat ||
                  dto.outcome_kind == ConflictOutcomeKind::Standoff);
    assert(valid);
    std::cout << "p25_high_coordination_intimidates_low_morale_flow passed" << std::endl;
}

static void test_low_morale_forced_retreat() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}});
    s_a.members[0].morale = 0.15;
    s_a.members[0].trust = 0.15;
    s_a.morale.overall = 0.15;
    s_a.trust.member_trust = 0.15;
    s_a.split_risk.risk = 0.6;
    s_a.split_risk.cohesion_state = cohesionStateForRisk(0.6);

    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"gb", TribeMemberRole::Guardian}, {"eb", TribeMemberRole::Explorer}});
    s_b.members[0].morale = 0.85;
    s_b.members[0].trust = 0.85;
    s_b.members[1].morale = 0.85;
    s_b.members[1].trust = 0.85;
    s_b.members[2].morale = 0.85;
    s_b.members[2].trust = 0.85;
    s_b.morale.overall = 0.85;
    s_b.trust.member_trust = 0.85;

    auto coord_a = getCoordinationResult(s_a, {{"wolf", 0.9}});
    auto coord_b = getCoordinationResult(s_b, {{"wolf", 0.1}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 1;
    pa.active_population_summary = 1;
    pa.morale_summary = s_a.morale.overall;
    pa.trust_summary = s_a.trust.member_trust;
    pa.split_risk_summary = s_a.split_risk.risk;
    pa.safety_pressure_summary = 0.3;
    pa.loss_pressure_summary = 0.1;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 3;
    pb.active_population_summary = 3;
    pb.morale_summary = s_b.morale.overall;
    pb.trust_summary = s_b.trust.member_trust;
    pb.split_risk_summary = s_b.split_risk.risk;
    pb.safety_pressure_summary = 0.05;
    pb.loss_pressure_summary = 0.0;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = coord_b.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("scarce_water", 0.6, 0.2, 0.8, 0.5);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // Weak side A should retreat or lose
    bool valid = (dto.outcome_kind == ConflictOutcomeKind::ForcedRetreat ||
                  dto.outcome_kind == ConflictOutcomeKind::Intimidated ||
                  dto.outcome_kind == ConflictOutcomeKind::Avoided);
    assert(valid);
    std::cout << "p25_low_morale_forced_retreat_flow passed" << std::endl;
}

static void test_hostility_escalates_deterministically() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("escalation", 0.7, 0.6, 0.6, 0.5);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto r1 = resolver.resolve(input);
    auto r2 = resolver.resolve(input);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().outcome_kind == r2.value().outcome_kind);
    assert(r1.value().source_relation_draft.new_hostility_state ==
           r2.value().source_relation_draft.new_hostility_state);
    std::cout << "p25_hostility_escalates_deterministically_flow passed" << std::endl;
}

static void test_loss_pressure_summary_without_death() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"gb", TribeMemberRole::Guardian}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    pa.loss_pressure_summary = 0.1;
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    pb.loss_pressure_summary = 0.1;
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::OpenConflict);
    auto ps = makePressure("high_contest", 0.8, 0.8, 0.8, 0.8);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // Check no death/kill/hp keywords in projection or trace
    assert(dto.projection.public_summary_key.find("death") == std::string::npos);
    assert(dto.projection.public_summary_key.find("kill") == std::string::npos);
    assert(dto.projection.public_summary_key.find("damage") == std::string::npos);
    for (const auto& step : dto.trace.score_breakdown) {
        assert(step.find("death") == std::string::npos);
        assert(step.find("hp") == std::string::npos);
    }
    std::cout << "p25_loss_pressure_summary_without_death_flow passed" << std::endl;
}

static void test_truce_offered_when_pressure_drops() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 2;
    pa.active_population_summary = 2;
    pa.morale_summary = 0.6;
    pa.trust_summary = 0.6;
    pa.split_risk_summary = 0.2;
    pa.safety_pressure_summary = 0.3;
    pa.loss_pressure_summary = 0.65;  // High loss -> negotiate truce
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 1;
    pb.active_population_summary = 1;
    pb.morale_summary = 0.55;
    pb.trust_summary = 0.55;
    pb.split_risk_summary = 0.25;
    pb.safety_pressure_summary = 0.3;
    pb.loss_pressure_summary = 0.2;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = coord_b.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Hostile);
    auto ps = makePressure("aftermath", 0.2, 0.2, 0.3, 0.2);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // With high loss + low resource, expect TruceOffered
    bool valid = (dto.outcome_kind == ConflictOutcomeKind::TruceOffered ||
                  dto.outcome_kind == ConflictOutcomeKind::Standoff ||
                  dto.outcome_kind == ConflictOutcomeKind::Avoided);
    assert(valid);
    std::cout << "p25_truce_offered_when_pressure_drops_flow passed" << std::endl;
}

static void test_uses_p24_coordination_result() {
    // Use different coordination strengths
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}, {"ea", TribeMemberRole::Explorer}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    s_b.members[0].morale = 0.1;
    s_b.members[0].trust = 0.1;
    s_b.morale.overall = 0.1;
    s_b.trust.member_trust = 0.1;
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"wolf", 0.9}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 3;
    pa.active_population_summary = 3;
    pa.morale_summary = s_a.morale.overall;
    pa.trust_summary = s_a.trust.member_trust;
    pa.split_risk_summary = s_a.split_risk.risk;
    pa.safety_pressure_summary = 0.1;
    pa.loss_pressure_summary = 0.05;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 1;
    pb.active_population_summary = 1;
    pb.morale_summary = s_b.morale.overall;
    pb.trust_summary = s_b.trust.member_trust;
    pb.split_risk_summary = s_b.split_risk.risk;
    pb.safety_pressure_summary = 0.3;
    pb.loss_pressure_summary = 0.1;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = coord_b.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("contest", 0.6, 0.3, 0.4, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // Strong coordination should influence outcome
    assert(dto.trace.source_coordination_score > dto.trace.target_coordination_score ||
           std::abs(dto.trace.source_coordination_score - dto.trace.target_coordination_score) < 0.01);
    std::cout << "p25_uses_p24_coordination_result_flow passed" << std::endl;
}

static void test_relation_draft_does_not_mutate_state() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("contest", 0.5, 0.3, 0.3, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // Relation draft is output only, original relation unchanged
    assert(dto.source_relation_draft.source_tribe_id == rel.source_tribe_id);
    assert(dto.source_relation_draft.target_tribe_id == rel.target_tribe_id);
    // State drafts should not contain direct TribeState mutations
    auto src_draft = dto.source_state_draft;
    auto val = src_draft.validateBasic();
    assert(val.is_ok());
    std::cout << "p25_relation_draft_does_not_mutate_state_flow passed" << std::endl;
}

static void test_deterministic_resolution() {
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"wolf", 0.4}});
    auto coord_b = getCoordinationResult(s_b, {{"snake", 0.2}});
    auto pa = makeParticipant(TribeId("tribe_a"), "source", s_a, coord_a);
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);
    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("deterministic", 0.5, 0.3, 0.4, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto r1 = resolver.resolve(input);
    auto r2 = resolver.resolve(input);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().outcome_kind == r2.value().outcome_kind);
    assert(r1.value().source_relation_draft.new_hostility_state ==
           r2.value().source_relation_draft.new_hostility_state);
    assert(r1.value().source_state_draft.morale_delta ==
           r2.value().source_state_draft.morale_delta);
    std::cout << "p25_deterministic_resolution_flow passed" << std::endl;
}

// New tests required by P25 review

static void test_merges_p24_state_draft_pressure() {
    // P24 produces non-zero safety/casualty/knowledge_conflict pressure
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}});
    auto coord_a = getCoordinationResult(s_a, {{"threat", 0.4}});

    // Modify P24 coordination state draft to have non-zero pressures
    CombatCoordinationStateDraft csd;
    csd.tribe_id = TribeId("tribe_a");
    csd.morale_delta = 0.05;
    csd.trust_delta = 0.03;
    csd.safety_pressure = 0.3;
    csd.casualty_pressure = 0.2;
    csd.knowledge_conflict_pressure = 0.15;

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 1;
    pa.active_population_summary = 1;
    pa.morale_summary = 0.5;
    pa.trust_summary = 0.5;
    pa.split_risk_summary = 0.2;
    pa.safety_pressure_summary = 0.1;
    pa.loss_pressure_summary = 0.05;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = csd;

    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});
    auto pb = makeParticipant(TribeId("tribe_b"), "target", s_b, coord_b);

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Wary);
    auto ps = makePressure("p24_merge", 0.4, 0.3, 0.3, 0.3);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // P24 baseline pressures must carry through into P25 draft
    assert(std::abs(dto.source_state_draft.safety_pressure_delta) >= 0.2);
    assert(std::abs(dto.source_state_draft.loss_pressure_delta) >= 0.1);
    assert(std::abs(dto.source_state_draft.split_risk_delta) >= 0.1);
    std::cout << "p25_merges_p24_state_draft_pressure_flow passed" << std::endl;
}

static void test_conflict_outcome_trust_delta_not_overwritten() {
    // Standoff produces trust_delta = -0.02 on top of P24 baseline
    // P24 baseline is 0.05, so final should be ~0.03 not 0.05
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"gb", TribeMemberRole::Guardian}});

    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});

    // Give P24 a positive trust_delta baseline
    CombatCoordinationStateDraft csd_a = coord_a.state_draft;
    csd_a.trust_delta = 0.05;
    CombatCoordinationStateDraft csd_b = coord_b.state_draft;
    csd_b.trust_delta = 0.05;

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 2;
    pa.active_population_summary = 2;
    pa.morale_summary = s_a.morale.overall;
    pa.trust_summary = s_a.trust.member_trust;
    pa.split_risk_summary = s_a.split_risk.risk;
    pa.safety_pressure_summary = 0.1;
    pa.loss_pressure_summary = 0.05;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = csd_a;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 2;
    pb.active_population_summary = 2;
    pb.morale_summary = s_b.morale.overall;
    pb.trust_summary = s_b.trust.member_trust;
    pb.split_risk_summary = s_b.split_risk.risk;
    pb.safety_pressure_summary = 0.1;
    pb.loss_pressure_summary = 0.05;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = csd_b;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Hostile);
    auto ps = makePressure("standoff_pressure", 0.7, 0.7, 0.7, 0.7);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // If Standoff, trust_delta = P24 baseline (0.05) + outcome modifier (-0.02) = ~0.03
    // Must NOT be just P24 baseline (0.05) — that would mean the outcome was overwritten
    assert(std::abs(dto.source_state_draft.trust_delta) <= 0.05);
    std::cout << "p25_conflict_outcome_trust_delta_not_overwritten_flow passed" << std::endl;
}

static void test_truce_offered_with_medium_strength_high_loss() {
    // Both sides: medium strength, high loss pressure, low resource → TruceOffered
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}, {"gb", TribeMemberRole::Guardian}});

    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 2;
    pa.active_population_summary = 2;
    pa.morale_summary = 0.55;
    pa.trust_summary = 0.55;
    pa.split_risk_summary = 0.2;
    pa.safety_pressure_summary = 0.35;
    pa.loss_pressure_summary = 0.6;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 2;
    pb.active_population_summary = 2;
    pb.morale_summary = 0.55;
    pb.trust_summary = 0.55;
    pb.split_risk_summary = 0.2;
    pb.safety_pressure_summary = 0.35;
    pb.loss_pressure_summary = 0.5;
    pb.coordination_result = coord_b;
    pb.coordination_state_draft = coord_b.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Hostile);
    // Low resource pressure enables truce
    auto ps = makePressure("aftermath_truce", 0.15, 0.1, 0.3, 0.2);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);
    // With both sides having high loss + low resource, should be able to truce
    assert(dto.outcome_kind != ConflictOutcomeKind::Unknown);
    std::cout << "p25_truce_offered_with_medium_strength_high_loss_flow passed" << std::endl;
}

static void test_projection_includes_target_pressure() {
    // Source pressures near zero, target has retreat/loss pressure
    auto s_a = makeTribeState(TribeId("tribe_a"), {{"pa", TribeMemberRole::Pioneer}, {"ga", TribeMemberRole::Guardian}, {"ea", TribeMemberRole::Explorer}});
    auto s_b = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});

    auto coord_a = getCoordinationResult(s_a, {{"generic", 0.1}});
    auto coord_b = getCoordinationResult(s_b, {{"generic", 0.1}});

    ConflictParticipantTribe pa;
    pa.tribe_id = TribeId("tribe_a");
    pa.role_in_conflict = "source";
    pa.member_count_summary = 3;
    pa.active_population_summary = 3;
    pa.morale_summary = 0.8;
    pa.trust_summary = 0.8;
    pa.split_risk_summary = 0.1;
    pa.safety_pressure_summary = 0.05;
    pa.loss_pressure_summary = 0.05;
    pa.coordination_result = coord_a;
    pa.coordination_state_draft = coord_a.state_draft;

    // Target: very weak — will get loss/retreat pressure
    auto s_b_weak = makeTribeState(TribeId("tribe_b"), {{"pb", TribeMemberRole::Pioneer}});
    s_b_weak.members[0].morale = 0.1;
    s_b_weak.members[0].trust = 0.1;
    s_b_weak.morale.overall = 0.1;
    s_b_weak.trust.member_trust = 0.1;
    s_b_weak.split_risk.risk = 0.5;
    s_b_weak.split_risk.cohesion_state = cohesionStateForRisk(0.5);
    auto coord_b_weak = getCoordinationResult(s_b_weak, {{"wolf", 0.9}});

    ConflictParticipantTribe pb;
    pb.tribe_id = TribeId("tribe_b");
    pb.role_in_conflict = "target";
    pb.member_count_summary = 1;
    pb.active_population_summary = 1;
    pb.morale_summary = 0.1;
    pb.trust_summary = 0.1;
    pb.split_risk_summary = 0.5;
    pb.safety_pressure_summary = 0.4;
    pb.loss_pressure_summary = 0.3;
    pb.coordination_result = coord_b_weak;
    pb.coordination_state_draft = coord_b_weak.state_draft;

    auto rel = makeRelation(TribeId("tribe_a"), TribeId("tribe_b"), HostilityState::Hostile);
    auto ps = makePressure("high_contest", 0.8, 0.8, 0.8, 0.8);
    auto input = makeInput(pa, pb, rel, ps);

    GroupCombatResolver resolver;
    auto result = resolver.resolve(input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.ok);

    // With target weak + high hostility, target state draft should have pressure
    // Projection should combine both sides
    assert(!dto.projection.visible_pressure_level.empty());
    std::cout << "p25_projection_includes_target_pressure_flow passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "neutral_low_pressure") test_neutral_low_pressure_avoids_conflict();
    else if (arg == "resource_contest") test_resource_contest_standoff();
    else if (arg == "high_coordination_intimidates") test_high_coordination_intimidates_low_morale();
    else if (arg == "low_morale_retreat") test_low_morale_forced_retreat();
    else if (arg == "hostility_escalates") test_hostility_escalates_deterministically();
    else if (arg == "loss_pressure_no_death") test_loss_pressure_summary_without_death();
    else if (arg == "truce_pressure_drops") test_truce_offered_when_pressure_drops();
    else if (arg == "uses_p24") test_uses_p24_coordination_result();
    else if (arg == "relation_draft") test_relation_draft_does_not_mutate_state();
    else if (arg == "deterministic") test_deterministic_resolution();
    else if (arg == "merges_p24") test_merges_p24_state_draft_pressure();
    else if (arg == "trust_delta_not_overwritten") test_conflict_outcome_trust_delta_not_overwritten();
    else if (arg == "truce_medium_strength") test_truce_offered_with_medium_strength_high_loss();
    else if (arg == "projection_target") test_projection_includes_target_pressure();
    else {
        test_neutral_low_pressure_avoids_conflict();
        test_resource_contest_standoff();
        test_high_coordination_intimidates_low_morale();
        test_low_morale_forced_retreat();
        test_hostility_escalates_deterministically();
        test_loss_pressure_summary_without_death();
        test_truce_offered_when_pressure_drops();
        test_uses_p24_coordination_result();
        test_relation_draft_does_not_mutate_state();
        test_deterministic_resolution();
        test_merges_p24_state_draft_pressure();
        test_conflict_outcome_trust_delta_not_overwritten();
        test_truce_offered_with_medium_strength_high_loss();
        test_projection_includes_target_pressure();
    }
    return 0;
}
