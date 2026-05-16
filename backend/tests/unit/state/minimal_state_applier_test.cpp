#include "pathfinder/state/minimal_state_applier.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/state/state_value.h"
#include "pathfinder/object/world_object.h"
#include "pathfinder/state/actor_state.h"
#include "pathfinder/foundation/time.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;
using namespace pathfinder::object;
using pathfinder::foundation::Tick;

void run_minimal_state_applier_tests() {
    // Test apply actor hunger change
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        ActorSurvivalState actor;
        actor.actor_id = EntityId("player1");
        actor.hunger = 80;
        actor.health = 100;
        state.actor_store.addActor(actor);

        StateChange change;
        change.change_id = StateChangeId("change1");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("player1");
        change.field_path = "actor.player1.hunger";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.before_value = StateValue::makeInt(80);
        change.after_value = StateValue::makeInt(70);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_ok());

        auto* updated_actor = state.actor_store.findActor(EntityId("player1"));
        assert(updated_actor != nullptr);
        assert(updated_actor->hunger == 70);
        assert(updated_actor->health == 100);
    }

    // Test apply object quantity change
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        ObjectDefinition def;
        def.definition_id = ObjectDefinitionId("fruit_def");
        def.category = ObjectCategory::Food;
        state.object_store.addDefinition(def);

        WorldObject obj;
        obj.object_id = ObjectId("fruit1");
        obj.definition_id = ObjectDefinitionId("fruit_def");
        obj.quantity = 5;
        state.object_store.addObject(obj);

        StateChange change;
        change.change_id = StateChangeId("change2");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Object;
        change.target_id = TargetId("fruit1");
        change.field_path = "object.fruit1.quantity";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.before_value = StateValue::makeInt(5);
        change.after_value = StateValue::makeInt(4);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_ok());

        auto* updated_obj = state.object_store.findObject(ObjectId("fruit1"));
        assert(updated_obj != nullptr);
        assert(updated_obj->quantity == 4);
    }

    // Test apply object consumed flag
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        ObjectDefinition def;
        def.definition_id = ObjectDefinitionId("fruit_def");
        def.category = ObjectCategory::Food;
        state.object_store.addDefinition(def);

        WorldObject obj;
        obj.object_id = ObjectId("fruit1");
        obj.definition_id = ObjectDefinitionId("fruit_def");
        obj.quantity = 1;
        state.object_store.addObject(obj);

        StateChange change;
        change.change_id = StateChangeId("change3");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Object;
        change.target_id = TargetId("fruit1");
        change.field_path = "object.fruit1.consumed";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.before_value = StateValue::makeBool(false);
        change.after_value = StateValue::makeBool(true);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_ok());

        auto* updated_obj = state.object_store.findObject(ObjectId("fruit1"));
        assert(updated_obj != nullptr);
        assert(updated_obj->flag == ObjectRuntimeFlag::Consumed);
    }

    // Test NoOp is skipped
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        ActorSurvivalState actor;
        actor.actor_id = EntityId("player1");
        actor.hunger = 80;
        actor.health = 100;
        state.actor_store.addActor(actor);

        StateChange change;
        change.change_id = StateChangeId("change4");
        change.operation = StateChangeOperation::NoOp;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("player1");
        change.field_path = "actor.player1.hunger";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_ok());

        auto* actor_ptr = state.actor_store.findActor(EntityId("player1"));
        assert(actor_ptr != nullptr);
        assert(actor_ptr->hunger == 80); // unchanged
    }

    // Test error on invalid field_path
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        StateChange change;
        change.change_id = StateChangeId("change5");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("player1");
        change.field_path = "invalid.path";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.after_value = StateValue::makeInt(100);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_error());
    }

    // Test error on missing actor
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);

        StateChange change;
        change.change_id = StateChangeId("change6");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("nonexistent");
        change.field_path = "actor.nonexistent.hunger";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.after_value = StateValue::makeInt(50);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_error());
    }

    // Test state version increments
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_1");
        state.metadata.current_tick = Tick(1);
        auto initial_version = state.metadata.state_version;

        ActorSurvivalState actor;
        actor.actor_id = EntityId("player1");
        actor.hunger = 80;
        actor.health = 100;
        state.actor_store.addActor(actor);

        StateChange change;
        change.change_id = StateChangeId("change7");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("player1");
        change.field_path = "actor.player1.hunger";
        change.before_hash = HashValue::fromString("before");
        change.after_hash = HashValue::fromString("after");
        change.status = StateChangeStatus::Draft;
        change.reason = "test";
        change.before_value = StateValue::makeInt(80);
        change.after_value = StateValue::makeInt(70);

        StateChangeSet changes;
        changes.addChange(change);

        auto result = MinimalStateApplier::apply(state, changes);
        assert(result.is_ok());
        assert(state.metadata.state_version == initial_version.next());
    }

    std::cout << "minimal_state_applier tests passed" << std::endl;
}
