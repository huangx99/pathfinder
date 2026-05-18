#include "pathfinder/civilization/civilization_types.h"
#include <cassert>
#include <iostream>
#include <set>

using namespace pathfinder::civilization;
using namespace pathfinder::foundation;

// 1. Civilization stage enum roundtrip
static void test_stage_enum_roundtrip() {
    assert(toString(CivilizationStage::Awakening) == "awakening");
    assert(toString(CivilizationStage::Foraging) == "foraging");
    assert(toString(CivilizationStage::FireKeeping) == "fire_keeping");
    assert(toString(CivilizationStage::ToolUsing) == "tool_using");
    assert(toString(CivilizationStage::Unknown) == "unknown");
    assert(toString(CivilizationStage::TestOnly) == "test_only");

    assert(civilizationStageFromString("awakening").value() == CivilizationStage::Awakening);
    assert(civilizationStageFromString("foraging").value() == CivilizationStage::Foraging);
    assert(civilizationStageFromString("proto_civilization").value() == CivilizationStage::ProtoCivilization);
    assert(civilizationStageFromString("invalid").is_error());
    assert(civilizationStageFromString("").is_error());
    std::cout << "civilization_stage_enum_roundtrip passed" << std::endl;
}

// 2. Capability type enum roundtrip
static void test_capability_type_enum_roundtrip() {
    assert(toString(CapabilityType::IdentifyEdible) == "identify_edible");
    assert(toString(CapabilityType::IdentifyDanger) == "identify_danger");
    assert(toString(CapabilityType::SafeForaging) == "safe_foraging");
    assert(toString(CapabilityType::ConflictDeescalation) == "conflict_deescalation");
    assert(toString(CapabilityType::Unknown) == "unknown");
    assert(toString(CapabilityType::TestOnly) == "test_only");

    assert(capabilityTypeFromString("identify_edible").value() == CapabilityType::IdentifyEdible);
    assert(capabilityTypeFromString("identify_danger").value() == CapabilityType::IdentifyDanger);
    assert(capabilityTypeFromString("conflict_deescalation").value() == CapabilityType::ConflictDeescalation);
    assert(capabilityTypeFromString("invalid").is_error());
    std::cout << "capability_type_enum_roundtrip passed" << std::endl;
}

// 3. Capability maturity enum roundtrip
static void test_capability_maturity_enum_roundtrip() {
    assert(toString(CapabilityMaturityState::Candidate) == "candidate");
    assert(toString(CapabilityMaturityState::Emerging) == "emerging");
    assert(toString(CapabilityMaturityState::Stable) == "stable");
    assert(toString(CapabilityMaturityState::Institutionalized) == "institutionalized");
    assert(toString(CapabilityMaturityState::Degraded) == "degraded");
    assert(toString(CapabilityMaturityState::Lost) == "lost");
    assert(toString(CapabilityMaturityState::Unknown) == "unknown");
    assert(toString(CapabilityMaturityState::TestOnly) == "test_only");

    assert(capabilityMaturityStateFromString("candidate").value() == CapabilityMaturityState::Candidate);
    assert(capabilityMaturityStateFromString("stable").value() == CapabilityMaturityState::Stable);
    assert(capabilityMaturityStateFromString("lost").value() == CapabilityMaturityState::Lost);
    assert(capabilityMaturityStateFromString("invalid").is_error());
    std::cout << "capability_maturity_enum_roundtrip passed" << std::endl;
}

// 4. Capability usability enum roundtrip
static void test_capability_usability_enum_roundtrip() {
    assert(toString(CapabilityUsabilityState::Usable) == "usable");
    assert(toString(CapabilityUsabilityState::BlockedByResource) == "blocked_by_resource");
    assert(toString(CapabilityUsabilityState::BlockedByCohesion) == "blocked_by_cohesion");
    assert(toString(CapabilityUsabilityState::BlockedByConflict) == "blocked_by_conflict");
    assert(toString(CapabilityUsabilityState::Suspended) == "suspended");
    assert(toString(CapabilityUsabilityState::Unknown) == "unknown");
    assert(toString(CapabilityUsabilityState::TestOnly) == "test_only");

    assert(capabilityUsabilityStateFromString("usable").value() == CapabilityUsabilityState::Usable);
    assert(capabilityUsabilityStateFromString("blocked_by_conflict").value() == CapabilityUsabilityState::BlockedByConflict);
    assert(capabilityUsabilityStateFromString("suspended").value() == CapabilityUsabilityState::Suspended);
    assert(capabilityUsabilityStateFromString("invalid").is_error());
    std::cout << "capability_usability_enum_roundtrip passed" << std::endl;
}

// 5. Capability change reason enum roundtrip
static void test_capability_change_reason_enum_roundtrip() {
    assert(toString(CapabilityChangeReason::NewEvidence) == "new_evidence");
    assert(toString(CapabilityChangeReason::RequirementMet) == "requirement_met");
    assert(toString(CapabilityChangeReason::RepeatedSuccess) == "repeated_success");
    assert(toString(CapabilityChangeReason::RepeatedFailure) == "repeated_failure");
    assert(toString(CapabilityChangeReason::ConflictChanged) == "conflict_changed");
    assert(toString(CapabilityChangeReason::Unknown) == "unknown");

    assert(capabilityChangeReasonFromString("new_evidence").value() == CapabilityChangeReason::NewEvidence);
    assert(capabilityChangeReasonFromString("conflict_changed").value() == CapabilityChangeReason::ConflictChanged);
    assert(capabilityChangeReasonFromString("invalid").is_error());
    std::cout << "capability_change_reason_enum_roundtrip passed" << std::endl;
}

// 6. Capability effect target enum roundtrip
static void test_capability_effect_target_enum_roundtrip() {
    assert(toString(CapabilityEffectTarget::Risk) == "risk");
    assert(toString(CapabilityEffectTarget::Knowledge) == "knowledge");
    assert(toString(CapabilityEffectTarget::TribeCohesion) == "tribe_cohesion");
    assert(toString(CapabilityEffectTarget::ConflictResolution) == "conflict_resolution");
    assert(toString(CapabilityEffectTarget::ActionAvailability) == "action_availability");
    assert(toString(CapabilityEffectTarget::Unknown) == "unknown");

    assert(capabilityEffectTargetFromString("risk").value() == CapabilityEffectTarget::Risk);
    assert(capabilityEffectTargetFromString("conflict_resolution").value() == CapabilityEffectTarget::ConflictResolution);
    assert(capabilityEffectTargetFromString("invalid").is_error());
    std::cout << "capability_effect_target_enum_roundtrip passed" << std::endl;
}

// 7. Effect operation type enum roundtrip
static void test_effect_operation_type_enum_roundtrip() {
    assert(toString(EffectOperationType::Add) == "add");
    assert(toString(EffectOperationType::Multiply) == "multiply");
    assert(toString(EffectOperationType::UnlockCandidate) == "unlock_candidate");
    assert(toString(EffectOperationType::EnableAction) == "enable_action");
    assert(toString(EffectOperationType::Unknown) == "unknown");

    assert(effectOperationTypeFromString("add").value() == EffectOperationType::Add);
    assert(effectOperationTypeFromString("multiply").value() == EffectOperationType::Multiply);
    assert(effectOperationTypeFromString("enable_action").value() == EffectOperationType::EnableAction);
    assert(effectOperationTypeFromString("invalid").is_error());
    std::cout << "effect_operation_type_enum_roundtrip passed" << std::endl;
}

// 8. Forbidden keys
static void test_civilization_forbidden_keys() {
    assert(containsCivilizationForbiddenKey("hidden_truth"));
    assert(containsCivilizationForbiddenKey("raw_state"));
    assert(containsCivilizationForbiddenKey("gamestate"));
    assert(containsCivilizationForbiddenKey("game_state"));
    assert(containsCivilizationForbiddenKey("savegame"));
    assert(containsCivilizationForbiddenKey("agentruntime"));
    assert(containsCivilizationForbiddenKey("unlocked"));
    assert(containsCivilizationForbiddenKey("unlock_bool"));
    assert(containsCivilizationForbiddenKey("bool_unlocked"));
    assert(containsCivilizationForbiddenKey("civilization_level"));
    assert(containsCivilizationForbiddenKey("stage_only_unlock"));
    assert(containsCivilizationForbiddenKey("frontend_unlock"));
    assert(containsCivilizationForbiddenKey("direct_unlock"));
    assert(containsCivilizationForbiddenKey("direct_mutate_state"));
    assert(containsCivilizationForbiddenKey("true_hp"));
    assert(containsCivilizationForbiddenKey("hp_delta"));
    assert(containsCivilizationForbiddenKey("hp_remaining"));
    assert(containsCivilizationForbiddenKey("actual_hp"));
    assert(containsCivilizationForbiddenKey("kill_count"));
    assert(containsCivilizationForbiddenKey("enemy_dead"));
    assert(containsCivilizationForbiddenKey("true_enemy_strength"));
    assert(containsCivilizationForbiddenKey("hidden_weapon"));

    // Safe keys should pass
    assert(!containsCivilizationForbiddenKey("membership"));
    assert(!containsCivilizationForbiddenKey("leadership"));
    assert(!containsCivilizationForbiddenKey("trust"));
    assert(!containsCivilizationForbiddenKey("coordination"));
    assert(!containsCivilizationForbiddenKey("knowledge"));
    assert(!containsCivilizationForbiddenKey("coverage"));
    assert(!containsCivilizationForbiddenKey("safe_foraging"));

    // Vector version
    assert(containsCivilizationForbiddenKey(std::vector<std::string>{"safe", "hidden_truth"}));
    assert(!containsCivilizationForbiddenKey(std::vector<std::string>{"safe", "trust", "knowledge"}));
    std::cout << "civilization_forbidden_keys passed" << std::endl;
}

// 9. CivilizationProgress validateBasic
static void test_progress_validate() {
    CivilizationProgress p;
    p.candidate_score = 0.5;
    p.emerging_score = 0.3;
    p.stable_score = 0.1;
    p.institutionalized_score = 0.0;
    assert(p.validateBasic().is_ok());

    p.candidate_score = -0.1;
    assert(p.validateBasic().is_error());
    p.candidate_score = 0.5;

    p.candidate_score = 1.5;
    assert(p.validateBasic().is_error());
    p.candidate_score = 0.5;

    p.success_count = -1;
    assert(p.validateBasic().is_error());
    p.success_count = 0;

    p.reason_keys = {"hidden_truth"};
    assert(p.validateBasic().is_error());
    p.reason_keys.clear();

    std::cout << "civilization_progress_validate passed" << std::endl;
}

// 10. CapabilityUnlockCondition validateBasic
static void test_capability_condition_validate() {
    CapabilityUnlockCondition c;
    c.condition_key = "test_condition";
    c.condition_type = "known_edible_count";
    c.required_score = 0.5;
    c.current_score = 0.3;
    assert(c.validateBasic().is_ok());

    c.condition_key = "";
    assert(c.validateBasic().is_error());
    c.condition_key = "test_condition";

    c.condition_type = "";
    assert(c.validateBasic().is_error());
    c.condition_type = "known_edible_count";

    c.required_score = -0.1;
    assert(c.validateBasic().is_error());
    c.required_score = 0.5;

    c.condition_key = "unlocked";
    assert(c.validateBasic().is_error());
    c.condition_key = "test_condition";

    c.condition_type = "hidden_truth";
    assert(c.validateBasic().is_error());
    c.condition_type = "stage";
    assert(c.validateBasic().is_error());
    c.condition_type = "not_a_supported_condition";
    assert(c.validateBasic().is_error());
    c.condition_type = "known_edible_count";

    CivilizationCapability cap;
    cap.capability_type = CapabilityType::IdentifyEdible;
    cap.display_key = "identify_edible";
    cap.domain_key = "knowledge";
    assert(cap.validateBasic().is_error());
    cap.required_conditions.push_back(c);
    assert(cap.validateBasic().is_ok());

    std::cout << "capability_condition_validate passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "stage_enum_roundtrip") test_stage_enum_roundtrip();
    else if (arg == "capability_type_enum_roundtrip") test_capability_type_enum_roundtrip();
    else if (arg == "maturity_enum_roundtrip") test_capability_maturity_enum_roundtrip();
    else if (arg == "usability_enum_roundtrip") test_capability_usability_enum_roundtrip();
    else if (arg == "change_reason_enum_roundtrip") test_capability_change_reason_enum_roundtrip();
    else if (arg == "effect_target_enum_roundtrip") test_capability_effect_target_enum_roundtrip();
    else if (arg == "effect_op_enum_roundtrip") test_effect_operation_type_enum_roundtrip();
    else if (arg == "forbidden_keys") test_civilization_forbidden_keys();
    else if (arg == "progress_validate") test_progress_validate();
    else if (arg == "condition_validate") test_capability_condition_validate();
    else {
        test_stage_enum_roundtrip();
        test_capability_type_enum_roundtrip();
        test_capability_maturity_enum_roundtrip();
        test_capability_usability_enum_roundtrip();
        test_capability_change_reason_enum_roundtrip();
        test_capability_effect_target_enum_roundtrip();
        test_effect_operation_type_enum_roundtrip();
        test_civilization_forbidden_keys();
        test_progress_validate();
        test_capability_condition_validate();
    }
    return 0;
}
