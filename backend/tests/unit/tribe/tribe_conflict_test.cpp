#include "pathfinder/tribe/tribe_conflict.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static CombatCoordinationResult makeValidCoordinationResult(const TribeId& t_id) {
    CombatCoordinationResult r;
    r.decision = "coordinated";
    r.intent.kind = GroupIntentKind::HoldTogether;
    r.intent.priority = 0.6;
    r.intent.participant_ids = {EntityId("member_a")};
    r.intent.summary_key = "test_intent";
    r.pack_tactic.kind = PackTacticKind::PairSupport;
    r.pack_tactic.quality = CoordinationQuality::Stable;
    r.pack_tactic.coordination_score = 0.65;
    r.pack_tactic.participant_ids = {EntityId("member_a")};
    r.pack_tactic.summary_key = "test_tactic";
    r.state_draft.tribe_id = t_id;
    r.projection.tribe_id = t_id;
    r.projection.intent_summary = "intent=test";
    r.projection.tactic_summary = "tactic=test";
    r.projection.participant_summary = "participants=test";
    r.projection.quality_summary = "quality=test";
    r.projection.risk_summary = "risk=test";
    r.trace.trace_key = "test_trace";
    r.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 0, 0, 0, 0.5, 0.0, 0.0, {"test"}});
    return r;
}

static CombatCoordinationStateDraft makeValidCoordinationStateDraft(const TribeId& t_id) {
    CombatCoordinationStateDraft d;
    d.tribe_id = t_id;
    d.morale_delta = 0.02;
    d.trust_delta = 0.01;
    d.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 0, 0, 0, 0.5, 0.0, 0.0, {"test"}});
    return d;
}

static ConflictParticipantTribe makeParticipant(const TribeId& t_id, const std::string& role) {
    ConflictParticipantTribe p;
    p.tribe_id = t_id;
    p.role_in_conflict = role;
    p.member_count_summary = 3;
    p.active_population_summary = 3;
    p.morale_summary = 0.7;
    p.trust_summary = 0.7;
    p.split_risk_summary = 0.1;
    p.safety_pressure_summary = 0.1;
    p.loss_pressure_summary = 0.05;
    p.coordination_result = makeValidCoordinationResult(t_id);
    p.coordination_state_draft = makeValidCoordinationStateDraft(t_id);
    return p;
}

// --- Unit tests ---

static void test_hostility_enum_roundtrip() {
    assert(toString(HostilityState::Neutral) == "neutral");
    assert(toString(HostilityState::Hostile) == "hostile");
    assert(toString(HostilityState::OpenConflict) == "open_conflict");
    assert(hostilityStateFromString("neutral").value() == HostilityState::Neutral);
    assert(hostilityStateFromString("wary").value() == HostilityState::Wary);
    assert(hostilityStateFromString("truce").value() == HostilityState::Truce);
    assert(hostilityStateFromString("invalid").is_error());
    std::cout << "tribe_conflict_hostility_enum_roundtrip passed" << std::endl;
}

static void test_conflict_intent_enum_roundtrip() {
    assert(toString(ConflictIntentKind::Avoid) == "avoid");
    assert(toString(ConflictIntentKind::ContestResource) == "contest_resource");
    assert(toString(ConflictIntentKind::NegotiateTruce) == "negotiate_truce");
    assert(conflictIntentKindFromString("avoid").value() == ConflictIntentKind::Avoid);
    assert(conflictIntentKindFromString("raid").value() == ConflictIntentKind::Raid);
    assert(conflictIntentKindFromString("invalid").is_error());
    std::cout << "tribe_conflict_intent_enum_roundtrip passed" << std::endl;
}

static void test_conflict_outcome_enum_roundtrip() {
    assert(toString(ConflictOutcomeKind::Standoff) == "standoff");
    assert(toString(ConflictOutcomeKind::Intimidated) == "intimidated");
    assert(conflictOutcomeKindFromString("forced_retreat").value() == ConflictOutcomeKind::ForcedRetreat);
    assert(conflictOutcomeKindFromString("truce_offered").value() == ConflictOutcomeKind::TruceOffered);
    assert(conflictOutcomeKindFromString("invalid").is_error());
    std::cout << "tribe_conflict_outcome_enum_roundtrip passed" << std::endl;
}

static void test_tribe_relation_validate() {
    TribeRelation rel;
    rel.source_tribe_id = TribeId("tribe_a");
    rel.target_tribe_id = TribeId("tribe_b");
    rel.hostility_state = HostilityState::Neutral;
    assert(rel.validateBasic().is_ok());

    rel.source_tribe_id = TribeId("");
    assert(rel.validateBasic().is_error());
    rel.source_tribe_id = TribeId("tribe_a");

    rel.hostility_state = HostilityState::Unknown;
    assert(rel.validateBasic().is_error());
    rel.hostility_state = HostilityState::Neutral;

    rel.source_tribe_id = TribeId("same");
    rel.target_tribe_id = TribeId("same");
    assert(rel.validateBasic().is_error());
    std::cout << "tribe_relation_validate passed" << std::endl;
}

static void test_conflict_pressure_summary_validate() {
    ConflictPressureSummary ps;
    ps.conflict_key = "resource_pressure";
    assert(ps.validateBasic().is_ok());

    ps.conflict_key = "";
    assert(ps.validateBasic().is_error());
    ps.conflict_key = "resource_pressure";

    ps.resource_pressure = 2.0;
    assert(ps.validateBasic().is_error());
    std::cout << "conflict_pressure_summary_validate passed" << std::endl;
}

static void test_conflict_participant_requires_coordination_result() {
    ConflictParticipantTribe p = makeParticipant(TribeId("tribe_a"), "source");
    assert(p.validateBasic().is_ok());

    // Missing coordination result: set intent to TestOnly
    p.coordination_result.intent.kind = GroupIntentKind::TestOnly;
    assert(p.coordination_result.validateBasic().is_error());

    // Restore
    p.coordination_result.intent.kind = GroupIntentKind::HoldTogether;
    assert(p.validateBasic().is_ok());
    std::cout << "conflict_participant_requires_coordination_result passed" << std::endl;
}

static void test_conflict_resolution_input_validate() {
    auto source = makeParticipant(TribeId("tribe_a"), "source");
    auto target = makeParticipant(TribeId("tribe_b"), "target");
    TribeRelation rel;
    rel.source_tribe_id = TribeId("tribe_a");
    rel.target_tribe_id = TribeId("tribe_b");
    rel.hostility_state = HostilityState::Neutral;
    ConflictPressureSummary ps;
    ps.conflict_key = "resource";

    ConflictResolutionInput input;
    input.source = source;
    input.target = target;
    input.source_relation_to_target = rel;
    input.pressure_summary = ps;

    assert(input.validateBasic().is_ok());

    // Same tribe
    input.target.tribe_id = TribeId("tribe_a");
    assert(input.validateBasic().is_error());
    std::cout << "conflict_resolution_input_validate passed" << std::endl;
}

static void test_conflict_resolution_result_validate() {
    ConflictResolutionResult result;
    result.ok = true;
    result.outcome_kind = ConflictOutcomeKind::Avoided;
    result.source_state_draft.tribe_id = TribeId("tribe_a");
    result.target_state_draft.tribe_id = TribeId("tribe_b");
    result.source_relation_draft.source_tribe_id = TribeId("tribe_a");
    result.source_relation_draft.target_tribe_id = TribeId("tribe_b");
    result.source_relation_draft.new_hostility_state = HostilityState::Neutral;
    result.projection.source_tribe_id = TribeId("tribe_a");
    result.projection.target_tribe_id = TribeId("tribe_b");
    result.projection.visible_hostility_state = HostilityState::Neutral;
    result.projection.visible_outcome = ConflictOutcomeKind::Avoided;
    result.projection.public_summary_key = "test_summary";
    result.trace.trace_id = EventId("trace_test");
    assert(result.validateBasic().is_ok());

    result.ok = false;
    result.outcome_kind = ConflictOutcomeKind::Standoff;
    assert(result.validateBasic().is_error());
    std::cout << "conflict_resolution_result_validate passed" << std::endl;
}

static void test_conflict_projection_safe_summary() {
    ConflictProjection proj;
    proj.source_tribe_id = TribeId("tribe_a");
    proj.target_tribe_id = TribeId("tribe_b");
    proj.visible_hostility_state = HostilityState::Wary;
    proj.visible_outcome = ConflictOutcomeKind::Standoff;
    proj.public_summary_key = "tribe_conflict_standoff";
    assert(proj.validateBasic().is_ok());

    proj.public_summary_key = "hidden_truth_summary";
    assert(proj.validateBasic().is_error());
    proj.public_summary_key = "tribe_conflict_standoff";

    proj.visible_hostility_state = HostilityState::TestOnly;
    assert(proj.validateBasic().is_error());
    std::cout << "conflict_projection_safe_summary passed" << std::endl;
}

static void test_conflict_trace_safe_summary() {
    ConflictTrace trace;
    trace.trace_id = EventId("trace_test");
    assert(trace.validateBasic().is_ok());

    trace.outcome_reason_keys = {"death_toll"};
    assert(trace.validateBasic().is_error());
    std::cout << "conflict_trace_safe_summary passed" << std::endl;
}

static void test_conflict_state_draft_unique_member_events() {
    // P25 first version: no member_event_drafts, uses tribe-level deltas only
    ConflictStateDraft draft;
    draft.tribe_id = TribeId("tribe_a");
    assert(draft.validateBasic().is_ok());

    draft.morale_delta = 2.0;
    assert(draft.validateBasic().is_error());
    std::cout << "conflict_state_draft_unique_member_events passed" << std::endl;
}

static void test_conflict_intent_rejects_invalid_stance_combo() {
    ConflictIntent intent;
    intent.tribe_id = TribeId("tribe_a");
    intent.intent_kind = ConflictIntentKind::Avoid;
    intent.stance_kind = ConflictStanceKind::Aggressive;
    intent.confidence = 0.5;
    intent.pressure_score = 0.3;
    assert(intent.validateBasic().is_error());

    intent.intent_kind = ConflictIntentKind::Retreat;
    intent.stance_kind = ConflictStanceKind::Aggressive;
    assert(intent.validateBasic().is_error());

    intent.intent_kind = ConflictIntentKind::NegotiateTruce;
    intent.stance_kind = ConflictStanceKind::Aggressive;
    assert(intent.validateBasic().is_error());

    // Valid combos should pass
    intent.intent_kind = ConflictIntentKind::Avoid;
    intent.stance_kind = ConflictStanceKind::Passive;
    assert(intent.validateBasic().is_ok());

    intent.intent_kind = ConflictIntentKind::Retreat;
    intent.stance_kind = ConflictStanceKind::Desperate;
    assert(intent.validateBasic().is_ok());

    intent.intent_kind = ConflictIntentKind::Intimidate;
    intent.stance_kind = ConflictStanceKind::Aggressive;
    assert(intent.validateBasic().is_ok());

    std::cout << "conflict_intent_rejects_invalid_stance_combo passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "hostility_enum_roundtrip") test_hostility_enum_roundtrip();
    else if (arg == "conflict_intent_enum_roundtrip") test_conflict_intent_enum_roundtrip();
    else if (arg == "conflict_outcome_enum_roundtrip") test_conflict_outcome_enum_roundtrip();
    else if (arg == "tribe_relation_validate") test_tribe_relation_validate();
    else if (arg == "conflict_pressure_summary_validate") test_conflict_pressure_summary_validate();
    else if (arg == "conflict_participant_requires_coordination") test_conflict_participant_requires_coordination_result();
    else if (arg == "conflict_resolution_input_validate") test_conflict_resolution_input_validate();
    else if (arg == "conflict_resolution_result_validate") test_conflict_resolution_result_validate();
    else if (arg == "conflict_projection_safe_summary") test_conflict_projection_safe_summary();
    else if (arg == "conflict_trace_safe_summary") test_conflict_trace_safe_summary();
    else if (arg == "conflict_state_draft_unique_member") test_conflict_state_draft_unique_member_events();
    else if (arg == "conflict_intent_invalid_stance") test_conflict_intent_rejects_invalid_stance_combo();
    else {
        test_hostility_enum_roundtrip();
        test_conflict_intent_enum_roundtrip();
        test_conflict_outcome_enum_roundtrip();
        test_tribe_relation_validate();
        test_conflict_pressure_summary_validate();
        test_conflict_participant_requires_coordination_result();
        test_conflict_resolution_input_validate();
        test_conflict_resolution_result_validate();
        test_conflict_projection_safe_summary();
        test_conflict_trace_safe_summary();
        test_conflict_state_draft_unique_member_events();
        test_conflict_intent_rejects_invalid_stance_combo();
    }
    return 0;
}
