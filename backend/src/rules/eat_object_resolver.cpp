#include "pathfinder/rules/eat_object_resolver.h"

namespace pathfinder::rules {

bool EatObjectResolver::canResolve(const pathfinder::command::CommandEnvelope& command) {
    return command.payload.action_id == pathfinder::foundation::ActionId("eat");
}

pathfinder::foundation::Result<EatObjectResolveDraft> EatObjectResolver::resolve(const EatObjectResolveInput& input) {
    using namespace pathfinder::foundation;
    using namespace pathfinder::state;
    using namespace pathfinder::event;
    using namespace pathfinder::cognition;
    using namespace pathfinder::object;

    const auto& command = input.command;
    const auto& state = input.state;

    // Validate actor exists
    auto* actor = state.actor_store.findActor(command.payload.actor_id);
    if (!actor) {
        return Result<EatObjectResolveDraft>::fail(
            makeError(ErrorCode::validation_failed, "EatObjectResolver: actor not found"));
    }

    // Validate target exists
    if (command.payload.targets.empty()) {
        return Result<EatObjectResolveDraft>::fail(
            makeError(ErrorCode::validation_failed, "EatObjectResolver: no targets"));
    }

    const auto& target = command.payload.targets[0];
    ObjectId object_id(target.target_id.value());
    auto* obj = state.object_store.findObject(object_id);
    if (!obj) {
        return Result<EatObjectResolveDraft>::fail(
            makeError(ErrorCode::validation_failed, "EatObjectResolver: object not found"));
    }

    // Check object is not already consumed
    if (obj->flag == ObjectRuntimeFlag::Consumed) {
        return Result<EatObjectResolveDraft>::fail(
            makeError(ErrorCode::validation_failed, "EatObjectResolver: object already consumed"));
    }

    // Get object definition
    auto* def = state.object_store.findDefinition(obj->definition_id);
    if (!def) {
        return Result<EatObjectResolveDraft>::fail(
            makeError(ErrorCode::validation_failed, "EatObjectResolver: object definition not found"));
    }

    // Build draft
    EatObjectResolveDraft draft;

    // Create state changes
    int new_hunger = actor->hunger + def->edible_profile.hunger_delta;
    int new_health = actor->health + def->edible_profile.health_delta;

    // Hunger change
    StateChange hunger_change;
    hunger_change.change_id = StateChangeId("change_hunger_" + command.command_id.value());
    hunger_change.operation = StateChangeOperation::Update;
    hunger_change.domain = StateDomain::Entity;
    hunger_change.target_id = TargetId(command.payload.actor_id.value());
    hunger_change.field_path = "actor." + command.payload.actor_id.value() + ".hunger";
    hunger_change.before_hash = HashValue::fromString("hunger_before");
    hunger_change.after_hash = HashValue::fromString("hunger_after");
    hunger_change.status = StateChangeStatus::Draft;
    hunger_change.reason = "eat effect";
    hunger_change.before_value = StateValue::makeInt(actor->hunger);
    hunger_change.after_value = StateValue::makeInt(new_hunger);
    draft.state_changes.addChange(hunger_change);

    // Health change (only if non-zero)
    if (def->edible_profile.health_delta != 0) {
        StateChange health_change;
        health_change.change_id = StateChangeId("change_health_" + command.command_id.value());
        health_change.operation = StateChangeOperation::Update;
        health_change.domain = StateDomain::Entity;
        health_change.target_id = TargetId(command.payload.actor_id.value());
        health_change.field_path = "actor." + command.payload.actor_id.value() + ".health";
        health_change.before_hash = HashValue::fromString("health_before");
        health_change.after_hash = HashValue::fromString("health_after");
        health_change.status = StateChangeStatus::Draft;
        health_change.reason = "eat effect";
        health_change.before_value = StateValue::makeInt(actor->health);
        health_change.after_value = StateValue::makeInt(new_health);
        draft.state_changes.addChange(health_change);
    }

    // Object consumed change
    StateChange consumed_change;
    consumed_change.change_id = StateChangeId("change_consumed_" + command.command_id.value());
    consumed_change.operation = StateChangeOperation::Update;
    consumed_change.domain = StateDomain::Object;
    consumed_change.target_id = target.target_id;
    consumed_change.field_path = "object." + target.target_id.value() + ".consumed";
    consumed_change.before_hash = HashValue::fromString("consumed_before");
    consumed_change.after_hash = HashValue::fromString("consumed_after");
    consumed_change.status = StateChangeStatus::Draft;
    consumed_change.reason = "eat effect";
    consumed_change.before_value = StateValue::makeBool(false);
    consumed_change.after_value = StateValue::makeBool(true);
    draft.state_changes.addChange(consumed_change);

    // Create events
    EventRecord action_event;
    action_event.event_id = EventId("event_action_" + command.command_id.value());
    action_event.event_type = EventType::ActionResolved;
    action_event.command_id = command.command_id;
    action_event.tick = command.issued_tick;
    action_event.state_version = state.metadata.state_version;
    action_event.source_id = command.payload.actor_id.value();
    action_event.target_ids.push_back(target.target_id);
    action_event.payload.payload_type = "action";
    action_event.payload.message_key = "eat_resolved";
    action_event.payload.debug_text = "Eat action resolved for " + obj->definition_id.value();
    action_event.visibility = EventVisibility::PlayerVisible;
    action_event.importance = EventImportance::Normal;
    draft.events.push_back(action_event);

    EventRecord feedback_event;
    feedback_event.event_id = EventId("event_feedback_" + command.command_id.value());
    feedback_event.event_type = EventType::FeedbackGenerated;
    feedback_event.command_id = command.command_id;
    feedback_event.tick = command.issued_tick;
    feedback_event.state_version = state.metadata.state_version;
    feedback_event.source_id = command.payload.actor_id.value();
    feedback_event.target_ids.push_back(target.target_id);
    feedback_event.payload.payload_type = "feedback";
    feedback_event.payload.message_key = "eat_feedback";
    feedback_event.payload.debug_text = "Hunger changed by " + std::to_string(def->edible_profile.hunger_delta);
    feedback_event.visibility = EventVisibility::PlayerVisible;
    feedback_event.importance = EventImportance::Normal;
    draft.events.push_back(feedback_event);

    // Create cognition evidence
    CognitionEvidence evidence;
    evidence.key.subject_id = command.payload.actor_id;
    evidence.key.object_definition_id = obj->definition_id;
    evidence.key.action_id = command.payload.action_id;
    evidence.key.effect_kind = CognitionEffectKind::Edible;
    evidence.source_event_id = feedback_event.event_id;
    evidence.confidence_delta = 0.3;
    evidence.observed_hunger_delta = def->edible_profile.hunger_delta;
    evidence.observed_health_delta = def->edible_profile.health_delta;
    draft.cognition_evidence.push_back(evidence);

    return Result<EatObjectResolveDraft>::ok(std::move(draft));
}

} // namespace pathfinder::rules
