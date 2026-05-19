#include "pathfinder/danger/hazard_resolver.h"
#include "pathfinder/danger/threat_profile_builder.h"
#include "pathfinder/object/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::danger;
using namespace pathfinder::foundation;
using namespace pathfinder::reaction;
using namespace pathfinder::knowledge;
using pathfinder::object::ObjectCategory;

static pathfinder::condition::ConditionExpressionRef cond(std::string key) {
    pathfinder::condition::ConditionExpressionRef ref;
    ref.inline_canonical_key = std::move(key);
    return ref;
}

static ReactionRuntimeObject runtimeObject(std::string id, std::string def, std::string key, ObjectCategory category, std::vector<std::string> tags, std::vector<std::string> states) {
    ReactionRuntimeObject object;
    object.object_id = ObjectId(std::move(id));
    object.definition_id = ObjectDefinitionId(std::move(def));
    object.object_key = std::move(key);
    object.category = category;
    object.quantity = 1;
    object.tag_keys = std::move(tags);
    object.state_keys = std::move(states);
    object.location_key = "test_area";
    return object;
}

static KnowledgeClaim claim(std::string id, std::string subject, std::string action, std::string effect) {
    KnowledgeClaim c;
    c.knowledge_id = KnowledgeId(std::move(id));
    c.owner.kind = KnowledgeOwnerKind::Actor;
    c.owner.entity_id = EntityId("actor_pioneer");
    c.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    c.subject.subject_id = std::move(subject);
    c.subject.subject_type_key = "object_definition";
    c.predicate.relation_type = KnowledgeRelationType::HasRisk;
    c.predicate.action_key = std::move(action);
    c.predicate.effect_key = std::move(effect);
    c.confidence.confidence = 0.6;
    c.confidence.band = KnowledgeConfidenceBand::Reliable;
    c.status = KnowledgeStatus::Hypothesis;
    c.projection_flags.usable_by_ai = true;
    c.projection_flags.usable_for_action = true;
    c.reason_keys = {"test_claim"};
    assert(c.validateBasic().is_ok());
    return c;
}

static HazardRule rottenRule() {
    HazardRule rule;
    rule.rule_key = "rotten_berry_poison_pressure";
    rule.trigger_kind = HazardTriggerKind::Eat;
    rule.source_kind = DangerSourceKind::ObjectReaction;
    rule.condition_refs = {cond("condition:target_state:eq:rotten")};
    rule.severity = DangerSeverity::Harm;
    rule.pressure_delta = 0.6;
    rule.fear_delta = 0.3;
    rule.morale_delta = -0.2;
    rule.trust_delta = -0.1;
    rule.split_risk_delta = 0.3;
    rule.feedback_key = "rotten_food_warning";
    rule.feedback_text = "你吃下腐烂的食物后感到明显不适。";
    rule.knowledge_effect_key = "poison";
    rule.safe_tags = {"food_risk"};
    return rule;
}

static HazardRule wolfRule() {
    HazardRule rule;
    rule.rule_key = "wolf_approach_pressure";
    rule.trigger_kind = HazardTriggerKind::Approach;
    rule.source_kind = DangerSourceKind::Creature;
    rule.severity = DangerSeverity::Severe;
    rule.pressure_delta = 0.7;
    rule.fear_delta = 0.5;
    rule.morale_delta = -0.3;
    rule.feedback_key = "wolf_warning";
    rule.feedback_text = "狼正在逼近，火光和协作能降低风险。";
    rule.knowledge_effect_key = "wolf_risk";
    CountermeasureRequirement torch;
    torch.kind = CountermeasureKind::Tool;
    torch.requirement_key = "light_source";
    torch.mitigation_score = 0.55;
    torch.reason_keys = {"torch_light_deterrence"};
    CountermeasureRequirement route;
    route.kind = CountermeasureKind::EscapeRoute;
    route.requirement_key = "escape_route";
    route.mitigation_score = 0.25;
    route.reason_keys = {"escape_route_available"};
    rule.countermeasure_requirements = {torch, route};
    return rule;
}

static HazardExposureInput inputFor(HazardTriggerKind trigger, ThreatProfile threat) {
    HazardExposureInput input;
    input.input_key = "danger_input";
    input.actor_id = EntityId("actor_pioneer");
    input.tribe_id = TribeId("tribe_first");
    input.trigger_kind = trigger;
    input.threats = {std::move(threat)};
    input.tick = Tick(10);
    return input;
}

static void test_rotten_berry_exposes_and_teaches() {
    auto object = runtimeObject("obj_rotten", "def_red_berry_rotten", "red_berry_rotten", ObjectCategory::Food, {"fruit"}, {"rotten"});
    auto threat = ThreatProfileBuilder{}.fromRuntimeObject(object);
    assert(threat.is_ok());
    auto input = inputFor(HazardTriggerKind::Eat, threat.value());
    auto result = HazardResolver{}.resolve(input, {rottenRule()});
    assert(result.is_ok());
    assert(result.value().decision == DangerDecision::Exposed);
    assert(result.value().severity == DangerSeverity::Harm);
    assert(result.value().injury_drafts.size() == 1);
    assert(result.value().knowledge_drafts.size() == 1);
    assert(result.value().fear_signals.size() == 1);
    assert(result.value().tribe_pressure_drafts.size() == 1);
}

static void test_torch_and_escape_mitigate_wolf() {
    auto threat = ThreatProfileBuilder{}.fromCreatureProjection(EntityId("wolf_001"), "wolf", DangerSeverity::Severe, 0.9);
    assert(threat.is_ok());
    auto input = inputFor(HazardTriggerKind::Approach, threat.value());
    input.available_objects.push_back(runtimeObject("obj_torch", "def_torch", "torch", ObjectCategory::Tool, {"tool", "light_source"}, {"burning", "usable"}));
    input.safe_context_keys.push_back("escape_route");
    auto result = HazardResolver{}.resolve(input, {wolfRule()});
    assert(result.is_ok());
    assert(result.value().decision == DangerDecision::Escaped || result.value().decision == DangerDecision::Avoided);
    assert(severityRank(result.value().severity) <= severityRank(DangerSeverity::Notice));
    assert(result.value().trace.applied_countermeasure_keys.size() == 2);
}

static void test_no_matching_and_blocked() {
    auto object = runtimeObject("obj_fresh", "def_red_berry_fresh", "red_berry_fresh", ObjectCategory::Food, {"fruit"}, {"fresh"});
    auto threat = ThreatProfileBuilder{}.fromRuntimeObject(object).value();
    auto input = inputFor(HazardTriggerKind::Use, threat);
    auto none = HazardResolver{}.resolve(input, {rottenRule()});
    assert(none.is_ok());
    assert(none.value().decision == DangerDecision::NoMatchingHazard);

    input.trigger_kind = HazardTriggerKind::Eat;
    auto blocked = HazardResolver{}.resolve(input, {rottenRule()});
    assert(blocked.is_ok());
    assert(blocked.value().decision == DangerDecision::BlockedByCondition);
}

static void test_knowledge_countermeasure() {
    auto threat = ThreatProfileBuilder{}.fromCreatureProjection(EntityId("wolf_002"), "wolf", DangerSeverity::Harm, 0.7).value();
    auto rule = wolfRule();
    rule.countermeasure_requirements.clear();
    CountermeasureRequirement knowledge;
    knowledge.kind = CountermeasureKind::Knowledge;
    knowledge.requirement_key = "wolf_risk";
    knowledge.mitigation_score = 0.45;
    rule.countermeasure_requirements.push_back(knowledge);
    auto input = inputFor(HazardTriggerKind::Approach, threat);
    input.actor_knowledge_claims.push_back(claim("know_wolf_risk", "wolf", "approach", "wolf_risk"));
    auto result = HazardResolver{}.resolve(input, {rule});
    assert(result.is_ok());
    assert(severityRank(result.value().severity) < severityRank(DangerSeverity::Severe));
    assert(!result.value().trace.applied_countermeasure_keys.empty());
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "rotten" || mode == "all") test_rotten_berry_exposes_and_teaches();
    if (mode == "torch" || mode == "all") test_torch_and_escape_mitigate_wolf();
    if (mode == "blocked" || mode == "all") test_no_matching_and_blocked();
    if (mode == "knowledge" || mode == "all") test_knowledge_countermeasure();
    std::cout << "hazard_resolver_test passed\n";
    return 0;
}
