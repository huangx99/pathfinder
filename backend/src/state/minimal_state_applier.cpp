#include "pathfinder/state/minimal_state_applier.h"
#include "pathfinder/state/state_change_gate.h"

namespace pathfinder::state {

pathfinder::foundation::Result<void> MinimalStateApplier::apply(GameState& state, const StateChangeSet& changes) {
    // First validate through StateChangeGate
    auto gate_result = StateChangeGate::validate(state, changes);
    if (gate_result.is_error()) {
        return pathfinder::foundation::Result<void>::fail(gate_result.errors());
    }
    if (gate_result.value().hasIssues()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "MinimalStateApplier: StateChangeGate found issues"));
    }

    // Apply each change
    for (const auto& change : changes.changes()) {
        if (change.operation == StateChangeOperation::NoOp) {
            continue;
        }

        // Parse field_path to determine what to modify
        // P3 supported paths: "actor.<id>.hunger", "actor.<id>.health",
        //                      "object.<id>.quantity", "object.<id>.consumed"

        const auto& fp = change.field_path;

        if (fp.find("actor.") == 0) {
            // actor.<id>.<field>
            auto dot1 = fp.find('.', 6);
            if (dot1 == std::string::npos) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: invalid actor field_path: " + fp));
            }
            std::string actor_id_str = fp.substr(6, dot1 - 6);
            std::string field = fp.substr(dot1 + 1);

            pathfinder::foundation::EntityId actor_id(actor_id_str);
            auto* actor = state.actor_store.findActor(actor_id);
            if (!actor) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: actor not found: " + actor_id_str));
            }

            if (change.after_value && change.after_value->type == StateValueType::Int) {
                int new_val = change.after_value->int_value;
                if (field == "hunger") {
                    actor->hunger = new_val;
                } else if (field == "health") {
                    actor->health = new_val;
                } else {
                    return pathfinder::foundation::Result<void>::fail(
                        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                            "MinimalStateApplier: unsupported actor field: " + field));
                }
            } else {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: actor change requires int after_value"));
            }
        } else if (fp.find("object.") == 0) {
            // object.<id>.<field>
            auto dot1 = fp.find('.', 7);
            if (dot1 == std::string::npos) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: invalid object field_path: " + fp));
            }
            std::string obj_id_str = fp.substr(7, dot1 - 7);
            std::string field = fp.substr(dot1 + 1);

            pathfinder::foundation::ObjectId obj_id(obj_id_str);
            auto* obj = state.object_store.findObject(obj_id);
            if (!obj) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: object not found: " + obj_id_str));
            }

            if (field == "quantity") {
                if (change.after_value && change.after_value->type == StateValueType::Int) {
                    obj->quantity = change.after_value->int_value;
                } else {
                    return pathfinder::foundation::Result<void>::fail(
                        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                            "MinimalStateApplier: quantity change requires int after_value"));
                }
            } else if (field == "consumed") {
                if (change.operation == StateChangeOperation::Update) {
                    if (change.after_value && change.after_value->type == StateValueType::Bool && change.after_value->bool_value) {
                        obj->flag = pathfinder::object::ObjectRuntimeFlag::Consumed;
                    } else {
                        return pathfinder::foundation::Result<void>::fail(
                            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                                "MinimalStateApplier: consumed change requires bool true after_value"));
                    }
                }
            } else {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                        "MinimalStateApplier: unsupported object field: " + field));
            }
        } else {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "MinimalStateApplier: unsupported field_path: " + fp));
        }
    }

    // Increment state version
    state.metadata.state_version = state.metadata.state_version.next();

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::state
