#include "pathfinder/danger/danger_knowledge_bridge.h"
#include "pathfinder/danger/hazard_resolver.h"
#include "pathfinder/danger/threat_profile_builder.h"
#include "pathfinder/danger/tribe_danger_bridge.h"
#include "pathfinder/object/types.h"
#include "pathfinder/tribe/tribe_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::danger;
using namespace pathfinder::foundation;
using namespace pathfinder::reaction;
using namespace pathfinder::tribe;
using pathfinder::object::ObjectCategory;

static pathfinder::condition::ConditionExpressionRef cond(std::string key) {
    pathfinder::condition::ConditionExpressionRef ref;
    ref.inline_canonical_key = std::move(key);
    return ref;
}

static ReactionRuntimeObject obj(std::string id, std::string def, std::string key, ObjectCategory category, std::vector<std::string> tags, std::vector<std::string> states) {
    ReactionRuntimeObject out;
    out.object_id = ObjectId(std::move(id));
    out.definition_id = ObjectDefinitionId(std::move(def));
    out.object_key = std::move(key);
    out.category = category;
    out.quantity = 1;
    out.tag_keys = std::move(tags);
    out.state_keys = std::move(states);
    out.location_key = "wild_area";
    return out;
}

static HazardRule rottenRule() {
    HazardRule rule;
    rule.rule_key = "rotten_food_learning_pressure";
    rule.trigger_kind = HazardTriggerKind::Eat;
    rule.source_kind = DangerSourceKind::ObjectReaction;
    rule.condition_refs = {cond("condition:target_state:eq:rotten")};
    rule.severity = DangerSeverity::Harm;
    rule.pressure_delta = 0.5;
    rule.fear_delta = 0.3;
    rule.morale_delta = -0.2;
    rule.trust_delta = -0.1;
    rule.split_risk_delta = 0.2;
    rule.feedback_key = "rotten_food_feedback";
    rule.feedback_text = "腐烂食物带来痛苦经验，也形成可传授的危险知识。";
    rule.knowledge_effect_key = "poison";
    return rule;
}

static HazardRule wolfRule() {
    HazardRule rule;
    rule.rule_key = "wolf_group_exploration_pressure";
    rule.trigger_kind = HazardTriggerKind::Approach;
    rule.source_kind = DangerSourceKind::Creature;
    rule.severity = DangerSeverity::Severe;
    rule.pressure_delta = 0.7;
    rule.fear_delta = 0.5;
    rule.morale_delta = -0.3;
    rule.feedback_key = "wolf_group_feedback";
    rule.feedback_text = "狼群威胁使探索压力升高，工具和路线能保护行动。";
    rule.knowledge_effect_key = "wolf_risk";
    CountermeasureRequirement torch;
    torch.kind = CountermeasureKind::Tool;
    torch.requirement_key = "light_source";
    torch.mitigation_score = 0.55;
    CountermeasureRequirement route;
    route.kind = CountermeasureKind::EscapeRoute;
    route.requirement_key = "escape_route";
    route.mitigation_score = 0.25;
    rule.countermeasure_requirements = {torch, route};
    return rule;
}

static TribeState tribeState() {
    TribeState state;
    state.tribe_id = TribeId("tribe_first");
    state.display_key = "first_tribe";
    TribeMember a;
    a.member_id = EntityId("actor_pioneer");
    a.role = TribeMemberRole::Pioneer;
    a.trust = 0.7;
    a.morale = 0.7;
    TribeMember b;
    b.member_id = EntityId("actor_companion");
    b.role = TribeMemberRole::Learner;
    b.trust = 0.6;
    b.morale = 0.6;
    state.members = {a, b};
    state.population_total = 2;
    state.active_population = 2;
    state.morale.overall = 0.7;
    state.trust.leader_trust = 0.7;
    state.trust.member_trust = 0.6;
    state.trust.teaching_fairness = 0.6;
    state.split_risk.risk = 0.1;
    state.split_risk.cohesion_state = TribeCohesionState::Stable;
    state.reason_keys = {"initial_tribe"};
    assert(state.validateBasic().is_ok());
    return state;
}

static void test_learning_teaching_tribe_flow() {
    auto rotten = obj("obj_rotten", "def_red_berry_rotten", "red_berry_rotten", ObjectCategory::Food, {"fruit"}, {"rotten"});
    auto threat = ThreatProfileBuilder{}.fromRuntimeObject(rotten).value();
    HazardExposureInput input;
    input.input_key = "eat_rotten_flow";
    input.actor_id = EntityId("actor_pioneer");
    input.tribe_id = TribeId("tribe_first");
    input.trigger_kind = HazardTriggerKind::Eat;
    input.threats = {threat};
    input.tick = Tick(30);

    auto resolved = HazardResolver{}.resolve(input, {rottenRule()});
    assert(resolved.is_ok());
    assert(resolved.value().knowledge_drafts.size() == 1);

    DangerKnowledgePlannerInput knowledge_input;
    knowledge_input.actor_id = EntityId("actor_pioneer");
    knowledge_input.observer_ids = {EntityId("actor_companion")};
    knowledge_input.taught_ids = {EntityId("actor_student")};
    knowledge_input.knowledge_drafts = resolved.value().knowledge_drafts;
    auto plan = DangerKnowledgePlanner{}.plan(knowledge_input);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 3);

    auto tribe_input = TribeDangerBridge{}.toTribeStateInput(resolved.value().tribe_pressure_drafts, TribeId("tribe_first"), Tick(31));
    assert(tribe_input.is_ok());
    auto tribe_result = TribeStateResolver{}.resolve(tribeState(), tribe_input.value());
    assert(tribe_result.is_ok());
    assert(tribe_result.value().updated_state.morale.overall < 0.7);
    assert(tribe_result.value().updated_state.split_risk.risk > 0.1);
}

static void test_craft_tool_then_safe_explore() {
    auto wolf = ThreatProfileBuilder{}.fromCreatureProjection(EntityId("wolf_003"), "wolf", DangerSeverity::Severe, 0.9).value();
    HazardExposureInput input;
    input.input_key = "wolf_with_torch_flow";
    input.actor_id = EntityId("actor_pioneer");
    input.tribe_id = TribeId("tribe_first");
    input.trigger_kind = HazardTriggerKind::Approach;
    input.threats = {wolf};
    input.available_objects = {obj("obj_torch", "def_torch", "torch", ObjectCategory::Tool, {"tool", "light_source"}, {"burning", "usable"})};
    input.safe_context_keys = {"escape_route"};
    input.tick = Tick(32);
    auto resolved = HazardResolver{}.resolve(input, {wolfRule()});
    assert(resolved.is_ok());
    assert(severityRank(resolved.value().severity) <= severityRank(DangerSeverity::Notice));
    assert(resolved.value().injury_drafts.empty());
    assert(!resolved.value().knowledge_drafts.empty());
}

static void test_security_rejects_forbidden_output() {
    auto rule = rottenRule();
    rule.feedback_key = "hidden_truth_feedback";
    auto rotten = obj("obj_rotten2", "def_red_berry_rotten", "red_berry_rotten", ObjectCategory::Food, {"fruit"}, {"rotten"});
    HazardExposureInput input;
    input.input_key = "security_flow";
    input.actor_id = EntityId("actor_pioneer");
    input.trigger_kind = HazardTriggerKind::Eat;
    input.threats = {ThreatProfileBuilder{}.fromRuntimeObject(rotten).value()};
    input.tick = Tick(33);
    auto resolved = HazardResolver{}.resolve(input, {rule});
    assert(resolved.is_error());
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "learning" || mode == "all") test_learning_teaching_tribe_flow();
    if (mode == "tool" || mode == "all") test_craft_tool_then_safe_explore();
    if (mode == "security" || mode == "all") test_security_rejects_forbidden_output();
    std::cout << "danger_exploration_flow_test passed\n";
    return 0;
}
