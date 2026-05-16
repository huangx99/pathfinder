#include "pathfinder/rules/eat_object_resolver.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::state;
using namespace pathfinder::object;
using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_eat_object_resolver_tests() {
    // Test canResolve with eat command
    {
        auto cmd = UnknownFruitFixture::createEatCommand();
        assert(EatObjectResolver::canResolve(cmd) == true);
    }

    // Test canResolve with non-eat command
    {
        auto cmd = UnknownFruitFixture::createEatCommand();
        cmd.payload.action_id = ActionId("look");
        assert(EatObjectResolver::canResolve(cmd) == false);
    }

    // Test resolve success
    {
        auto state = UnknownFruitFixture::createInitialState();
        auto cmd = UnknownFruitFixture::createEatCommand();

        EatObjectResolveInput input{cmd, state};
        auto result = EatObjectResolver::resolve(input);
        assert(result.is_ok());

        auto& draft = result.value();

        // Should have state changes
        assert(!draft.state_changes.empty());
        assert(draft.state_changes.size() >= 2); // hunger + consumed

        // Should have events
        assert(draft.events.size() >= 2); // action + feedback

        // Should have cognition evidence
        assert(draft.cognition_evidence.size() == 1);

        // Check cognition evidence
        auto& evidence = draft.cognition_evidence[0];
        assert(evidence.key.subject_id == EntityId("actor_001"));
        assert(evidence.key.object_definition_id == ObjectDefinitionId("unknown_fruit"));
        assert(evidence.key.action_id == ActionId("eat"));
        assert(evidence.key.effect_kind == CognitionEffectKind::Edible);
        assert(evidence.confidence_delta > 0);
        assert(evidence.observed_hunger_delta == -20);
        assert(evidence.observed_health_delta == 0);

        // Verify state not modified
        auto* actor = state.actor_store.findActor(EntityId("actor_001"));
        assert(actor->hunger == 80);
        assert(actor->health == 100);
        auto* obj = state.object_store.findObject(ObjectId("obj_unknown_fruit_001"));
        assert(obj->flag == ObjectRuntimeFlag::None);
    }

    // Test resolve fails with missing actor
    {
        auto state = UnknownFruitFixture::createInitialState();
        auto cmd = UnknownFruitFixture::createEatCommand();
        cmd.payload.actor_id = EntityId("nonexistent");

        EatObjectResolveInput input{cmd, state};
        auto result = EatObjectResolver::resolve(input);
        assert(result.is_error());
    }

    // Test resolve fails with missing object
    {
        auto state = UnknownFruitFixture::createInitialState();
        auto cmd = UnknownFruitFixture::createEatCommand();
        cmd.payload.targets[0].target_id = TargetId("nonexistent");

        EatObjectResolveInput input{cmd, state};
        auto result = EatObjectResolver::resolve(input);
        assert(result.is_error());
    }

    // Test resolve fails with consumed object
    {
        auto state = UnknownFruitFixture::createInitialState();
        auto cmd = UnknownFruitFixture::createEatCommand();

        // Mark object as consumed
        auto* obj = state.object_store.findObject(ObjectId("obj_unknown_fruit_001"));
        obj->flag = ObjectRuntimeFlag::Consumed;

        EatObjectResolveInput input{cmd, state};
        auto result = EatObjectResolver::resolve(input);
        assert(result.is_error());
    }

    std::cout << "eat_object_resolver tests passed" << std::endl;
}
