#pragma once

#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// WorldTeachingLoopBridge
// ---------------------------------------------------------------------------
// Bridges a validated teaching request to the knowledge propagation system.
// P50 uses KnowledgePropagationService directly (instead of LearningLoopService)
// because LearningLoopService propagates ALL transferable actor claims,
// while P50 must propagate exactly the specified knowledge_id with
// explicit teachable validation.
// ---------------------------------------------------------------------------

class WorldTeachingLoopBridge {
public:
    struct BridgeResult {
        bool ok = false;
        WorldTeachingDecision decision = WorldTeachingDecision::Unknown;
        WorldTeachingFailureKind failure_kind = WorldTeachingFailureKind::None;
        std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_created_claims;
        std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_updated_claims;
        std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_snapshot_after;
        std::vector<std::string> reason_keys;
        std::vector<std::string> warning_keys;
    };

    BridgeResult propagate(
        const pathfinder::knowledge::KnowledgeClaim& source_claim,
        const pathfinder::knowledge::KnowledgeOwner& recipient_owner,
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& recipient_existing_claims,
        const pathfinder::knowledge::KnowledgePropagationChannel& channel,
        const pathfinder::knowledge::KnowledgePropagationTrustBand& trust_band,
        double trust_score,
        pathfinder::foundation::Tick tick,
        const std::string& loop_key) const;
};

} // namespace pathfinder::world_teaching
