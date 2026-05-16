#include "pathfinder/state/actor_state.h"

namespace pathfinder::state {

Result<void> ActorSurvivalState::validateBasic() const {
    if (actor_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "ActorSurvivalState: actor_id is empty"));
    }
    if (hunger < 0 || hunger > 100) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "ActorSurvivalState: hunger must be 0-100"));
    }
    if (health < 0 || health > 100) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "ActorSurvivalState: health must be 0-100"));
    }
    return Result<void>::ok();
}

Result<void> ActorStateStore::addActor(ActorSurvivalState actor) {
    auto vr = actor.validateBasic();
    if (vr.is_error()) return vr;
    for (const auto& existing : actors_) {
        if (existing.actor_id == actor.actor_id) {
            return Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "ActorStateStore: duplicate actor_id"));
        }
    }
    actors_.push_back(std::move(actor));
    return Result<void>::ok();
}

ActorSurvivalState* ActorStateStore::findActor(const EntityId& id) {
    for (auto& a : actors_) {
        if (a.actor_id == id) return &a;
    }
    return nullptr;
}

const ActorSurvivalState* ActorStateStore::findActor(const EntityId& id) const {
    for (const auto& a : actors_) {
        if (a.actor_id == id) return &a;
    }
    return nullptr;
}

Result<void> ActorStateStore::updateActor(const EntityId& id, int hunger, int health) {
    auto* actor = findActor(id);
    if (!actor) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "ActorStateStore: actor not found"));
    }
    if (hunger < 0 || hunger > 100) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "ActorStateStore: hunger must be 0-100"));
    }
    if (health < 0 || health > 100) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "ActorStateStore: health must be 0-100"));
    }
    actor->hunger = hunger;
    actor->health = health;
    return Result<void>::ok();
}

} // namespace pathfinder::state
