#include "pathfinder/rules/unknown_fruit_fixture.h"

namespace pathfinder::rules {

pathfinder::state::GameState UnknownFruitFixture::createInitialState() {
    using namespace pathfinder::state;
    using namespace pathfinder::object;
    using namespace pathfinder::foundation;
    using namespace pathfinder::cognition;

    GameState state;

    // Set metadata
    state.metadata.state_id = GameStateId("fixture_state_001");
    state.metadata.current_tick = Tick(1);
    state.metadata.state_version = StateVersion(1);

    // Create unknown_fruit definition
    ObjectDefinition fruit_def;
    fruit_def.definition_id = ObjectDefinitionId("unknown_fruit");
    fruit_def.category = ObjectCategory::Food;
    fruit_def.allowed_action_ids.push_back(ActionId("eat"));
    fruit_def.edible_profile.effect_kind = EdibleEffectKind::Edible;
    fruit_def.edible_profile.hunger_delta = -20;
    fruit_def.edible_profile.health_delta = 0;
    fruit_def.edible_profile.risk_level = 0;

    state.object_store.addDefinition(fruit_def);

    // Create world object instance
    WorldObject fruit_obj;
    fruit_obj.object_id = ObjectId("obj_unknown_fruit_001");
    fruit_obj.definition_id = ObjectDefinitionId("unknown_fruit");
    fruit_obj.quantity = 1;
    fruit_obj.flag = ObjectRuntimeFlag::None;

    state.object_store.addObject(fruit_obj);

    // Create actor
    ActorSurvivalState actor;
    actor.actor_id = EntityId("actor_001");
    actor.hunger = 80;
    actor.health = 100;

    state.actor_store.addActor(actor);

    // Cognition state starts empty (no claims)

    return state;
}

pathfinder::command::CommandEnvelope UnknownFruitFixture::createEatCommand() {
    using namespace pathfinder::command;
    using namespace pathfinder::foundation;

    CommandEnvelope envelope;
    envelope.command_id = CommandId("cmd_try_eat_001");
    envelope.source = CommandSource::Test;
    envelope.issued_tick = Tick(1);

    envelope.payload.action_id = ActionId("eat");
    envelope.payload.actor_id = EntityId("actor_001");
    envelope.payload.intent = CommandIntent::Experiment;

    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId("obj_unknown_fruit_001");
    target.role = ActionTargetRole::Primary;

    envelope.payload.targets.push_back(target);

    return envelope;
}

} // namespace pathfinder::rules
