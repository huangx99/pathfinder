#include "pathfinder/world_teaching/world_teaching_types.h"
#include <algorithm>

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

namespace pathfinder::world_teaching {

// ============================================================
// WorldTeachingFailureKind
// ============================================================

std::string toString(WorldTeachingFailureKind kind) {
    switch (kind) {
        case WorldTeachingFailureKind::Unknown: return "Unknown";
        case WorldTeachingFailureKind::None: return "None";
        case WorldTeachingFailureKind::InvalidRequest: return "InvalidRequest";
        case WorldTeachingFailureKind::TeacherMissing: return "TeacherMissing";
        case WorldTeachingFailureKind::RecipientMissing: return "RecipientMissing";
        case WorldTeachingFailureKind::SameOwnerNotAllowed: return "SameOwnerNotAllowed";
        case WorldTeachingFailureKind::OutOfRange: return "OutOfRange";
        case WorldTeachingFailureKind::SourceClaimMissing: return "SourceClaimMissing";
        case WorldTeachingFailureKind::SourceClaimOwnerMismatch: return "SourceClaimOwnerMismatch";
        case WorldTeachingFailureKind::ClaimNotTeachable: return "ClaimNotTeachable";
        case WorldTeachingFailureKind::RecipientRejected: return "RecipientRejected";
        case WorldTeachingFailureKind::PropagationFailed: return "PropagationFailed";
        case WorldTeachingFailureKind::StoreFailed: return "StoreFailed";
        case WorldTeachingFailureKind::ProjectionFailed: return "ProjectionFailed";
        case WorldTeachingFailureKind::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

Result<WorldTeachingFailureKind> worldTeachingFailureKindFromString(const std::string& str) {
    if (str == "Unknown") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::Unknown);
    if (str == "None") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::None);
    if (str == "InvalidRequest") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::InvalidRequest);
    if (str == "TeacherMissing") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::TeacherMissing);
    if (str == "RecipientMissing") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::RecipientMissing);
    if (str == "SameOwnerNotAllowed") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::SameOwnerNotAllowed);
    if (str == "OutOfRange") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::OutOfRange);
    if (str == "SourceClaimMissing") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::SourceClaimMissing);
    if (str == "SourceClaimOwnerMismatch") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::SourceClaimOwnerMismatch);
    if (str == "ClaimNotTeachable") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::ClaimNotTeachable);
    if (str == "RecipientRejected") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::RecipientRejected);
    if (str == "PropagationFailed") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::PropagationFailed);
    if (str == "StoreFailed") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::StoreFailed);
    if (str == "ProjectionFailed") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::ProjectionFailed);
    if (str == "TestOnly") return Result<WorldTeachingFailureKind>::ok(WorldTeachingFailureKind::TestOnly);
    return Result<WorldTeachingFailureKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "unknown_world_teaching_failure_kind", "Unknown WorldTeachingFailureKind: " + str));
}

// ============================================================
// WorldTeachingDecision
// ============================================================

std::string toString(WorldTeachingDecision decision) {
    switch (decision) {
        case WorldTeachingDecision::Unknown: return "Unknown";
        case WorldTeachingDecision::TaughtNewClaim: return "TaughtNewClaim";
        case WorldTeachingDecision::ReinforcedExistingClaim: return "ReinforcedExistingClaim";
        case WorldTeachingDecision::RevisedExistingClaim: return "RevisedExistingClaim";
        case WorldTeachingDecision::SkippedAlreadyKnown: return "SkippedAlreadyKnown";
        case WorldTeachingDecision::Rejected: return "Rejected";
        case WorldTeachingDecision::Failed: return "Failed";
        case WorldTeachingDecision::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

Result<WorldTeachingDecision> worldTeachingDecisionFromString(const std::string& str) {
    if (str == "TaughtNewClaim") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::TaughtNewClaim);
    if (str == "ReinforcedExistingClaim") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::ReinforcedExistingClaim);
    if (str == "RevisedExistingClaim") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::RevisedExistingClaim);
    if (str == "SkippedAlreadyKnown") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::SkippedAlreadyKnown);
    if (str == "Rejected") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::Rejected);
    if (str == "Failed") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::Failed);
    if (str == "TestOnly") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::TestOnly);
    if (str == "Unknown") return Result<WorldTeachingDecision>::ok(WorldTeachingDecision::Unknown);
    return Result<WorldTeachingDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "unknown_world_teaching_decision", "Unknown WorldTeachingDecision: " + str));
}

// ============================================================
// NpcActionKnowledgeGateDecision
// ============================================================

std::string toString(NpcActionKnowledgeGateDecision decision) {
    switch (decision) {
        case NpcActionKnowledgeGateDecision::Unknown: return "Unknown";
        case NpcActionKnowledgeGateDecision::AllowedByKnowledge: return "AllowedByKnowledge";
        case NpcActionKnowledgeGateDecision::BlockedNoKnowledge: return "BlockedNoKnowledge";
        case NpcActionKnowledgeGateDecision::BlockedLowConfidence: return "BlockedLowConfidence";
        case NpcActionKnowledgeGateDecision::BlockedDangerWithoutGoal: return "BlockedDangerWithoutGoal";
        case NpcActionKnowledgeGateDecision::BlockedClaimDisproven: return "BlockedClaimDisproven";
        case NpcActionKnowledgeGateDecision::Failed: return "Failed";
        case NpcActionKnowledgeGateDecision::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

Result<NpcActionKnowledgeGateDecision> npcActionKnowledgeGateDecisionFromString(const std::string& str) {
    if (str == "AllowedByKnowledge") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::AllowedByKnowledge);
    if (str == "BlockedNoKnowledge") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::BlockedNoKnowledge);
    if (str == "BlockedLowConfidence") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::BlockedLowConfidence);
    if (str == "BlockedDangerWithoutGoal") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::BlockedDangerWithoutGoal);
    if (str == "BlockedClaimDisproven") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::BlockedClaimDisproven);
    if (str == "Failed") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::Failed);
    if (str == "TestOnly") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::TestOnly);
    if (str == "Unknown") return Result<NpcActionKnowledgeGateDecision>::ok(NpcActionKnowledgeGateDecision::Unknown);
    return Result<NpcActionKnowledgeGateDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "unknown_npc_action_knowledge_gate_decision", "Unknown NpcActionKnowledgeGateDecision: " + str));
}

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> worldTeachingForbiddenKeys() {
    return {
        "hidden_truth",
        "true_trait",
        "real_effect",
        "raw_state",
        "actual_hp",
        "true_hp",
        "hp_delta",
        "death",
        "kill",
        "corpse",
        "loot",
        "drop_internal",
        "frontend_unlock",
        "direct_state"
    };
}

bool containsWorldTeachingForbiddenKey(const std::string& text) {
    for (const auto& key : worldTeachingForbiddenKeys()) {
        if (text.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsWorldTeachingForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsWorldTeachingForbiddenKey(v)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// DTO validateBasic
// ============================================================

Result<void> WorldTeachingRequest::validateBasic() const {
    if (request_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "request_id_empty", "request_id is empty"));
    }
    if (teacher_actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "teacher_actor_key_empty", "teacher_actor_key is empty"));
    }
    if (recipient_actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "recipient_actor_key_empty", "recipient_actor_key is empty"));
    }
    if (source_knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "source_knowledge_id_empty", "source_knowledge_id is empty"));
    }
    if (max_distance <= 0.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "max_distance_invalid", "max_distance must be > 0"));
    }
    auto cmd_res = source_command.validateBasic();
    if (!cmd_res.is_ok()) return cmd_res;
    for (const auto& c : teacher_claims) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    for (const auto& c : recipient_claims) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    if (containsWorldTeachingForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "reason_keys_contain_forbidden", "reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> WorldTeachingEligibilityResult::validateBasic() const {
    if (containsWorldTeachingForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "reason_keys_contain_forbidden", "reason_keys contain forbidden key"));
    }
    if (containsWorldTeachingForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "warning_keys_contain_forbidden", "warning_keys contain forbidden key"));
    }
    auto owner_r = teacher_owner.validateBasic();
    if (!owner_r.is_ok()) return owner_r;
    auto rec_r = recipient_owner.validateBasic();
    if (!rec_r.is_ok()) return rec_r;
    if (source_claim.has_value()) {
        auto sc_r = source_claim->validateBasic();
        if (!sc_r.is_ok()) return sc_r;
    }
    return Result<void>::ok();
}

Result<void> WorldTeachingResult::validateBasic() const {
    for (const auto& c : recipient_created_claims) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    for (const auto& c : recipient_updated_claims) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    for (const auto& c : recipient_snapshot_after) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    for (const auto& e : events) {
        auto r = e.validateBasic();
        if (!r.is_ok()) return r;
    }
    for (const auto& sd : state_deltas) {
        auto r = sd.validateBasic();
        if (!r.is_ok()) return r;
    }
    if (projection_patch.has_value()) {
        auto pp_r = projection_patch->validateBasic();
        if (!pp_r.is_ok()) return pp_r;
    }
    if (learning_result.has_value()) {
        auto lr_r = learning_result->validateBasic();
        if (!lr_r.is_ok()) return lr_r;
    }
    if (containsWorldTeachingForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "reason_keys_contain_forbidden", "reason_keys contain forbidden key"));
    }
    if (containsWorldTeachingForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "warning_keys_contain_forbidden", "warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> NpcActionKnowledgeGateRequest::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "actor_key_empty", "actor_key is empty"));
    }
    if (subject_object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "subject_object_key_empty", "subject_object_key is empty"));
    }
    if (action_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "action_key_empty", "action_key is empty"));
    }
    for (const auto& c : actor_claims) {
        auto r = c.validateBasic();
        if (!r.is_ok()) return r;
    }
    return Result<void>::ok();
}

Result<void> NpcActionKnowledgeGateResult::validateBasic() const {
    if (containsWorldTeachingForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "reason_keys_contain_forbidden", "reason_keys contain forbidden key"));
    }
    if (containsWorldTeachingForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "warning_keys_contain_forbidden", "warning_keys contain forbidden key"));
    }
    if (matched_claim.has_value()) {
        auto r = matched_claim->validateBasic();
        if (!r.is_ok()) return r;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_teaching
