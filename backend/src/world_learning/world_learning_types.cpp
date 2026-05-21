#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include <algorithm>

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

namespace pathfinder::world_learning {

// ============================================================
// WorldLearningSourceKind
// ============================================================

std::string toString(WorldLearningSourceKind kind) {
    switch (kind) {
        case WorldLearningSourceKind::Unknown: return "Unknown";
        case WorldLearningSourceKind::DirectExperience: return "DirectExperience";
        case WorldLearningSourceKind::Observation: return "Observation";
        case WorldLearningSourceKind::Taught: return "Taught";
        case WorldLearningSourceKind::SystemGranted: return "SystemGranted";
        case WorldLearningSourceKind::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

pathfinder::foundation::Result<WorldLearningSourceKind> worldLearningSourceKindFromString(const std::string& str) {
    if (str == "DirectExperience") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::DirectExperience);
    if (str == "Observation") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::Observation);
    if (str == "Taught") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::Taught);
    if (str == "SystemGranted") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::SystemGranted);
    if (str == "TestOnly") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::TestOnly);
    if (str == "Unknown") return pathfinder::foundation::Result<WorldLearningSourceKind>::ok(WorldLearningSourceKind::Unknown);
    return Result<WorldLearningSourceKind>::fail(
        makeError(ErrorCode::validation_enum_unknown,
        "unknown_world_learning_source_kind",
        "Unknown WorldLearningSourceKind: " + str));
}

// ============================================================
// WorldLearningExperienceKind
// ============================================================

std::string toString(WorldLearningExperienceKind kind) {
    switch (kind) {
        case WorldLearningExperienceKind::Unknown: return "Unknown";
        case WorldLearningExperienceKind::ObjectAction: return "ObjectAction";
        case WorldLearningExperienceKind::ResourceHarvest: return "ResourceHarvest";
        case WorldLearningExperienceKind::ReactionCraft: return "ReactionCraft";
        case WorldLearningExperienceKind::EnvironmentEvent: return "EnvironmentEvent";
        case WorldLearningExperienceKind::ThreatEncounter: return "ThreatEncounter";
        case WorldLearningExperienceKind::Teaching: return "Teaching";
        case WorldLearningExperienceKind::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

pathfinder::foundation::Result<WorldLearningExperienceKind> worldLearningExperienceKindFromString(const std::string& str) {
    if (str == "ObjectAction") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::ObjectAction);
    if (str == "ResourceHarvest") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::ResourceHarvest);
    if (str == "ReactionCraft") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::ReactionCraft);
    if (str == "EnvironmentEvent") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::EnvironmentEvent);
    if (str == "ThreatEncounter") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::ThreatEncounter);
    if (str == "Teaching") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::Teaching);
    if (str == "TestOnly") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::TestOnly);
    if (str == "Unknown") return Result<WorldLearningExperienceKind>::ok(WorldLearningExperienceKind::Unknown);
    return Result<WorldLearningExperienceKind>::fail(
        makeError(ErrorCode::validation_enum_unknown,
        "unknown_world_learning_experience_kind",
        "Unknown WorldLearningExperienceKind: " + str));
}

// ============================================================
// WorldLearningFailureKind
// ============================================================

std::string toString(WorldLearningFailureKind kind) {
    switch (kind) {
        case WorldLearningFailureKind::None: return "None";
        case WorldLearningFailureKind::InvalidRequest: return "InvalidRequest";
        case WorldLearningFailureKind::NoExperience: return "NoExperience";
        case WorldLearningFailureKind::ActorMissing: return "ActorMissing";
        case WorldLearningFailureKind::TemplateMissing: return "TemplateMissing";
        case WorldLearningFailureKind::TemplateInvalid: return "TemplateInvalid";
        case WorldLearningFailureKind::UnsafeExperience: return "UnsafeExperience";
        case WorldLearningFailureKind::FeedbackBuildFailed: return "FeedbackBuildFailed";
        case WorldLearningFailureKind::LearningLoopFailed: return "LearningLoopFailed";
        case WorldLearningFailureKind::StoreFailed: return "StoreFailed";
        case WorldLearningFailureKind::ProjectionFailed: return "ProjectionFailed";
        case WorldLearningFailureKind::PartialFailed: return "PartialFailed";
    }
    return "None";
}

pathfinder::foundation::Result<WorldLearningFailureKind> worldLearningFailureKindFromString(const std::string& str) {
    if (str == "InvalidRequest") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::InvalidRequest);
    if (str == "NoExperience") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::NoExperience);
    if (str == "ActorMissing") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::ActorMissing);
    if (str == "TemplateMissing") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::TemplateMissing);
    if (str == "TemplateInvalid") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::TemplateInvalid);
    if (str == "UnsafeExperience") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::UnsafeExperience);
    if (str == "FeedbackBuildFailed") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::FeedbackBuildFailed);
    if (str == "LearningLoopFailed") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::LearningLoopFailed);
    if (str == "StoreFailed") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::StoreFailed);
    if (str == "ProjectionFailed") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::ProjectionFailed);
    if (str == "PartialFailed") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::PartialFailed);
    if (str == "None") return Result<WorldLearningFailureKind>::ok(WorldLearningFailureKind::None);
    return Result<WorldLearningFailureKind>::fail(
        makeError(ErrorCode::validation_enum_unknown,
        "unknown_world_learning_failure_kind",
        "Unknown WorldLearningFailureKind: " + str));
}

// ============================================================
// WorldKnowledgeLearningDecision
// ============================================================

std::string toString(WorldKnowledgeLearningDecision decision) {
    switch (decision) {
        case WorldKnowledgeLearningDecision::Unknown: return "Unknown";
        case WorldKnowledgeLearningDecision::Learned: return "Learned";
        case WorldKnowledgeLearningDecision::Reinforced: return "Reinforced";
        case WorldKnowledgeLearningDecision::Revised: return "Revised";
        case WorldKnowledgeLearningDecision::SkippedNoTemplate: return "SkippedNoTemplate";
        case WorldKnowledgeLearningDecision::SkippedUnsafe: return "SkippedUnsafe";
        case WorldKnowledgeLearningDecision::Failed: return "Failed";
        case WorldKnowledgeLearningDecision::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

pathfinder::foundation::Result<WorldKnowledgeLearningDecision> worldKnowledgeLearningDecisionFromString(const std::string& str) {
    if (str == "Learned") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::Learned);
    if (str == "Reinforced") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::Reinforced);
    if (str == "Revised") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::Revised);
    if (str == "SkippedNoTemplate") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::SkippedNoTemplate);
    if (str == "SkippedUnsafe") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::SkippedUnsafe);
    if (str == "Failed") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::Failed);
    if (str == "TestOnly") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::TestOnly);
    if (str == "Unknown") return Result<WorldKnowledgeLearningDecision>::ok(WorldKnowledgeLearningDecision::Unknown);
    return Result<WorldKnowledgeLearningDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown,
        "unknown_world_knowledge_learning_decision",
        "Unknown WorldKnowledgeLearningDecision: " + str));
}

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> worldLearningForbiddenKeys() {
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

bool containsWorldLearningForbiddenKey(const std::string& text) {
    for (const auto& key : worldLearningForbiddenKeys()) {
        if (text.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsWorldLearningForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsWorldLearningForbiddenKey(v)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// DTO validateBasic
// ============================================================

Result<void> WorldKnowledgeLearningRequest::validateBasic() const {
    if (request_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field,
            "request_id_empty", "request_id is empty"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field,
            "actor_key_empty", "actor_key is empty"));
    }
    if (source_kind == WorldLearningSourceKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown,
            "source_kind_unknown", "source_kind is Unknown"));
    }
    auto cmd_res = source_command.validateBasic();
    if (!cmd_res.is_ok()) return cmd_res;
    auto cr_res = command_result.validateBasic();
    if (!cr_res.is_ok()) return cr_res;
    return Result<void>::ok();
}

Result<void> WorldExperienceLearningDraft::validateBasic() const {
    if (draft_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field,
            "draft_id_empty", "draft_id is empty"));
    }
    if (experience_kind == WorldLearningExperienceKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown,
            "experience_kind_unknown", "experience_kind is Unknown"));
    }
    auto exp_res = experience.validateBasic();
    if (!exp_res.is_ok()) return exp_res;
    for (const auto& li : learning_inputs) {
        auto li_res = li.validateBasic();
        if (!li_res.is_ok()) return li_res;
    }
    return Result<void>::ok();
}

Result<void> WorldKnowledgeDelta::validateBasic() const {
    if (delta_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field,
            "delta_id_empty", "delta_id is empty"));
    }
    if (owner_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field,
            "owner_key_empty", "owner_key is empty"));
    }
    return Result<void>::ok();
}

Result<void> WorldKnowledgeLearningResult::validateBasic() const {
    for (const auto& d : drafts) {
        auto d_res = d.validateBasic();
        if (!d_res.is_ok()) return d_res;
    }
    for (const auto& lr : learning_results) {
        auto lr_res = lr.validateBasic();
        if (!lr_res.is_ok()) return lr_res;
    }
    for (const auto& c : learned_claims) {
        auto c_res = c.validateBasic();
        if (!c_res.is_ok()) return c_res;
    }
    for (const auto& e : events) {
        auto e_res = e.validateBasic();
        if (!e_res.is_ok()) return e_res;
    }
    for (const auto& sd : state_deltas) {
        auto sd_res = sd.validateBasic();
        if (!sd_res.is_ok()) return sd_res;
    }
    if (projection_patch.has_value()) {
        auto pp_res = projection_patch.value().validateBasic();
        if (!pp_res.is_ok()) return pp_res;
    }
    return Result<void>::ok();
}

// ============================================================
// classifyExperienceKind
// ============================================================

WorldLearningExperienceKind classifyExperienceKind(const std::string& command_key) {
    if (command_key == "gather" || command_key == "chop" || command_key == "mine" || command_key == "dig") {
        return WorldLearningExperienceKind::ResourceHarvest;
    }
    if (command_key == "craft" || command_key == "combine" || command_key == "use") {
        return WorldLearningExperienceKind::ReactionCraft;
    }
    if (command_key == "eat" || command_key == "inspect" || command_key == "pickup" || command_key == "drop") {
        return WorldLearningExperienceKind::ObjectAction;
    }
    return WorldLearningExperienceKind::Unknown;
}

} // namespace pathfinder::world_learning
