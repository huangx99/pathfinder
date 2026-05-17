#include "pathfinder/memory/types.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// Hidden Truth Guard
// ============================================================

static const std::vector<std::string> kMemoryForbiddenKeys = {
    "ObjectDefinition hidden",
    "edible_profile",
    "hunger_delta",
    "health_delta",
    "effect_kind",
    "GameState",
    "AgentTickRecord",
    "reward_value",
    "done =",
    "is_done",
    "SaveGame",
    "SaveManager",
    "raw_state",
    "hidden_truth"
};

std::vector<std::string> memoryForbiddenKeys() {
    return kMemoryForbiddenKeys;
}

static std::string toLower(const std::string& text) {
    std::string lower;
    lower.reserve(text.size());
    for (unsigned char c : text) {
        lower.push_back(static_cast<char>(std::tolower(c)));
    }
    return lower;
}

bool containsMemoryForbiddenKey(const std::string& text) {
    std::string lower = toLower(text);
    for (const auto& key : kMemoryForbiddenKeys) {
        if (lower.find(toLower(key)) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsMemoryForbiddenKey(const std::vector<std::string>& texts) {
    for (const auto& text : texts) {
        if (containsMemoryForbiddenKey(text)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// MemoryOwnerKind
// ============================================================

std::string toString(MemoryOwnerKind kind) {
    switch (kind) {
        case MemoryOwnerKind::Unknown: return "unknown";
        case MemoryOwnerKind::Actor: return "actor";
        case MemoryOwnerKind::Agent: return "agent";
        case MemoryOwnerKind::Group: return "group";
        case MemoryOwnerKind::Tribe: return "tribe";
        case MemoryOwnerKind::Civilization: return "civilization";
        case MemoryOwnerKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemoryOwnerKind> memoryOwnerKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Unknown);
    if (str == "actor") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Actor);
    if (str == "agent") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Agent);
    if (str == "group") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Group);
    if (str == "tribe") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Tribe);
    if (str == "civilization") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Civilization);
    if (str == "test_only") return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::TestOnly);
    return Result<MemoryOwnerKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryOwnerKind: " + str));
}

// ============================================================
// MemorySubjectKind
// ============================================================

std::string toString(MemorySubjectKind kind) {
    switch (kind) {
        case MemorySubjectKind::Unknown: return "unknown";
        case MemorySubjectKind::ObjectDefinition: return "object_definition";
        case MemorySubjectKind::WorldObject: return "world_object";
        case MemorySubjectKind::ObjectCategory: return "object_category";
        case MemorySubjectKind::AgentSpecies: return "agent_species";
        case MemorySubjectKind::EnvironmentFeature: return "environment_feature";
        case MemorySubjectKind::ActionOutcome: return "action_outcome";
        case MemorySubjectKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemorySubjectKind> memorySubjectKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemorySubjectKind>::ok(MemorySubjectKind::Unknown);
    if (str == "object_definition") return Result<MemorySubjectKind>::ok(MemorySubjectKind::ObjectDefinition);
    if (str == "world_object") return Result<MemorySubjectKind>::ok(MemorySubjectKind::WorldObject);
    if (str == "object_category") return Result<MemorySubjectKind>::ok(MemorySubjectKind::ObjectCategory);
    if (str == "agent_species") return Result<MemorySubjectKind>::ok(MemorySubjectKind::AgentSpecies);
    if (str == "environment_feature") return Result<MemorySubjectKind>::ok(MemorySubjectKind::EnvironmentFeature);
    if (str == "action_outcome") return Result<MemorySubjectKind>::ok(MemorySubjectKind::ActionOutcome);
    if (str == "test_only") return Result<MemorySubjectKind>::ok(MemorySubjectKind::TestOnly);
    return Result<MemorySubjectKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemorySubjectKind: " + str));
}

// ============================================================
// MemoryKind
// ============================================================

std::string toString(MemoryKind kind) {
    switch (kind) {
        case MemoryKind::Unknown: return "unknown";
        case MemoryKind::Experiment: return "experiment";
        case MemoryKind::Success: return "success";
        case MemoryKind::Failure: return "failure";
        case MemoryKind::Warning: return "warning";
        case MemoryKind::Accident: return "accident";
        case MemoryKind::Discovery: return "discovery";
        case MemoryKind::Contradiction: return "contradiction";
        case MemoryKind::TeachingRelated: return "teaching_related";
        case MemoryKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemoryKind> memoryKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryKind>::ok(MemoryKind::Unknown);
    if (str == "experiment") return Result<MemoryKind>::ok(MemoryKind::Experiment);
    if (str == "success") return Result<MemoryKind>::ok(MemoryKind::Success);
    if (str == "failure") return Result<MemoryKind>::ok(MemoryKind::Failure);
    if (str == "warning") return Result<MemoryKind>::ok(MemoryKind::Warning);
    if (str == "accident") return Result<MemoryKind>::ok(MemoryKind::Accident);
    if (str == "discovery") return Result<MemoryKind>::ok(MemoryKind::Discovery);
    if (str == "contradiction") return Result<MemoryKind>::ok(MemoryKind::Contradiction);
    if (str == "teaching_related") return Result<MemoryKind>::ok(MemoryKind::TeachingRelated);
    if (str == "test_only") return Result<MemoryKind>::ok(MemoryKind::TestOnly);
    return Result<MemoryKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryKind: " + str));
}

// ============================================================
// MemoryScope
// ============================================================

std::string toString(MemoryScope scope) {
    switch (scope) {
        case MemoryScope::Unknown: return "unknown";
        case MemoryScope::ShortTerm: return "short_term";
        case MemoryScope::MidTerm: return "mid_term";
        case MemoryScope::LongTerm: return "long_term";
    }
    return "unknown";
}

Result<MemoryScope> memoryScopeFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryScope>::ok(MemoryScope::Unknown);
    if (str == "short_term") return Result<MemoryScope>::ok(MemoryScope::ShortTerm);
    if (str == "mid_term") return Result<MemoryScope>::ok(MemoryScope::MidTerm);
    if (str == "long_term") return Result<MemoryScope>::ok(MemoryScope::LongTerm);
    return Result<MemoryScope>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryScope: " + str));
}

// ============================================================
// MemoryLifecycle
// ============================================================

std::string toString(MemoryLifecycle lifecycle) {
    switch (lifecycle) {
        case MemoryLifecycle::Unknown: return "unknown";
        case MemoryLifecycle::Active: return "active";
        case MemoryLifecycle::Fading: return "fading";
        case MemoryLifecycle::Forgotten: return "forgotten";
        case MemoryLifecycle::Protected: return "protected";
    }
    return "unknown";
}

Result<MemoryLifecycle> memoryLifecycleFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryLifecycle>::ok(MemoryLifecycle::Unknown);
    if (str == "active") return Result<MemoryLifecycle>::ok(MemoryLifecycle::Active);
    if (str == "fading") return Result<MemoryLifecycle>::ok(MemoryLifecycle::Fading);
    if (str == "forgotten") return Result<MemoryLifecycle>::ok(MemoryLifecycle::Forgotten);
    if (str == "protected") return Result<MemoryLifecycle>::ok(MemoryLifecycle::Protected);
    return Result<MemoryLifecycle>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryLifecycle: " + str));
}

// ============================================================
// MemoryStrengthBand
// ============================================================

std::string toString(MemoryStrengthBand band) {
    switch (band) {
        case MemoryStrengthBand::Unknown: return "unknown";
        case MemoryStrengthBand::Trace: return "trace";
        case MemoryStrengthBand::Weak: return "weak";
        case MemoryStrengthBand::Stable: return "stable";
        case MemoryStrengthBand::Strong: return "strong";
        case MemoryStrengthBand::Core: return "core";
    }
    return "unknown";
}

Result<MemoryStrengthBand> memoryStrengthBandFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Unknown);
    if (str == "trace") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Trace);
    if (str == "weak") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Weak);
    if (str == "stable") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Stable);
    if (str == "strong") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Strong);
    if (str == "core") return Result<MemoryStrengthBand>::ok(MemoryStrengthBand::Core);
    return Result<MemoryStrengthBand>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryStrengthBand: " + str));
}

// ============================================================
// MemoryImportance
// ============================================================

std::string toString(MemoryImportance importance) {
    switch (importance) {
        case MemoryImportance::Unknown: return "unknown";
        case MemoryImportance::Trivial: return "trivial";
        case MemoryImportance::Normal: return "normal";
        case MemoryImportance::Important: return "important";
        case MemoryImportance::Critical: return "critical";
    }
    return "unknown";
}

Result<MemoryImportance> memoryImportanceFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryImportance>::ok(MemoryImportance::Unknown);
    if (str == "trivial") return Result<MemoryImportance>::ok(MemoryImportance::Trivial);
    if (str == "normal") return Result<MemoryImportance>::ok(MemoryImportance::Normal);
    if (str == "important") return Result<MemoryImportance>::ok(MemoryImportance::Important);
    if (str == "critical") return Result<MemoryImportance>::ok(MemoryImportance::Critical);
    return Result<MemoryImportance>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryImportance: " + str));
}

// ============================================================
// MemorySourceKind
// ============================================================

std::string toString(MemorySourceKind kind) {
    switch (kind) {
        case MemorySourceKind::Unknown: return "unknown";
        case MemorySourceKind::CognitionEvidence: return "cognition_evidence";
        case MemorySourceKind::CognitionUpdate: return "cognition_update";
        case MemorySourceKind::DirectEvent: return "direct_event";
        case MemorySourceKind::ImportedSafeProjection: return "imported_safe_projection";
        case MemorySourceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemorySourceKind> memorySourceKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemorySourceKind>::ok(MemorySourceKind::Unknown);
    if (str == "cognition_evidence") return Result<MemorySourceKind>::ok(MemorySourceKind::CognitionEvidence);
    if (str == "cognition_update") return Result<MemorySourceKind>::ok(MemorySourceKind::CognitionUpdate);
    if (str == "direct_event") return Result<MemorySourceKind>::ok(MemorySourceKind::DirectEvent);
    if (str == "imported_safe_projection") return Result<MemorySourceKind>::ok(MemorySourceKind::ImportedSafeProjection);
    if (str == "test_only") return Result<MemorySourceKind>::ok(MemorySourceKind::TestOnly);
    return Result<MemorySourceKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemorySourceKind: " + str));
}

// ============================================================
// MemoryWriteDecision
// ============================================================

std::string toString(MemoryWriteDecision decision) {
    switch (decision) {
        case MemoryWriteDecision::Unknown: return "unknown";
        case MemoryWriteDecision::Created: return "created";
        case MemoryWriteDecision::Reinforced: return "reinforced";
        case MemoryWriteDecision::Promoted: return "promoted";
        case MemoryWriteDecision::Weakened: return "weakened";
        case MemoryWriteDecision::Ignored: return "ignored";
        case MemoryWriteDecision::Rejected: return "rejected";
    }
    return "unknown";
}

Result<MemoryWriteDecision> memoryWriteDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Unknown);
    if (str == "created") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Created);
    if (str == "reinforced") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Reinforced);
    if (str == "promoted") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Promoted);
    if (str == "weakened") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Weakened);
    if (str == "ignored") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Ignored);
    if (str == "rejected") return Result<MemoryWriteDecision>::ok(MemoryWriteDecision::Rejected);
    return Result<MemoryWriteDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryWriteDecision: " + str));
}

// ============================================================
// MemoryDecayDecision
// ============================================================

std::string toString(MemoryDecayDecision decision) {
    switch (decision) {
        case MemoryDecayDecision::Unknown: return "unknown";
        case MemoryDecayDecision::Unchanged: return "unchanged";
        case MemoryDecayDecision::Decayed: return "decayed";
        case MemoryDecayDecision::Faded: return "faded";
        case MemoryDecayDecision::Forgotten: return "forgotten";
        case MemoryDecayDecision::ProtectedSkipped: return "protected_skipped";
    }
    return "unknown";
}

Result<MemoryDecayDecision> memoryDecayDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::Unknown);
    if (str == "unchanged") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::Unchanged);
    if (str == "decayed") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::Decayed);
    if (str == "faded") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::Faded);
    if (str == "forgotten") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::Forgotten);
    if (str == "protected_skipped") return Result<MemoryDecayDecision>::ok(MemoryDecayDecision::ProtectedSkipped);
    return Result<MemoryDecayDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryDecayDecision: " + str));
}

// ============================================================
// MemoryRejectReason
// ============================================================

std::string toString(MemoryRejectReason reason) {
    switch (reason) {
        case MemoryRejectReason::Unknown: return "unknown";
        case MemoryRejectReason::EmptyMemoryId: return "empty_memory_id";
        case MemoryRejectReason::EmptyOwner: return "empty_owner";
        case MemoryRejectReason::EmptySubject: return "empty_subject";
        case MemoryRejectReason::EmptyCandidateId: return "empty_candidate_id";
        case MemoryRejectReason::EmptySource: return "empty_source";
        case MemoryRejectReason::UnknownOwnerKind: return "unknown_owner_kind";
        case MemoryRejectReason::UnknownSubjectKind: return "unknown_subject_kind";
        case MemoryRejectReason::UnknownMemoryKind: return "unknown_memory_kind";
        case MemoryRejectReason::UnknownScope: return "unknown_scope";
        case MemoryRejectReason::UnknownLifecycle: return "unknown_lifecycle";
        case MemoryRejectReason::StrengthOutOfRange: return "strength_out_of_range";
        case MemoryRejectReason::ImportanceUnknown: return "importance_unknown";
        case MemoryRejectReason::MissingSourceEvent: return "missing_source_event";
        case MemoryRejectReason::HiddenTruthDetected: return "hidden_truth_detected";
        case MemoryRejectReason::RawStateDetected: return "raw_state_detected";
        case MemoryRejectReason::TooManyRecords: return "too_many_records";
        case MemoryRejectReason::SchemaVersionUnsupported: return "schema_version_unsupported";
    }
    return "unknown";
}

Result<MemoryRejectReason> memoryRejectReasonFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryRejectReason>::ok(MemoryRejectReason::Unknown);
    if (str == "empty_memory_id") return Result<MemoryRejectReason>::ok(MemoryRejectReason::EmptyMemoryId);
    if (str == "empty_owner") return Result<MemoryRejectReason>::ok(MemoryRejectReason::EmptyOwner);
    if (str == "empty_subject") return Result<MemoryRejectReason>::ok(MemoryRejectReason::EmptySubject);
    if (str == "empty_candidate_id") return Result<MemoryRejectReason>::ok(MemoryRejectReason::EmptyCandidateId);
    if (str == "empty_source") return Result<MemoryRejectReason>::ok(MemoryRejectReason::EmptySource);
    if (str == "unknown_owner_kind") return Result<MemoryRejectReason>::ok(MemoryRejectReason::UnknownOwnerKind);
    if (str == "unknown_subject_kind") return Result<MemoryRejectReason>::ok(MemoryRejectReason::UnknownSubjectKind);
    if (str == "unknown_memory_kind") return Result<MemoryRejectReason>::ok(MemoryRejectReason::UnknownMemoryKind);
    if (str == "unknown_scope") return Result<MemoryRejectReason>::ok(MemoryRejectReason::UnknownScope);
    if (str == "unknown_lifecycle") return Result<MemoryRejectReason>::ok(MemoryRejectReason::UnknownLifecycle);
    if (str == "strength_out_of_range") return Result<MemoryRejectReason>::ok(MemoryRejectReason::StrengthOutOfRange);
    if (str == "importance_unknown") return Result<MemoryRejectReason>::ok(MemoryRejectReason::ImportanceUnknown);
    if (str == "missing_source_event") return Result<MemoryRejectReason>::ok(MemoryRejectReason::MissingSourceEvent);
    if (str == "hidden_truth_detected") return Result<MemoryRejectReason>::ok(MemoryRejectReason::HiddenTruthDetected);
    if (str == "raw_state_detected") return Result<MemoryRejectReason>::ok(MemoryRejectReason::RawStateDetected);
    if (str == "too_many_records") return Result<MemoryRejectReason>::ok(MemoryRejectReason::TooManyRecords);
    if (str == "schema_version_unsupported") return Result<MemoryRejectReason>::ok(MemoryRejectReason::SchemaVersionUnsupported);
    return Result<MemoryRejectReason>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryRejectReason: " + str));
}

// ============================================================
// Helpers
// ============================================================

MemoryStrengthBand strengthToBand(double strength) {
    if (strength < 0.01) return MemoryStrengthBand::Unknown;
    if (strength < 0.20) return MemoryStrengthBand::Trace;
    if (strength < 0.45) return MemoryStrengthBand::Weak;
    if (strength < 0.70) return MemoryStrengthBand::Stable;
    if (strength < 0.90) return MemoryStrengthBand::Strong;
    return MemoryStrengthBand::Core;
}

double importanceToInitialStrength(MemoryImportance importance) {
    switch (importance) {
        case MemoryImportance::Trivial: return 0.15;
        case MemoryImportance::Normal: return 0.35;
        case MemoryImportance::Important: return 0.55;
        case MemoryImportance::Critical: return 0.80;
        default: return 0.0;
    }
}

double importanceToReinforceGain(MemoryImportance importance) {
    switch (importance) {
        case MemoryImportance::Trivial: return 0.05;
        case MemoryImportance::Normal: return 0.15;
        case MemoryImportance::Important: return 0.25;
        case MemoryImportance::Critical: return 0.40;
        default: return 0.0;
    }
}

} // namespace pathfinder::memory
