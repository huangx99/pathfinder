#include "pathfinder/tribe/tribe_coordination.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static void test_forbidden_keys() {
    ThreatPressureSummary threat;
    threat.threat_id = EntityId("threat1");
    threat.threat_key = "hidden_truth";
    threat.reason_keys = {"test"};
    assert(threat.validateBasic().is_error());

    threat.threat_key = "wolf_like_threat";
    threat.reason_keys = {"raw_state"};
    assert(threat.validateBasic().is_error());

    CombatCoordinationInput input;
    input.tribe_id = TribeId("tribe_test");
    input.input_tick = Tick(1);
    input.reason_keys = {"GameState"};
    assert(input.validateBasic().is_error());

    GroupIntent intent;
    intent.kind = GroupIntentKind::HoldTogether;
    intent.summary_key = "random_hit";
    assert(intent.validateBasic().is_error());

    intent.summary_key = "valid_summary";
    intent.priority = 0.5;
    intent.reason_keys = {"kill_result"};
    assert(intent.validateBasic().is_error());
    std::cout << "p24_security_forbidden_keys passed" << std::endl;
}

static void test_rejects_damage_or_death() {
    CombatCoordinationResult result;
    result.decision = "coordinated";
    result.intent.kind = GroupIntentKind::HoldTogether;
    result.intent.priority = 0.5;
    result.intent.summary_key = "intent_summary";
    result.intent.participant_ids = {EntityId("member_a")};
    result.pack_tactic.kind = PackTacticKind::PairSupport;
    result.pack_tactic.quality = CoordinationQuality::Stable;
    result.pack_tactic.participant_ids = {EntityId("member_a")};
    result.pack_tactic.summary_key = "pack_tactic_summary";
    result.state_draft.tribe_id = TribeId("tribe_test");
    result.trace.trace_key = "test_trace";

    CombatCoordinationProjectionBuilder builder;
    auto proj_result = builder.build(result);
    assert(proj_result.is_ok());
    auto proj = proj_result.value();

    assert(proj.intent_summary.find("damage") == std::string::npos);
    assert(proj.tactic_summary.find("death") == std::string::npos);
    assert(proj.quality_summary.find("kill") == std::string::npos);
    assert(proj.risk_summary.find("loot") == std::string::npos);

    // Reason keys with damage/death/kill semantics rejected
    result.reason_keys = {"true_hp"};
    assert(result.validateBasic().is_error());
    result.reason_keys.clear();
    result.warning_keys = {"actual_damage"};
    assert(result.validateBasic().is_error());
    std::cout << "p24_security_rejects_damage_or_death passed" << std::endl;
}

static void test_rejects_duplicate_member_effects() {
    CombatCoordinationStateDraft draft;
    draft.tribe_id = TribeId("tribe_test");
    draft.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 1, 0, 0, 0.5, 0.0, 0.0, {"test"}});
    draft.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 0, 1, 0, 0.4, 0.0, 0.0, {"test"}});
    assert(draft.validateBasic().is_error());

    CombatCoordinationResult result;
    result.decision = "coordinated";
    result.intent.kind = GroupIntentKind::HoldTogether;
    result.intent.priority = 0.5;
    result.intent.summary_key = "test_intent";
    result.intent.participant_ids = {EntityId("member_a")};
    result.pack_tactic.kind = PackTacticKind::PairSupport;
    result.pack_tactic.quality = CoordinationQuality::Stable;
    result.pack_tactic.participant_ids = {EntityId("member_a")};
    result.pack_tactic.summary_key = "test_tactic";
    result.state_draft.tribe_id = TribeId("tribe_test");
    result.projection.tribe_id = TribeId("tribe_test");
    result.projection.intent_summary = "intent=test";
    result.projection.tactic_summary = "tactic=test";
    result.projection.participant_summary = "participants=test";
    result.projection.quality_summary = "quality=test";
    result.projection.risk_summary = "risk=test";
    result.trace.trace_key = "test_trace";
    result.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 1, 0, 0, 0.5, 0.0, 0.0, {"test"}});
    result.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 0, 1, 0, 0.4, 0.0, 0.0, {"test"}});
    assert(result.validateBasic().is_error());
    std::cout << "p24_security_rejects_duplicate_member_effects passed" << std::endl;
}

static void test_rejects_test_only_enums() {
    GroupIntent intent;
    intent.kind = GroupIntentKind::TestOnly;
    intent.summary_key = "test";
    assert(intent.validateBasic().is_error());

    CombatParticipant cp;
    cp.member_id = EntityId("member_a");
    cp.role = TribeMemberRole::TestOnly;
    assert(cp.validateBasic().is_error());

    PackTactic pt;
    pt.kind = PackTacticKind::None;
    pt.quality = CoordinationQuality::TestOnly;
    pt.summary_key = "test";
    assert(pt.validateBasic().is_error());
    std::cout << "p24_security_rejects_test_only_enums passed" << std::endl;
}

static void test_empty_ids_rejected() {
    ThreatPressureSummary threat;
    threat.threat_id = EntityId("");
    threat.threat_key = "wolf";
    assert(threat.validateBasic().is_error());

    CombatParticipant cp;
    cp.member_id = EntityId("");
    cp.role = TribeMemberRole::Pioneer;
    assert(cp.validateBasic().is_error());

    AssistAction aa;
    aa.kind = AssistActionKind::Warn;
    aa.actor_member_id = EntityId("");
    aa.target_member_id = EntityId("member_b");
    aa.summary_key = "test";
    assert(aa.validateBasic().is_error());
    std::cout << "p24_security_empty_ids_rejected passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "forbidden_keys") test_forbidden_keys();
    else if (arg == "rejects_damage_or_death") test_rejects_damage_or_death();
    else if (arg == "rejects_duplicate_member_effects") test_rejects_duplicate_member_effects();
    else if (arg == "rejects_test_only_enums") test_rejects_test_only_enums();
    else if (arg == "empty_ids_rejected") test_empty_ids_rejected();
    else {
        test_forbidden_keys();
        test_rejects_damage_or_death();
        test_rejects_duplicate_member_effects();
        test_rejects_test_only_enums();
        test_empty_ids_rejected();
    }
    return 0;
}
