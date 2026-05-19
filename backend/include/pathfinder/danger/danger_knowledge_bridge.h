#pragma once

#include "pathfinder/danger/danger_types.h"

namespace pathfinder::danger {

struct DangerKnowledgePlannerInput {
    pathfinder::foundation::EntityId actor_id;
    std::vector<pathfinder::foundation::EntityId> observer_ids;
    std::vector<pathfinder::foundation::EntityId> taught_ids;
    std::vector<DangerKnowledgeDraft> knowledge_drafts;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class DangerKnowledgePlanner {
public:
    pathfinder::foundation::Result<pathfinder::reaction::ReactionKnowledgePlan> plan(
        const DangerKnowledgePlannerInput& input) const;
};

pathfinder::foundation::Result<pathfinder::reaction::ReactionKnowledgeDraft> toReactionKnowledgeDraft(
    const DangerKnowledgeDraft& draft);

} // namespace pathfinder::danger
