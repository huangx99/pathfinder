#include "pathfinder/danger/danger_knowledge_bridge.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::danger;
using namespace pathfinder::foundation;
using namespace pathfinder::reaction;

static DangerKnowledgeDraft draft() {
    DangerKnowledgeDraft out;
    out.knowledge_key = "danger_knowledge_rotten_berry";
    out.subject_id = "def_red_berry_rotten";
    out.action_key = "eat";
    out.effect_key = "poison";
    out.severity = DangerSeverity::Harm;
    out.reason_keys = {"rotten_berry_poison_pressure"};
    return out;
}

static void test_delivery_sources() {
    DangerKnowledgePlannerInput input;
    input.actor_id = EntityId("actor_pioneer");
    input.observer_ids = {EntityId("actor_companion")};
    input.taught_ids = {EntityId("actor_student")};
    input.knowledge_drafts = {draft()};
    auto plan = DangerKnowledgePlanner{}.plan(input);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 3);
    assert(plan.value().deliveries[0].source_kind == ReactionLearningSourceKind::DirectExperience);
    assert(plan.value().deliveries[1].source_kind == ReactionLearningSourceKind::Observation);
    assert(plan.value().deliveries[2].source_kind == ReactionLearningSourceKind::Taught);
    assert(plan.value().deliveries[2].confidence_hint == 0.75);
}

int main() {
    test_delivery_sources();
    std::cout << "danger_knowledge_bridge_test passed\n";
    return 0;
}
