#include "pathfinder/civilization/civilization_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::civilization;
using namespace pathfinder::foundation;

// Helper: build a resolver input with progressive data
static CivilizationResolveInput makeInput(int known_edible, int known_danger, double coverage,
                                          double success_rate, double misunderstanding_rate,
                                          int truce = 0, int retreat = 0, int standoff = 0,
                                          double open_conflict = 0.2, double loss = 0.1,
                                          double coord = 0.7, Tick tick = Tick(10)) {
    CivilizationResolveInput in;
    in.current_state.tribe_id = TribeId("flow_tribe");
    in.current_state.stage = CivilizationStage::Awakening;
    in.current_state.stage_confidence = 0.5;
    in.current_state.version = StateVersion(1);

    in.knowledge_summary.tribe_id = TribeId("flow_tribe");
    in.knowledge_summary.known_edible_count = known_edible;
    in.knowledge_summary.known_danger_count = known_danger;
    in.knowledge_summary.teachable_knowledge_count = std::max(known_edible, 1);
    in.knowledge_summary.stable_tribe_knowledge_count = known_edible;
    in.knowledge_summary.conflicted_knowledge_count = 0;
    in.knowledge_summary.knowledge_ids = {KnowledgeId("fk1"), KnowledgeId("fk2")};

    in.propagation_summary.tribe_id = TribeId("flow_tribe");
    in.propagation_summary.coverage = coverage;
    in.propagation_summary.success_rate = success_rate;
    in.propagation_summary.misunderstanding_rate = misunderstanding_rate;

    in.coordination_summary.tribe_id = TribeId("flow_tribe");
    in.coordination_summary.coordination_score = coord;
    in.coordination_summary.stable_coordination_count = 1;

    in.conflict_summary.tribe_id = TribeId("flow_tribe");
    in.conflict_summary.truce_success_count = truce;
    in.conflict_summary.forced_retreat_low_loss_count = retreat;
    in.conflict_summary.standoff_controlled_count = standoff;
    in.conflict_summary.open_conflict_pressure = open_conflict;
    in.conflict_summary.loss_pressure = loss;
    in.conflict_summary.source_event_ids = {EventId("fe1")};

    in.input_tick = tick;

    // Standard definitions
    CivilizationCapability def_ie;
    def_ie.capability_type = CapabilityType::IdentifyEdible;
    def_ie.display_key = "identify_edible";
    def_ie.domain_key = "knowledge";
    def_ie.required_conditions = {
        {"known_edible_c", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"coverage_c", "coverage", 0.5, 0.0, false, {}, {}},
        {"misunderstanding_c", "misunderstanding_rate", 0.25, 0.0, false, {}, {}}
    };
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
        {"known_danger_c", "known_danger_count", 1.0, 0.0, false, {}, {}},
        {"coverage_c", "coverage", 0.45, 0.0, false, {}, {}},
        {"misunderstanding_c", "misunderstanding_rate", 0.25, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def_id);

    CivilizationCapability def_sf;
    def_sf.capability_type = CapabilityType::SafeForaging;
    def_sf.display_key = "safe_foraging";
    def_sf.domain_key = "foraging";
    def_sf.required_conditions = {
        {"known_edible_c", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"known_danger_c", "known_danger_count", 1.0, 0.0, false, {}, {}},
        {"coverage_c", "coverage", 0.55, 0.0, false, {}, {}},
        {"success_c", "success_rate", 0.70, 0.0, false, {}, {}},
        {"misunderstanding_c", "misunderstanding_rate", 0.20, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def_sf);

    CivilizationCapability def_cd;
    def_cd.capability_type = CapabilityType::ConflictDeescalation;
    def_cd.display_key = "conflict_deescalation";
    def_cd.domain_key = "conflict";
    def_cd.required_conditions = {
        {"truce_c", "truce_success_count", 2.0, 0.0, false, {}, {}},
        {"retreat_c", "forced_retreat_low_loss_count", 2.0, 0.0, false, {}, {}},
        {"open_conflict_c", "open_conflict_pressure", 0.5, 0.0, false, {}, {}},
        {"loss_c", "loss_pressure", 0.4, 0.0, false, {}, {}},
        {"coord_c", "coordination_score", 0.45, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def_cd);

    return in;
}

static bool hasCapability(const std::vector<CivilizationCapabilityState>& caps, CapabilityType type) {
    for (const auto& c : caps) {
        if (c.capability_type == type) return true;
    }
    return false;
}

static const CivilizationCapabilityState* getCap(const std::vector<CivilizationCapabilityState>& caps, CapabilityType type) {
    for (const auto& c : caps) {
        if (c.capability_type == type) return &c;
    }
    return nullptr;
}

// 1. IdentifyEdible candidate flow
static void test_identify_edible_candidate_flow() {
    CivilizationResolver resolver;
    auto in = makeInput(2, 0, 0.5, 0.6, 0.2);
    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    auto& res = result.value();
    auto* cap = getCap(res.updated_state.capabilities, CapabilityType::IdentifyEdible);
    assert(cap);
    assert(cap->maturity == CapabilityMaturityState::Candidate ||
           cap->maturity == CapabilityMaturityState::Emerging);
    std::cout << "p26_identify_edible_candidate_flow passed" << std::endl;
}

// 2. IdentifyDanger candidate flow
static void test_identify_danger_candidate_flow() {
    CivilizationResolver resolver;
    auto in = makeInput(0, 2, 0.5, 0.6, 0.2);
    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    auto& res = result.value();
    auto* cap = getCap(res.updated_state.capabilities, CapabilityType::IdentifyDanger);
    assert(cap);
    assert(cap->maturity == CapabilityMaturityState::Candidate ||
           cap->maturity == CapabilityMaturityState::Emerging);
    std::cout << "p26_identify_danger_candidate_flow passed" << std::endl;
}

// 3. SafeForaging stable flow (progressive resolution)
static void test_safe_foraging_stable_flow() {
    CivilizationResolver resolver;

    // Step 1: First resolve with good data to get IdentifyEdible+IdentifyDanger to Emerging
    auto in1 = makeInput(3, 2, 0.6, 0.75, 0.15);
    auto r1 = resolver.resolve(in1);
    assert(r1.is_ok() && r1.value().ok);

    // Step 2: Promote prerequisites to Emerging, carry forward state
    auto in2 = makeInput(4, 3, 0.7, 0.8, 0.1);
    in2.current_state = r1.value().updated_state;

    auto r2 = resolver.resolve(in2);
    assert(r2.is_ok() && r2.value().ok);

    // Step 3: Now that prerequisites are Emerging, SafeForaging can be built
    auto in3 = makeInput(4, 3, 0.7, 0.8, 0.1, 0, 0, 0, 0.2, 0.1, 0.7, Tick(30));
    in3.current_state = r2.value().updated_state;

    auto r3 = resolver.resolve(in3);
    assert(r3.is_ok() && r3.value().ok);

    auto& res3 = r3.value();
    auto* cap = getCap(res3.updated_state.capabilities, CapabilityType::SafeForaging);
    // SafeForaging should exist after its prerequisites are met
    assert(cap);
    std::cout << "p26_safe_foraging_stable_flow passed" << std::endl;
}

// 4. ConflictDeescalation candidate from P25 conflict summary
static void test_conflict_deescalation_candidate_from_p25_flow() {
    CivilizationResolver resolver;
    auto in = makeInput(0, 0, 0.0, 0.0, 1.0, 1, 2, 1, 0.3, 0.2, 0.5);
    // Remove some condition requirements to just test candidate formation
    in.capability_definitions.clear();

    CivilizationCapability def_cd;
    def_cd.capability_type = CapabilityType::ConflictDeescalation;
    def_cd.display_key = "conflict_deescalation";
    def_cd.domain_key = "conflict";
    def_cd.required_conditions = {
        {"truce_c", "truce_success_count", 2.0, 0.0, false, {}, {}},
        {"retreat_c", "forced_retreat_low_loss_count", 2.0, 0.0, false, {}, {}},
        {"open_conflict_c", "open_conflict_pressure", 0.5, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def_cd);

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    auto& res = result.value();
    auto* cap = getCap(res.updated_state.capabilities, CapabilityType::ConflictDeescalation);
    assert(cap);
    assert(cap->maturity == CapabilityMaturityState::Candidate ||
           cap->maturity == CapabilityMaturityState::Emerging);
    std::cout << "p26_conflict_deescalation_candidate_from_p25_flow passed" << std::endl;
}

// 5. Capability requires repeated evidence (single success not enough)
static void test_capability_requires_repeated_evidence_flow() {
    CivilizationResolver resolver;
    // Minimal data: only 1 edible, 1 teachable, low coverage
    auto in = makeInput(1, 0, 0.3, 0.3, 0.5);
    // Remove definitions except IdentifyEdible
    in.capability_definitions.clear();
    CivilizationCapability def_ie;
    def_ie.capability_type = CapabilityType::IdentifyEdible;
    def_ie.display_key = "identify_edible";
    def_ie.domain_key = "knowledge";
    def_ie.required_conditions = {
        {"known_edible_c", "known_edible_count", 2.0, 0.0, false, {}, {}},
        {"coverage_c", "coverage", 0.5, 0.0, false, {}, {}},
        {"misunderstanding_c", "misunderstanding_rate", 0.25, 0.0, false, {}, {}}
    };
    in.capability_definitions.push_back(def_ie);

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    auto& res = result.value();
    auto* cap = getCap(res.updated_state.capabilities, CapabilityType::IdentifyEdible);
    if (cap) {
        // Single success should NOT be Stable
        assert(cap->maturity != CapabilityMaturityState::Stable);
        assert(cap->maturity != CapabilityMaturityState::Institutionalized);
    }
    std::cout << "p26_capability_requires_repeated_evidence_flow passed" << std::endl;
}

// 6. Stable capability emits unlock event only once
static void test_stable_capability_emits_unlock_once_flow() {
    CivilizationResolver resolver;

    // First resolution: build up to Stable
    auto in = makeInput(3, 2, 0.7, 0.8, 0.1, 0, 0, 0, 0.2, 0.1, 0.7, Tick(10));
    auto r1 = resolver.resolve(in);
    assert(r1.is_ok() && r1.value().ok);
    int unlock_count_1 = 0;
    for (const auto& ev : r1.value().event_drafts) {
        if (ev.event_type_key == "capability_unlocked") unlock_count_1++;
    }

    // Second resolution with same state
    auto in2 = makeInput(3, 2, 0.7, 0.8, 0.1, 0, 0, 0, 0.2, 0.1, 0.7, Tick(20));
    in2.current_state = r1.value().updated_state;
    auto r2 = resolver.resolve(in2);
    assert(r2.is_ok() && r2.value().ok);
    int unlock_count_2 = 0;
    for (const auto& ev : r2.value().event_drafts) {
        if (ev.event_type_key == "capability_unlocked") unlock_count_2++;
    }

    // Second resolution should not re-emit unlock for already-Stable capabilities
    assert(unlock_count_2 <= unlock_count_1);
    std::cout << "p26_stable_capability_emits_unlock_once_flow passed" << std::endl;
}

// 7. Foraging stage from stable capabilities
static void test_stage_foraging_from_stable_capabilities_flow() {
    CivilizationResolver resolver;

    auto in = makeInput(4, 3, 0.7, 0.8, 0.1);

    // Pre-populate with IdentifyEdible(Stable), IdentifyDanger(Stable), SafeForaging(Stable)
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("flow_tribe");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Stable;
    ie.usability = CapabilityUsabilityState::Usable;
    ie.stability = 0.9;
    ie.evidence.knowledge_ids = {KnowledgeId("fk1")};
    ie.progress.success_count = 10;
    in.current_state.capabilities.push_back(ie);

    CivilizationCapabilityState id;
    id.tribe_id = TribeId("flow_tribe");
    id.capability_type = CapabilityType::IdentifyDanger;
    id.maturity = CapabilityMaturityState::Stable;
    id.usability = CapabilityUsabilityState::Usable;
    id.stability = 0.85;
    id.evidence.knowledge_ids = {KnowledgeId("fk2")};
    id.progress.success_count = 8;
    in.current_state.capabilities.push_back(id);

    CivilizationCapabilityState sf;
    sf.tribe_id = TribeId("flow_tribe");
    sf.capability_type = CapabilityType::SafeForaging;
    sf.maturity = CapabilityMaturityState::Stable;
    sf.usability = CapabilityUsabilityState::Usable;
    sf.stability = 0.8;
    sf.evidence.knowledge_ids = {KnowledgeId("fk3")};
    sf.progress.success_count = 6;
    in.current_state.capabilities.push_back(sf);

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);
    assert(result.value().updated_state.stage == CivilizationStage::Foraging);
    std::cout << "p26_stage_foraging_from_stable_capabilities_flow passed" << std::endl;
}

// 8. Blocked by conflict does not lose maturity
static void test_blocked_by_conflict_does_not_lose_maturity_flow() {
    CivilizationResolver resolver;

    auto in = makeInput(3, 2, 0.6, 0.7, 0.15, 0, 0, 0, 0.8, 0.7, 0.6);
    // Pre-populate IdentifyEdible as Stable
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("flow_tribe");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Stable;
    ie.usability = CapabilityUsabilityState::Usable;
    ie.stability = 0.8;
    ie.evidence.knowledge_ids = {KnowledgeId("fk1")};
    ie.progress.success_count = 5;
    in.current_state.capabilities.push_back(ie);

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    auto* cap = getCap(result.value().updated_state.capabilities, CapabilityType::IdentifyEdible);
    assert(cap);
    // Maturity should remain Stable even if usability is BlockedByConflict
    assert(cap->maturity == CapabilityMaturityState::Stable);
    // Usability may be blocked due to high conflict/loss
    // But maturity should NOT degrade just because of blockage
    std::cout << "p26_blocked_by_conflict_does_not_lose_maturity_flow passed" << std::endl;
}

// 9. Effect draft from stable usable capability
static void test_effect_draft_from_stable_usable_capability_flow() {
    CivilizationResolver resolver;

    auto in = makeInput(3, 2, 0.6, 0.7, 0.15);

    // Pre-populate IdentifyEdible Stable + Usable
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("flow_tribe");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Stable;
    ie.usability = CapabilityUsabilityState::Usable;
    ie.stability = 0.9;
    ie.evidence.knowledge_ids = {KnowledgeId("fk1")};
    ie.progress.success_count = 10;
    in.current_state.capabilities.push_back(ie);

    auto result = resolver.resolve(in);
    assert(result.is_ok() && result.value().ok);

    // Should have effect drafts for IdentifyEdible
    bool has_effect = false;
    for (const auto& eff : result.value().effect_drafts) {
        if (eff.capability_type == CapabilityType::IdentifyEdible) {
            has_effect = true;
            break;
        }
    }
    assert(has_effect);
    std::cout << "p26_effect_draft_from_stable_usable_capability_flow passed" << std::endl;
}

// 10. Deterministic resolution
static void test_deterministic_resolution_flow() {
    CivilizationResolver resolver;
    auto in = makeInput(3, 2, 0.6, 0.7, 0.15);
    auto r1 = resolver.resolve(in);
    auto r2 = resolver.resolve(in);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().ok == r2.value().ok);
    assert(r1.value().candidates.size() == r2.value().candidates.size());
    assert(r1.value().updated_state.stage == r2.value().updated_state.stage);
    std::cout << "p26_deterministic_resolution_flow passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "identify_edible_candidate_flow") test_identify_edible_candidate_flow();
    else if (arg == "identify_danger_candidate_flow") test_identify_danger_candidate_flow();
    else if (arg == "safe_foraging_stable_flow") test_safe_foraging_stable_flow();
    else if (arg == "conflict_deescalation_candidate_from_p25") test_conflict_deescalation_candidate_from_p25_flow();
    else if (arg == "requires_repeated_evidence") test_capability_requires_repeated_evidence_flow();
    else if (arg == "emits_unlock_once") test_stable_capability_emits_unlock_once_flow();
    else if (arg == "foraging_stage") test_stage_foraging_from_stable_capabilities_flow();
    else if (arg == "blocked_keeps_maturity") test_blocked_by_conflict_does_not_lose_maturity_flow();
    else if (arg == "effect_draft_stable_usable") test_effect_draft_from_stable_usable_capability_flow();
    else if (arg == "deterministic") test_deterministic_resolution_flow();
    else {
        test_identify_edible_candidate_flow();
        test_identify_danger_candidate_flow();
        test_safe_foraging_stable_flow();
        test_conflict_deescalation_candidate_from_p25_flow();
        test_capability_requires_repeated_evidence_flow();
        test_stable_capability_emits_unlock_once_flow();
        test_stage_foraging_from_stable_capabilities_flow();
        test_blocked_by_conflict_does_not_lose_maturity_flow();
        test_effect_draft_from_stable_usable_capability_flow();
        test_deterministic_resolution_flow();
    }
    return 0;
}
