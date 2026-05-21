#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/learning/learning_loop.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_teaching {

// ============================================================
// Enums
// ============================================================

enum class WorldTeachingFailureKind {
    Unknown,
    None,
    InvalidRequest,
    TeacherMissing,
    RecipientMissing,
    SameOwnerNotAllowed,
    OutOfRange,
    SourceClaimMissing,
    SourceClaimOwnerMismatch,
    ClaimNotTeachable,
    RecipientRejected,
    PropagationFailed,
    StoreFailed,
    ProjectionFailed,
    TestOnly
};

std::string toString(WorldTeachingFailureKind kind);
pathfinder::foundation::Result<WorldTeachingFailureKind> worldTeachingFailureKindFromString(const std::string& str);

enum class WorldTeachingDecision {
    Unknown,
    TaughtNewClaim,
    ReinforcedExistingClaim,
    RevisedExistingClaim,
    SkippedAlreadyKnown,
    Rejected,
    Failed,
    TestOnly
};

std::string toString(WorldTeachingDecision decision);
pathfinder::foundation::Result<WorldTeachingDecision> worldTeachingDecisionFromString(const std::string& str);

enum class NpcActionKnowledgeGateDecision {
    Unknown,
    AllowedByKnowledge,
    BlockedNoKnowledge,
    BlockedLowConfidence,
    BlockedDangerWithoutGoal,
    BlockedClaimDisproven,
    Failed,
    TestOnly
};

std::string toString(NpcActionKnowledgeGateDecision decision);
pathfinder::foundation::Result<NpcActionKnowledgeGateDecision> npcActionKnowledgeGateDecisionFromString(const std::string& str);

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> worldTeachingForbiddenKeys();
bool containsWorldTeachingForbiddenKey(const std::string& text);
bool containsWorldTeachingForbiddenKey(const std::vector<std::string>& values);

// ============================================================
// DTOs
// ============================================================

struct WorldTeachingRequest {
    std::string request_id;
    world_command::WorldCommandDto source_command;
    std::string teacher_actor_key;
    std::string recipient_actor_key;
    pathfinder::foundation::KnowledgeId source_knowledge_id;
    uint64_t tick = 0;
    double max_distance = 2.0;
    pathfinder::knowledge::KnowledgePropagationChannel channel;
    pathfinder::knowledge::KnowledgePropagationTrustBand trust_band;
    double trust_score = 0.8;
    std::vector<pathfinder::knowledge::KnowledgeClaim> teacher_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_claims;
    std::vector<std::string> condition_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldTeachingEligibilityResult {
    bool allowed = false;
    WorldTeachingFailureKind failure_kind = WorldTeachingFailureKind::None;
    std::optional<pathfinder::knowledge::KnowledgeClaim> source_claim;
    pathfinder::knowledge::KnowledgeOwner teacher_owner;
    pathfinder::knowledge::KnowledgeOwner recipient_owner;
    double distance = 0.0;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldTeachingResult {
    bool ok = false;
    WorldTeachingDecision decision = WorldTeachingDecision::Unknown;
    WorldTeachingFailureKind failure_kind = WorldTeachingFailureKind::None;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_created_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_updated_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_snapshot_after;
    std::optional<pathfinder::learning::LearningLoopResult> learning_result;
    std::vector<world_command::WorldEventDto> events;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
    std::optional<world_command::WorldProjectionPatchDto> projection_patch;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct NpcActionKnowledgeGateRequest {
    std::string actor_key;
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string target_object_key;
    bool allow_hypothesis = true;
    bool allow_risk_action = false;
    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_claims;
    std::vector<std::string> condition_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct NpcActionKnowledgeGateResult {
    bool allowed = false;
    NpcActionKnowledgeGateDecision decision = NpcActionKnowledgeGateDecision::Unknown;
    std::optional<pathfinder::knowledge::KnowledgeClaim> matched_claim;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::world_teaching
