#include "pathfinder/cognition/cognition_state.h"
#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/reaction/reaction_applier.h"
#include "pathfinder/reaction/reaction_learning_bridge.h"
#include "pathfinder/reaction/reaction_resolver.h"
#include "pathfinder/tribe/tribe_conflict.h"
#include "pathfinder/tribe/tribe_coordination.h"
#include "pathfinder/tribe/tribe_state.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace pathfinder::reaction;
using namespace pathfinder::knowledge;
using namespace pathfinder::tribe;
using pathfinder::cognition::CognitionEffectKind;
using pathfinder::cognition::CognitionEvidence;
using pathfinder::cognition::CognitionKey;
using pathfinder::cognition::CognitionState;
using pathfinder::cognition::CognitionUpdater;
using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::ActionId;
using pathfinder::foundation::EntityId;
using pathfinder::foundation::EventId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Tick;
using pathfinder::foundation::TribeId;
using pathfinder::object::ObjectCategory;

static ConditionExpressionRef condition(const std::string& key) {
    ConditionExpressionRef ref;
    ref.inline_canonical_key = key;
    return ref;
}

static ItemDefinitionContent item(
    const std::string& definition_id,
    const std::string& object_key,
    ObjectCategory category,
    const std::vector<std::string>& tags,
    const std::vector<std::string>& states) {
    ItemDefinitionContent value;
    value.definition_id = ObjectDefinitionId(definition_id);
    value.object_key = object_key;
    value.display_name = object_key;
    value.category = category;
    value.tag_keys = tags;
    value.default_state_keys = states;
    value.safe_description = object_key;
    return value;
}

static ReactionObjectPattern pattern(
    ReactionObjectRole role,
    const std::string& definition_id,
    ObjectCategory category,
    const std::vector<std::string>& required_tags = {}) {
    ReactionObjectPattern value;
    value.role = role;
    value.definition_id = ObjectDefinitionId(definition_id);
    value.category = category;
    value.required_tag_keys = required_tags;
    return value;
}

static ReactionOutputTemplate output(
    ReactionOutputKind kind,
    ReactionObjectRole role,
    const std::string& product_definition_id = "",
    const std::string& state_key = "",
    const std::string& resource_key = "",
    double resource_delta = 0.0,
    int quantity_delta = 0) {
    ReactionOutputTemplate value;
    value.output_kind = kind;
    value.target_role = role;
    if (!product_definition_id.empty()) value.product_definition_id = ObjectDefinitionId(product_definition_id);
    value.state_key = state_key;
    value.resource_key = resource_key;
    value.resource_delta = resource_delta;
    value.quantity_delta = quantity_delta;
    return value;
}

static ObjectReactionRule rule(
    const std::string& rule_key,
    ReactionActionKind action,
    std::vector<ReactionObjectPattern> patterns,
    std::vector<ConditionExpressionRef> conditions,
    std::vector<ReactionOutputTemplate> outputs,
    const std::string& effect_key) {
    ObjectReactionRule value;
    value.rule_key = rule_key;
    value.action_kind = action;
    value.object_patterns = std::move(patterns);
    value.condition_refs = std::move(conditions);
    value.output_templates = std::move(outputs);
    value.feedback_key = "reaction." + rule_key;
    value.feedback_text = "系统压力测试反馈";
    value.knowledge_effect_key = effect_key;
    value.priority = 10;
    return value;
}

static ReactionKnowledgeTemplate tmpl(const std::string& rule_key, const std::string& subject_id, const std::string& action_key, const std::string& effect_key) {
    ReactionKnowledgeTemplate value;
    value.rule_key = rule_key;
    value.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    value.subject_id = subject_id;
    value.action_key = action_key;
    value.effect_key = effect_key;
    value.shareable = true;
    value.teachable = true;
    value.npc_learnable = true;
    value.observation_learnable = true;
    return value;
}

static ReactionContentPack stressPack() {
    ReactionContentPack pack;
    assert(pack.catalog.addItem(item("def_fire_source", "fire_source", ObjectCategory::Hazard, {"fire_source"}, {"burning"})).is_ok());
    assert(pack.catalog.addItem(item("def_dry_branch", "dry_branch", ObjectCategory::Material, {"combustible", "branch_like"}, {"dry"})).is_ok());
    assert(pack.catalog.addItem(item("def_wet_branch", "wet_branch", ObjectCategory::Material, {"combustible", "branch_like"}, {"wet"})).is_ok());
    auto torch = item("def_torch", "torch", ObjectCategory::Tool, {"tool", "light_source", "combustible"}, {"burning", "usable"});
    torch.default_resource_values["fuel"] = 100.0;
    assert(pack.catalog.addItem(torch).is_ok());
    assert(pack.catalog.addItem(item("def_clean_water", "clean_water", ObjectCategory::Food, {"drinkable", "water_source"}, {"clean", "available"})).is_ok());
    assert(pack.catalog.addItem(item("def_stone_flake", "stone_flake", ObjectCategory::Tool, {"tool", "sharp"}, {"usable"})).is_ok());
    assert(pack.catalog.addItem(item("def_spear", "spear", ObjectCategory::Tool, {"tool", "weapon"}, {"usable"})).is_ok());
    assert(pack.catalog.addItem(item("def_vine", "vine", ObjectCategory::Material, {"binding"}, {"flexible"})).is_ok());
    assert(pack.catalog.addItem(item("def_branch_bundle", "branch_bundle", ObjectCategory::Material, {"bundle", "branch_like"}, {"bound"})).is_ok());
    assert(pack.catalog.addItem(item("def_red_berry_fresh", "red_berry_fresh", ObjectCategory::Food, {"edible_candidate", "fruit"}, {"fresh"})).is_ok());
    assert(pack.catalog.addItem(item("def_red_berry_rotten", "red_berry_rotten", ObjectCategory::Food, {"edible_candidate", "fruit"}, {"rotten"})).is_ok());

    pack.rules = {
        rule("make_torch", ReactionActionKind::Combine,
             {pattern(ReactionObjectRole::Source, "def_fire_source", ObjectCategory::Hazard, {"fire_source"}),
              pattern(ReactionObjectRole::Material, "def_dry_branch", ObjectCategory::Material, {"combustible", "branch_like"})},
             {condition("condition:source_state:eq:burning"), condition("condition:target_state:eq:dry")},
             {output(ReactionOutputKind::TransformObject, ReactionObjectRole::Material, "def_torch")}, "transform_to_torch"),
        rule("use_torch", ReactionActionKind::Use,
             {pattern(ReactionObjectRole::Target, "def_torch", ObjectCategory::Tool, {"light_source"})},
             {condition("condition:target_state:eq:burning"), condition("condition:target_state:eq:usable")},
             {output(ReactionOutputKind::ResourceDelta, ReactionObjectRole::Target, "", "", "fuel", -10.0)}, "light_source_usable"),
        rule("extinguish_fire", ReactionActionKind::Combine,
             {pattern(ReactionObjectRole::Source, "def_clean_water", ObjectCategory::Food, {"water_source"}),
              pattern(ReactionObjectRole::Target, "def_fire_source", ObjectCategory::Hazard, {"fire_source"})},
             {condition("condition:source_state:eq:clean"), condition("condition:target_state:eq:burning")},
             {output(ReactionOutputKind::RemoveState, ReactionObjectRole::Target, "", "burning")}, "extinguish_fire"),
        rule("drink_water", ReactionActionKind::Use,
             {pattern(ReactionObjectRole::Target, "def_clean_water", ObjectCategory::Food, {"drinkable"})},
             {condition("condition:target_state:eq:available")},
             {output(ReactionOutputKind::ConsumeObject, ReactionObjectRole::Target, "", "", "", 0.0, -1)}, "safe_to_drink"),
        rule("craft_spear", ReactionActionKind::Combine,
             {pattern(ReactionObjectRole::Source, "def_stone_flake", ObjectCategory::Tool, {"sharp"}),
              pattern(ReactionObjectRole::Material, "def_dry_branch", ObjectCategory::Material, {"branch_like"})},
             {condition("condition:source_state:eq:usable"), condition("condition:target_state:eq:dry")},
             {output(ReactionOutputKind::TransformObject, ReactionObjectRole::Material, "def_spear")}, "craft_spear"),
        rule("bind_bundle", ReactionActionKind::Combine,
             {pattern(ReactionObjectRole::Source, "def_vine", ObjectCategory::Material, {"binding"}),
              pattern(ReactionObjectRole::Material, "def_dry_branch", ObjectCategory::Material, {"branch_like"})},
             {condition("condition:source_state:eq:flexible"), condition("condition:target_state:eq:dry")},
             {output(ReactionOutputKind::ProduceObject, ReactionObjectRole::Product, "def_branch_bundle", "", "", 0.0, 1)}, "produce_bundle"),
        rule("eat_fresh", ReactionActionKind::Use,
             {pattern(ReactionObjectRole::Target, "def_red_berry_fresh", ObjectCategory::Food, {"edible_candidate"})},
             {condition("condition:target_state:eq:fresh")},
             {output(ReactionOutputKind::ConsumeObject, ReactionObjectRole::Target, "", "", "", 0.0, -1)}, "restore_hunger"),
        rule("eat_rotten", ReactionActionKind::Use,
             {pattern(ReactionObjectRole::Target, "def_red_berry_rotten", ObjectCategory::Food, {"edible_candidate"})},
             {condition("condition:target_state:eq:rotten")},
             {output(ReactionOutputKind::ConsumeObject, ReactionObjectRole::Target, "", "", "", 0.0, -1)}, "poison")
    };

    pack.knowledge_templates = {
        tmpl("make_torch", "def_dry_branch", "combine_with_fire", "transform_to_torch"),
        tmpl("use_torch", "def_torch", "use", "light_source_usable"),
        tmpl("extinguish_fire", "def_fire_source", "combine_with_water", "extinguish_fire"),
        tmpl("drink_water", "def_clean_water", "drink", "safe_to_drink"),
        tmpl("craft_spear", "def_dry_branch", "craft_with_stone", "craft_spear"),
        tmpl("bind_bundle", "def_dry_branch", "bind_with_vine", "produce_bundle"),
        tmpl("eat_fresh", "def_red_berry_fresh", "eat", "restore_hunger"),
        tmpl("eat_rotten", "def_red_berry_rotten", "eat", "poison")
    };
    assert(ReactionContentValidator{}.validate(pack).is_ok());
    return pack;
}

static ReactionRuntimeWorld worldWith(const ReactionContentPack& pack, const std::vector<std::pair<std::string, std::string>>& objects) {
    ReactionRuntimeWorld world;
    for (const auto& [object_id, definition_id] : objects) {
        auto object = pack.catalog.instantiate(ObjectId(object_id), ObjectDefinitionId(definition_id), "test_area");
        assert(object.is_ok());
        assert(world.addObject(object.value()).is_ok());
    }
    return world;
}

static ReactionResult resolve(
    const ReactionContentPack& pack,
    const ReactionRuntimeWorld& world,
    ReactionActionKind action,
    const std::vector<std::pair<std::string, ReactionObjectRole>>& roles,
    Tick tick = Tick(1)) {
    std::vector<std::pair<ObjectId, ReactionObjectRole>> object_roles;
    for (const auto& [id, role] : roles) object_roles.push_back({ObjectId(id), role});
    auto input = ReactionInputBuilder{}.buildInput(world, action, EntityId("actor_pioneer"), object_roles, tick, "input_" + std::to_string(tick.value()));
    assert(input.is_ok());
    auto result = ObjectReactionResolver{}.resolve(input.value(), pack.rules);
    assert(result.is_ok());
    return result.value();
}

static ReactionRuntimeWorld apply(const ReactionContentPack& pack, const ReactionRuntimeWorld& world, const ReactionResult& result) {
    auto applied = ReactionChangeApplier{}.apply(world, pack.catalog, result);
    assert(applied.is_ok());
    return applied.value().world;
}

static ReactionKnowledgePlan planKnowledge(
    const ReactionContentPack& pack,
    const ReactionResult& result,
    std::vector<EntityId> observers = {},
    std::vector<EntityId> taught = {}) {
    ReactionKnowledgePlannerInput input;
    input.actor_id = EntityId("actor_pioneer");
    input.observer_ids = std::move(observers);
    input.taught_ids = std::move(taught);
    input.result = result;
    input.knowledge_templates = pack.knowledge_templates;
    auto plan = ReactionKnowledgePlanner{}.plan(input);
    assert(plan.is_ok());
    return plan.value();
}

static KnowledgeOwner owner(const std::string& id) {
    KnowledgeOwner value;
    value.kind = KnowledgeOwnerKind::Agent;
    value.entity_id = EntityId(id);
    return value;
}

static KnowledgeStatus statusFromConfidence(double confidence) {
    if (confidence >= 0.75) return KnowledgeStatus::Active;
    if (confidence >= 0.35) return KnowledgeStatus::Shared;
    if (confidence >= 0.10) return KnowledgeStatus::Hypothesis;
    return KnowledgeStatus::Candidate;
}

static KnowledgeClaim claimFromDelivery(const ReactionKnowledgeDeliveryDraft& delivery, Tick tick, size_t index) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("know_" + delivery.owner_id.value() + "_" + delivery.knowledge.subject_id + "_" + delivery.knowledge.effect_key + "_" + std::to_string(index));
    claim.owner = owner(delivery.owner_id.value());
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = delivery.knowledge.subject_id;
    claim.subject.subject_type_key = "reaction_item";
    claim.predicate.relation_type = delivery.knowledge.effect_key == "poison" ? KnowledgeRelationType::HasRisk : KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = delivery.knowledge.action_key;
    claim.predicate.effect_key = delivery.knowledge.effect_key;
    claim.conditions = delivery.knowledge.conditions;
    claim.confidence.confidence = delivery.confidence_hint;
    claim.confidence.band = confidenceToBand(delivery.confidence_hint);
    claim.confidence.support_count = 1;
    claim.status = statusFromConfidence(delivery.confidence_hint);
    claim.created_tick = tick;
    claim.updated_tick = tick;
    claim.teaching_profile.teachable = delivery.teachable;
    claim.teaching_profile.required_confidence = 0.60;
    if (delivery.teachable) claim.teaching_profile.teaching_message_key = "teach_reaction_knowledge";
    claim.projection_flags.usable_for_action = true;
    claim.projection_flags.usable_for_teaching = delivery.teachable;
    KnowledgeEvidence evidence;
    evidence.evidence_id = KnowledgeEvidenceId("kevd_" + claim.knowledge_id.value() + "_0");
    evidence.evidence_kind = delivery.source_kind == ReactionLearningSourceKind::Taught ? KnowledgeEvidenceKind::Teaching : KnowledgeEvidenceKind::DirectExperience;
    evidence.source_summary_id = pathfinder::memory::MemorySummaryId("summary_" + claim.knowledge_id.value());
    evidence.supports_claim = true;
    evidence.confidence_delta = delivery.confidence_hint;
    evidence.reliability = delivery.confidence_hint;
    evidence.observed_tick = tick;
    claim.evidence_refs.push_back(evidence);
    assert(claim.validateBasic().is_ok());
    return claim;
}

static std::vector<KnowledgeClaim> teach(const std::vector<KnowledgeClaim>& source, const KnowledgeOwner& from, const KnowledgeOwner& to, const std::vector<KnowledgeClaim>& existing) {
    KnowledgePropagationAttempt attempt;
    attempt.attempt_key = "attempt_" + from.entity_id.value() + "_to_" + to.entity_id.value();
    attempt.propagator.owner = from;
    attempt.target.owner = to;
    attempt.context.channel = KnowledgePropagationChannel::DirectTeaching;
    attempt.context.trust_band = KnowledgePropagationTrustBand::High;
    attempt.context.trust_score = 0.9;
    attempt.context.propagation_tick = Tick(50);
    for (const auto& claim : source) {
        KnowledgePropagationSourceClaim sc;
        sc.claim = claim;
        sc.explicit_teaching_intent = true;
        attempt.source_claims.push_back(sc);
    }
    attempt.target_existing_claims = existing;
    auto result = KnowledgePropagationService{}.propagate(attempt);
    assert(result.is_ok());
    auto snapshot = KnowledgePropagationApplier{}.applyToTargetSnapshot(existing, result.value());
    assert(snapshot.is_ok());
    return snapshot.value();
}

static TribeState makeTribe(const std::string& id) {
    TribeState state;
    state.tribe_id = TribeId(id);
    state.display_key = id;
    state.morale.overall = 0.7;
    state.trust.leader_trust = 0.7;
    state.trust.member_trust = 0.7;
    state.trust.teaching_fairness = 0.7;
    state.split_risk.risk = 0.2;
    state.split_risk.resource_pressure = 0.1;
    state.split_risk.trust_pressure = 0.3;
    state.split_risk.knowledge_conflict_pressure = 0.1;
    state.split_risk.casualty_pressure = 0.1;
    state.split_risk.cohesion_state = cohesionStateForRisk(0.2);
    for (auto [member_id, role] : std::vector<std::pair<std::string, TribeMemberRole>>{{"pioneer", TribeMemberRole::Pioneer}, {"guardian", TribeMemberRole::Guardian}, {"learner", TribeMemberRole::Learner}}) {
        TribeMember member;
        member.member_id = EntityId(member_id + "_" + id);
        member.role = role;
        member.trust = 0.7;
        member.morale = 0.7;
        member.joined_tick = Tick(1);
        member.updated_tick = Tick(1);
        state.members.push_back(member);
    }
    state.population_total = static_cast<int>(state.members.size());
    state.active_population = state.population_total;
    assert(state.validateBasic().is_ok());
    return state;
}

static CombatCoordinationResult coordinate(const TribeState& state, double threat_pressure = 0.4) {
    CombatCoordinationInput input;
    input.tribe_id = state.tribe_id;
    input.input_tick = Tick(60);
    input.tribe_state = state;
    input.requested_intent = GroupIntentKind::DefendGroup;
    input.safety_pressure = threat_pressure;
    ThreatPressureSummary threat;
    threat.threat_id = EntityId("threat_wolf");
    threat.threat_key = "wolf";
    threat.threat_pressure = threat_pressure;
    threat.proximity_pressure = threat_pressure * 0.5;
    threat.target_pressure = threat_pressure * 0.4;
    if (!state.shared_knowledge_ids.empty()) threat.known_counter_knowledge_ids = {state.shared_knowledge_ids.front()};
    input.threats.push_back(threat);
    auto result = CombatCoordinationResolver{}.resolve(input);
    assert(result.is_ok());
    return result.value();
}

static ConflictParticipantTribe participant(const TribeState& state, const std::string& role, const CombatCoordinationResult& coord) {
    ConflictParticipantTribe value;
    value.tribe_id = state.tribe_id;
    value.role_in_conflict = role;
    value.member_count_summary = state.population_total;
    value.active_population_summary = state.active_population;
    value.morale_summary = state.morale.overall;
    value.trust_summary = state.trust.member_trust;
    value.split_risk_summary = state.split_risk.risk;
    value.safety_pressure_summary = state.morale.safety_pressure;
    value.loss_pressure_summary = state.split_risk.casualty_pressure;
    value.coordination_result = coord;
    value.coordination_state_draft = coord.state_draft;
    value.public_knowledge_ids = state.shared_knowledge_ids;
    return value;
}

static TribeRelation relation(const TribeId& source, const TribeId& target, HostilityState hostility) {
    TribeRelation value;
    value.source_tribe_id = source;
    value.target_tribe_id = target;
    value.hostility_state = hostility;
    value.trust_score = 0.2;
    value.fear_pressure = 0.4;
    value.resource_tension = 0.4;
    return value;
}

static ConflictPressureSummary pressure(double resource, double fear, double survival) {
    ConflictPressureSummary value;
    value.conflict_key = "stress_conflict";
    value.resource_pressure = resource;
    value.territory_pressure = 0.5;
    value.fear_pressure = fear;
    value.survival_pressure = survival;
    return value;
}

static void applyCognition(CognitionState& state, const std::string& actor, const std::string& definition, CognitionEffectKind effect, double delta, const std::string& event_id) {
    CognitionEvidence evidence;
    evidence.key = CognitionKey{EntityId(actor), ObjectDefinitionId(definition), ActionId("eat"), effect};
    evidence.source_event_id = EventId(event_id);
    evidence.confidence_delta = delta;
    auto result = CognitionUpdater::applyEvidence(state, evidence);
    assert(result.is_ok());
}

static void t01_full_synthesis_to_knowledge_tribe_cognition() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"fire", "def_fire_source"}, {"branch", "def_dry_branch"}, {"berry", "def_red_berry_fresh"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"fire", ReactionObjectRole::Source}, {"branch", ReactionObjectRole::Material}}, Tick(1));
    assert(result.decision == ReactionDecision::Reacted);
    world = apply(pack, world, result);
    assert(world.findObject(ObjectId("branch"))->definition_id == ObjectDefinitionId("def_torch"));
    auto plan = planKnowledge(pack, result);
    auto claim = claimFromDelivery(plan.deliveries.front(), Tick(1), 0);
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(2);
    input.knowledge_events = {{claim.knowledge_id, {"torch_shared"}}};
    auto tribe_result = TribeStateResolver{}.resolve(makeTribe("tribe_alpha"), input);
    assert(tribe_result.is_ok());
    assert(tribe_result.value().updated_state.shared_knowledge_ids.size() == 1);
    CognitionState cognition;
    applyCognition(cognition, "actor_pioneer", "def_red_berry_fresh", CognitionEffectKind::Edible, 0.4, "evt_cog_1");
    assert(cognition.findEdibleClaim(EntityId("actor_pioneer"), ObjectDefinitionId("def_red_berry_fresh"), ActionId("eat")) != nullptr);
}

static void t02_use_generated_torch_consumes_fuel() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"torch", "def_torch"}});
    auto before = world.findObject(ObjectId("torch"))->resource_values.at("fuel");
    auto result = resolve(pack, world, ReactionActionKind::Use, {{"torch", ReactionObjectRole::Target}}, Tick(2));
    auto after_world = apply(pack, world, result);
    assert(after_world.findObject(ObjectId("torch"))->resource_values.at("fuel") == before - 10.0);
}

static void t03_missing_material_then_supply_and_retry() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"stone", "def_stone_flake"}});
    auto missing = ReactionInputBuilder{}.buildInput(world, ReactionActionKind::Combine, EntityId("actor_pioneer"), {{ObjectId("stone"), ReactionObjectRole::Source}, {ObjectId("branch_missing"), ReactionObjectRole::Material}}, Tick(3), "missing_branch");
    assert(missing.is_error());
    assert(world.addObject(pack.catalog.instantiate(ObjectId("branch"), ObjectDefinitionId("def_dry_branch"), "test_area").value()).is_ok());
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"stone", ReactionObjectRole::Source}, {"branch", ReactionObjectRole::Material}}, Tick(4));
    assert(result.decision == ReactionDecision::Reacted);
}

static void t04_wet_branch_blocks_torch() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"fire", "def_fire_source"}, {"wet", "def_wet_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"fire", ReactionObjectRole::Source}, {"wet", ReactionObjectRole::Material}}, Tick(5));
    assert(result.decision == ReactionDecision::NoMatchingRule || result.decision == ReactionDecision::ConditionBlocked);
}

static void t05_wrong_action_no_match() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"fire", "def_fire_source"}, {"branch", "def_dry_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Use, {{"branch", ReactionObjectRole::Target}}, Tick(6));
    assert(result.decision == ReactionDecision::NoMatchingRule);
}

static void t06_swapped_roles_no_match() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"fire", "def_fire_source"}, {"branch", "def_dry_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"branch", ReactionObjectRole::Source}, {"fire", ReactionObjectRole::Material}}, Tick(7));
    assert(result.decision == ReactionDecision::NoMatchingRule);
}

static void t07_consumed_water_cannot_drink_again() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"water", "def_clean_water"}});
    auto first = resolve(pack, world, ReactionActionKind::Use, {{"water", ReactionObjectRole::Target}}, Tick(8));
    world = apply(pack, world, first);
    assert(world.findObject(ObjectId("water"))->quantity == 0);
    auto second = resolve(pack, world, ReactionActionKind::Use, {{"water", ReactionObjectRole::Target}}, Tick(9));
    assert(second.decision == ReactionDecision::NoMatchingRule);
}

static void t08_extinguish_fire_removes_burning() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"water", "def_clean_water"}, {"fire", "def_fire_source"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"water", ReactionObjectRole::Source}, {"fire", ReactionObjectRole::Target}}, Tick(10));
    world = apply(pack, world, result);
    const auto& states = world.findObject(ObjectId("fire"))->state_keys;
    assert(std::find(states.begin(), states.end(), "burning") == states.end());
}

static void t09_craft_spear_transform_branch() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"stone", "def_stone_flake"}, {"branch", "def_dry_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"stone", ReactionObjectRole::Source}, {"branch", ReactionObjectRole::Material}}, Tick(11));
    world = apply(pack, world, result);
    assert(world.findObject(ObjectId("branch"))->definition_id == ObjectDefinitionId("def_spear"));
}

static void t10_bind_bundle_produces_new_object() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"vine", "def_vine"}, {"branch", "def_dry_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"vine", ReactionObjectRole::Source}, {"branch", ReactionObjectRole::Material}}, Tick(12));
    auto applied = ReactionChangeApplier{}.apply(world, pack.catalog, result, ReactionApplyOptions{"test_area", "obj_bundle", 1});
    assert(applied.is_ok());
    assert(applied.value().world.findObject(ObjectId("obj_bundle_1")) != nullptr);
}

static void t11_actor_observer_and_taught_knowledge_drafts() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"fire", "def_fire_source"}, {"branch", "def_dry_branch"}});
    auto result = resolve(pack, world, ReactionActionKind::Combine, {{"fire", ReactionObjectRole::Source}, {"branch", ReactionObjectRole::Material}}, Tick(13));
    auto plan = planKnowledge(pack, result, {EntityId("actor_observer")}, {EntityId("actor_student")});
    assert(plan.deliveries.size() == 3);
    assert(plan.deliveries[0].source_kind == ReactionLearningSourceKind::DirectExperience);
    assert(plan.deliveries[1].source_kind == ReactionLearningSourceKind::Observation);
    assert(plan.deliveries[2].source_kind == ReactionLearningSourceKind::Taught);
}

static void t12_teaching_reinforces_without_duplicate() {
    auto pack = stressPack();
    auto world = worldWith(pack, {{"berry", "def_red_berry_fresh"}});
    auto result = resolve(pack, world, ReactionActionKind::Use, {{"berry", ReactionObjectRole::Target}}, Tick(14));
    auto delivery = planKnowledge(pack, result).deliveries.front();
    auto teacher_claim = claimFromDelivery(delivery, Tick(14), 0);
    auto student = teach({teacher_claim}, owner("actor_pioneer"), owner("actor_student"), {});
    assert(student.size() == 1);
    auto reinforced = teach({teacher_claim}, owner("actor_pioneer"), owner("actor_student"), student);
    assert(reinforced.size() == 1);
    assert(reinforced[0].confidence.support_count >= student[0].confidence.support_count + 1);
}

static void t13_mixed_create_and_reinforce_stays_two_claims() {
    ReactionKnowledgeDeliveryDraft fresh;
    fresh.owner_id = EntityId("actor_teacher");
    fresh.source_kind = ReactionLearningSourceKind::DirectExperience;
    fresh.knowledge.knowledge_key = "fresh";
    fresh.knowledge.subject_id = "def_red_berry_fresh";
    fresh.knowledge.action_key = "eat";
    fresh.knowledge.effect_key = "restore_hunger";
    fresh.confidence_hint = 0.9;
    auto rotten = fresh;
    rotten.knowledge.knowledge_key = "rotten";
    rotten.knowledge.subject_id = "def_red_berry_rotten";
    rotten.knowledge.effect_key = "poison";
    std::vector<KnowledgeClaim> teacher{claimFromDelivery(fresh, Tick(15), 0), claimFromDelivery(rotten, Tick(15), 1)};
    fresh.owner_id = EntityId("actor_student");
    fresh.confidence_hint = 0.4;
    std::vector<KnowledgeClaim> existing{claimFromDelivery(fresh, Tick(15), 0)};
    auto student = teach(teacher, owner("actor_teacher"), owner("actor_student"), existing);
    assert(student.size() == 2);
}

static void t14_cognition_repeated_fresh_berry_reinforces_edible() {
    CognitionState state;
    applyCognition(state, "actor_pioneer", "def_red_berry_fresh", CognitionEffectKind::Edible, 0.3, "evt_cog_14a");
    applyCognition(state, "actor_pioneer", "def_red_berry_fresh", CognitionEffectKind::Edible, 0.4, "evt_cog_14b");
    auto* claim = state.findEdibleClaim(EntityId("actor_pioneer"), ObjectDefinitionId("def_red_berry_fresh"), ActionId("eat"));
    assert(claim != nullptr);
    assert(claim->confidence == 0.7);
    assert(claim->evidence_count == 2);
}

static void t15_cognition_rotten_harmful_separate_from_fresh_edible() {
    CognitionState state;
    applyCognition(state, "actor_pioneer", "def_red_berry_fresh", CognitionEffectKind::Edible, 0.4, "evt_cog_15a");
    applyCognition(state, "actor_pioneer", "def_red_berry_rotten", CognitionEffectKind::Harmful, 0.6, "evt_cog_15b");
    assert(state.claims().size() == 2);
    assert(state.findHarmfulClaim(EntityId("actor_pioneer"), ObjectDefinitionId("def_red_berry_rotten"), ActionId("eat")) != nullptr);
}

static void t16_cognition_taught_companion_can_act_on_claim() {
    CognitionState state;
    applyCognition(state, "actor_companion", "def_red_berry_rotten", CognitionEffectKind::Harmful, 0.35, "evt_taught_16");
    auto* claim = state.findHarmfulClaim(EntityId("actor_companion"), ObjectDefinitionId("def_red_berry_rotten"), ActionId("eat"));
    assert(claim != nullptr);
    assert(claim->confidence >= 0.35);
}

static void t17_tribe_links_shared_knowledge_once() {
    auto state = makeTribe("tribe_alpha");
    TribeStateInput input;
    input.tribe_id = state.tribe_id;
    input.input_tick = Tick(17);
    input.knowledge_events = {{KnowledgeId("know_torch"), {"shared"}}};
    auto first = TribeStateResolver{}.resolve(state, input);
    assert(first.is_ok());
    input.input_tick = Tick(18);
    auto second = TribeStateResolver{}.resolve(first.value().updated_state, input);
    assert(second.is_ok());
    assert(second.value().updated_state.shared_knowledge_ids.size() == 1);
}

static void t18_tribe_high_conflict_pressure_fractures() {
    auto state = makeTribe("tribe_alpha");
    TribeStateInput input;
    input.tribe_id = state.tribe_id;
    input.input_tick = Tick(18);
    input.resource_pressure = 0.9;
    input.safety_pressure = 0.8;
    input.knowledge_conflict_pressure = 0.9;
    input.casualty_pressure = 0.7;
    input.trust_delta = -0.4;
    auto result = TribeStateResolver{}.resolve(state, input);
    assert(result.is_ok());
    assert(result.value().updated_state.split_risk.risk >= 0.5);
    assert(result.value().updated_state.split_risk.cohesion_state == TribeCohesionState::Fracturing ||
           result.value().updated_state.split_risk.cohesion_state == TribeCohesionState::Splintered ||
           result.value().updated_state.split_risk.cohesion_state == TribeCohesionState::Tense);
}

static void t19_member_removed_after_split_pressure() {
    auto state = makeTribe("tribe_alpha");
    TribeStateInput input;
    input.tribe_id = state.tribe_id;
    input.input_tick = Tick(19);
    TribeMemberEvent remove;
    remove.kind = TribeStateChangeKind::MemberRemoved;
    remove.member_id = state.members.back().member_id;
    remove.reason_keys = {"splinter_group_left"};
    input.member_events.push_back(remove);
    input.knowledge_conflict_pressure = 0.8;
    auto result = TribeStateResolver{}.resolve(state, input);
    assert(result.is_ok());
    assert(result.value().updated_state.population_total == state.population_total - 1);
    assert(std::any_of(result.value().state_changes.begin(), result.value().state_changes.end(), [](const auto& change) { return change.kind == TribeStateChangeKind::MemberRemoved; }));
}

static void t20_coordination_and_intertribe_conflict_remain_safe() {
    auto alpha = makeTribe("tribe_alpha");
    alpha.shared_knowledge_ids = {KnowledgeId("know_spear")};
    auto beta = makeTribe("tribe_beta");
    beta.morale.overall = 0.35;
    beta.trust.member_trust = 0.35;
    auto coord_a = coordinate(alpha, 0.5);
    auto coord_b = coordinate(beta, 0.7);
    assert(coord_a.validateBasic().is_ok());
    ConflictResolutionInput input;
    input.source = participant(alpha, "source", coord_a);
    input.target = participant(beta, "target", coord_b);
    input.source_relation_to_target = relation(alpha.tribe_id, beta.tribe_id, HostilityState::Threatened);
    input.pressure_summary = pressure(0.8, 0.7, 0.6);
    auto result = GroupCombatResolver{}.resolve(input);
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().outcome_kind != ConflictOutcomeKind::Unknown);
    assert(result.value().projection.public_summary_key.find("death") == std::string::npos);
    assert(result.value().projection.public_summary_key.find("loot") == std::string::npos);
}

static const std::map<std::string, std::function<void()>>& tests() {
    static const std::map<std::string, std::function<void()>> value = {
        {"01_full_pipeline", t01_full_synthesis_to_knowledge_tribe_cognition},
        {"02_use_generated_torch", t02_use_generated_torch_consumes_fuel},
        {"03_missing_material_retry", t03_missing_material_then_supply_and_retry},
        {"04_wet_branch_blocked", t04_wet_branch_blocks_torch},
        {"05_wrong_action", t05_wrong_action_no_match},
        {"06_swapped_roles", t06_swapped_roles_no_match},
        {"07_consumed_water", t07_consumed_water_cannot_drink_again},
        {"08_extinguish_fire", t08_extinguish_fire_removes_burning},
        {"09_craft_spear", t09_craft_spear_transform_branch},
        {"10_bind_bundle", t10_bind_bundle_produces_new_object},
        {"11_knowledge_drafts", t11_actor_observer_and_taught_knowledge_drafts},
        {"12_teach_reinforce", t12_teaching_reinforces_without_duplicate},
        {"13_mixed_teaching", t13_mixed_create_and_reinforce_stays_two_claims},
        {"14_cognition_repeat", t14_cognition_repeated_fresh_berry_reinforces_edible},
        {"15_cognition_separate", t15_cognition_rotten_harmful_separate_from_fresh_edible},
        {"16_cognition_taught", t16_cognition_taught_companion_can_act_on_claim},
        {"17_tribe_knowledge", t17_tribe_links_shared_knowledge_once},
        {"18_tribe_split_pressure", t18_tribe_high_conflict_pressure_fractures},
        {"19_member_split", t19_member_removed_after_split_pressure},
        {"20_coordination_conflict", t20_coordination_and_intertribe_conflict_remain_safe},
    };
    return value;
}

int main(int argc, char* argv[]) {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "all") {
        for (const auto& [name, fn] : tests()) {
            fn();
            std::cout << name << " passed" << std::endl;
        }
        return 0;
    }
    auto it = tests().find(mode);
    assert(it != tests().end());
    it->second();
    std::cout << mode << " passed" << std::endl;
    return 0;
}
