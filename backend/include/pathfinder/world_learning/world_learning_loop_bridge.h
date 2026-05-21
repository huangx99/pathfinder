#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/memory/memory_record.h"
#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldLearningLoopBridge
// ---------------------------------------------------------------------------
// Bridges world experiences to the existing LearningLoopService.
// Does not rewrite learning algorithms.
// ---------------------------------------------------------------------------

class WorldLearningLoopBridge {
public:
    struct BridgeResult {
        pathfinder::learning::LearningLoopResult loop_result;
        WorldLearningFailureKind failure_kind = WorldLearningFailureKind::None;
        std::vector<std::string> warning_keys;
    };

    WorldLearningLoopBridge(
        const pathfinder::learning::LearningLoopService& learning_service,
        const pathfinder::learning::LearningLoopOptions& options = {});

    BridgeResult run(
        const pathfinder::learning::LearningSafeFeedbackInput& feedback,
        const std::string& actor_key,
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& existing_claims,
        const std::vector<pathfinder::memory::MemoryRecord>& existing_memories,
        const std::vector<pathfinder::memory::MemorySummary>& existing_summaries,
        uint64_t tick,
        const std::string& loop_key) const;

private:
    const pathfinder::learning::LearningLoopService& learning_service_;
    pathfinder::learning::LearningLoopOptions options_;
};

} // namespace pathfinder::world_learning
