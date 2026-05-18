#include "pathfinder/civilization/civilization_types.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

namespace pathfinder::civilization {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isValidRatio(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

static double clampRatio(double value) {
    if (!std::isfinite(value)) return 0.0;
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

static Result<void> validateRatio(double value, const std::string& field) {
    if (!isValidRatio(value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    }
    return Result<void>::ok();
}

static bool isValidStage(CivilizationStage stage) {
    return stage != CivilizationStage::Unknown && stage != CivilizationStage::TestOnly;
}

static bool isValidCapabilityType(CapabilityType type) {
    return type != CapabilityType::Unknown && type != CapabilityType::TestOnly;
}

static bool isValidMaturity(CapabilityMaturityState state) {
    return state != CapabilityMaturityState::Unknown && state != CapabilityMaturityState::TestOnly;
}

static bool isValidUsability(CapabilityUsabilityState state) {
    return state != CapabilityUsabilityState::Unknown && state != CapabilityUsabilityState::TestOnly;
}

static bool isValidEffectTarget(CapabilityEffectTarget target) {
    return target != CapabilityEffectTarget::Unknown && target != CapabilityEffectTarget::TestOnly;
}

static bool isValidEffectOp(EffectOperationType op) {
    return op != EffectOperationType::Unknown && op != EffectOperationType::TestOnly;
}

static Result<void> validateReasonKeys(const std::vector<std::string>& keys, const std::string& owner) {
    if (containsCivilizationForbiddenKey(keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

static Result<void> validateSafeString(const std::string& value, const std::string& field) {
    if (value == "capability_unlocked" || value.rfind("capability_unlocked:", 0) == 0) {
        return Result<void>::ok();
    }
    if (containsCivilizationForbiddenKey(value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " contains forbidden key"));
    }
    return Result<void>::ok();
}

static Result<void> validateSafeStrings(const std::vector<std::string>& values, const std::string& field) {
    if (containsCivilizationForbiddenKey(values)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " contains forbidden key"));
    }
    return Result<void>::ok();
}

static bool isSupportedConditionType(const std::string& condition_type) {
    static const std::set<std::string> supported = {
        "known_edible_count",
        "known_danger_count",
        "stable_tribe_knowledge_count",
        "teachable_knowledge_count",
        "coverage",
        "success_rate",
        "truce_success_count",
        "forced_retreat_low_loss_count",
        "standoff_controlled_count",
        "coordination_score",
        "intimidation_success_count",
        "retreat_success_count",
        "resource_defense_success_count",
        "stable_coordination_count",
        "misunderstanding_rate",
        "conflicted_knowledge_count",
        "open_conflict_pressure",
        "loss_pressure"
    };
    return supported.find(condition_type) != supported.end();
}

static Result<void> validateEventIdsUnique(const std::vector<EventId>& ids, const std::string& owner) {
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::id_missing, owner + " event_id missing"));
        }
        if (!isValidIdString(id.value())) {
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, owner + " event_id invalid"));
        }
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " event_id duplicated"));
        }
    }
    return Result<void>::ok();
}

static Result<void> validateKnowledgeIdsUnique(const std::vector<KnowledgeId>& ids, const std::string& owner) {
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::id_missing, owner + " knowledge_id missing"));
        }
        if (!isValidIdString(id.value())) {
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, owner + " knowledge_id invalid"));
        }
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " knowledge_id duplicated"));
        }
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Enum toString / fromString
// ---------------------------------------------------------------------------

std::string toString(CivilizationStage stage) {
    switch (stage) {
        case CivilizationStage::Unknown: return "unknown";
        case CivilizationStage::Awakening: return "awakening";
        case CivilizationStage::Foraging: return "foraging";
        case CivilizationStage::FireKeeping: return "fire_keeping";
        case CivilizationStage::ToolUsing: return "tool_using";
        case CivilizationStage::OrganizedTribe: return "organized_tribe";
        case CivilizationStage::SymbolicCulture: return "symbolic_culture";
        case CivilizationStage::ProtoCivilization: return "proto_civilization";
        case CivilizationStage::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CivilizationStage> civilizationStageFromString(const std::string& str) {
    if (str == "unknown") return Result<CivilizationStage>::ok(CivilizationStage::Unknown);
    if (str == "awakening") return Result<CivilizationStage>::ok(CivilizationStage::Awakening);
    if (str == "foraging") return Result<CivilizationStage>::ok(CivilizationStage::Foraging);
    if (str == "fire_keeping") return Result<CivilizationStage>::ok(CivilizationStage::FireKeeping);
    if (str == "tool_using") return Result<CivilizationStage>::ok(CivilizationStage::ToolUsing);
    if (str == "organized_tribe") return Result<CivilizationStage>::ok(CivilizationStage::OrganizedTribe);
    if (str == "symbolic_culture") return Result<CivilizationStage>::ok(CivilizationStage::SymbolicCulture);
    if (str == "proto_civilization") return Result<CivilizationStage>::ok(CivilizationStage::ProtoCivilization);
    if (str == "test_only") return Result<CivilizationStage>::ok(CivilizationStage::TestOnly);
    return Result<CivilizationStage>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CivilizationStage: " + str));
}

std::string toString(CapabilityType type) {
    switch (type) {
        case CapabilityType::Unknown: return "unknown";
        case CapabilityType::IdentifyEdible: return "identify_edible";
        case CapabilityType::IdentifyDanger: return "identify_danger";
        case CapabilityType::SafeForaging: return "safe_foraging";
        case CapabilityType::TeachingPractice: return "teaching_practice";
        case CapabilityType::BasicToolUse: return "basic_tool_use";
        case CapabilityType::FireKeeping: return "fire_keeping";
        case CapabilityType::FireDefense: return "fire_defense";
        case CapabilityType::OrganizedRetreat: return "organized_retreat";
        case CapabilityType::ConflictDeescalation: return "conflict_deescalation";
        case CapabilityType::ResourceDefense: return "resource_defense";
        case CapabilityType::TaskAssignment: return "task_assignment";
        case CapabilityType::LongTermMemory: return "long_term_memory";
        case CapabilityType::SymbolRecording: return "symbol_recording";
        case CapabilityType::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CapabilityType> capabilityTypeFromString(const std::string& str) {
    if (str == "unknown") return Result<CapabilityType>::ok(CapabilityType::Unknown);
    if (str == "identify_edible") return Result<CapabilityType>::ok(CapabilityType::IdentifyEdible);
    if (str == "identify_danger") return Result<CapabilityType>::ok(CapabilityType::IdentifyDanger);
    if (str == "safe_foraging") return Result<CapabilityType>::ok(CapabilityType::SafeForaging);
    if (str == "teaching_practice") return Result<CapabilityType>::ok(CapabilityType::TeachingPractice);
    if (str == "basic_tool_use") return Result<CapabilityType>::ok(CapabilityType::BasicToolUse);
    if (str == "fire_keeping") return Result<CapabilityType>::ok(CapabilityType::FireKeeping);
    if (str == "fire_defense") return Result<CapabilityType>::ok(CapabilityType::FireDefense);
    if (str == "organized_retreat") return Result<CapabilityType>::ok(CapabilityType::OrganizedRetreat);
    if (str == "conflict_deescalation") return Result<CapabilityType>::ok(CapabilityType::ConflictDeescalation);
    if (str == "resource_defense") return Result<CapabilityType>::ok(CapabilityType::ResourceDefense);
    if (str == "task_assignment") return Result<CapabilityType>::ok(CapabilityType::TaskAssignment);
    if (str == "long_term_memory") return Result<CapabilityType>::ok(CapabilityType::LongTermMemory);
    if (str == "symbol_recording") return Result<CapabilityType>::ok(CapabilityType::SymbolRecording);
    if (str == "test_only") return Result<CapabilityType>::ok(CapabilityType::TestOnly);
    return Result<CapabilityType>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CapabilityType: " + str));
}

std::string toString(CapabilityMaturityState state) {
    switch (state) {
        case CapabilityMaturityState::Unknown: return "unknown";
        case CapabilityMaturityState::Candidate: return "candidate";
        case CapabilityMaturityState::Emerging: return "emerging";
        case CapabilityMaturityState::Stable: return "stable";
        case CapabilityMaturityState::Institutionalized: return "institutionalized";
        case CapabilityMaturityState::Degraded: return "degraded";
        case CapabilityMaturityState::Lost: return "lost";
        case CapabilityMaturityState::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CapabilityMaturityState> capabilityMaturityStateFromString(const std::string& str) {
    if (str == "unknown") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Unknown);
    if (str == "candidate") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Candidate);
    if (str == "emerging") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Emerging);
    if (str == "stable") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Stable);
    if (str == "institutionalized") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Institutionalized);
    if (str == "degraded") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Degraded);
    if (str == "lost") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::Lost);
    if (str == "test_only") return Result<CapabilityMaturityState>::ok(CapabilityMaturityState::TestOnly);
    return Result<CapabilityMaturityState>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CapabilityMaturityState: " + str));
}

std::string toString(CapabilityUsabilityState state) {
    switch (state) {
        case CapabilityUsabilityState::Unknown: return "unknown";
        case CapabilityUsabilityState::Usable: return "usable";
        case CapabilityUsabilityState::BlockedByResource: return "blocked_by_resource";
        case CapabilityUsabilityState::BlockedByEnvironment: return "blocked_by_environment";
        case CapabilityUsabilityState::BlockedByCohesion: return "blocked_by_cohesion";
        case CapabilityUsabilityState::BlockedByConflict: return "blocked_by_conflict";
        case CapabilityUsabilityState::Suspended: return "suspended";
        case CapabilityUsabilityState::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CapabilityUsabilityState> capabilityUsabilityStateFromString(const std::string& str) {
    if (str == "unknown") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::Unknown);
    if (str == "usable") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::Usable);
    if (str == "blocked_by_resource") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::BlockedByResource);
    if (str == "blocked_by_environment") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::BlockedByEnvironment);
    if (str == "blocked_by_cohesion") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::BlockedByCohesion);
    if (str == "blocked_by_conflict") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::BlockedByConflict);
    if (str == "suspended") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::Suspended);
    if (str == "test_only") return Result<CapabilityUsabilityState>::ok(CapabilityUsabilityState::TestOnly);
    return Result<CapabilityUsabilityState>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CapabilityUsabilityState: " + str));
}

std::string toString(CapabilityChangeReason reason) {
    switch (reason) {
        case CapabilityChangeReason::Unknown: return "unknown";
        case CapabilityChangeReason::NewEvidence: return "new_evidence";
        case CapabilityChangeReason::RequirementMet: return "requirement_met";
        case CapabilityChangeReason::RequirementMissing: return "requirement_missing";
        case CapabilityChangeReason::RepeatedSuccess: return "repeated_success";
        case CapabilityChangeReason::RepeatedFailure: return "repeated_failure";
        case CapabilityChangeReason::PropagationImproved: return "propagation_improved";
        case CapabilityChangeReason::PropagationFailed: return "propagation_failed";
        case CapabilityChangeReason::ResourceChanged: return "resource_changed";
        case CapabilityChangeReason::PopulationChanged: return "population_changed";
        case CapabilityChangeReason::ConflictChanged: return "conflict_changed";
        case CapabilityChangeReason::MemoryDecayed: return "memory_decayed";
        case CapabilityChangeReason::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CapabilityChangeReason> capabilityChangeReasonFromString(const std::string& str) {
    if (str == "unknown") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::Unknown);
    if (str == "new_evidence") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::NewEvidence);
    if (str == "requirement_met") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::RequirementMet);
    if (str == "requirement_missing") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::RequirementMissing);
    if (str == "repeated_success") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::RepeatedSuccess);
    if (str == "repeated_failure") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::RepeatedFailure);
    if (str == "propagation_improved") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::PropagationImproved);
    if (str == "propagation_failed") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::PropagationFailed);
    if (str == "resource_changed") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::ResourceChanged);
    if (str == "population_changed") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::PopulationChanged);
    if (str == "conflict_changed") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::ConflictChanged);
    if (str == "memory_decayed") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::MemoryDecayed);
    if (str == "test_only") return Result<CapabilityChangeReason>::ok(CapabilityChangeReason::TestOnly);
    return Result<CapabilityChangeReason>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CapabilityChangeReason: " + str));
}

std::string toString(CapabilityEffectTarget target) {
    switch (target) {
        case CapabilityEffectTarget::Unknown: return "unknown";
        case CapabilityEffectTarget::ActionAvailability: return "action_availability";
        case CapabilityEffectTarget::Risk: return "risk";
        case CapabilityEffectTarget::Knowledge: return "knowledge";
        case CapabilityEffectTarget::Propagation: return "propagation";
        case CapabilityEffectTarget::TribeCohesion: return "tribe_cohesion";
        case CapabilityEffectTarget::CombatCoordination: return "combat_coordination";
        case CapabilityEffectTarget::ConflictResolution: return "conflict_resolution";
        case CapabilityEffectTarget::CivilizationStage: return "civilization_stage";
        case CapabilityEffectTarget::AiSignal: return "ai_signal";
        case CapabilityEffectTarget::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CapabilityEffectTarget> capabilityEffectTargetFromString(const std::string& str) {
    if (str == "unknown") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::Unknown);
    if (str == "action_availability") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::ActionAvailability);
    if (str == "risk") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::Risk);
    if (str == "knowledge") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::Knowledge);
    if (str == "propagation") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::Propagation);
    if (str == "tribe_cohesion") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::TribeCohesion);
    if (str == "combat_coordination") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::CombatCoordination);
    if (str == "conflict_resolution") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::ConflictResolution);
    if (str == "civilization_stage") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::CivilizationStage);
    if (str == "ai_signal") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::AiSignal);
    if (str == "test_only") return Result<CapabilityEffectTarget>::ok(CapabilityEffectTarget::TestOnly);
    return Result<CapabilityEffectTarget>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CapabilityEffectTarget: " + str));
}

std::string toString(EffectOperationType op) {
    switch (op) {
        case EffectOperationType::Unknown: return "unknown";
        case EffectOperationType::Add: return "add";
        case EffectOperationType::Multiply: return "multiply";
        case EffectOperationType::UnlockCandidate: return "unlock_candidate";
        case EffectOperationType::EnableAction: return "enable_action";
        case EffectOperationType::AddObservationFeature: return "add_observation_feature";
        case EffectOperationType::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<EffectOperationType> effectOperationTypeFromString(const std::string& str) {
    if (str == "unknown") return Result<EffectOperationType>::ok(EffectOperationType::Unknown);
    if (str == "add") return Result<EffectOperationType>::ok(EffectOperationType::Add);
    if (str == "multiply") return Result<EffectOperationType>::ok(EffectOperationType::Multiply);
    if (str == "unlock_candidate") return Result<EffectOperationType>::ok(EffectOperationType::UnlockCandidate);
    if (str == "enable_action") return Result<EffectOperationType>::ok(EffectOperationType::EnableAction);
    if (str == "add_observation_feature") return Result<EffectOperationType>::ok(EffectOperationType::AddObservationFeature);
    if (str == "test_only") return Result<EffectOperationType>::ok(EffectOperationType::TestOnly);
    return Result<EffectOperationType>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid EffectOperationType: " + str));
}

// ---------------------------------------------------------------------------
// Forbidden keys
// ---------------------------------------------------------------------------

bool containsCivilizationForbiddenKey(const std::string& key) {
    const std::string lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        // P25 inherited
        "hidden_truth",
        "real_effect",
        "true_trait",
        "object_truth",
        "raw_state",
        "gamestate",
        "game_state",
        "savegame",
        "save_game",
        "agentruntime",
        "agent_runtime",
        "policy",
        "random_split",
        "probability_split",
        "true_hp",
        "actual_hp",
        "hp_delta",
        "hp_remaining",
        "kill_count",
        "enemy_dead",
        "true_enemy_strength",
        "hidden_weapon",
        "enemy_killed",
        "damage_direct",
        "death_count",
        "loot_drop",
        // Civilization-specific
        "unlocked",
        "unlock_bool",
        "civilization_level",
        "stage_only_unlock",
        "frontend_unlock",
        "bool_unlocked",
        "direct_unlock",
        "direct_mutate_state"
    };
    for (const auto& token : forbidden) {
        if (lower.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsCivilizationForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
        return containsCivilizationForbiddenKey(key);
    });
}

// ---------------------------------------------------------------------------
// validateBasic
// ---------------------------------------------------------------------------

Result<void> CivilizationProgress::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{candidate_score, "CivilizationProgress candidate_score"},
                            {emerging_score, "CivilizationProgress emerging_score"},
                            {stable_score, "CivilizationProgress stable_score"},
                            {institutionalized_score, "CivilizationProgress institutionalized_score"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    if (success_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationProgress success_count negative"));
    if (failure_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationProgress failure_count negative"));
    return validateReasonKeys(reason_keys, "CivilizationProgress");
}

Result<void> CapabilityUnlockCondition::validateBasic() const {
    if (condition_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityUnlockCondition condition_key empty"));
    if (containsCivilizationForbiddenKey(condition_key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityUnlockCondition condition_key forbidden"));
    if (condition_type.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityUnlockCondition condition_type empty"));
    auto type_safe_result = validateSafeString(condition_type, "CapabilityUnlockCondition condition_type");
    if (type_safe_result.is_error()) return type_safe_result;
    if (!isSupportedConditionType(condition_type)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityUnlockCondition condition_type unsupported"));
    if (required_score < 0.0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CapabilityUnlockCondition required_score negative"));
    // required_score is a threshold (can be >1 for count-based conditions)
    auto cur_result = validateRatio(current_score, "CapabilityUnlockCondition current_score");
    if (cur_result.is_error()) return cur_result;
    return validateReasonKeys(reason_keys, "CapabilityUnlockCondition");
}

Result<void> CapabilityEvidenceBundle::validateBasic() const {
    auto k_result = validateKnowledgeIdsUnique(knowledge_ids, "CapabilityEvidenceBundle");
    if (k_result.is_error()) return k_result;
    for (const auto& ids : {propagation_event_ids, practice_event_ids, conflict_event_ids}) {
        auto e_result = validateEventIdsUnique(ids, "CapabilityEvidenceBundle");
        if (e_result.is_error()) return e_result;
    }
    auto conf_result = validateRatio(confidence, "CapabilityEvidenceBundle confidence");
    if (conf_result.is_error()) return conf_result;
    return validateReasonKeys(reason_keys, "CapabilityEvidenceBundle");
}

Result<void> CapabilityEffectDefinition::validateBasic() const {
    if (effect_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDefinition effect_key empty"));
    if (containsCivilizationForbiddenKey(effect_key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDefinition effect_key forbidden"));
    if (!isValidEffectTarget(target)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CapabilityEffectDefinition target invalid"));
    if (!isValidEffectOp(operation)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CapabilityEffectDefinition operation invalid"));
    if (value_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDefinition value_key empty"));
    if (containsCivilizationForbiddenKey(value_key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDefinition value_key forbidden"));
    return validateReasonKeys(reason_keys, "CapabilityEffectDefinition");
}

Result<void> CivilizationCapability::validateBasic() const {
    if (!isValidCapabilityType(capability_type)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationCapability capability_type invalid"));
    if (display_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapability display_key empty"));
    if (containsCivilizationForbiddenKey(display_key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapability display_key forbidden"));
    if (domain_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapability domain_key empty"));
    if (containsCivilizationForbiddenKey(domain_key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapability domain_key forbidden"));
    if (required_conditions.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapability required_conditions empty"));
    for (const auto& cond : required_conditions) {
        auto result = cond.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& eff : effect_definitions) {
        auto result = eff.validateBasic();
        if (result.is_error()) return result;
    }
    return validateReasonKeys(reason_keys, "CivilizationCapability");
}

Result<void> CivilizationCapabilityState::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationCapabilityState tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationCapabilityState tribe_id invalid"));
    if (!isValidCapabilityType(capability_type)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationCapabilityState capability_type invalid"));
    if (!isValidMaturity(maturity)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationCapabilityState maturity invalid"));
    if (!isValidUsability(usability)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationCapabilityState usability invalid"));

    if (maturity == CapabilityMaturityState::Stable || maturity == CapabilityMaturityState::Institutionalized) {
        if (evidence.knowledge_ids.empty() && evidence.propagation_event_ids.empty() && evidence.practice_event_ids.empty() && evidence.conflict_event_ids.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapabilityState Stable/Institutionalized must have evidence"));
        }
    }
    if (maturity == CapabilityMaturityState::Lost) {
        if (!active_effect_keys.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapabilityState Lost must not have active_effect_keys"));
        }
    }
    if (usability != CapabilityUsabilityState::Usable) {
        if (blocked_reasons.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationCapabilityState non-Usable must have blocked_reasons"));
        }
    }

    auto prog_result = progress.validateBasic();
    if (prog_result.is_error()) return prog_result;
    auto stab_result = validateRatio(stability, "CivilizationCapabilityState stability");
    if (stab_result.is_error()) return stab_result;
    auto cov_result = validateRatio(coverage, "CivilizationCapabilityState coverage");
    if (cov_result.is_error()) return cov_result;
    auto ev_result = evidence.validateBasic();
    if (ev_result.is_error()) return ev_result;
    auto active_effects_result = validateSafeStrings(active_effect_keys, "CivilizationCapabilityState active_effect_keys");
    if (active_effects_result.is_error()) return active_effects_result;
    return validateReasonKeys(reason_keys, "CivilizationCapabilityState");
}

Result<void> CapabilityEffectDraft::validateBasic() const {
    if (draft_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CapabilityEffectDraft draft_id missing"));
    if (!isValidIdString(draft_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CapabilityEffectDraft draft_id invalid"));
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CapabilityEffectDraft tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CapabilityEffectDraft tribe_id invalid"));
    if (!isValidCapabilityType(capability_type)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CapabilityEffectDraft capability_type invalid"));
    if (effect_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDraft effect_key empty"));
    auto effect_key_result = validateSafeString(effect_key, "CapabilityEffectDraft effect_key");
    if (effect_key_result.is_error()) return effect_key_result;
    if (!isValidEffectTarget(target)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CapabilityEffectDraft target invalid"));
    if (!isValidEffectOp(operation)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CapabilityEffectDraft operation invalid"));
    if (value_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CapabilityEffectDraft value_key empty"));
    auto value_key_result = validateSafeString(value_key, "CapabilityEffectDraft value_key");
    if (value_key_result.is_error()) return value_key_result;
    auto str_result = validateRatio(strength, "CapabilityEffectDraft strength");
    if (str_result.is_error()) return str_result;
    auto ev_result = validateEventIdsUnique(source_evidence_ids, "CapabilityEffectDraft");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CapabilityEffectDraft");
}

Result<void> CivilizationState::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationState tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationState tribe_id invalid"));
    if (!isValidStage(stage)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationState stage invalid"));
    auto conf_result = validateRatio(stage_confidence, "CivilizationState stage_confidence");
    if (conf_result.is_error()) return conf_result;

    std::set<std::string> seen_types;
    for (const auto& cap : capabilities) {
        auto result = cap.validateBasic();
        if (result.is_error()) return result;
        auto type_str = toString(cap.capability_type);
        if (!seen_types.insert(type_str).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationState duplicate capability_type: " + type_str));
        }
    }
    for (const auto& eff : active_effects) {
        auto result = eff.validateBasic();
        if (result.is_error()) return result;
    }
    return validateReasonKeys(reason_keys, "CivilizationState");
}

Result<void> CapabilityRequirementSnapshot::validateBasic() const {
    auto score_result = validateRatio(total_score, "CapabilityRequirementSnapshot total_score");
    if (score_result.is_error()) return score_result;
    return validateReasonKeys(reason_keys, "CapabilityRequirementSnapshot");
}

Result<void> CivilizationCapabilityCandidate::validateBasic() const {
    if (candidate_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationCapabilityCandidate candidate_id missing"));
    if (!isValidIdString(candidate_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationCapabilityCandidate candidate_id invalid"));
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationCapabilityCandidate tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationCapabilityCandidate tribe_id invalid"));
    if (!isValidCapabilityType(capability_type)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationCapabilityCandidate capability_type invalid"));
    auto read_result = validateRatio(readiness, "CivilizationCapabilityCandidate readiness");
    if (read_result.is_error()) return read_result;
    auto snap_result = requirement_snapshot.validateBasic();
    if (snap_result.is_error()) return snap_result;
    auto ev_result = evidence.validateBasic();
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationCapabilityCandidate");
}

Result<void> CivilizationStateChangeDraft::validateBasic() const {
    if (change_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationStateChangeDraft change_id missing"));
    if (!isValidIdString(change_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationStateChangeDraft change_id invalid"));
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationStateChangeDraft tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationStateChangeDraft tribe_id invalid"));
    if (deterministic_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationStateChangeDraft deterministic_key empty"));
    auto deterministic_key_result = validateSafeString(deterministic_key, "CivilizationStateChangeDraft deterministic_key");
    if (deterministic_key_result.is_error()) return deterministic_key_result;
    auto ev_result = validateEventIdsUnique(evidence_ids, "CivilizationStateChangeDraft");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationStateChangeDraft");
}

Result<void> CivilizationProjection::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationProjection tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationProjection tribe_id invalid"));
    if (!isValidStage(stage)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CivilizationProjection stage invalid"));
    for (const auto& item : {std::pair<const std::string*, const char*>{&stage_label_key, "CivilizationProjection stage_label_key"},
                            {&explanation_key, "CivilizationProjection explanation_key"}}) {
        auto result = validateSafeString(*item.first, item.second);
        if (result.is_error()) return result;
    }
    for (const auto& item : {std::pair<const std::vector<std::string>*, const char*>{&active_effect_summary_keys, "CivilizationProjection active_effect_summary_keys"},
                            {&risk_warning_keys, "CivilizationProjection risk_warning_keys"}}) {
        auto result = validateSafeStrings(*item.first, item.second);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> CivilizationTrace::validateBasic() const {
    if (trace_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationTrace trace_id missing"));
    if (!isValidIdString(trace_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationTrace trace_id invalid"));
    for (const auto& item : {std::pair<const std::vector<std::string>*, const char*>{&input_summary_keys, "CivilizationTrace input_summary_keys"},
                            {&candidate_steps, "CivilizationTrace candidate_steps"},
                            {&requirement_steps, "CivilizationTrace requirement_steps"},
                            {&state_transition_steps, "CivilizationTrace state_transition_steps"},
                            {&effect_steps, "CivilizationTrace effect_steps"},
                            {&stage_steps, "CivilizationTrace stage_steps"},
                            {&rejected_unsafe_keys, "CivilizationTrace rejected_unsafe_keys"}}) {
        auto result = validateSafeStrings(*item.first, item.second);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> CivilizationEventDraft::validateBasic() const {
    if (event_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationEventDraft event_id missing"));
    if (!isValidIdString(event_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationEventDraft event_id invalid"));
    if (event_type_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "CivilizationEventDraft event_type_key empty"));
    auto event_type_result = validateSafeString(event_type_key, "CivilizationEventDraft event_type_key");
    if (event_type_result.is_error()) return event_type_result;
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationEventDraft tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationEventDraft tribe_id invalid"));
    auto message_result = validateSafeString(message_key, "CivilizationEventDraft message_key");
    if (message_result.is_error()) return message_result;
    auto ev_result = validateEventIdsUnique(evidence_ids, "CivilizationEventDraft");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationEventDraft");
}

// ---------------------------------------------------------------------------
// Security summary validateBasic
// ---------------------------------------------------------------------------

Result<void> CivilizationKnowledgeSummary::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationKnowledgeSummary tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationKnowledgeSummary tribe_id invalid"));
    if (known_edible_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationKnowledgeSummary known_edible_count negative"));
    if (known_danger_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationKnowledgeSummary known_danger_count negative"));
    if (stable_tribe_knowledge_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationKnowledgeSummary stable_tribe_knowledge_count negative"));
    if (conflicted_knowledge_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationKnowledgeSummary conflicted_knowledge_count negative"));
    if (teachable_knowledge_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationKnowledgeSummary teachable_knowledge_count negative"));
    auto k_result = validateKnowledgeIdsUnique(knowledge_ids, "CivilizationKnowledgeSummary");
    if (k_result.is_error()) return k_result;
    return validateReasonKeys(reason_keys, "CivilizationKnowledgeSummary");
}

Result<void> CivilizationPropagationSummary::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationPropagationSummary tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationPropagationSummary tribe_id invalid"));
    for (const auto& item : {std::pair<double, const char*>{coverage, "CivilizationPropagationSummary coverage"},
                            {success_rate, "CivilizationPropagationSummary success_rate"},
                            {misunderstanding_rate, "CivilizationPropagationSummary misunderstanding_rate"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    auto ev_result = validateEventIdsUnique(teaching_event_ids, "CivilizationPropagationSummary");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationPropagationSummary");
}

Result<void> CivilizationCoordinationSummary::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationCoordinationSummary tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationCoordinationSummary tribe_id invalid"));
    auto coord_result = validateRatio(coordination_score, "CivilizationCoordinationSummary coordination_score");
    if (coord_result.is_error()) return coord_result;
    if (stable_coordination_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationCoordinationSummary stable_coordination_count negative"));
    if (retreat_success_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationCoordinationSummary retreat_success_count negative"));
    if (resource_defense_success_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationCoordinationSummary resource_defense_success_count negative"));
    auto ev_result = validateEventIdsUnique(source_event_ids, "CivilizationCoordinationSummary");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationCoordinationSummary");
}

Result<void> CivilizationConflictSummary::validateBasic() const {
    if (tribe_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CivilizationConflictSummary tribe_id missing"));
    if (!isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CivilizationConflictSummary tribe_id invalid"));
    if (intimidation_success_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationConflictSummary intimidation_success_count negative"));
    if (forced_retreat_low_loss_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationConflictSummary forced_retreat_low_loss_count negative"));
    if (truce_success_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationConflictSummary truce_success_count negative"));
    if (standoff_controlled_count < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "CivilizationConflictSummary standoff_controlled_count negative"));
    for (const auto& item : {std::pair<double, const char*>{open_conflict_pressure, "CivilizationConflictSummary open_conflict_pressure"},
                            {loss_pressure, "CivilizationConflictSummary loss_pressure"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    auto ev_result = validateEventIdsUnique(source_event_ids, "CivilizationConflictSummary");
    if (ev_result.is_error()) return ev_result;
    return validateReasonKeys(reason_keys, "CivilizationConflictSummary");
}

// ---------------------------------------------------------------------------
// Input / Result validateBasic
// ---------------------------------------------------------------------------

Result<void> CivilizationResolveInput::validateBasic() const {
    auto state_result = current_state.validateBasic();
    if (state_result.is_error()) return state_result;
    auto ks_result = knowledge_summary.validateBasic();
    if (ks_result.is_error()) return ks_result;
    auto ps_result = propagation_summary.validateBasic();
    if (ps_result.is_error()) return ps_result;
    auto cs_result = coordination_summary.validateBasic();
    if (cs_result.is_error()) return cs_result;
    auto cfs_result = conflict_summary.validateBasic();
    if (cfs_result.is_error()) return cfs_result;
    for (const auto& def : capability_definitions) {
        auto result = def.validateBasic();
        if (result.is_error()) return result;
    }
    return validateReasonKeys(reason_keys, "CivilizationResolveInput");
}

Result<void> CivilizationResolveResult::validateBasic() const {
    if (!ok) return Result<void>::ok();
    auto state_result = updated_state.validateBasic();
    if (state_result.is_error()) return state_result;
    for (const auto& c : candidates) {
        auto result = c.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& sc : state_changes) {
        auto result = sc.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& ed : effect_drafts) {
        auto result = ed.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& ev : event_drafts) {
        auto result = ev.validateBasic();
        if (result.is_error()) return result;
    }
    auto proj_result = projection.validateBasic();
    if (proj_result.is_error()) return proj_result;
    auto trace_result = trace.validateBasic();
    if (trace_result.is_error()) return trace_result;
    return Result<void>::ok();
}

} // namespace pathfinder::civilization
