#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/memory/memory_record.h"
#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/content/content_types.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_learning {

// ============================================================
// Enums
// ============================================================

enum class WorldLearningSourceKind {
    Unknown,
    DirectExperience,
    Observation,
    Taught,
    SystemGranted,
    TestOnly
};

std::string toString(WorldLearningSourceKind kind);
pathfinder::foundation::Result<WorldLearningSourceKind> worldLearningSourceKindFromString(const std::string& str);

enum class WorldLearningExperienceKind {
    Unknown,
    ObjectAction,
    ResourceHarvest,
    ReactionCraft,
    EnvironmentEvent,
    ThreatEncounter,
    Teaching,
    TestOnly
};

std::string toString(WorldLearningExperienceKind kind);
pathfinder::foundation::Result<WorldLearningExperienceKind> worldLearningExperienceKindFromString(const std::string& str);

enum class WorldLearningFailureKind {
    None,
    InvalidRequest,
    NoExperience,
    ActorMissing,
    TemplateMissing,
    TemplateInvalid,
    UnsafeExperience,
    FeedbackBuildFailed,
    LearningLoopFailed,
    StoreFailed,
    ProjectionFailed,
    PartialFailed
};

std::string toString(WorldLearningFailureKind kind);
pathfinder::foundation::Result<WorldLearningFailureKind> worldLearningFailureKindFromString(const std::string& str);

enum class WorldKnowledgeLearningDecision {
    Unknown,
    Learned,
    Reinforced,
    Revised,
    SkippedNoTemplate,
    SkippedUnsafe,
    Failed,
    TestOnly
};

std::string toString(WorldKnowledgeLearningDecision decision);
pathfinder::foundation::Result<WorldKnowledgeLearningDecision> worldKnowledgeLearningDecisionFromString(const std::string& str);

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> worldLearningForbiddenKeys();
bool containsWorldLearningForbiddenKey(const std::string& text);
bool containsWorldLearningForbiddenKey(const std::vector<std::string>& values);

// ============================================================
// DTOs
// ============================================================

struct WorldKnowledgeLearningRequest {
    std::string request_id;
    world_command::WorldCommandDto source_command;
    world_command::WorldCommandExecutionResult command_result;
    WorldLearningSourceKind source_kind = WorldLearningSourceKind::Unknown;
    std::string actor_key;
    std::vector<pathfinder::knowledge::KnowledgeClaim> existing_actor_claims;
    std::vector<pathfinder::memory::MemoryRecord> existing_memories;
    std::vector<pathfinder::memory::MemorySummary> existing_summaries;
    uint64_t tick = 0;
    std::map<std::string, std::string> parameters;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldExperienceLearningDraft {
    std::string draft_id;
    world_command::WorldExperienceDto experience;
    WorldLearningExperienceKind experience_kind = WorldLearningExperienceKind::Unknown;
    std::vector<std::string> candidate_template_keys;
    std::vector<pathfinder::content::KnowledgeTemplateContent> templates;
    std::vector<pathfinder::learning::LearningLoopInput> learning_inputs;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldKnowledgeDelta {
    std::string delta_id;
    pathfinder::foundation::KnowledgeId knowledge_id;
    std::string owner_key;
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string target_object_key;
    std::string status;
    std::string decision;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldKnowledgeLearningResult {
    bool ok = false;
    WorldKnowledgeLearningDecision decision = WorldKnowledgeLearningDecision::Unknown;
    WorldLearningFailureKind failure_kind = WorldLearningFailureKind::None;
    std::vector<WorldExperienceLearningDraft> drafts;
    std::vector<pathfinder::learning::LearningLoopResult> learning_results;
    std::vector<pathfinder::knowledge::KnowledgeClaim> learned_claims;
    std::vector<WorldKnowledgeDelta> knowledge_deltas;
    std::vector<world_command::WorldEventDto> events;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
    std::optional<world_command::WorldProjectionPatchDto> projection_patch;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Experience -> ExperienceKind mapping helper
// ============================================================

WorldLearningExperienceKind classifyExperienceKind(const std::string& command_key);

} // namespace pathfinder::world_learning
