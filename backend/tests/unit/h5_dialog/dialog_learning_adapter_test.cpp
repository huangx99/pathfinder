#include "pathfinder/h5_dialog/dialog_learning_adapter.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;
using namespace pathfinder::learning;
using namespace pathfinder::cognition;
using namespace pathfinder::memory;
using namespace pathfinder::knowledge;

static DialogSessionState makeTestSession() {
    DialogSessionState state;
    state.session_id = "test_session";
    state.scenario_key = "p22_minimal";
    state.turn_index = 1;
    state.current_tick = Tick(1);
    state.actor.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, EntityId("pioneer"), {}, {}, "pioneer"};
    state.actor.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, EntityId("pioneer"), {}, "pioneer"};
    state.actor.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, EntityId("pioneer"), std::nullopt};
    state.actor.display_key = "pioneer";
    state.recipient.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, EntityId("companion"), {}, {}, "companion"};
    state.recipient.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, EntityId("companion"), {}, "companion"};
    state.recipient.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, EntityId("companion"), std::nullopt};
    state.recipient.display_key = "companion";
    state.visible_object_keys = {"red_berry"};
    return state;
}

static void test_valid_input() {
    DialogLearningAdapter adapter;
    auto state = makeTestSession();
    DialogIntent intent;
    intent.kind = DialogIntentKind::TryEat;
    intent.action = DialogActionKind::Eat;
    intent.object_key = "red_berry";

    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    auto fb_r = catalog.findFeedback(scenario_r.value(), "red_berry", DialogActionKind::Eat);
    assert(fb_r.is_ok());

    auto input_r = adapter.buildLearningInput(state, intent, fb_r.value());
    assert(input_r.is_ok());
    auto input = input_r.value();
    assert(input.story_kind == LearningLoopStoryKind::DirectDiscovery);
    assert(input.actor.display_key == "pioneer");
    assert(input.feedback.effect_key == "restore_hunger");
    std::cout << "valid_input passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_valid_input();
    std::cout << "All h5_dialog_learning_adapter tests passed" << std::endl;
    return 0;
}
