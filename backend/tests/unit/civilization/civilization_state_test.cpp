#include "pathfinder/civilization/civilization_state.h"
#include <cassert>
#include <iostream>
#include <tuple>

using namespace pathfinder::civilization;
using namespace pathfinder::foundation;

// Helper: make a minimal input with known_edible data
static CivilizationResolveInput makeInput(int known_edible, int known_danger, double coverage,
                                          double success_rate, double misunderstanding_rate,
                                          int teachable = 0, int stable_tribe = 0,
                                          int conflicted = 0, Tick tick = Tick(10)) {
    CivilizationResolveInput in;
    in.current_state.tribe_id = TribeId("tribe_test");
    in.current_state.stage = CivilizationStage::Awakening;
    in.current_state.stage_confidence = 0.5;
    in.current_state.version = StateVersion(1);

    in.knowledge_summary.tribe_id = TribeId("tribe_test");
    in.knowledge_summary.known_edible_count = known_edible;
    in.knowledge_summary.known_danger_count = known_danger;
    in.knowledge_summary.teachable_knowledge_count = teachable > 0 ? teachable : known_edible;
    in.knowledge_summary.stable_tribe_knowledge_count = stable_tribe > 0 ? stable_tribe : known_edible;
    in.knowledge_summary.conflicted_knowledge_count = conflicted;
    in.knowledge_summary.knowledge_ids = {KnowledgeId("k1"), KnowledgeId("k2")};

    in.propagation_summary.tribe_id = TribeId("tribe_test");
    in.propagation_summary.coverage = coverage;
    in.propagation_summary.success_rate = success_rate;
    in.propagation_summary.misunderstanding_rate = misunderstanding_rate;

    in.coordination_summary.tribe_id = TribeId("tribe_test");
    in.coordination_summary.coordination_score = 0.6;
    in.coordination_summary.stable_coordination_count = 1;

    in.conflict_summary.tribe_id = TribeId("tribe_test");
    in.conflict_summary.open_conflict_pressure = 0.2;
    in.conflict_summary.loss_pressure = 0.1;

    in.input_tick = tick;
    return in;
}

static CivilizationResolveInput makeConflictInput(int truce, int retreat, int standoff,
                                                   double open_conflict, double loss_pressure,
                                                   double coord_score = 0.5) {
    CivilizationResolveInput in = makeInput(0, 0, 0.0, 0.0, 1.0);
    in.conflict_summary.truce_success_count = truce;
    in.conflict_summary.forced_retreat_low_loss_count = retreat;
    in.conflict_summary.standoff_controlled_count = standoff;
    in.conflict_summary.open_conflict_pressure = open_conflict;
    in.conflict_summary.loss_pressure = loss_pressure;
    in.conflict_summary.source_event_ids = {EventId("ce1"), EventId("ce2")};
    in.coordination_summary.coordination_score = coord_score;
    return in;
}

static CivilizationCapability makeDef(CapabilityType type,
                                      const std::vector<std::pair<std::string, double>>& conditions,
                                      const std::vector<std::tuple<std::string, CapabilityEffectTarget, EffectOperationType, std::string, double>>& effects = {}) {
    CivilizationCapability def;
    def.capability_type = type;
    def.display_key = toString(type);
    def.domain_key = "civilization";
    for (const auto& [ckey, cscore] : conditions) {
        CapabilityUnlockCondition c;
        c.condition_key = ckey;
        c.condition_type = ckey;
        c.required_score = cscore;
        def.required_conditions.push_back(std::move(c));
    }
    for (const auto& [ekey, target, op, vkey, vnum] : effects) {
        CapabilityEffectDefinition eff;
        eff.effect_key = ekey;
        eff.target = target;
        eff.operation = op;
        eff.value_key = vkey;
        eff.value_number = vnum;
        def.effect_definitions.push_back(std::move(eff));
    }
    return def;
}

// 1. CandidateBuilder: IdentifyEdible from knowledge data
static void test_candidate_builder_identify_edible() {
    CivilizationCandidateBuilder builder;
    auto in = makeInput(2, 0, 0.6, 0.5, 0.2);
    auto result = builder.build(in);
    assert(result.is_ok());
    auto& cands = result.value();
    bool found = false;
    for (const auto& c : cands) {
        if (c.capability_type == CapabilityType::IdentifyEdible) {
            found = true;
            assert(c.readiness > 0.3);
            break;
        }
    }
    assert(found);
    std::cout << "candidate_builder_identify_edible passed" << std::endl;
}

// 2. CandidateBuilder: IdentifyDanger from knowledge data
static void test_candidate_builder_identify_danger() {
    CivilizationCandidateBuilder builder;
    auto in = makeInput(0, 2, 0.5, 0.6, 0.2);
    auto result = builder.build(in);
    assert(result.is_ok());
    auto& cands = result.value();
    bool found = false;
    for (const auto& c : cands) {
        if (c.capability_type == CapabilityType::IdentifyDanger) {
            found = true;
            assert(c.readiness > 0.2);
            break;
        }
    }
    assert(found);
    std::cout << "candidate_builder_identify_danger passed" << std::endl;
}

// 3. CandidateBuilder: SafeForaging requires prerequisites
static void test_candidate_builder_safe_foraging_prereq() {
    CivilizationCandidateBuilder builder;

    // Without prerequisites in current_state — no SafeForaging candidate
    auto in = makeInput(3, 2, 0.6, 0.8, 0.15);
    auto result = builder.build(in);
    assert(result.is_ok());
    for (const auto& c : result.value()) {
        assert(c.capability_type != CapabilityType::SafeForaging);
    }

    // Add IdentifyEdible(Emerging) and IdentifyDanger(Emerging) to current_state
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("tribe_test");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Emerging;
    ie.usability = CapabilityUsabilityState::Usable;
    ie.stability = 0.6;
    in.current_state.capabilities.push_back(ie);

    CivilizationCapabilityState id;
    id.tribe_id = TribeId("tribe_test");
    id.capability_type = CapabilityType::IdentifyDanger;
    id.maturity = CapabilityMaturityState::Emerging;
    id.usability = CapabilityUsabilityState::Usable;
    id.stability = 0.6;
    in.current_state.capabilities.push_back(id);

    auto result2 = builder.build(in);
    assert(result2.is_ok());
    bool found = false;
    for (const auto& c : result2.value()) {
        if (c.capability_type == CapabilityType::SafeForaging) {
            found = true;
            break;
        }
    }
    assert(found);
    std::cout << "candidate_builder_safe_foraging_prereq passed" << std::endl;
}

// 4. RequirementEvaluator: scores conditions
static void test_requirement_evaluator_scores() {
    CapabilityRequirementEvaluator eval;
    auto def = makeDef(CapabilityType::IdentifyEdible, {
        {"known_edible_count", 2.0},
        {"coverage", 0.5},
        {"misunderstanding_rate", 0.25}
    });
    auto in = makeInput(3, 0, 0.6, 0.5, 0.2);
    auto result = eval.evaluate(def, in);
    assert(result.is_ok());
    auto& snap = result.value();
    assert(snap.total_score > 0.5);
    // With good data, most conditions should be met
    assert(snap.met_condition_keys.size() >= 1);
    std::cout << "requirement_evaluator_scores passed" << std::endl;
}

// 5. StateResolver: Unknown → Candidate
static void test_state_resolver_unknown_to_candidate() {
    CapabilityStateResolver resolver;
    CapabilityRequirementEvaluator eval;

    auto def = makeDef(CapabilityType::IdentifyEdible, {{"known_edible_count", 2.0}});
    auto in = makeInput(2, 0, 0.6, 0.5, 0.2);

    // Build candidate
    CivilizationCandidateBuilder builder;
    auto cand_result = builder.build(in);
    assert(cand_result.is_ok());
    auto eval_result = eval.evaluate(def, in);
    assert(eval_result.is_ok());

    CivilizationCapabilityState current;
    current.tribe_id = TribeId("tribe_test");
    current.capability_type = CapabilityType::IdentifyEdible;
    current.maturity = CapabilityMaturityState::Unknown;
    current.usability = CapabilityUsabilityState::Unknown;

    CivilizationCapabilityCandidate cand = cand_result.value().front();
    cand.requirement_snapshot = std::move(eval_result).value();

    auto result = resolver.resolveState(current, cand, def);
    assert(result.is_ok());
    auto& state = result.value();
    assert(state.maturity == CapabilityMaturityState::Candidate);
    std::cout << "state_resolver_unknown_to_candidate passed" << std::endl;
}

// 6. StateResolver: Candidate → Emerging
static void test_state_resolver_candidate_to_emerging() {
    CapabilityStateResolver resolver;

    auto def = makeDef(CapabilityType::IdentifyEdible, {{"known_edible_count", 2.0}});
    auto in = makeInput(3, 0, 0.65, 0.7, 0.15);

    CivilizationCandidateBuilder builder;
    auto cand_result = builder.build(in);
    assert(cand_result.is_ok());
    CapabilityRequirementEvaluator eval;
    auto eval_result = eval.evaluate(def, in);
    assert(eval_result.is_ok());

    CivilizationCapabilityState current;
    current.tribe_id = TribeId("tribe_test");
    current.capability_type = CapabilityType::IdentifyEdible;
    current.maturity = CapabilityMaturityState::Candidate;
    current.usability = CapabilityUsabilityState::Usable;
    current.progress.success_count = 2;

    CivilizationCapabilityCandidate cand = cand_result.value().front();
    cand.requirement_snapshot = std::move(eval_result).value();

    auto result = resolver.resolveState(current, cand, def);
    assert(result.is_ok());
    auto& state = result.value();
    // With good conditions + success history → should promote
    assert(state.maturity == CapabilityMaturityState::Emerging);
    std::cout << "state_resolver_candidate_to_emerging passed" << std::endl;
}

// 7. StateResolver: Emerging → Stable
static void test_state_resolver_emerging_to_stable() {
    CapabilityStateResolver resolver;

    auto def = makeDef(CapabilityType::IdentifyEdible, {{"known_edible_count", 2.0}});
    auto in = makeInput(4, 0, 0.7, 0.8, 0.1, 2, 2);

    CivilizationCandidateBuilder builder;
    auto cand_result = builder.build(in);
    assert(cand_result.is_ok());
    CapabilityRequirementEvaluator eval;
    auto eval_result = eval.evaluate(def, in);
    assert(eval_result.is_ok());

    CivilizationCapabilityState current;
    current.tribe_id = TribeId("tribe_test");
    current.capability_type = CapabilityType::IdentifyEdible;
    current.maturity = CapabilityMaturityState::Emerging;
    current.usability = CapabilityUsabilityState::Usable;
    current.progress.success_count = 5;
    current.progress.failure_count = 0;

    CivilizationCapabilityCandidate cand = cand_result.value().front();
    cand.requirement_snapshot = std::move(eval_result).value();

    auto result = resolver.resolveState(current, cand, def);
    assert(result.is_ok());
    auto& state = result.value();
    assert(state.maturity == CapabilityMaturityState::Stable);
    assert(state.stability > 0.0);
    std::cout << "state_resolver_emerging_to_stable passed" << std::endl;
}

// 8. Usability: BlockedByConflict
static void test_usability_blocked_by_conflict() {
    CapabilityUsabilityResolver resolver;

    auto in = makeConflictInput(0, 0, 0, 0.7, 0.7);

    CivilizationCapabilityState state;
    state.tribe_id = TribeId("tribe_test");
    state.capability_type = CapabilityType::ConflictDeescalation;
    state.maturity = CapabilityMaturityState::Stable;
    state.usability = CapabilityUsabilityState::Usable;

    auto result = resolver.resolveUsability(state, in);
    assert(result.is_ok());
    assert(result.value().usability == CapabilityUsabilityState::BlockedByConflict);
    assert(!result.value().blocked_reasons.empty());
    std::cout << "usability_blocked_by_conflict passed" << std::endl;
}

// 9. StageResolver: Foraging derived from stable capabilities
static void test_stage_foraging_derived() {
    CivilizationStageResolver resolver;

    CivilizationState state;
    state.tribe_id = TribeId("tribe_test");

    // Add IdentifyEdible(Stable), IdentifyDanger(Emerging), SafeForaging(Stable)
    CivilizationCapabilityState ie;
    ie.tribe_id = TribeId("tribe_test");
    ie.capability_type = CapabilityType::IdentifyEdible;
    ie.maturity = CapabilityMaturityState::Stable;
    ie.usability = CapabilityUsabilityState::Usable;
    state.capabilities.push_back(ie);

    CivilizationCapabilityState id;
    id.tribe_id = TribeId("tribe_test");
    id.capability_type = CapabilityType::IdentifyDanger;
    id.maturity = CapabilityMaturityState::Emerging;
    id.usability = CapabilityUsabilityState::Usable;
    state.capabilities.push_back(id);

    CivilizationCapabilityState sf;
    sf.tribe_id = TribeId("tribe_test");
    sf.capability_type = CapabilityType::SafeForaging;
    sf.maturity = CapabilityMaturityState::Stable;
    sf.usability = CapabilityUsabilityState::Usable;
    state.capabilities.push_back(sf);

    auto result = resolver.resolveStage(state);
    assert(result.is_ok());
    assert(result.value().stage == CivilizationStage::Foraging);
    std::cout << "stage_foraging_derived passed" << std::endl;
}

// 10. EffectResolver: Stable capability produces effect drafts
static void test_effect_resolver_stable_capability() {
    CapabilityEffectResolver resolver;

    std::vector<CivilizationCapability> defs = {
        makeDef(CapabilityType::IdentifyEdible, {{"known_edible_count", 2.0}},
                {{"reduce_unknown_food_risk", CapabilityEffectTarget::Risk,
                  EffectOperationType::Multiply, "reduce_unknown_food_risk", 0.90}})
    };

    CivilizationState state;
    state.tribe_id = TribeId("tribe_test");

    CivilizationCapabilityState cap;
    cap.tribe_id = TribeId("tribe_test");
    cap.capability_type = CapabilityType::IdentifyEdible;
    cap.maturity = CapabilityMaturityState::Stable;
    cap.usability = CapabilityUsabilityState::Usable;
    cap.stability = 0.8;
    state.capabilities.push_back(cap);

    auto result = resolver.resolveEffects(state, defs);
    assert(result.is_ok());
    auto& drafts = result.value();
    assert(!drafts.empty());
    assert(drafts[0].effect_key == "reduce_unknown_food_risk");
    assert(drafts[0].target == CapabilityEffectTarget::Risk);
    assert(drafts[0].operation == EffectOperationType::Multiply);
    assert(drafts[0].strength > 0.0);
    std::cout << "effect_resolver_stable_capability passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "candidate_builder_identify_edible") test_candidate_builder_identify_edible();
    else if (arg == "candidate_builder_identify_danger") test_candidate_builder_identify_danger();
    else if (arg == "candidate_builder_safe_foraging_prereq") test_candidate_builder_safe_foraging_prereq();
    else if (arg == "requirement_evaluator_scores") test_requirement_evaluator_scores();
    else if (arg == "state_resolver_unknown_to_candidate") test_state_resolver_unknown_to_candidate();
    else if (arg == "state_resolver_candidate_to_emerging") test_state_resolver_candidate_to_emerging();
    else if (arg == "state_resolver_emerging_to_stable") test_state_resolver_emerging_to_stable();
    else if (arg == "usability_blocked_by_conflict") test_usability_blocked_by_conflict();
    else if (arg == "stage_foraging_derived") test_stage_foraging_derived();
    else if (arg == "effect_resolver_stable_capability") test_effect_resolver_stable_capability();
    else {
        test_candidate_builder_identify_edible();
        test_candidate_builder_identify_danger();
        test_candidate_builder_safe_foraging_prereq();
        test_requirement_evaluator_scores();
        test_state_resolver_unknown_to_candidate();
        test_state_resolver_candidate_to_emerging();
        test_state_resolver_emerging_to_stable();
        test_usability_blocked_by_conflict();
        test_stage_foraging_derived();
        test_effect_resolver_stable_capability();
    }
    return 0;
}
