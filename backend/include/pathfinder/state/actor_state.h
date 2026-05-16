#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::state {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::Result;

struct ActorSurvivalState {
    EntityId actor_id;
    int hunger = 80;  // 0-100, lower = less hungry
    int health = 100; // 0-100

    Result<void> validateBasic() const;
};

class ActorStateStore {
public:
    Result<void> addActor(ActorSurvivalState actor);
    ActorSurvivalState* findActor(const EntityId& id);
    const ActorSurvivalState* findActor(const EntityId& id) const;
    Result<void> updateActor(const EntityId& id, int hunger, int health);

    const std::vector<ActorSurvivalState>& actors() const { return actors_; }

private:
    std::vector<ActorSurvivalState> actors_;
};

} // namespace pathfinder::state
