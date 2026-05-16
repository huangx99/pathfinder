#include "pathfinder/state/actor_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;

void run_actor_state_tests() {
    // ActorSurvivalState validateBasic
    {
        ActorSurvivalState actor;
        assert(actor.validateBasic().is_error()); // empty actor_id

        actor.actor_id = EntityId("actor_001");
        assert(actor.validateBasic().is_ok());
        assert(actor.hunger == 80);
        assert(actor.health == 100);
    }

    // Invalid hunger
    {
        ActorSurvivalState actor;
        actor.actor_id = EntityId("actor_001");
        actor.hunger = -1;
        assert(actor.validateBasic().is_error());

        actor.hunger = 101;
        assert(actor.validateBasic().is_error());
    }

    // Invalid health
    {
        ActorSurvivalState actor;
        actor.actor_id = EntityId("actor_001");
        actor.health = -1;
        assert(actor.validateBasic().is_error());

        actor.health = 101;
        assert(actor.validateBasic().is_error());
    }

    // ActorStateStore add and find
    {
        ActorStateStore store;
        ActorSurvivalState actor;
        actor.actor_id = EntityId("actor_001");
        actor.hunger = 80;
        actor.health = 100;
        assert(store.addActor(actor).is_ok());

        auto* found = store.findActor(EntityId("actor_001"));
        assert(found != nullptr);
        assert(found->hunger == 80);
        assert(found->health == 100);

        assert(store.findActor(EntityId("nonexistent")) == nullptr);
    }

    // Duplicate actor_id fails
    {
        ActorStateStore store;
        ActorSurvivalState actor;
        actor.actor_id = EntityId("actor_001");
        assert(store.addActor(actor).is_ok());
        assert(store.addActor(actor).is_error());
    }

    // updateActor
    {
        ActorStateStore store;
        ActorSurvivalState actor;
        actor.actor_id = EntityId("actor_001");
        store.addActor(actor);

        assert(store.updateActor(EntityId("actor_001"), 60, 90).is_ok());
        auto* found = store.findActor(EntityId("actor_001"));
        assert(found->hunger == 60);
        assert(found->health == 90);

        // Invalid update
        assert(store.updateActor(EntityId("actor_001"), -1, 90).is_error());
        assert(store.updateActor(EntityId("actor_001"), 60, 101).is_error());

        // Not found
        assert(store.updateActor(EntityId("nonexistent"), 60, 90).is_error());
    }

    std::cout << "actor_state tests passed" << std::endl;
}
