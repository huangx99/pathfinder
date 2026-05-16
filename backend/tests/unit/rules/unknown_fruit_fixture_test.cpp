#include "pathfinder/rules/unknown_fruit_fixture.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::state;
using namespace pathfinder::object;
using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_unknown_fruit_fixture_tests() {
    // Test createInitialState
    {
        auto state = UnknownFruitFixture::createInitialState();

        // State should validate
        auto result = state.validateBasic();
        assert(result.is_ok());

        // Check metadata
        assert(state.metadata.state_id == GameStateId("fixture_state_001"));
        assert(state.metadata.current_tick == Tick(1));

        // Check actor exists
        auto* actor = state.actor_store.findActor(EntityId("actor_001"));
        assert(actor != nullptr);
        assert(actor->hunger == 80);
        assert(actor->health == 100);

        // Check object definition exists
        auto* def = state.object_store.findDefinition(ObjectDefinitionId("unknown_fruit"));
        assert(def != nullptr);
        assert(def->category == ObjectCategory::Food);
        assert(def->edible_profile.effect_kind == EdibleEffectKind::Edible);
        assert(def->edible_profile.hunger_delta == -20);
        assert(def->edible_profile.health_delta == 0);
        assert(def->edible_profile.risk_level == 0);

        // Check world object exists
        auto* obj = state.object_store.findObject(ObjectId("obj_unknown_fruit_001"));
        assert(obj != nullptr);
        assert(obj->definition_id == ObjectDefinitionId("unknown_fruit"));
        assert(obj->quantity == 1);
        assert(obj->flag == ObjectRuntimeFlag::None);

        // Check cognition is empty
        assert(state.cognition_state.claims().empty());
    }

    // Test createEatCommand
    {
        auto cmd = UnknownFruitFixture::createEatCommand();

        // Command should validate
        auto result = cmd.validateBasic();
        assert(result.is_ok());

        // Check command fields
        assert(cmd.command_id == CommandId("cmd_try_eat_001"));
        assert(cmd.source == pathfinder::command::CommandSource::Test);
        assert(cmd.issued_tick == Tick(1));

        // Check payload
        assert(cmd.payload.action_id == ActionId("eat"));
        assert(cmd.payload.actor_id == EntityId("actor_001"));
        assert(cmd.payload.intent == pathfinder::command::CommandIntent::Experiment);
        assert(cmd.payload.targets.size() == 1);

        // Check target
        auto& target = cmd.payload.targets[0];
        assert(target.target_type == pathfinder::command::ActionTargetType::Object);
        assert(target.target_id == TargetId("obj_unknown_fruit_001"));
        assert(target.role == pathfinder::command::ActionTargetRole::Primary);
    }

    // Test fixture consistency - creating state twice produces same result
    {
        auto state1 = UnknownFruitFixture::createInitialState();
        auto state2 = UnknownFruitFixture::createInitialState();

        assert(state1.metadata.state_id == state2.metadata.state_id);
        assert(state1.metadata.current_tick == state2.metadata.current_tick);

        auto* actor1 = state1.actor_store.findActor(EntityId("actor_001"));
        auto* actor2 = state2.actor_store.findActor(EntityId("actor_001"));
        assert(actor1->hunger == actor2->hunger);
        assert(actor1->health == actor2->health);
    }

    std::cout << "unknown_fruit_fixture tests passed" << std::endl;
}
