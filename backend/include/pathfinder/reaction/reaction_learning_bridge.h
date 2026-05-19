#pragma once

#include "pathfinder/reaction/reaction_content.h"
#include "pathfinder/reaction/reaction_resolver.h"

namespace pathfinder::reaction {

enum class ReactionLearningSourceKind {
    Unknown,
    DirectExperience,
    Observation,
    Taught,
    TestOnly
};

std::string toString(ReactionLearningSourceKind kind);
pathfinder::foundation::Result<ReactionLearningSourceKind> reactionLearningSourceKindFromString(const std::string& value);

struct ReactionKnowledgeDeliveryDraft {
    pathfinder::foundation::EntityId owner_id;
    ReactionLearningSourceKind source_kind{ReactionLearningSourceKind::Unknown};
    ReactionKnowledgeDraft knowledge;
    double confidence_hint{0.0};
    bool shareable{true};
    bool teachable{true};
    bool npc_learnable{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionKnowledgePlan {
    std::vector<ReactionKnowledgeDeliveryDraft> deliveries;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionKnowledgePlannerInput {
    pathfinder::foundation::EntityId actor_id;
    std::vector<pathfinder::foundation::EntityId> observer_ids;
    std::vector<pathfinder::foundation::EntityId> taught_ids;
    ReactionResult result;
    std::vector<ReactionKnowledgeTemplate> knowledge_templates;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ReactionKnowledgePlanner {
public:
    pathfinder::foundation::Result<ReactionKnowledgePlan> plan(const ReactionKnowledgePlannerInput& input) const;
};

} // namespace pathfinder::reaction
