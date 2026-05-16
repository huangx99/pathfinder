#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/pipeline/context.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/event/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::pipeline;
using namespace pathfinder::state;
using namespace pathfinder::object;
using namespace pathfinder::cognition;
using namespace pathfinder::foundation;
using namespace pathfinder::event;

void run_p3_integration_tests() {
    // Create initial state
    auto state = UnknownFruitFixture::createInitialState();

    // Verify initial state
    auto state_result = state.validateBasic();
    assert(state_result.is_ok());

    // Verify initial cognition is empty
    assert(state.cognition_state.claims().empty());

    // Create eat command
    auto cmd = UnknownFruitFixture::createEatCommand();

    // Create pipeline context
    PipelineContext context;
    context.command = cmd;
    context.state_metadata = state.metadata;
    context.game_state = &state;
    context.random_seed = RandomSeed(42);

    // Execute pipeline
    RulePipeline pipeline;
    auto result = pipeline.execute(context);
    assert(result.is_ok());

    auto& pipeline_result = result.value();

    // Verify pipeline succeeded
    assert(pipeline_result.status == PipelineStatus::Succeeded);

    // Verify state changes were made
    assert(!pipeline_result.state_changes.empty());
    assert(pipeline_result.state_changes.size() >= 2);

    // Verify events were generated
    assert(!pipeline_result.events.empty());
    assert(pipeline_result.events.size() >= 2);

    // Check for specific event types
    bool has_action_resolved = false;
    bool has_feedback_generated = false;
    bool has_state_changed = false;
    bool has_cognition_updated = false;
    for (const auto& event : pipeline_result.events.events()) {
        if (event.event_type == EventType::ActionResolved) {
            has_action_resolved = true;
        }
        if (event.event_type == EventType::FeedbackGenerated) {
            has_feedback_generated = true;
        }
        if (event.event_type == EventType::StateChanged) {
            has_state_changed = true;
        }
        if (event.event_type == EventType::CognitionUpdated) {
            has_cognition_updated = true;
        }
    }
    assert(has_action_resolved);
    assert(has_feedback_generated);
    assert(has_state_changed);
    assert(has_cognition_updated);

    // Verify actor state changed
    auto* actor = state.actor_store.findActor(EntityId("actor_001"));
    assert(actor != nullptr);
    assert(actor->hunger == 60); // 80 + (-20) = 60
    assert(actor->health == 100); // 100 + 0 = 100

    // Verify object consumed
    auto* obj = state.object_store.findObject(ObjectId("obj_unknown_fruit_001"));
    assert(obj != nullptr);
    assert(obj->flag == ObjectRuntimeFlag::Consumed);

    // Verify cognition claim was created
    assert(state.cognition_state.claims().size() == 1);

    CognitionKey expected_key;
    expected_key.subject_id = EntityId("actor_001");
    expected_key.object_definition_id = ObjectDefinitionId("unknown_fruit");
    expected_key.action_id = ActionId("eat");
    expected_key.effect_kind = CognitionEffectKind::Edible;

    auto* claim = state.cognition_state.findClaim(expected_key);
    assert(claim != nullptr);
    assert(claim->confidence > 0.0);
    assert(claim->evidence_count == 1);

    // Save results for deterministic comparison
    size_t state_change_count = pipeline_result.state_changes.size();
    size_t event_count = pipeline_result.events.size();
    int final_hunger = actor->hunger;
    int final_health = actor->health;
    double final_confidence = claim->confidence;
    int final_evidence_count = claim->evidence_count;

    // Run again with same initial state, command, and seed (deterministic replay test)
    auto state_b = UnknownFruitFixture::createInitialState();
    auto cmd_b = UnknownFruitFixture::createEatCommand();

    PipelineContext context_b;
    context_b.command = cmd_b;
    context_b.state_metadata = state_b.metadata;
    context_b.game_state = &state_b;
    context_b.random_seed = RandomSeed(42);

    RulePipeline pipeline_b;
    auto result_b = pipeline_b.execute(context_b);
    assert(result_b.is_ok());

    auto& pipeline_result_b = result_b.value();

    // Verify deterministic results
    assert(pipeline_result_b.status == PipelineStatus::Succeeded);
    assert(pipeline_result_b.state_changes.size() == state_change_count);
    assert(pipeline_result_b.events.size() == event_count);

    auto* actor_b = state_b.actor_store.findActor(EntityId("actor_001"));
    assert(actor_b->hunger == final_hunger);
    assert(actor_b->health == final_health);

    auto* claim_b = state_b.cognition_state.findClaim(expected_key);
    assert(claim_b != nullptr);
    assert(claim_b->confidence == final_confidence);
    assert(claim_b->evidence_count == final_evidence_count);

    std::cout << "p3_integration tests passed" << std::endl;
}
