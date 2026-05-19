#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/reaction/reaction_applier.h"
#include "pathfinder/reaction/reaction_learning_bridge.h"
#include "pathfinder/reaction/reaction_resolver.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace pathfinder::knowledge;
using namespace pathfinder::reaction;
using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::EntityId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Tick;
using pathfinder::object::ObjectCategory;

static ConditionExpressionRef condition(const std::string& key) {
    ConditionExpressionRef ref;
    ref.inline_canonical_key = key;
    return ref;
}

static KnowledgeOwner owner(const std::string& id) {
    KnowledgeOwner value;
    value.kind = KnowledgeOwnerKind::Agent;
    value.entity_id = EntityId(id);
    return value;
}

static ItemDefinitionContent item(
    const std::string& definition_id,
    const std::string& object_key,
    const std::string& display_name,
    const std::vector<std::string>& tags,
    const std::vector<std::string>& states) {
    ItemDefinitionContent value;
    value.definition_id = ObjectDefinitionId(definition_id);
    value.object_key = object_key;
    value.display_name = display_name;
    value.category = ObjectCategory::Food;
    value.tag_keys = tags;
    value.default_state_keys = states;
    value.safe_description = display_name;
    return value;
}

static ObjectReactionRule eatRule(
    const std::string& rule_key,
    const std::string& definition_id,
    const std::string& required_state,
    const std::string& effect_key,
    const std::string& feedback_text) {
    ObjectReactionRule rule;
    rule.rule_key = rule_key;
    rule.action_kind = ReactionActionKind::Use;

    ReactionObjectPattern target;
    target.role = ReactionObjectRole::Target;
    target.definition_id = ObjectDefinitionId(definition_id);
    target.category = ObjectCategory::Food;
    target.required_tag_keys = {"edible_candidate"};
    rule.object_patterns = {target};

    rule.condition_refs = {condition("condition:target_state:eq:" + required_state)};

    ReactionOutputTemplate consume;
    consume.output_kind = ReactionOutputKind::ConsumeObject;
    consume.target_role = ReactionObjectRole::Target;
    consume.quantity_delta = -1;
    rule.output_templates = {consume};

    rule.feedback_key = "reaction." + rule_key;
    rule.feedback_text = feedback_text;
    rule.knowledge_effect_key = effect_key;
    rule.priority = 10;
    return rule;
}

static ReactionKnowledgeTemplate knowledgeTemplate(
    const std::string& rule_key,
    const std::string& subject_id,
    const std::string& effect_key) {
    ReactionKnowledgeTemplate tmpl;
    tmpl.rule_key = rule_key;
    tmpl.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    tmpl.subject_id = subject_id;
    tmpl.action_key = "eat";
    tmpl.effect_key = effect_key;
    tmpl.shareable = true;
    tmpl.teachable = true;
    tmpl.npc_learnable = true;
    tmpl.observation_learnable = true;
    return tmpl;
}

static ReactionContentPack berryPack() {
    ReactionContentPack pack;
    assert(pack.catalog.addItem(item("def_red_berry_fresh", "red_berry_fresh", "红果", {"edible_candidate", "fruit"}, {"fresh"})).is_ok());
    assert(pack.catalog.addItem(item("def_red_berry_rotten", "red_berry_rotten", "腐烂红果", {"edible_candidate", "fruit"}, {"rotten"})).is_ok());
    assert(pack.catalog.addItem(item("def_bitter_leaf", "bitter_leaf", "苦叶", {"edible_candidate", "leaf"}, {"bitter"})).is_ok());

    pack.rules = {
        eatRule("eat_fresh_red_berry", "def_red_berry_fresh", "fresh", "restore_hunger", "你吃了红果，饥饿得到缓解。"),
        eatRule("eat_rotten_red_berry", "def_red_berry_rotten", "rotten", "poison", "你吃了腐烂红果，身体变得不适。"),
        eatRule("eat_bitter_leaf", "def_bitter_leaf", "bitter", "no_visible_effect", "你吃了苦叶，暂时没有明显变化。"),
    };

    pack.knowledge_templates = {
        knowledgeTemplate("eat_fresh_red_berry", "def_red_berry_fresh", "restore_hunger"),
        knowledgeTemplate("eat_rotten_red_berry", "def_red_berry_rotten", "poison"),
        knowledgeTemplate("eat_bitter_leaf", "def_bitter_leaf", "no_visible_effect"),
    };
    return pack;
}

static KnowledgeStatus statusFromConfidence(double confidence) {
    if (confidence >= 0.75) return KnowledgeStatus::Active;
    if (confidence >= 0.35) return KnowledgeStatus::Shared;
    if (confidence >= 0.10) return KnowledgeStatus::Hypothesis;
    return KnowledgeStatus::Candidate;
}

static KnowledgeEvidenceKind evidenceKind(ReactionLearningSourceKind source_kind) {
    if (source_kind == ReactionLearningSourceKind::DirectExperience) return KnowledgeEvidenceKind::DirectExperience;
    if (source_kind == ReactionLearningSourceKind::Observation) return KnowledgeEvidenceKind::MemorySummary;
    if (source_kind == ReactionLearningSourceKind::Taught) return KnowledgeEvidenceKind::Teaching;
    return KnowledgeEvidenceKind::MemorySummary;
}

static bool sameClaimSemantics(const KnowledgeClaim& lhs, const KnowledgeClaim& rhs) {
    return lhs.subject.subject_id == rhs.subject.subject_id &&
           lhs.predicate.relation_type == rhs.predicate.relation_type &&
           lhs.predicate.action_key == rhs.predicate.action_key &&
           lhs.predicate.effect_key == rhs.predicate.effect_key;
}

static bool sameClaimSemantics(const KnowledgeClaim& claim, const ReactionKnowledgeDeliveryDraft& delivery) {
    return claim.subject.subject_id == delivery.knowledge.subject_id &&
           claim.predicate.action_key == delivery.knowledge.action_key &&
           claim.predicate.effect_key == delivery.knowledge.effect_key;
}

static KnowledgeClaim claimFromDelivery(
    const ReactionKnowledgeDeliveryDraft& delivery,
    Tick tick,
    size_t sequence_index) {
    const auto owner_id = delivery.owner_id.value();
    const auto subject_id = delivery.knowledge.subject_id;
    const auto action_key = delivery.knowledge.action_key;
    const auto effect_key = delivery.knowledge.effect_key;

    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(
        "know_" + owner_id + "_" + subject_id + "_" + action_key + "_" + effect_key + "_" + std::to_string(sequence_index));
    claim.owner = owner(owner_id);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject_id;
    claim.subject.subject_type_key = "reaction_item";
    claim.subject.safe_tags = {"reaction_learning"};
    claim.predicate.relation_type = effect_key == "poison" ? KnowledgeRelationType::HasRisk : KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = action_key;
    claim.predicate.effect_key = effect_key;
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
    claim.reason_keys = {"from_reaction_delivery", toString(delivery.source_kind)};

    KnowledgeEvidence evidence;
    evidence.evidence_id = KnowledgeEvidenceId("kevd_" + claim.knowledge_id.value() + "_0");
    evidence.evidence_kind = evidenceKind(delivery.source_kind);
    evidence.source_summary_id = pathfinder::memory::MemorySummaryId("summary_" + claim.knowledge_id.value());
    evidence.supports_claim = true;
    evidence.confidence_delta = delivery.confidence_hint;
    evidence.reliability = delivery.confidence_hint;
    evidence.observed_tick = tick;
    evidence.reason_keys = {"reaction_learning_delivery"};
    claim.evidence_refs.push_back(evidence);

    auto valid = claim.validateBasic();
    if (valid.is_error()) {
        for (const auto& error : valid.errors()) std::cerr << error.message_key << std::endl;
    }
    assert(valid.is_ok());
    return claim;
}

static void absorbDelivery(
    std::vector<KnowledgeClaim>& claims,
    const ReactionKnowledgeDeliveryDraft& delivery,
    Tick tick) {
    for (auto& existing : claims) {
        if (!sameClaimSemantics(existing, delivery)) continue;
        const auto old_confidence = existing.confidence.confidence;
        existing.confidence.confidence = std::min(1.0, existing.confidence.confidence + delivery.confidence_hint * 0.20);
        existing.confidence.band = confidenceToBand(existing.confidence.confidence);
        existing.confidence.support_count += 1;
        existing.status = statusFromConfidence(existing.confidence.confidence);
        existing.updated_tick = tick;
        KnowledgeEvidence evidence;
        evidence.evidence_id = KnowledgeEvidenceId("kevd_" + existing.knowledge_id.value() + "_" + std::to_string(existing.evidence_refs.size()));
        evidence.evidence_kind = evidenceKind(delivery.source_kind);
        evidence.source_summary_id = pathfinder::memory::MemorySummaryId("summary_" + existing.knowledge_id.value() + "_" + std::to_string(existing.evidence_refs.size()));
        evidence.supports_claim = true;
        evidence.confidence_delta = existing.confidence.confidence - old_confidence;
        evidence.reliability = delivery.confidence_hint;
        evidence.observed_tick = tick;
        evidence.reason_keys = {"repeated_reaction_learning"};
        existing.evidence_refs.push_back(evidence);
        auto valid = existing.validateBasic();
        if (valid.is_error()) {
            for (const auto& error : valid.errors()) std::cerr << error.message_key << std::endl;
        }
        assert(valid.is_ok());
        return;
    }
    claims.push_back(claimFromDelivery(delivery, tick, claims.size()));
}

static std::vector<ReactionKnowledgeDeliveryDraft> experience(
    const ReactionContentPack& pack,
    ReactionRuntimeWorld& world,
    const std::string& actor_id,
    const std::string& object_id,
    Tick tick) {
    ReactionInputBuilder builder;
    auto input = builder.buildInput(
        world,
        ReactionActionKind::Use,
        EntityId(actor_id),
        {{ObjectId(object_id), ReactionObjectRole::Target}},
        tick,
        "input_" + actor_id + "_" + object_id + "_" + std::to_string(tick.value()));
    assert(input.is_ok());

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(input.value(), pack.rules);
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);

    ReactionChangeApplier applier;
    auto applied = applier.apply(world, pack.catalog, result.value());
    assert(applied.is_ok());
    world = applied.value().world;

    ReactionKnowledgePlannerInput planner_input;
    planner_input.actor_id = EntityId(actor_id);
    planner_input.result = result.value();
    planner_input.knowledge_templates = pack.knowledge_templates;
    ReactionKnowledgePlanner planner;
    auto plan = planner.plan(planner_input);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 1);
    assert(plan.value().deliveries.front().source_kind == ReactionLearningSourceKind::DirectExperience);
    return plan.value().deliveries;
}

static KnowledgePropagationAttempt makeAttempt(
    const KnowledgeOwner& source,
    const KnowledgeOwner& target,
    const std::vector<KnowledgeClaim>& source_claims,
    const std::vector<KnowledgeClaim>& target_existing,
    Tick tick) {
    KnowledgePropagationAttempt attempt;
    attempt.attempt_key = "attempt_" + source.entity_id.value() + "_to_" + target.entity_id.value() + "_" + std::to_string(tick.value());
    attempt.propagator.owner = source;
    attempt.target.owner = target;
    attempt.context.channel = KnowledgePropagationChannel::DirectTeaching;
    attempt.context.trust_band = KnowledgePropagationTrustBand::High;
    attempt.context.trust_score = 0.9;
    attempt.context.distance_factor = 1.0;
    attempt.context.channel_quality = 1.0;
    attempt.context.noise_factor = 0.0;
    attempt.context.receiver_can_ask = true;
    attempt.context.propagation_tick = tick;
    for (const auto& claim : source_claims) {
        KnowledgePropagationSourceClaim source_claim;
        source_claim.claim = claim;
        source_claim.selected = true;
        source_claim.explicit_teaching_intent = true;
        attempt.source_claims.push_back(source_claim);
        attempt.propagator.available_knowledge_ids.push_back(claim.knowledge_id);
    }
    for (const auto& claim : target_existing) {
        attempt.target_existing_claims.push_back(claim);
        attempt.target.known_knowledge_ids.push_back(claim.knowledge_id);
    }
    return attempt;
}

static std::vector<KnowledgeClaim> teach(
    const std::vector<KnowledgeClaim>& source_claims,
    const KnowledgeOwner& source,
    const KnowledgeOwner& target,
    const std::vector<KnowledgeClaim>& target_existing,
    Tick tick) {
    KnowledgePropagationService service;
    KnowledgePropagationApplier applier;
    auto result = service.propagate(makeAttempt(source, target, source_claims, target_existing, tick));
    if (result.is_error()) {
        for (const auto& error : result.errors()) std::cerr << error.message_key << std::endl;
    }
    assert(result.is_ok());
    assert(result.value().decision == KnowledgePropagationDecision::CreatedRecipientClaim ||
           result.value().decision == KnowledgePropagationDecision::ReinforcedRecipientClaim ||
           result.value().decision == KnowledgePropagationDecision::UpdatedRecipientClaim);
    auto applied = applier.applyToTargetSnapshot(target_existing, result.value());
    assert(applied.is_ok());
    return applied.value();
}

static size_t countBySubjectEffect(
    const std::vector<KnowledgeClaim>& claims,
    const std::string& subject_id,
    const std::string& effect_key) {
    return static_cast<size_t>(std::count_if(claims.begin(), claims.end(), [&](const KnowledgeClaim& claim) {
        return claim.subject.subject_id == subject_id && claim.predicate.effect_key == effect_key;
    }));
}

static const KnowledgeClaim* findClaim(
    const std::vector<KnowledgeClaim>& claims,
    const std::string& subject_id,
    const std::string& effect_key) {
    auto it = std::find_if(claims.begin(), claims.end(), [&](const KnowledgeClaim& claim) {
        return claim.subject.subject_id == subject_id && claim.predicate.effect_key == effect_key;
    });
    return it == claims.end() ? nullptr : &*it;
}

static void test_repeated_learning_teaching_relearning_chain() {
    auto pack = berryPack();
    ReactionContentValidator validator;
    assert(validator.validate(pack).is_ok());

    ReactionRuntimeWorld world;
    assert(world.addObject(pack.catalog.instantiate(ObjectId("obj_fresh_a"), ObjectDefinitionId("def_red_berry_fresh"), "camp").value()).is_ok());
    assert(world.addObject(pack.catalog.instantiate(ObjectId("obj_fresh_b"), ObjectDefinitionId("def_red_berry_fresh"), "camp").value()).is_ok());
    assert(world.addObject(pack.catalog.instantiate(ObjectId("obj_rotten_a"), ObjectDefinitionId("def_red_berry_rotten"), "camp").value()).is_ok());
    assert(world.addObject(pack.catalog.instantiate(ObjectId("obj_rotten_b"), ObjectDefinitionId("def_red_berry_rotten"), "camp").value()).is_ok());

    std::vector<KnowledgeClaim> pioneer;
    std::vector<KnowledgeClaim> companion;
    std::vector<KnowledgeClaim> scout;

    for (const auto& delivery : experience(pack, world, "actor_pioneer", "obj_fresh_a", Tick(1))) absorbDelivery(pioneer, delivery, Tick(1));
    for (const auto& delivery : experience(pack, world, "actor_pioneer", "obj_fresh_b", Tick(2))) absorbDelivery(pioneer, delivery, Tick(2));
    for (const auto& delivery : experience(pack, world, "actor_pioneer", "obj_rotten_a", Tick(3))) absorbDelivery(pioneer, delivery, Tick(3));

    assert(pioneer.size() == 2);
    assert(countBySubjectEffect(pioneer, "def_red_berry_fresh", "restore_hunger") == 1);
    assert(countBySubjectEffect(pioneer, "def_red_berry_rotten", "poison") == 1);
    assert(findClaim(pioneer, "def_red_berry_fresh", "restore_hunger")->confidence.support_count >= 2);

    companion = teach(pioneer, owner("actor_pioneer"), owner("actor_companion"), companion, Tick(10));
    assert(companion.size() == 2);
    assert(countBySubjectEffect(companion, "def_red_berry_fresh", "restore_hunger") == 1);
    assert(countBySubjectEffect(companion, "def_red_berry_rotten", "poison") == 1);

    const double companion_poison_before = findClaim(companion, "def_red_berry_rotten", "poison")->confidence.confidence;
    for (const auto& delivery : experience(pack, world, "actor_companion", "obj_rotten_b", Tick(11))) absorbDelivery(companion, delivery, Tick(11));
    assert(companion.size() == 2);
    assert(findClaim(companion, "def_red_berry_rotten", "poison")->confidence.confidence >= companion_poison_before);
    assert(findClaim(companion, "def_red_berry_rotten", "poison")->confidence.support_count >= 2);

    scout = teach(companion, owner("actor_companion"), owner("actor_scout"), scout, Tick(20));
    assert(scout.size() == 2);

    const auto pioneer_before = pioneer;
    pioneer = teach(scout, owner("actor_scout"), owner("actor_pioneer"), pioneer, Tick(30));
    assert(pioneer.size() == 2);
    assert(countBySubjectEffect(pioneer, "def_red_berry_fresh", "restore_hunger") == 1);
    assert(countBySubjectEffect(pioneer, "def_red_berry_rotten", "poison") == 1);
    assert(findClaim(pioneer, "def_red_berry_fresh", "restore_hunger")->confidence.confidence >=
           findClaim(pioneer_before, "def_red_berry_fresh", "restore_hunger")->confidence.confidence);
    assert(findClaim(pioneer, "def_red_berry_rotten", "poison")->confidence.confidence >=
           findClaim(pioneer_before, "def_red_berry_rotten", "poison")->confidence.confidence);

    std::cout << "repeated_learning_teaching_relearning_chain passed" << std::endl;
}

static void test_mixed_create_and_reinforce_teaching_does_not_duplicate_existing() {
    ReactionKnowledgeDeliveryDraft fresh_delivery;
    fresh_delivery.owner_id = EntityId("actor_teacher");
    fresh_delivery.source_kind = ReactionLearningSourceKind::DirectExperience;
    fresh_delivery.knowledge.knowledge_key = "know_fresh";
    fresh_delivery.knowledge.subject_id = "def_red_berry_fresh";
    fresh_delivery.knowledge.action_key = "eat";
    fresh_delivery.knowledge.effect_key = "restore_hunger";
    fresh_delivery.confidence_hint = 0.95;

    ReactionKnowledgeDeliveryDraft rotten_delivery = fresh_delivery;
    rotten_delivery.knowledge.knowledge_key = "know_rotten";
    rotten_delivery.knowledge.subject_id = "def_red_berry_rotten";
    rotten_delivery.knowledge.effect_key = "poison";

    auto teacher_fresh = claimFromDelivery(fresh_delivery, Tick(1), 0);
    auto teacher_rotten = claimFromDelivery(rotten_delivery, Tick(2), 1);
    std::vector<KnowledgeClaim> teacher = {teacher_fresh, teacher_rotten};

    fresh_delivery.owner_id = EntityId("actor_student");
    fresh_delivery.source_kind = ReactionLearningSourceKind::Taught;
    fresh_delivery.confidence_hint = 0.45;
    auto weak_existing_fresh = claimFromDelivery(fresh_delivery, Tick(3), 0);
    weak_existing_fresh.confidence.confidence = 0.45;
    weak_existing_fresh.confidence.band = confidenceToBand(0.45);
    weak_existing_fresh.status = KnowledgeStatus::Shared;
    assert(weak_existing_fresh.validateBasic().is_ok());

    std::vector<KnowledgeClaim> student = {weak_existing_fresh};
    student = teach(teacher, owner("actor_teacher"), owner("actor_student"), student, Tick(4));

    assert(student.size() == 2);
    assert(countBySubjectEffect(student, "def_red_berry_fresh", "restore_hunger") == 1);
    assert(countBySubjectEffect(student, "def_red_berry_rotten", "poison") == 1);
    assert(findClaim(student, "def_red_berry_fresh", "restore_hunger")->confidence.confidence > 0.45);

    std::cout << "mixed_create_and_reinforce_teaching_does_not_duplicate_existing passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "chain") test_repeated_learning_teaching_relearning_chain();
    else if (mode == "mixed") test_mixed_create_and_reinforce_teaching_does_not_duplicate_existing();
    else {
        test_repeated_learning_teaching_relearning_chain();
        test_mixed_create_and_reinforce_teaching_does_not_duplicate_existing();
    }
    return 0;
}
