#include "pathfinder/reaction/reaction_learning_bridge.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using pathfinder::foundation::EntityId;

static ReactionResult knowledgeResult() {
    ReactionResult result;
    result.decision = ReactionDecision::Reacted;
    result.selected_rule_key = "fire_dry_branch_to_torch";
    result.trace.decision = ReactionDecision::Reacted;
    result.trace.input_key = "input_fire_branch";
    result.trace.selected_rule_key = result.selected_rule_key;
    ReactionKnowledgeDraft knowledge;
    knowledge.knowledge_key = "reaction_knowledge_fire_dry_branch_to_torch";
    knowledge.subject_id = "dry_branch";
    knowledge.action_key = "combine";
    knowledge.effect_key = "transform_to_torch";
    knowledge.reason_keys = {"fire_dry_branch_to_torch"};
    result.knowledge_drafts = {knowledge};
    return result;
}

static ReactionKnowledgeTemplate templateForRule() {
    ReactionKnowledgeTemplate tmpl;
    tmpl.rule_key = "fire_dry_branch_to_torch";
    tmpl.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    tmpl.subject_id = "def_dry_branch";
    tmpl.action_key = "combine_with_fire";
    tmpl.effect_key = "transform_to_torch";
    tmpl.shareable = true;
    tmpl.teachable = true;
    tmpl.npc_learnable = true;
    tmpl.observation_learnable = true;
    return tmpl;
}

static void test_planner_delivers_actor_observer_and_taught() {
    ReactionKnowledgePlannerInput input;
    input.actor_id = EntityId("actor_pioneer");
    input.observer_ids = {EntityId("actor_companion")};
    input.taught_ids = {EntityId("actor_student")};
    input.result = knowledgeResult();
    input.knowledge_templates = {templateForRule()};

    ReactionKnowledgePlanner planner;
    auto plan = planner.plan(input);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 3);
    assert(plan.value().deliveries[0].owner_id.value() == "actor_pioneer");
    assert(plan.value().deliveries[0].source_kind == ReactionLearningSourceKind::DirectExperience);
    assert(plan.value().deliveries[0].knowledge.subject_id == "def_dry_branch");
    assert(plan.value().deliveries[1].owner_id.value() == "actor_companion");
    assert(plan.value().deliveries[1].source_kind == ReactionLearningSourceKind::Observation);
    assert(plan.value().deliveries[1].confidence_hint == 0.6);
    assert(plan.value().deliveries[2].source_kind == ReactionLearningSourceKind::Taught);
    std::cout << "planner_delivers_actor_observer_and_taught passed" << std::endl;
}

static void test_planner_respects_observation_flag() {
    ReactionKnowledgePlannerInput input;
    input.actor_id = EntityId("actor_pioneer");
    input.observer_ids = {EntityId("actor_companion")};
    input.result = knowledgeResult();
    auto tmpl = templateForRule();
    tmpl.observation_learnable = false;
    input.knowledge_templates = {tmpl};

    ReactionKnowledgePlanner planner;
    auto plan = planner.plan(input);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 1);
    assert(plan.value().deliveries.front().source_kind == ReactionLearningSourceKind::DirectExperience);
    std::cout << "planner_respects_observation_flag passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "delivery") test_planner_delivers_actor_observer_and_taught();
    else if (mode == "observation_flag") test_planner_respects_observation_flag();
    else {
        test_planner_delivers_actor_observer_and_taught();
        test_planner_respects_observation_flag();
    }
    return 0;
}
