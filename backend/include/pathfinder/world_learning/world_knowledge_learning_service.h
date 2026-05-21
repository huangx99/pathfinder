#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_learning/world_experience_extractor.h"
#include "pathfinder/world_learning/world_knowledge_template_resolver.h"
#include "pathfinder/world_learning/world_safe_feedback_builder.h"
#include "pathfinder/world_learning/world_learning_loop_bridge.h"
#include "pathfinder/world_learning/world_knowledge_store_port.h"
#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldKnowledgeLearningService
// ---------------------------------------------------------------------------
// Orchestrates the full P49 learning pipeline:
//   extract -> resolve -> build -> learn -> apply -> project
// Does not directly mutate world runtime or inventory state.
// ---------------------------------------------------------------------------

class WorldKnowledgeLearningService {
public:
    WorldKnowledgeLearningService(
        const pathfinder::content::ContentRegistry& content_registry,
        const pathfinder::learning::LearningLoopService& learning_service,
        pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
        const pathfinder::learning::LearningLoopOptions& options = {});

    pathfinder::foundation::Result<WorldKnowledgeLearningResult> learnFromCommandResult(
        const WorldKnowledgeLearningRequest& request);

private:
    const pathfinder::content::ContentRegistry& content_registry_;
    WorldExperienceExtractor extractor_;
    WorldKnowledgeTemplateResolver resolver_;
    WorldSafeFeedbackBuilder feedback_builder_;
    WorldLearningLoopBridge loop_bridge_;
    WorldKnowledgeStorePort store_port_;
    WorldKnowledgeProjectionBridge projection_bridge_;

    pathfinder::foundation::Result<WorldKnowledgeLearningResult> processExperience(
        const world_command::WorldExperienceDto& experience,
        const WorldKnowledgeLearningRequest& request,
        WorldKnowledgeLearningResult& aggregate_result);

    WorldKnowledgeDelta makeDelta(
        const pathfinder::knowledge::KnowledgeClaim& claim,
        const std::string& decision_label) const;

    bool isCommandResultSuccessful(
        const world_command::WorldCommandExecutionResult& command_result) const;
};

} // namespace pathfinder::world_learning
