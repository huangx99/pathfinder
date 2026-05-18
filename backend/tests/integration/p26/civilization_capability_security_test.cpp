#include "pathfinder/civilization/civilization_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::civilization;
using namespace pathfinder::foundation;

static CivilizationResolveInput makeGoodInput() {
    CivilizationResolveInput in;
    in.current_state.tribe_id = TribeId("sec_tribe");
    in.current_state.stage = CivilizationStage::Awakening;
    in.current_state.stage_confidence = 0.5;
    in.current_state.version = StateVersion(1);

    in.knowledge_summary.tribe_id = TribeId("sec_tribe");
    in.knowledge_summary.known_edible_count = 3;
    in.knowledge_summary.known_danger_count = 2;
    in.knowledge_summary.teachable_knowledge_count = 2;
    in.knowledge_summary.stable_tribe_knowledge_count = 2;
    in.knowledge_summary.conflicted_knowledge_count = 0;
    in.knowledge_summary.knowledge_ids = {KnowledgeId("sk1"), KnowledgeId("sk2")};

    in.propagation_summary.tribe_id = TribeId("sec_tribe");
    in.propagation_summary.coverage = 0.7;
    in.propagation_summary.success_rate = 0.75;
    in.propagation_summary.misunderstanding_rate = 0.1;

    in.coordination_summary.tribe_id = TribeId("sec_tribe");
    in.coordination_summary.coordination_score = 0.7;

    in.conflict_summary.tribe_id = TribeId("sec_tribe");
    in.conflict_summary.open_conflict_pressure = 0.2;
    in.conflict_summary.loss_pressure = 0.1;

    in.input_tick = Tick(10);

    CivilizationCapability def;
    def.capability_type = CapabilityType::IdentifyEdible;
    def.display_key = "identify_edible";
    def.domain_key = "knowledge";
    def.required_conditions = {
        {"known_edible_c", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"coverage_c", "coverage", 0.5, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def);

    return in;
}

// 1. Reject unlocked bool shortcut
static void test_rejects_unlocked_bool_shortcut() {
    // forbidden keys include "unlocked", "unlock_bool", "bool_unlocked"
    assert(containsCivilizationForbiddenKey("unlocked"));
    assert(containsCivilizationForbiddenKey("unlock_bool"));
    assert(containsCivilizationForbiddenKey("bool_unlocked"));

    // DTO with forbidden key in reason_keys should fail validation
    CivilizationCapability cap;
    cap.capability_type = CapabilityType::IdentifyEdible;
    cap.display_key = "test";
    cap.domain_key = "test";
    cap.required_conditions = {{"coverage_c", "coverage", 0.5, 0.0, false, {}, {}}};
    cap.reason_keys = {"unlocked"};
    assert(cap.validateBasic().is_error());

    // Without forbidden key, should pass
    cap.reason_keys = {"evidence_based"};
    assert(cap.validateBasic().is_ok());
    std::cout << "p26_security_rejects_unlocked_bool_shortcut passed" << std::endl;
}

// 2. Reject stage-only unlock
static void test_rejects_stage_only_unlock() {
    assert(containsCivilizationForbiddenKey("stage_only_unlock"));

    // Condition key must not contain forbidden tokens
    CapabilityUnlockCondition cond;
    cond.condition_key = "stage_only_unlock_condition";
    cond.condition_type = "stage";
    cond.required_score = 0.5;
    cond.current_score = 0.3;
    assert(cond.validateBasic().is_error());

    cond.condition_key = "coverage_based_condition";
    cond.condition_type = "coverage";
    assert(cond.validateBasic().is_ok());
    cond.condition_type = "stage";
    assert(cond.validateBasic().is_error());
    cond.condition_type = "unknown_condition";
    assert(cond.validateBasic().is_error());
    std::cout << "p26_security_rejects_stage_only_unlock passed" << std::endl;
}

// 3. Reject frontend unlock source
static void test_rejects_frontend_unlock_source() {
    assert(containsCivilizationForbiddenKey("frontend_unlock"));

    CivilizationCapability cap;
    cap.capability_type = CapabilityType::IdentifyEdible;
    cap.display_key = "test";
    cap.domain_key = "test";
    cap.required_conditions = {{"coverage_c", "coverage", 0.5, 0.0, false, {}, {}}};
    cap.reason_keys = {"frontend_unlock"};
    assert(cap.validateBasic().is_error());
    std::cout << "p26_security_rejects_frontend_unlock_source passed" << std::endl;
}

// 4. Reject hidden truth keys
static void test_rejects_hidden_truth_keys() {
    assert(containsCivilizationForbiddenKey("hidden_truth"));
    assert(containsCivilizationForbiddenKey("true_trait"));
    assert(containsCivilizationForbiddenKey("true_enemy_strength"));
    assert(containsCivilizationForbiddenKey("hidden_weapon"));

    CivilizationResolveInput in = makeGoodInput();
    in.reason_keys = {"hidden_truth"};
    auto result = CivilizationResolver().resolve(in);
    assert(result.is_ok());
    assert(!result.value().ok);

    CivilizationProjection projection;
    projection.tribe_id = TribeId("sec_tribe");
    projection.stage = CivilizationStage::Awakening;
    projection.active_effect_summary_keys = {"hidden_truth"};
    assert(projection.validateBasic().is_error());

    CivilizationTrace trace;
    trace.trace_id = EventId("trace1");
    trace.effect_steps = {"hidden_truth"};
    assert(trace.validateBasic().is_error());
    std::cout << "p26_security_rejects_hidden_truth_keys passed" << std::endl;
}

// 5. Reject raw game state keys
static void test_rejects_raw_game_state_keys() {
    assert(containsCivilizationForbiddenKey("raw_state"));
    assert(containsCivilizationForbiddenKey("gamestate"));
    assert(containsCivilizationForbiddenKey("game_state"));
    assert(containsCivilizationForbiddenKey("savegame"));
    assert(containsCivilizationForbiddenKey("save_game"));
    assert(containsCivilizationForbiddenKey("agentruntime"));
    assert(containsCivilizationForbiddenKey("agent_runtime"));

    CivilizationResolveInput in = makeGoodInput();
    in.knowledge_summary.reason_keys = {"raw_state_data"};
    auto result = CivilizationResolver().resolve(in);
    assert(result.is_ok());
    assert(!result.value().ok);
    std::cout << "p26_security_rejects_raw_game_state_keys passed" << std::endl;
}

// 6. Reject HP/death/kill/loot keys
static void test_rejects_hp_death_kill_loot_keys() {
    assert(containsCivilizationForbiddenKey("true_hp"));
    assert(containsCivilizationForbiddenKey("actual_hp"));
    assert(containsCivilizationForbiddenKey("hp_delta"));
    assert(containsCivilizationForbiddenKey("hp_remaining"));
    assert(containsCivilizationForbiddenKey("kill_count"));
    assert(containsCivilizationForbiddenKey("enemy_dead"));
    assert(containsCivilizationForbiddenKey("enemy_killed"));
    assert(containsCivilizationForbiddenKey("damage_direct"));
    assert(containsCivilizationForbiddenKey("death_count"));
    assert(containsCivilizationForbiddenKey("loot_drop"));

    // Safe keys should not be rejected
    assert(!containsCivilizationForbiddenKey("membership"));
    assert(!containsCivilizationForbiddenKey("leadership"));
    assert(!containsCivilizationForbiddenKey("trust"));
    assert(!containsCivilizationForbiddenKey("coordination"));
    assert(!containsCivilizationForbiddenKey("hp")); // "hp" alone is safe
    std::cout << "p26_security_rejects_hp_death_kill_loot_keys passed" << std::endl;
}

// 7. Reject direct state mutation effect
static void test_rejects_direct_state_mutation_effect() {
    assert(containsCivilizationForbiddenKey("direct_mutate_state"));
    assert(containsCivilizationForbiddenKey("direct_unlock"));

    // Effect definition must not have forbidden effect key
    CapabilityEffectDefinition eff;
    eff.effect_key = "direct_mutate_state";
    eff.target = CapabilityEffectTarget::Risk;
    eff.operation = EffectOperationType::Multiply;
    eff.value_key = "test";
    assert(eff.validateBasic().is_error());

    eff.effect_key = "reduce_unknown_food_risk";
    assert(eff.validateBasic().is_ok());

    CapabilityEffectDraft draft;
    draft.draft_id = EventId("draft1");
    draft.tribe_id = TribeId("sec_tribe");
    draft.capability_type = CapabilityType::IdentifyEdible;
    draft.effect_key = "frontend_unlock";
    draft.target = CapabilityEffectTarget::Risk;
    draft.operation = EffectOperationType::Multiply;
    draft.value_key = "safe_value";
    draft.strength = 0.5;
    assert(draft.validateBasic().is_error());

    CivilizationEventDraft event;
    event.event_id = EventId("event1");
    event.event_type_key = "safe_event";
    event.tribe_id = TribeId("sec_tribe");
    event.message_key = "frontend_unlock";
    assert(event.validateBasic().is_error());
    std::cout << "p26_security_rejects_direct_state_mutation_effect passed" << std::endl;
}

// 8. Reject duplicate capability type in state
static void test_rejects_duplicate_capability_state() {
    CivilizationState state;
    state.tribe_id = TribeId("sec_tribe");
    state.stage = CivilizationStage::Awakening;
    state.stage_confidence = 0.5;

    CivilizationCapabilityState cap1;
    cap1.tribe_id = TribeId("sec_tribe");
    cap1.capability_type = CapabilityType::IdentifyEdible;
    cap1.maturity = CapabilityMaturityState::Candidate;
    cap1.usability = CapabilityUsabilityState::Usable;
    cap1.progress.candidate_score = 0.5;
    state.capabilities.push_back(cap1);

    CivilizationCapabilityState cap2;
    cap2.tribe_id = TribeId("sec_tribe");
    cap2.capability_type = CapabilityType::IdentifyEdible; // DUPLICATE
    cap2.maturity = CapabilityMaturityState::Emerging;
    cap2.usability = CapabilityUsabilityState::Usable;
    cap2.progress.candidate_score = 0.6;
    state.capabilities.push_back(cap2);

    assert(state.validateBasic().is_error());
    std::cout << "p26_security_rejects_duplicate_capability_state passed" << std::endl;
}

// 9. Reject TestOnly enum in production (validateBasic rejects Unknown/TestOnly)
static void test_rejects_test_only_enum_in_production() {
    // Capability type TestOnly should be rejected in validateBasic
    CivilizationCapability cap;
    cap.capability_type = CapabilityType::TestOnly;
    cap.display_key = "test";
    cap.domain_key = "test";
    assert(cap.validateBasic().is_error());

    cap.capability_type = CapabilityType::Unknown;
    assert(cap.validateBasic().is_error());

    cap.capability_type = CapabilityType::IdentifyEdible;
    cap.required_conditions = {{"coverage_c", "coverage", 0.5, 0.0, false, {}, {}}};
    assert(cap.validateBasic().is_ok());
    std::cout << "p26_security_rejects_test_only_enum_in_production passed" << std::endl;
}

// 10. Reject single success as stable
static void test_rejects_single_success_as_stable() {
    CivilizationResolver resolver;

    auto in = makeGoodInput();
    // Minimal data: only 1 known edible, barely any coverage
    in.knowledge_summary.known_edible_count = 1;
    in.knowledge_summary.teachable_knowledge_count = 1;
    in.propagation_summary.coverage = 0.2;
    in.propagation_summary.success_rate = 0.2;
    in.propagation_summary.misunderstanding_rate = 0.6;

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    // Single success should not produce Stable capability
    for (const auto& cap : result.value().updated_state.capabilities) {
        assert(cap.maturity != CapabilityMaturityState::Stable);
        assert(cap.maturity != CapabilityMaturityState::Institutionalized);
    }
    std::cout << "p26_security_rejects_single_success_as_stable passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "unlocked_bool") test_rejects_unlocked_bool_shortcut();
    else if (arg == "stage_only_unlock") test_rejects_stage_only_unlock();
    else if (arg == "frontend_unlock") test_rejects_frontend_unlock_source();
    else if (arg == "hidden_truth") test_rejects_hidden_truth_keys();
    else if (arg == "raw_game_state") test_rejects_raw_game_state_keys();
    else if (arg == "hp_death_kill_loot") test_rejects_hp_death_kill_loot_keys();
    else if (arg == "direct_state_mutation") test_rejects_direct_state_mutation_effect();
    else if (arg == "duplicate_capability") test_rejects_duplicate_capability_state();
    else if (arg == "test_only_enum") test_rejects_test_only_enum_in_production();
    else if (arg == "single_success") test_rejects_single_success_as_stable();
    else {
        test_rejects_unlocked_bool_shortcut();
        test_rejects_stage_only_unlock();
        test_rejects_frontend_unlock_source();
        test_rejects_hidden_truth_keys();
        test_rejects_raw_game_state_keys();
        test_rejects_hp_death_kill_loot_keys();
        test_rejects_direct_state_mutation_effect();
        test_rejects_duplicate_capability_state();
        test_rejects_test_only_enum_in_production();
        test_rejects_single_success_as_stable();
    }
    return 0;
}
