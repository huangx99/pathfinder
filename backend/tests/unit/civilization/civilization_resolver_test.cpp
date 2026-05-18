#include "pathfinder/civilization/civilization_state.h"
#include <cassert>
#include <iostream>
#include <set>

using namespace pathfinder::civilization;
using namespace pathfinder::foundation;

static CivilizationResolveInput makeFullInput() {
    CivilizationResolveInput in;
    in.current_state.tribe_id = TribeId("tribe_test");
    in.current_state.stage = CivilizationStage::Awakening;
    in.current_state.stage_confidence = 0.5;
    in.current_state.version = StateVersion(1);

    // Add IdentifyEdible(Emerging) and IdentifyDanger(Emerging) to enable SafeForaging
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("tribe_test");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Emerging;
    ie.usability = CapabilityUsabilityState::Usable;
    ie.stability = 0.7;
    ie.progress.success_count = 3;
    ie.evidence.knowledge_ids = {KnowledgeId("k1")};
    in.current_state.capabilities.push_back(ie);

    CivilizationCapabilityState id;
    id.tribe_id = TribeId("tribe_test");
    id.capability_type = CapabilityType::IdentifyDanger;
    id.maturity = CapabilityMaturityState::Emerging;
    id.usability = CapabilityUsabilityState::Usable;
    id.stability = 0.6;
    id.progress.success_count = 2;
    id.evidence.knowledge_ids = {KnowledgeId("k2")};
    in.current_state.capabilities.push_back(id);

    in.knowledge_summary.tribe_id = TribeId("tribe_test");
    in.knowledge_summary.known_edible_count = 4;
    in.knowledge_summary.known_danger_count = 2;
    in.knowledge_summary.teachable_knowledge_count = 3;
    in.knowledge_summary.stable_tribe_knowledge_count = 2;
    in.knowledge_summary.conflicted_knowledge_count = 0;
    in.knowledge_summary.knowledge_ids = {KnowledgeId("k1"), KnowledgeId("k2"), KnowledgeId("k3")};

    in.propagation_summary.tribe_id = TribeId("tribe_test");
    in.propagation_summary.coverage = 0.7;
    in.propagation_summary.success_rate = 0.75;
    in.propagation_summary.misunderstanding_rate = 0.1;

    in.coordination_summary.tribe_id = TribeId("tribe_test");
    in.coordination_summary.coordination_score = 0.7;
    in.coordination_summary.stable_coordination_count = 1;

    in.conflict_summary.tribe_id = TribeId("tribe_test");
    in.conflict_summary.truce_success_count = 1;
    in.conflict_summary.forced_retreat_low_loss_count = 2;
    in.conflict_summary.open_conflict_pressure = 0.2;
    in.conflict_summary.loss_pressure = 0.1;
    in.conflict_summary.source_event_ids = {EventId("ce1")};

    in.input_tick = Tick(10);

    // Capability definitions for the 4 types
    CivilizationCapability def_ie;
    def_ie.capability_type = CapabilityType::IdentifyEdible;
    def_ie.display_key = "identify_edible";
    def_ie.domain_key = "knowledge";
    def_ie.required_conditions = {
        {"known_edible_count", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"coverage", "coverage", 0.5, 0.0, false, {}, {}},
        {"misunderstanding_rate", "misunderstanding_rate", 0.25, 0.0, false, {}, {}}
    };
    // Manually set keys to avoid brace init issues
    for (auto& c : def_ie.required_conditions) {
        c.condition_key = c.condition_type;
    }
    {
        CapabilityEffectDefinition eff;
        eff.effect_key = "reduce_unknown_food_risk";
        eff.target = CapabilityEffectTarget::Risk;
        eff.operation = EffectOperationType::Multiply;
        eff.value_key = "reduce_unknown_food_risk";
        eff.value_number = 0.90;
        def_ie.effect_definitions.push_back(eff);
    }
    in.capability_definitions.push_back(def_ie);

    CivilizationCapability def_id;
    def_id.capability_type = CapabilityType::IdentifyDanger;
    def_id.display_key = "identify_danger";
    def_id.domain_key = "knowledge";
    def_id.required_conditions = {
        {"known_danger_count", "known_danger_count", 1.0, 0.0, false, {}, {}},
        {"coverage", "coverage", 0.45, 0.0, false, {}, {}},
        {"misunderstanding_rate", "misunderstanding_rate", 0.25, 0.0, false, {}, {}}
    };
    for (auto& c : def_id.required_conditions) c.condition_key = c.condition_type;
    in.capability_definitions.push_back(def_id);

    CivilizationCapability def_sf;
    def_sf.capability_type = CapabilityType::SafeForaging;
    def_sf.display_key = "safe_foraging";
    def_sf.domain_key = "foraging";
    def_sf.required_conditions = {
        {"known_edible_count", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"known_danger_count", "known_danger_count", 1.0, 0.0, false, {}, {}},
        {"coverage", "coverage", 0.55, 0.0, false, {}, {}},
        {"success_rate", "success_rate", 0.70, 0.0, false, {}, {}},
        {"misunderstanding_rate", "misunderstanding_rate", 0.20, 0.0, false, {}, {}}
    };
    for (auto& c : def_sf.required_conditions) c.condition_key = c.condition_type;
    {
        CapabilityEffectDefinition eff;
        eff.effect_key = "safe_foraging_actions";
        eff.target = CapabilityEffectTarget::ActionAvailability;
        eff.operation = EffectOperationType::UnlockCandidate;
        eff.value_key = "safe_foraging_actions";
        def_sf.effect_definitions.push_back(eff);
    }
    in.capability_definitions.push_back(def_sf);

    CivilizationCapability def_cd;
    def_cd.capability_type = CapabilityType::ConflictDeescalation;
    def_cd.display_key = "conflict_deescalation";
    def_cd.domain_key = "conflict";
    def_cd.required_conditions = {
        {"truce_success_count", "truce_success_count", 2.0, 0.0, false, {}, {}},
        {"forced_retreat_low_loss_count", "forced_retreat_low_loss_count", 2.0, 0.0, false, {}, {}},
        {"open_conflict_pressure", "open_conflict_pressure", 0.5, 0.0, false, {}, {}},
        {"loss_pressure", "loss_pressure", 0.4, 0.0, false, {}, {}},
        {"coordination_score", "coordination_score", 0.45, 0.0, false, {}, {}}
    };
    for (auto& c : def_cd.required_conditions) c.condition_key = c.condition_type;
    in.capability_definitions.push_back(def_cd);

    return in;
}

// 1. Resolver validates input
static void test_resolver_validates_input() {
    CivilizationResolver resolver;
    CivilizationResolveInput in;
    // Missing tribe_id should fail validation
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    assert(!result.value().ok);
    std::cout << "resolver_validates_input passed" << std::endl;
}

// 2. Resolver builds candidates
static void test_resolver_builds_candidates() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.ok);
    // Should find IdentifyEdible, IdentifyDanger, SafeForaging candidates
    assert(res.candidates.size() >= 3);
    std::cout << "resolver_builds_candidates passed" << std::endl;
}

// 3. Resolver full pipeline
static void test_resolver_full_pipeline() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.ok);
    // Should produce updated_state with capabilities
    assert(!res.updated_state.capabilities.empty());
    // Should produce projection
    assert(res.projection.tribe_id == in.current_state.tribe_id);
    // Should produce trace
    assert(!res.trace.trace_id.empty());
    // Should have effect drafts if any Stable/Usable capability exists
    // (IdentifyEdible may become Stable)
    std::cout << "resolver_full_pipeline passed" << std::endl;
}

// 4. Resolver deterministic
static void test_resolver_deterministic() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    // Run twice, compare results
    auto r1 = resolver.resolve(in);
    auto r2 = resolver.resolve(in);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().ok == r2.value().ok);
    assert(r1.value().candidates.size() == r2.value().candidates.size());
    assert(r1.value().updated_state.stage == r2.value().updated_state.stage);
    std::cout << "resolver_deterministic passed" << std::endl;
}

// 5. Resolver rejects forbidden keys
static void test_resolver_rejects_forbidden_keys() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    in.reason_keys = {"hidden_truth"};
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    assert(!result.value().ok);
    std::cout << "resolver_rejects_forbidden_keys passed" << std::endl;
}

// 6. Resolver emits events on unlock
static void test_resolver_events_on_unlock() {
    CivilizationResolver resolver;
    auto in = makeFullInput();

    // Seed IdentifyEdible as Emerging close to Stable
    for (auto& cap : in.current_state.capabilities) {
        if (cap.capability_type == CapabilityType::IdentifyEdible) {
            cap.maturity = CapabilityMaturityState::Emerging;
            cap.progress.success_count = 10;
        }
    }

    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    // Check for capability_unlocked events
    bool has_unlock = false;
    for (const auto& ev : res.event_drafts) {
        if (ev.event_type_key == "capability_unlocked") {
            has_unlock = true;
        }
    }
    // Should have unlock event if any capability reached Stable
    // (IdentifyEdible should)
    assert(has_unlock);
    std::cout << "resolver_events_on_unlock passed" << std::endl;
}

// 7. Single success should not produce Stable
static void test_resolver_single_success_not_stable() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    // Only 1 known edible, low coverage
    in.knowledge_summary.known_edible_count = 1;
    in.knowledge_summary.teachable_knowledge_count = 1;
    in.propagation_summary.coverage = 0.3;
    in.propagation_summary.success_rate = 0.3;
    in.propagation_summary.misunderstanding_rate = 0.5;
    // Remove existing capabilities
    in.current_state.capabilities.clear();

    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    // Should NOT have any Stable capability
    for (const auto& cap : res.updated_state.capabilities) {
        assert(cap.maturity != CapabilityMaturityState::Stable);
    }
    std::cout << "resolver_single_success_not_stable passed" << std::endl;
}

// 8. Stage doesn't reverse-unlock capabilities
static void test_resolver_stage_no_reverse_unlock() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    // Set stage artificially high
    in.current_state.stage = CivilizationStage::SymbolicCulture;

    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    // Stage should be recalculated from capabilities, not preserved
    assert(res.updated_state.stage != CivilizationStage::SymbolicCulture);
    std::cout << "resolver_stage_no_reverse_unlock passed" << std::endl;
}

// 9. Projection safe
static void test_resolver_projection_safe() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    // Projection must not contain raw evidence
    // Projection must not contain forbidden keys
    assert(!res.projection.tribe_id.empty());
    // projection's explanation should be non-empty
    assert(!res.projection.explanation_key.empty());
    assert(res.projection.validateBasic().is_ok());

    CivilizationProjection unsafe_projection = res.projection;
    unsafe_projection.active_effect_summary_keys.push_back("hidden_truth");
    assert(unsafe_projection.validateBasic().is_error());
    std::cout << "resolver_projection_safe passed" << std::endl;
}

// 10. Trace safe
static void test_resolver_trace_safe() {
    CivilizationResolver resolver;
    auto in = makeFullInput();
    auto result = resolver.resolve(in);
    assert(result.is_ok());
    auto& res = result.value();
    // Trace should record steps
    assert(!res.trace.trace_id.empty());
    assert(!res.trace.candidate_steps.empty());
    // Trace should not contain forbidden content
    assert(res.trace.rejected_unsafe_keys.empty());
    assert(res.trace.validateBasic().is_ok());

    CivilizationTrace unsafe_trace = res.trace;
    unsafe_trace.effect_steps.push_back("frontend_unlock");
    assert(unsafe_trace.validateBasic().is_error());
    std::cout << "resolver_trace_safe passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "validates_input") test_resolver_validates_input();
    else if (arg == "builds_candidates") test_resolver_builds_candidates();
    else if (arg == "full_pipeline") test_resolver_full_pipeline();
    else if (arg == "deterministic") test_resolver_deterministic();
    else if (arg == "rejects_forbidden_keys") test_resolver_rejects_forbidden_keys();
    else if (arg == "events_on_unlock") test_resolver_events_on_unlock();
    else if (arg == "single_success_not_stable") test_resolver_single_success_not_stable();
    else if (arg == "stage_no_reverse_unlock") test_resolver_stage_no_reverse_unlock();
    else if (arg == "projection_safe") test_resolver_projection_safe();
    else if (arg == "trace_safe") test_resolver_trace_safe();
    else {
        test_resolver_validates_input();
        test_resolver_builds_candidates();
        test_resolver_full_pipeline();
        test_resolver_deterministic();
        test_resolver_rejects_forbidden_keys();
        test_resolver_events_on_unlock();
        test_resolver_single_success_not_stable();
        test_resolver_stage_no_reverse_unlock();
        test_resolver_projection_safe();
        test_resolver_trace_safe();
    }
    return 0;
}
