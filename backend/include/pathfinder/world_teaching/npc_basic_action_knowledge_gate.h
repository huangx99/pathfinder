#pragma once

#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/foundation/result.h"
#include <vector>
#include <string>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// NpcBasicActionKnowledgeGate
// ---------------------------------------------------------------------------
// Determines whether an NPC has sufficient knowledge to attempt a single-step
// action. Does not plan paths or evaluate resource availability.
// ---------------------------------------------------------------------------

class NpcBasicActionKnowledgeGate {
public:
    NpcActionKnowledgeGateResult check(
        const NpcActionKnowledgeGateRequest& request) const;

    // Convenience overload that queries repository for actor claims.
    NpcActionKnowledgeGateResult check(
        const std::string& actor_key,
        const std::string& subject_object_key,
        const std::string& action_key,
        const std::string& effect_key,
        const std::string& target_object_key,
        bool allow_hypothesis,
        bool allow_risk_action,
        const pathfinder::knowledge::KnowledgeRepository& repository) const;
};

} // namespace pathfinder::world_teaching
