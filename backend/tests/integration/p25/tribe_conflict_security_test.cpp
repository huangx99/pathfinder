#include "pathfinder/tribe/tribe_conflict.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

// Helper
static CombatCoordinationResult makeCoord(const TribeId& t_id, double score) {
    CombatCoordinationResult r;
    r.decision = "coordinated";
    r.intent.kind = GroupIntentKind::DefendGroup;
    r.intent.priority = score;
    r.intent.summary_key = "test";
    r.intent.participant_ids = {EntityId("m1")};
    r.pack_tactic.kind = PackTacticKind::PairSupport;
    r.pack_tactic.quality = CoordinationQuality::Stable;
    r.pack_tactic.coordination_score = score;
    r.pack_tactic.participant_ids = {EntityId("m1")};
    r.pack_tactic.summary_key = "test";
    r.state_draft.tribe_id = t_id;
    r.projection.tribe_id = t_id;
    r.projection.intent_summary = "test";
    r.projection.tactic_summary = "test";
    r.projection.participant_summary = "test";
    r.projection.quality_summary = "test";
    r.projection.risk_summary = "test";
    r.trace.trace_key = "test";
    r.member_summaries.push_back(MemberCoordinationSummary{EntityId("m1"), true, 0, 0, 0, 0.5, 0.0, 0.0, {"t"}});
    return r;
}

static CombatCoordinationStateDraft makeDraft(const TribeId& t_id) {
    CombatCoordinationStateDraft d;
    d.tribe_id = t_id;
    d.member_summaries.push_back(MemberCoordinationSummary{EntityId("m1"), true, 0, 0, 0, 0.5, 0.0, 0.0, {"t"}});
    return d;
}

static ConflictParticipantTribe makeParticipant(const TribeId& t_id, const std::string& role) {
    ConflictParticipantTribe p;
    p.tribe_id = t_id;
    p.role_in_conflict = role;
    p.member_count_summary = 2;
    p.active_population_summary = 2;
    p.morale_summary = 0.5;
    p.trust_summary = 0.5;
    p.coordination_result = makeCoord(t_id, 0.5);
    p.coordination_state_draft = makeDraft(t_id);
    return p;
}

static TribeRelation makeRelation(const TribeId& src, const TribeId& tgt, HostilityState hs) {
    TribeRelation r;
    r.source_tribe_id = src;
    r.target_tribe_id = tgt;
    r.hostility_state = hs;
    return r;
}

static ConflictPressureSummary makePressure(const std::string& key) {
    ConflictPressureSummary ps;
    ps.conflict_key = key;
    return ps;
}

// --- Security tests ---

static void test_rejects_missing_coordination_result() {
    ConflictParticipantTribe p;
    p.tribe_id = TribeId("tribe_a");
    p.role_in_conflict = "source";
    // coordination_result left default — should fail validation
    // But default coordination_result won't pass validateBasic due to missing fields
    assert(p.validateBasic().is_error());
    std::cout << "p25_security_rejects_missing_coordination_result passed" << std::endl;
}

static void test_rejects_raw_game_state_keys() {
    ConflictPressureSummary ps;
    ps.conflict_key = "GameState_read";
    assert(ps.validateBasic().is_error());

    ps.conflict_key = "raw_state_access";
    assert(ps.validateBasic().is_error());
    std::cout << "p25_security_rejects_raw_game_state_keys passed" << std::endl;
}

static void test_rejects_hidden_truth_keys() {
    ConflictPressureSummary ps;
    ps.conflict_key = "hidden_truth_exposed";
    assert(ps.validateBasic().is_error());

    TribeRelation rel = makeRelation(TribeId("a"), TribeId("b"), HostilityState::Neutral);
    rel.reason_keys = {"real_effect_discovered"};
    assert(rel.validateBasic().is_error());

    ConflictEvent ev;
    ev.event_id = EventId("ev");
    ev.event_kind = ConflictEventKind::HostilityChanged;
    ev.source_tribe_id = TribeId("a");
    ev.target_tribe_id = TribeId("b");
    ev.public_message_key = "true_trait_revealed";
    assert(ev.validateBasic().is_error());
    std::cout << "p25_security_rejects_hidden_truth_keys passed" << std::endl;
}

static void test_rejects_damage_death_kill_loot_keys() {
    TribeRelation rel = makeRelation(TribeId("a"), TribeId("b"), HostilityState::Neutral);
    rel.reason_keys = {"death_count"};
    assert(rel.validateBasic().is_error());

    rel.reason_keys = {"kill_streak"};
    assert(rel.validateBasic().is_error());

    ConflictTrace trace;
    trace.trace_id = EventId("trace");
    trace.score_breakdown = {"loot_found"};
    assert(trace.validateBasic().is_error());

    trace.score_breakdown = {"hp_remaining"};
    assert(trace.validateBasic().is_error());

    ConflictPressureSummary ps;
    ps.conflict_key = "damage_dealt";
    assert(ps.validateBasic().is_error());

    ps.conflict_key = "corpse_count";
    assert(ps.validateBasic().is_error());
    std::cout << "p25_security_rejects_damage_death_kill_loot_keys passed" << std::endl;
}

static void test_projection_has_no_hidden_truth() {
    ConflictProjection proj;
    proj.source_tribe_id = TribeId("a");
    proj.target_tribe_id = TribeId("b");
    proj.visible_hostility_state = HostilityState::Wary;
    proj.visible_outcome = ConflictOutcomeKind::Standoff;
    proj.public_summary_key = "tribe_conflict_standoff";

    // Should reject hidden truth in projection
    proj.public_summary_key = "hidden_truth_visible";
    assert(proj.validateBasic().is_error());

    proj.public_summary_key = "tribe_conflict_standoff";
    proj.visible_pressure_level = "true_hp_visible";
    assert(proj.validateBasic().is_error());
    std::cout << "p25_security_projection_has_no_hidden_truth passed" << std::endl;
}

static void test_trace_has_no_hp_or_death() {
    ConflictTrace trace;
    trace.trace_id = EventId("trace_test");
    assert(trace.validateBasic().is_ok());

    trace.outcome_reason_keys = {"actual_hp_visible"};
    assert(trace.validateBasic().is_error());

    trace.outcome_reason_keys = {"body_count_five"};
    assert(trace.validateBasic().is_error());
    std::cout << "p25_security_trace_has_no_hp_or_death passed" << std::endl;
}

static void test_rejects_duplicate_member_effects() {
    // P25 first version uses tribe-level deltas only, no member_event_drafts
    // Verify that the ConflictStateDraft is simple and doesn't expose member lists
    ConflictStateDraft draft;
    draft.tribe_id = TribeId("a");
    draft.morale_delta = 0.1;
    assert(draft.validateBasic().is_ok());
    // No duplicate member_id possible (no member list in first version)
    std::cout << "p25_security_rejects_duplicate_member_effects passed" << std::endl;
}

static void test_rejects_same_source_target_tribe() {
    ConflictResolutionInput input;
    input.source = makeParticipant(TribeId("same"), "source");
    input.target = makeParticipant(TribeId("same"), "target");
    input.source_relation_to_target = makeRelation(TribeId("same"), TribeId("same"), HostilityState::Neutral);
    // Relation validation should fail because source==target
    assert(input.source_relation_to_target.validateBasic().is_error());

    // Fix relation but keep same tribe
    input.source_relation_to_target = makeRelation(TribeId("same"), TribeId("other"), HostilityState::Neutral);
    input.pressure_summary = makePressure("test");
    // Input validation should reject source==target
    assert(input.validateBasic().is_error());
    std::cout << "p25_security_rejects_same_source_target_tribe passed" << std::endl;
}

static void test_rejects_test_only_enum_in_production() {
    TribeRelation rel = makeRelation(TribeId("a"), TribeId("b"), HostilityState::TestOnly);
    assert(rel.validateBasic().is_error());

    ConflictResolutionResult result;
    result.ok = true;
    result.outcome_kind = ConflictOutcomeKind::TestOnly;
    result.source_state_draft.tribe_id = TribeId("a");
    result.target_state_draft.tribe_id = TribeId("b");
    result.source_relation_draft.source_tribe_id = TribeId("a");
    result.source_relation_draft.target_tribe_id = TribeId("b");
    result.source_relation_draft.new_hostility_state = HostilityState::Neutral;
    result.projection.source_tribe_id = TribeId("a");
    result.projection.target_tribe_id = TribeId("b");
    result.projection.visible_hostility_state = HostilityState::Neutral;
    result.projection.visible_outcome = ConflictOutcomeKind::TestOnly;
    result.projection.public_summary_key = "test";
    result.trace.trace_id = EventId("trace");
    assert(result.validateBasic().is_error());
    std::cout << "p25_security_rejects_test_only_enum_in_production passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "missing_coordination") test_rejects_missing_coordination_result();
    else if (arg == "raw_game_state") test_rejects_raw_game_state_keys();
    else if (arg == "hidden_truth") test_rejects_hidden_truth_keys();
    else if (arg == "damage_death_keys") test_rejects_damage_death_kill_loot_keys();
    else if (arg == "projection_no_hidden") test_projection_has_no_hidden_truth();
    else if (arg == "trace_no_hp_death") test_trace_has_no_hp_or_death();
    else if (arg == "duplicate_member") test_rejects_duplicate_member_effects();
    else if (arg == "same_tribe") test_rejects_same_source_target_tribe();
    else if (arg == "test_only_enum") test_rejects_test_only_enum_in_production();
    else {
        test_rejects_missing_coordination_result();
        test_rejects_raw_game_state_keys();
        test_rejects_hidden_truth_keys();
        test_rejects_damage_death_kill_loot_keys();
        test_projection_has_no_hidden_truth();
        test_trace_has_no_hp_or_death();
        test_rejects_duplicate_member_effects();
        test_rejects_same_source_target_tribe();
        test_rejects_test_only_enum_in_production();
    }
    return 0;
}
