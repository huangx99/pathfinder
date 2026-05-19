#include "pathfinder/story/story_system.h"
#include "pathfinder/story/story_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::story;

static void test_enum_roundtrip() {
    assert(toString(StoryScenarioKind::FirstDaySurvival) == "first_day_survival");
    assert(storyScenarioKindFromString("first_day_survival").value() == StoryScenarioKind::FirstDaySurvival);
    assert(toString(StoryTimePhase::Dusk) == "dusk");
    assert(storyTimePhaseFromString("dusk").value() == StoryTimePhase::Dusk);
    assert(toString(StoryObjectiveStatus::Blocked) == "blocked");
    assert(storyObjectiveStatusFromString("blocked").value() == StoryObjectiveStatus::Blocked);
    assert(toString(StoryThreatKind::ApproachingBeast) == "approaching_beast");
    assert(storyThreatKindFromString("approaching_beast").value() == StoryThreatKind::ApproachingBeast);
    assert(toString(StoryMaterialRole::ThreatCounter) == "threat_counter");
    assert(storyMaterialRoleFromString("threat_counter").value() == StoryMaterialRole::ThreatCounter);
    assert(toString(StoryActionRole::TargetUse) == "target_use");
    assert(storyActionRoleFromString("target_use").value() == StoryActionRole::TargetUse);
    assert(storyActionRoleFromString("bad_value").is_error());
}

static void test_scenario_validation() {
    StoryScenarioRegistry registry;
    auto scenario = registry.firstDaySurvival();
    assert(scenario.is_ok());
    assert(scenario.value().validateBasic().is_ok());
    assert(scenario.value().material_entries.size() >= 10);
    assert(!scenario.value().threats.empty());

    auto broken = scenario.value();
    broken.threats[0].counter_object_keys.clear();
    assert(broken.validateBasic().is_error());

    broken = scenario.value();
    broken.material_entries[0].object_key = "hidden_truth";
    assert(broken.validateBasic().is_error());
}

static void test_runtime_progression_success() {
    StoryScenarioRegistry registry;
    StoryRuntimeFactory factory;
    StoryProgressionService progression;
    auto scenario = registry.firstDaySurvival().value();
    auto state = factory.createInitialState(scenario).value();

    StoryEvaluationContext context;
    context.last_input_text = "吃红果";
    context.actor_knowledge_effect_keys = {"restore_hunger"};
    auto r1 = progression.applyTurn(scenario, state, context);
    assert(r1.is_ok());
    state = r1.value().new_state;
    assert(state.current_phase == StoryTimePhase::Morning);

    context.last_input_text = "用斧头砍木头";
    context.actor_knowledge_effect_keys.push_back("cut_wood");
    auto r2 = progression.applyTurn(scenario, state, context);
    assert(r2.is_ok());
    state = r2.value().new_state;
    assert(state.current_phase == StoryTimePhase::Afternoon);

    context.last_input_text = "用火种点燃干草";
    context.actor_knowledge_effect_keys.push_back("ignite_fire");
    context.available_object_keys.push_back("camp_fire");
    auto r3 = progression.applyTurn(scenario, state, context);
    assert(r3.is_ok());
    state = r3.value().new_state;

    context.last_input_text = "制作火把";
    context.actor_knowledge_effect_keys.push_back("make_torch");
    context.available_object_keys.push_back("torch");
    auto r4 = progression.applyTurn(scenario, state, context);
    assert(r4.is_ok());
    state = r4.value().new_state;
    assert(!state.generated_object_keys.empty());

    context.last_input_text = "教同伴";
    context.teach_action_happened = true;
    context.recipient_knowledge_effect_keys.push_back("make_torch");
    auto r5 = progression.applyTurn(scenario, state, context);
    assert(r5.is_ok());
    state = r5.value().new_state;
    assert(state.current_phase == StoryTimePhase::Dusk);

    context.last_input_text = "用火把驱赶野兽";
    context.actor_knowledge_effect_keys.push_back("repel_beast");
    auto r6 = progression.applyTurn(scenario, state, context);
    assert(r6.is_ok());
    state = r6.value().new_state;
    assert(state.current_phase == StoryTimePhase::Dusk || state.current_phase == StoryTimePhase::Night || state.current_phase == StoryTimePhase::Survived);

    context.last_input_text = "等待一会";
    auto r7 = progression.applyTurn(scenario, state, context);
    assert(r7.is_ok());
    state = r7.value().new_state;
    context.last_input_text = "等待一会";
    auto r8 = progression.applyTurn(scenario, state, context);
    assert(r8.is_ok());
    state = r8.value().new_state;
    assert(state.current_phase == StoryTimePhase::Survived);
}

static void test_runtime_progression_failure() {
    StoryScenarioRegistry registry;
    StoryRuntimeFactory factory;
    StoryProgressionService progression;
    auto scenario = registry.firstDaySurvival().value();
    auto state = factory.createInitialState(scenario).value();
    StoryEvaluationContext context;
    for (int index = 0; index < 8; ++index) {
        context.last_input_text = "等待一会";
        auto result = progression.applyTurn(scenario, state, context);
        assert(result.is_ok());
        state = result.value().new_state;
    }
    assert(state.current_phase == StoryTimePhase::Failed);
}

static void test_projection_player_safe() {
    StoryScenarioRegistry registry;
    StoryRuntimeFactory factory;
    StoryProjectionAdapter adapter;
    auto scenario = registry.firstDaySurvival().value();
    auto state = factory.createInitialState(scenario).value();
    StoryEvaluationContext context;
    auto projection = adapter.composeProjection(scenario, state, context, pathfinder::h5_projection::ProjectionAudience::Player);
    assert(projection.is_ok());
    assert(projection.value().validateBasic().is_ok());
    assert(projection.value().debug_keys.empty());
    assert(!projection.value().objective_cards.empty());
    assert(!projection.value().suggested_actions.empty());
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "enum") test_enum_roundtrip();
    else if (name == "scenario") test_scenario_validation();
    else if (name == "success") test_runtime_progression_success();
    else if (name == "failure") test_runtime_progression_failure();
    else if (name == "projection") test_projection_player_safe();
    else return 2;
    std::cout << "story " << name << " passed\n";
    return 0;
}
