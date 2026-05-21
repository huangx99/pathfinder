#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_runtime;

static KnowledgeClaim makeRiskClaim(const std::string& subject_id, const std::string& target_key) {
    KnowledgeClaim claim;
    claim.knowledge_id = pathfinder::foundation::KnowledgeId("k_" + subject_id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = pathfinder::foundation::EntityId("beast_1");
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject_id;
    claim.subject.related_subject_ids.push_back(target_key);
    claim.predicate.relation_type = KnowledgeRelationType::IsDangerousUnder;
    claim.status = KnowledgeStatus::Active;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = pathfinder::foundation::Tick(1);
    claim.updated_tick = pathfinder::foundation::Tick(1);
    return claim;
}

void run_world_beast_ecology_policy_tests() {
    std::cout << "Running world_beast_ecology policy tests:" << std::endl;

    BeastDecisionPolicy policy;

    BeastAgentProfile profile;
    profile.profile_key = "test_predator";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.vision_radius = 5;
    profile.base_aggression = 60.0;
    profile.base_fear = 20.0;
    profile.prey_tags.push_back("prey_tag");
    profile.danger_tags.push_back("danger_effect_tag");
    profile.deterrent_tags.push_back("deterrent_tag");

    // 1. No targets -> Wait (P52 safe fallback without wander algorithm)
    BeastDecisionPolicy::PolicyContext ctx1;
    ctx1.actor_key = "beast_1";
    ctx1.profile = profile;
    ctx1.tick = 1;
    auto r1 = policy.selectIntent(ctx1);
    assert(r1.is_ok());
    assert(r1.value().kind == BeastActionIntentKind::Wait);
    assert(r1.value().reason_kind == BeastDecisionReasonKind::NoValidAction);

    // 2. Prey tag -> Approach
    BeastPerceptionItem prey;
    prey.perception_id = "p1";
    prey.kind = BeastPerceptionKind::Actor;
    prey.target_ref = "prey_actor_1";
    prey.target_key = "prey_entity";
    prey.coord = WorldCellCoord{2, 0, "surface"};
    prey.distance = 2;
    prey.visible = true;
    prey.tag_keys.push_back("prey_tag");

    BeastDecisionPolicy::PolicyContext ctx2;
    ctx2.actor_key = "beast_1";
    ctx2.profile = profile;
    ctx2.perceptions.push_back(prey);
    ctx2.tick = 2;
    auto r2 = policy.selectIntent(ctx2);
    assert(r2.is_ok());
    assert(r2.value().kind == BeastActionIntentKind::Approach);
    assert(r2.value().reason_kind == BeastDecisionReasonKind::PerceivedPrey);
    assert(r2.value().target_ref == "prey_actor_1");

    // 3. Prey adjacent -> Attack
    BeastPerceptionItem prey_adjacent;
    prey_adjacent.perception_id = "p2";
    prey_adjacent.kind = BeastPerceptionKind::Actor;
    prey_adjacent.target_ref = "prey_actor_2";
    prey_adjacent.target_key = "prey_entity";
    prey_adjacent.coord = WorldCellCoord{1, 0, "surface"};
    prey_adjacent.distance = 1;
    prey_adjacent.visible = true;
    prey_adjacent.tag_keys.push_back("prey_tag");

    BeastDecisionPolicy::PolicyContext ctx3;
    ctx3.actor_key = "beast_1";
    ctx3.profile = profile;
    ctx3.perceptions.push_back(prey_adjacent);
    ctx3.tick = 3;
    auto r3 = policy.selectIntent(ctx3);
    assert(r3.is_ok());
    assert(r3.value().kind == BeastActionIntentKind::Attack);
    assert(r3.value().reason_kind == BeastDecisionReasonKind::PerceivedPrey);

    // 4. Danger tag -> Flee
    BeastPerceptionItem danger;
    danger.perception_id = "p3";
    danger.kind = BeastPerceptionKind::Effect;
    danger.target_ref = "effect_fire_1";
    danger.target_key = "danger_effect_tag";
    danger.coord = WorldCellCoord{1, 1, "surface"};
    danger.distance = 2;
    danger.visible = true;
    danger.tag_keys.push_back("danger_effect_tag");

    BeastDecisionPolicy::PolicyContext ctx4;
    ctx4.actor_key = "beast_1";
    ctx4.profile = profile;
    ctx4.perceptions.push_back(danger);
    ctx4.tick = 4;
    auto r4 = policy.selectIntent(ctx4);
    assert(r4.is_ok());
    assert(r4.value().kind == BeastActionIntentKind::Flee);
    assert(r4.value().reason_kind == BeastDecisionReasonKind::PerceivedDanger);
    assert(r4.value().target_ref == "effect_fire_1");

    // 5. Deterrent tag -> Flee
    BeastPerceptionItem deterrent;
    deterrent.perception_id = "p4";
    deterrent.kind = BeastPerceptionKind::Object;
    deterrent.target_ref = "obj_torch_1";
    deterrent.target_key = "deterrent_object";
    deterrent.coord = WorldCellCoord{1, 0, "surface"};
    deterrent.distance = 1;
    deterrent.visible = true;
    deterrent.tag_keys.push_back("deterrent_tag");

    BeastDecisionPolicy::PolicyContext ctx5;
    ctx5.actor_key = "beast_1";
    ctx5.profile = profile;
    ctx5.perceptions.push_back(deterrent);
    ctx5.tick = 5;
    auto r5 = policy.selectIntent(ctx5);
    assert(r5.is_ok());
    assert(r5.value().kind == BeastActionIntentKind::Flee);
    assert(r5.value().reason_kind == BeastDecisionReasonKind::PerceivedDanger);

    // 6. Learned risk (IsDangerousUnder) -> Flee (even without danger tag)
    BeastPerceptionItem neutral_item;
    neutral_item.perception_id = "p5";
    neutral_item.kind = BeastPerceptionKind::Object;
    neutral_item.target_ref = "obj_trap_1";
    neutral_item.target_key = "trap_object";
    neutral_item.coord = WorldCellCoord{1, 0, "surface"};
    neutral_item.distance = 1;
    neutral_item.visible = true;

    BeastDecisionPolicy::PolicyContext ctx6;
    ctx6.actor_key = "beast_1";
    ctx6.profile = profile;
    ctx6.perceptions.push_back(neutral_item);
    ctx6.learned_claims.push_back(makeRiskClaim("trap_object", "trap_object"));
    ctx6.tick = 6;
    auto r6 = policy.selectIntent(ctx6);
    assert(r6.is_ok());
    assert(r6.value().kind == BeastActionIntentKind::Flee);
    assert(r6.value().reason_kind == BeastDecisionReasonKind::LearnedRisk);

    // 6b. HasRisk claim also causes Flee
    KnowledgeClaim has_risk_claim;
    has_risk_claim.knowledge_id = pathfinder::foundation::KnowledgeId("k_risky");
    has_risk_claim.owner.kind = KnowledgeOwnerKind::Actor;
    has_risk_claim.owner.entity_id = pathfinder::foundation::EntityId("beast_1");
    has_risk_claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    has_risk_claim.subject.subject_id = "risky_object";
    has_risk_claim.predicate.relation_type = KnowledgeRelationType::HasRisk;
    has_risk_claim.status = KnowledgeStatus::Active;
    has_risk_claim.projection_flags.usable_by_ai = true;
    has_risk_claim.confidence.confidence = 0.75;
    has_risk_claim.created_tick = pathfinder::foundation::Tick(1);
    has_risk_claim.updated_tick = pathfinder::foundation::Tick(1);

    BeastPerceptionItem risky_item;
    risky_item.perception_id = "p5b";
    risky_item.kind = BeastPerceptionKind::Object;
    risky_item.target_ref = "obj_risky_1";
    risky_item.target_key = "risky_object";
    risky_item.coord = WorldCellCoord{1, 0, "surface"};
    risky_item.distance = 1;
    risky_item.visible = true;

    BeastDecisionPolicy::PolicyContext ctx6b;
    ctx6b.actor_key = "beast_1";
    ctx6b.profile = profile;
    ctx6b.perceptions.push_back(risky_item);
    ctx6b.learned_claims.push_back(has_risk_claim);
    ctx6b.tick = 6;
    auto r6b = policy.selectIntent(ctx6b);
    assert(r6b.is_ok());
    assert(r6b.value().kind == BeastActionIntentKind::Flee);
    assert(r6b.value().reason_kind == BeastDecisionReasonKind::LearnedRisk);

    // 7. Curious temperament -> Observe
    BeastAgentProfile curious_profile;
    curious_profile.profile_key = "curious_test";
    curious_profile.temperament = BeastTemperamentKind::Curious;
    curious_profile.vision_radius = 5;
    curious_profile.base_aggression = 10.0;
    curious_profile.base_fear = 10.0;

    BeastPerceptionItem interesting_obj;
    interesting_obj.perception_id = "p6";
    interesting_obj.kind = BeastPerceptionKind::Object;
    interesting_obj.target_ref = "obj_shiny_1";
    interesting_obj.target_key = "shiny_object";
    interesting_obj.coord = WorldCellCoord{2, 0, "surface"};
    interesting_obj.distance = 2;
    interesting_obj.visible = true;
    interesting_obj.intensity = 80.0;

    BeastDecisionPolicy::PolicyContext ctx7;
    ctx7.actor_key = "beast_1";
    ctx7.profile = curious_profile;
    ctx7.perceptions.push_back(interesting_obj);
    ctx7.tick = 7;
    auto r7 = policy.selectIntent(ctx7);
    assert(r7.is_ok());
    assert(r7.value().kind == BeastActionIntentKind::Observe);

    std::cout << "All policy tests passed!" << std::endl;
}
