#include "pathfinder/tribe/tribe_coordination.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static void test_intent_enum_roundtrip() {
    assert(toString(GroupIntentKind::HoldTogether) == "hold_together");
    assert(toString(GroupIntentKind::DefendGroup) == "defend_group");
    assert(groupIntentKindFromString("hold_together").value() == GroupIntentKind::HoldTogether);
    assert(groupIntentKindFromString("focus_threat").value() == GroupIntentKind::FocusThreat);
    assert(groupIntentKindFromString("invalid").is_error());
    std::cout << "tribe_coordination_intent_enum_roundtrip passed" << std::endl;
}

static void test_action_enum_roundtrip() {
    assert(toString(AssistActionKind::Warn) == "warn");
    assert(toString(DefendActionKind::BlockThreat) == "block_threat");
    assert(toString(PackTacticKind::FocusPressure) == "focus_pressure");
    assert(assistActionKindFromString("warn").value() == AssistActionKind::Warn);
    assert(defendActionKindFromString("block_threat").value() == DefendActionKind::BlockThreat);
    assert(packTacticKindFromString("focus_pressure").value() == PackTacticKind::FocusPressure);
    assert(assistActionKindFromString("invalid").is_error());
    assert(defendActionKindFromString("invalid").is_error());
    assert(packTacticKindFromString("invalid").is_error());
    std::cout << "tribe_coordination_action_enum_roundtrip passed" << std::endl;
}

static void test_quality_threshold() {
    assert(qualityForScore(0.0) == CoordinationQuality::Failed);
    assert(qualityForScore(0.19) == CoordinationQuality::Failed);
    assert(qualityForScore(0.20) == CoordinationQuality::Weak);
    assert(qualityForScore(0.44) == CoordinationQuality::Weak);
    assert(qualityForScore(0.45) == CoordinationQuality::Stable);
    assert(qualityForScore(0.69) == CoordinationQuality::Stable);
    assert(qualityForScore(0.70) == CoordinationQuality::Strong);
    assert(qualityForScore(1.0) == CoordinationQuality::Strong);
    std::cout << "tribe_coordination_quality_threshold passed" << std::endl;
}

static void test_input_validate() {
    CombatCoordinationInput input;
    TribeState state;
    state.tribe_id = TribeId("tribe_alpha");
    state.display_key = "tribe_alpha";
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(10);
    input.tribe_state = state;
    assert(input.validateBasic().is_ok());

    input.tribe_id = TribeId("");
    assert(input.validateBasic().is_error());
    input.tribe_id = TribeId("tribe_alpha");

    input.tribe_state.tribe_id = TribeId("other");
    assert(input.validateBasic().is_error());
    input.tribe_state.tribe_id = TribeId("tribe_alpha");

    input.threats.push_back(ThreatPressureSummary{EntityId("threat1"), "wolf_like_threat", 0.5, 0.3, 0.2, {}, {"test"}});
    input.threats.push_back(ThreatPressureSummary{EntityId("threat1"), "wolf_like_threat", 0.5, 0.3, 0.2, {}, {"test"}});
    assert(input.validateBasic().is_error());
    std::cout << "tribe_coordination_input_validate passed" << std::endl;
}

static void test_result_validate() {
    CombatCoordinationResult result;
    result.decision = "coordinated";
    result.intent.kind = GroupIntentKind::HoldTogether;
    result.intent.priority = 0.5;
    result.intent.summary_key = "test_intent";
    result.pack_tactic.kind = PackTacticKind::PairSupport;
    result.pack_tactic.quality = CoordinationQuality::Stable;
    result.pack_tactic.summary_key = "test_tactic";
    result.state_draft.tribe_id = TribeId("tribe_alpha");
    result.projection.tribe_id = TribeId("tribe_alpha");
    result.projection.intent_summary = "intent=hold_together";
    result.projection.tactic_summary = "tactic=pair_support";
    result.projection.participant_summary = "participants=a";
    result.projection.quality_summary = "quality=stable";
    result.projection.risk_summary = "risk_reduction=0.5";
    result.trace.trace_key = "test_trace";

    auto val = result.validateBasic();
    // decision=coordinated requires pack_tactic to have participant_ids
    result.pack_tactic.participant_ids = {EntityId("member_a")};
    result.intent.participant_ids = {EntityId("member_a")};
    assert(result.validateBasic().is_ok());

    result.decision = "invalid_decision";
    assert(result.validateBasic().is_error());
    std::cout << "tribe_coordination_result_validate passed" << std::endl;
}

static void test_projection_safe_summary() {
    CombatCoordinationResult result;
    result.decision = "coordinated";
    result.intent.kind = GroupIntentKind::DefendGroup;
    result.intent.priority = 0.6;
    result.intent.participant_ids = {EntityId("guardian"), EntityId("pioneer")};
    result.intent.summary_key = "test";
    result.pack_tactic.kind = PackTacticKind::RotatingDefense;
    result.pack_tactic.quality = CoordinationQuality::Stable;
    result.pack_tactic.coordination_score = 0.65;
    result.pack_tactic.risk_reduction = 0.3;
    result.pack_tactic.summary_key = "test_tactic";
    result.pack_tactic.participant_ids = {EntityId("guardian"), EntityId("pioneer")};
    result.state_draft.tribe_id = TribeId("tribe_alpha");
    result.trace.trace_key = "test_trace";

    CombatCoordinationProjectionBuilder builder;
    auto proj_result = builder.build(result);
    assert(proj_result.is_ok());
    auto proj = proj_result.value();
    assert(proj.intent_summary.find("defend_group") != std::string::npos);
    assert(proj.tactic_summary.find("rotating_defense") != std::string::npos);
    assert(proj.validateBasic().is_ok());
    std::cout << "tribe_coordination_projection_safe_summary passed" << std::endl;
}

static void test_member_summary_unique() {
    CombatCoordinationStateDraft draft;
    draft.tribe_id = TribeId("tribe_alpha");
    draft.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 1, 0, 0, 0.5, 0.0, 0.0, {"test"}});
    draft.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_b"), true, 0, 1, 0, 0.6, 0.0, 0.0, {"test"}});
    assert(draft.validateBasic().is_ok());

    draft.member_summaries.push_back(MemberCoordinationSummary{EntityId("member_a"), true, 0, 0, 1, 0.4, 0.0, 0.0, {"test"}});
    assert(draft.validateBasic().is_error());
    std::cout << "tribe_coordination_member_summary_unique passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "intent_enum_roundtrip") test_intent_enum_roundtrip();
    else if (arg == "action_enum_roundtrip") test_action_enum_roundtrip();
    else if (arg == "quality_threshold") test_quality_threshold();
    else if (arg == "input_validate") test_input_validate();
    else if (arg == "result_validate") test_result_validate();
    else if (arg == "projection_safe_summary") test_projection_safe_summary();
    else if (arg == "member_summary_unique") test_member_summary_unique();
    else {
        test_intent_enum_roundtrip();
        test_action_enum_roundtrip();
        test_quality_threshold();
        test_input_validate();
        test_result_validate();
        test_projection_safe_summary();
        test_member_summary_unique();
    }
    return 0;
}
