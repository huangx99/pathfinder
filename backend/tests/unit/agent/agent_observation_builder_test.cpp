#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/agent/binding.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/object/world_object.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/pipeline/context.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::rules;
using namespace pathfinder::pipeline;
using pathfinder::object::ObjectRuntimeFlag;

static ObservationBuildRequest makeValidRequest(const GameState& state) {
    ObservationBuildRequest req;
    req.binding.agent_id = AgentId(std::string("agent_player_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    return req;
}

void test_empty_observation_valid() {
    auto state = UnknownFruitFixture::createInitialState();
    auto req = makeValidRequest(state);
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& obs = result.value().observation;
    assert(obs.agent_id == AgentId(std::string("agent_player_001")));
    assert(obs.observer_actor_id == EntityId(std::string("actor_001")));
    assert(obs.state_version == state.metadata.state_version);
    assert(obs.objects.empty());
    std::cout << "  PASS: empty_observation_valid\n";
}

void test_null_state_fails() {
    ObservationBuildRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = nullptr;
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: null_state_fails\n";
}

void test_invalid_binding_fails() {
    auto state = UnknownFruitFixture::createInitialState();
    ObservationBuildRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    // missing primary_actor_id
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: invalid_binding_fails\n";
}

void test_actor_not_found_fails() {
    auto state = UnknownFruitFixture::createInitialState();
    ObservationBuildRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("nonexistent_actor"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: actor_not_found_fails\n";
}

void test_visible_object_projected() {
    auto state = UnknownFruitFixture::createInitialState();
    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.observation.objects.size() == 1);
    assert(res.observation.objects[0].object_id == ObjectId(std::string("obj_unknown_fruit_001")));
    assert(res.observation.objects[0].known_edible_confidence == 0.0);
    assert(res.observation.objects[0].evidence_count == 0);
    assert(res.trace.included_object_ids.size() == 1);
    std::cout << "  PASS: visible_object_projected\n";
}

void test_nonexistent_object_errors() {
    auto state = UnknownFruitFixture::createInitialState();
    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("nonexistent_obj")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: nonexistent_object_errors\n";
}

void test_consumed_object_skipped() {
    auto state = UnknownFruitFixture::createInitialState();
    // Mark the object as consumed
    auto* obj = state.object_store.findObject(ObjectId(std::string("obj_unknown_fruit_001")));
    assert(obj != nullptr);
    obj->flag = ObjectRuntimeFlag::Consumed;

    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.observation.objects.empty());
    assert(res.trace.skipped_object_ids.size() == 1);
    std::cout << "  PASS: consumed_object_skipped\n";
}

void test_builder_does_not_modify_state() {
    auto state = UnknownFruitFixture::createInitialState();
    auto state_hash_before = state.metadata.state_hash;

    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());

    // State should not be modified
    assert(state.metadata.state_hash == state_hash_before);
    auto* actor = state.actor_store.findActor(EntityId(std::string("actor_001")));
    assert(actor != nullptr);
    assert(actor->hunger == 80);  // unchanged
    std::cout << "  PASS: builder_does_not_modify_state\n";
}

void test_fire_threat_seed_projected() {
    auto state = UnknownFruitFixture::createInitialState();
    auto req = makeValidRequest(state);

    ObservedThreatSeed fire_seed;
    fire_seed.source_id = EntityId(std::string("fire_001"));
    fire_seed.threat_type = AgentThreatType::Fire;
    fire_seed.severity = 0.95;
    fire_seed.confidence = 0.99;
    req.visibility.threat_seeds.push_back(fire_seed);

    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.observation.threats.size() == 1);
    assert(res.observation.threats[0].threat_type == AgentThreatType::Fire);
    assert(res.observation.threats[0].severity == 0.95);
    std::cout << "  PASS: fire_threat_seed_projected\n";
}

void test_no_effect_data_leaked() {
    auto state = UnknownFruitFixture::createInitialState();
    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    // Verify no real effect data is in the observation
    auto& obs = result.value().observation;
    assert(obs.objects[0].known_edible_confidence == 0.0);  // no cognition claims yet
    // The observation struct itself has no raw effect profile fields
    std::cout << "  PASS: no_effect_data_leaked\n";
}

void test_cognition_claim_projected() {
    // First, eat the fruit to create a cognition claim
    auto state = UnknownFruitFixture::createInitialState();
    auto cmd = UnknownFruitFixture::createEatCommand();

    // Execute pipeline to create cognition claim
    pathfinder::pipeline::PipelineContext ctx;
    ctx.command = cmd;
    ctx.state_metadata = state.metadata;
    ctx.game_state = &state;
    ctx.random_seed = RandomSeed(42);
    pathfinder::pipeline::RulePipeline pipeline;
    auto pipeline_result = pipeline.execute(ctx);
    assert(pipeline_result.is_ok());

    // The original fruit is now consumed. Add a second fruit with same definition
    // so the observation builder can project cognition claims onto it.
    pathfinder::object::WorldObject fruit2;
    fruit2.object_id = ObjectId(std::string("obj_unknown_fruit_002"));
    fruit2.definition_id = ObjectDefinitionId(std::string("unknown_fruit"));
    state.object_store.addObject(fruit2);

    // Now build observation with cognition claims
    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_002")));
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.observation.objects.size() == 1);
    // After eating once, confidence is 0.3 (delta=0.3), below 0.5 threshold
    assert(res.observation.objects[0].known_edible_confidence > 0.0);
    assert(res.observation.objects[0].evidence_count == 1);
    assert(res.observation.objects[0].can_teach_hint == false);  // confidence 0.3 < 0.5
    std::cout << "  PASS: cognition_claim_projected\n";
}

void test_include_cognition_claims_false() {
    auto state = UnknownFruitFixture::createInitialState();
    auto cmd = UnknownFruitFixture::createEatCommand();
    pathfinder::pipeline::PipelineContext ctx;
    ctx.command = cmd;
    ctx.state_metadata = state.metadata;
    ctx.game_state = &state;
    ctx.random_seed = RandomSeed(42);
    pathfinder::pipeline::RulePipeline pipeline;
    auto pipeline_result = pipeline.execute(ctx);
    assert(pipeline_result.is_ok());

    // Add a second fruit since the original is now consumed
    pathfinder::object::WorldObject fruit2;
    fruit2.object_id = ObjectId(std::string("obj_unknown_fruit_003"));
    fruit2.definition_id = ObjectDefinitionId(std::string("unknown_fruit"));
    state.object_store.addObject(fruit2);

    auto req = makeValidRequest(state);
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_003")));
    req.include_cognition_claims = false;
    ObservationBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().observation.objects.size() == 1);
    // Without cognition claims, confidence should be 0.0
    assert(result.value().observation.objects[0].known_edible_confidence == 0.0);
    std::cout << "  PASS: include_cognition_claims_false\n";
}

void run_agent_observation_builder_tests() {
    test_empty_observation_valid();
    test_null_state_fails();
    test_invalid_binding_fails();
    test_actor_not_found_fails();
    test_visible_object_projected();
    test_nonexistent_object_errors();
    test_consumed_object_skipped();
    test_builder_does_not_modify_state();
    test_fire_threat_seed_projected();
    test_no_effect_data_leaked();
    test_cognition_claim_projected();
    test_include_cognition_claims_false();
    std::cout << "All agent observation builder tests passed!\n";
}
