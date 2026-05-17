#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/foundation/id.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// KnowledgeOwnerKind
// ============================================================

std::string toString(KnowledgeOwnerKind kind) {
    switch (kind) {
        case KnowledgeOwnerKind::Unknown: return "unknown";
        case KnowledgeOwnerKind::Agent: return "agent";
        case KnowledgeOwnerKind::Actor: return "actor";
        case KnowledgeOwnerKind::Tribe: return "tribe";
        case KnowledgeOwnerKind::Region: return "region";
        case KnowledgeOwnerKind::Civilization: return "civilization";
        case KnowledgeOwnerKind::ExternalRecord: return "external_record";
        case KnowledgeOwnerKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeOwnerKind> knowledgeOwnerKindFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Unknown);
    if (str == "agent") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Agent);
    if (str == "actor") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Actor);
    if (str == "tribe") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Tribe);
    if (str == "region") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Region);
    if (str == "civilization") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::Civilization);
    if (str == "external_record") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::ExternalRecord);
    if (str == "test_only") return Result<KnowledgeOwnerKind>::ok(KnowledgeOwnerKind::TestOnly);
    return Result<KnowledgeOwnerKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeOwnerKind: " + str));
}

// ============================================================
// KnowledgeSubjectKind
// ============================================================

std::string toString(KnowledgeSubjectKind kind) {
    switch (kind) {
        case KnowledgeSubjectKind::Unknown: return "unknown";
        case KnowledgeSubjectKind::ObjectDefinition: return "object_definition";
        case KnowledgeSubjectKind::ObjectInstance: return "object_instance";
        case KnowledgeSubjectKind::Trait: return "trait";
        case KnowledgeSubjectKind::Region: return "region";
        case KnowledgeSubjectKind::Action: return "action";
        case KnowledgeSubjectKind::Combination: return "combination";
        case KnowledgeSubjectKind::Agent: return "agent";
        case KnowledgeSubjectKind::Tribe: return "tribe";
        case KnowledgeSubjectKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeSubjectKind> knowledgeSubjectKindFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Unknown);
    if (str == "object_definition") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::ObjectDefinition);
    if (str == "object_instance") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::ObjectInstance);
    if (str == "trait") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Trait);
    if (str == "region") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Region);
    if (str == "action") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Action);
    if (str == "combination") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Combination);
    if (str == "agent") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Agent);
    if (str == "tribe") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::Tribe);
    if (str == "test_only") return Result<KnowledgeSubjectKind>::ok(KnowledgeSubjectKind::TestOnly);
    return Result<KnowledgeSubjectKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeSubjectKind: " + str));
}

// ============================================================
// KnowledgeRelationType
// ============================================================

std::string toString(KnowledgeRelationType type) {
    switch (type) {
        case KnowledgeRelationType::Unknown: return "unknown";
        case KnowledgeRelationType::HasEffect: return "has_effect";
        case KnowledgeRelationType::HasRisk: return "has_risk";
        case KnowledgeRelationType::IsEdibleUnder: return "is_edible_under";
        case KnowledgeRelationType::IsDangerousUnder: return "is_dangerous_under";
        case KnowledgeRelationType::IsUsableFor: return "is_usable_for";
        case KnowledgeRelationType::ReactsWith: return "reacts_with";
        case KnowledgeRelationType::TransformsInto: return "transforms_into";
        case KnowledgeRelationType::Produces: return "produces";
        case KnowledgeRelationType::Prevents: return "prevents";
        case KnowledgeRelationType::Requires: return "requires";
        case KnowledgeRelationType::FailsUnder: return "fails_under";
        case KnowledgeRelationType::LocatedAt: return "located_at";
        case KnowledgeRelationType::Indicates: return "indicates";
        case KnowledgeRelationType::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeRelationType> knowledgeRelationTypeFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::Unknown);
    if (str == "has_effect") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::HasEffect);
    if (str == "has_risk") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::HasRisk);
    if (str == "is_edible_under") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::IsEdibleUnder);
    if (str == "is_dangerous_under") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::IsDangerousUnder);
    if (str == "is_usable_for") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::IsUsableFor);
    if (str == "reacts_with") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::ReactsWith);
    if (str == "transforms_into") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::TransformsInto);
    if (str == "produces") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::Produces);
    if (str == "prevents") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::Prevents);
    if (str == "requires") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::Requires);
    if (str == "fails_under") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::FailsUnder);
    if (str == "located_at") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::LocatedAt);
    if (str == "indicates") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::Indicates);
    if (str == "test_only") return Result<KnowledgeRelationType>::ok(KnowledgeRelationType::TestOnly);
    return Result<KnowledgeRelationType>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeRelationType: " + str));
}

// ============================================================
// KnowledgeConfidenceBand
// ============================================================

std::string toString(KnowledgeConfidenceBand band) {
    switch (band) {
        case KnowledgeConfidenceBand::Unknown: return "unknown";
        case KnowledgeConfidenceBand::Weak: return "weak";
        case KnowledgeConfidenceBand::Plausible: return "plausible";
        case KnowledgeConfidenceBand::Reliable: return "reliable";
        case KnowledgeConfidenceBand::Strong: return "strong";
        case KnowledgeConfidenceBand::Established: return "established";
    }
    return "unknown";
}

Result<KnowledgeConfidenceBand> knowledgeConfidenceBandFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Unknown);
    if (str == "weak") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Weak);
    if (str == "plausible") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Plausible);
    if (str == "reliable") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Reliable);
    if (str == "strong") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Strong);
    if (str == "established") return Result<KnowledgeConfidenceBand>::ok(KnowledgeConfidenceBand::Established);
    return Result<KnowledgeConfidenceBand>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeConfidenceBand: " + str));
}

// ============================================================
// KnowledgeStatus
// ============================================================

std::string toString(KnowledgeStatus status) {
    switch (status) {
        case KnowledgeStatus::Unknown: return "unknown";
        case KnowledgeStatus::Candidate: return "candidate";
        case KnowledgeStatus::Hypothesis: return "hypothesis";
        case KnowledgeStatus::Active: return "active";
        case KnowledgeStatus::Teachable: return "teachable";
        case KnowledgeStatus::Shared: return "shared";
        case KnowledgeStatus::Operational: return "operational";
        case KnowledgeStatus::Deprecated: return "deprecated";
        case KnowledgeStatus::Disproven: return "disproven";
        case KnowledgeStatus::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeStatus> knowledgeStatusFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Unknown);
    if (str == "candidate") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Candidate);
    if (str == "hypothesis") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Hypothesis);
    if (str == "active") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Active);
    if (str == "teachable") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Teachable);
    if (str == "shared") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Shared);
    if (str == "operational") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Operational);
    if (str == "deprecated") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Deprecated);
    if (str == "disproven") return Result<KnowledgeStatus>::ok(KnowledgeStatus::Disproven);
    if (str == "test_only") return Result<KnowledgeStatus>::ok(KnowledgeStatus::TestOnly);
    return Result<KnowledgeStatus>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeStatus: " + str));
}

// ============================================================
// KnowledgeEvidenceKind
// ============================================================

std::string toString(KnowledgeEvidenceKind kind) {
    switch (kind) {
        case KnowledgeEvidenceKind::Unknown: return "unknown";
        case KnowledgeEvidenceKind::MemoryRecord: return "memory_record";
        case KnowledgeEvidenceKind::MemorySummary: return "memory_summary";
        case KnowledgeEvidenceKind::CognitionEvidence: return "cognition_evidence";
        case KnowledgeEvidenceKind::DirectExperience: return "direct_experience";
        case KnowledgeEvidenceKind::RepeatedExperience: return "repeated_experience";
        case KnowledgeEvidenceKind::Teaching: return "teaching";
        case KnowledgeEvidenceKind::ExternalRecord: return "external_record";
        case KnowledgeEvidenceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeEvidenceKind> knowledgeEvidenceKindFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::Unknown);
    if (str == "memory_record") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::MemoryRecord);
    if (str == "memory_summary") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::MemorySummary);
    if (str == "cognition_evidence") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::CognitionEvidence);
    if (str == "direct_experience") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::DirectExperience);
    if (str == "repeated_experience") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::RepeatedExperience);
    if (str == "teaching") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::Teaching);
    if (str == "external_record") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::ExternalRecord);
    if (str == "test_only") return Result<KnowledgeEvidenceKind>::ok(KnowledgeEvidenceKind::TestOnly);
    return Result<KnowledgeEvidenceKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeEvidenceKind: " + str));
}

// ============================================================
// KnowledgeFormationDecision
// ============================================================

std::string toString(KnowledgeFormationDecision decision) {
    switch (decision) {
        case KnowledgeFormationDecision::Unknown: return "unknown";
        case KnowledgeFormationDecision::Skipped: return "skipped";
        case KnowledgeFormationDecision::CreatedClaim: return "created_claim";
        case KnowledgeFormationDecision::UpdatedClaim: return "updated_claim";
        case KnowledgeFormationDecision::Rejected: return "rejected";
    }
    return "unknown";
}

Result<KnowledgeFormationDecision> knowledgeFormationDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeFormationDecision>::ok(KnowledgeFormationDecision::Unknown);
    if (str == "skipped") return Result<KnowledgeFormationDecision>::ok(KnowledgeFormationDecision::Skipped);
    if (str == "created_claim") return Result<KnowledgeFormationDecision>::ok(KnowledgeFormationDecision::CreatedClaim);
    if (str == "updated_claim") return Result<KnowledgeFormationDecision>::ok(KnowledgeFormationDecision::UpdatedClaim);
    if (str == "rejected") return Result<KnowledgeFormationDecision>::ok(KnowledgeFormationDecision::Rejected);
    return Result<KnowledgeFormationDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeFormationDecision: " + str));
}

// ============================================================
// KnowledgeQueryMode
// ============================================================

std::string toString(KnowledgeQueryMode mode) {
    switch (mode) {
        case KnowledgeQueryMode::Unknown: return "unknown";
        case KnowledgeQueryMode::ByOwner: return "by_owner";
        case KnowledgeQueryMode::ExactSubject: return "exact_subject";
        case KnowledgeQueryMode::ByRelation: return "by_relation";
        case KnowledgeQueryMode::Actionable: return "actionable";
        case KnowledgeQueryMode::Teachable: return "teachable";
        case KnowledgeQueryMode::RiskRelated: return "risk_related";
        case KnowledgeQueryMode::HighConfidence: return "high_confidence";
        case KnowledgeQueryMode::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeQueryMode> knowledgeQueryModeFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::Unknown);
    if (str == "by_owner") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::ByOwner);
    if (str == "exact_subject") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::ExactSubject);
    if (str == "by_relation") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::ByRelation);
    if (str == "actionable") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::Actionable);
    if (str == "teachable") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::Teachable);
    if (str == "risk_related") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::RiskRelated);
    if (str == "high_confidence") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::HighConfidence);
    if (str == "test_only") return Result<KnowledgeQueryMode>::ok(KnowledgeQueryMode::TestOnly);
    return Result<KnowledgeQueryMode>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeQueryMode: " + str));
}

// ============================================================
// KnowledgeConflictType
// ============================================================

std::string toString(KnowledgeConflictType type) {
    switch (type) {
        case KnowledgeConflictType::Unknown: return "unknown";
        case KnowledgeConflictType::SameClaimSupport: return "same_claim_support";
        case KnowledgeConflictType::OppositeEffect: return "opposite_effect";
        case KnowledgeConflictType::RiskDiscovered: return "risk_discovered";
        case KnowledgeConflictType::ConditionMismatch: return "condition_mismatch";
        case KnowledgeConflictType::Overgeneralized: return "overgeneralized";
        case KnowledgeConflictType::DuplicateClaim: return "duplicate_claim";
        case KnowledgeConflictType::WeakContradiction: return "weak_contradiction";
        case KnowledgeConflictType::StrongContradiction: return "strong_contradiction";
        case KnowledgeConflictType::SourceInvalid: return "source_invalid";
        case KnowledgeConflictType::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeConflictType> knowledgeConflictTypeFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::Unknown);
    if (str == "same_claim_support") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::SameClaimSupport);
    if (str == "opposite_effect") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::OppositeEffect);
    if (str == "risk_discovered") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::RiskDiscovered);
    if (str == "condition_mismatch") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::ConditionMismatch);
    if (str == "overgeneralized") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::Overgeneralized);
    if (str == "duplicate_claim") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::DuplicateClaim);
    if (str == "weak_contradiction") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::WeakContradiction);
    if (str == "strong_contradiction") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::StrongContradiction);
    if (str == "source_invalid") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::SourceInvalid);
    if (str == "test_only") return Result<KnowledgeConflictType>::ok(KnowledgeConflictType::TestOnly);
    return Result<KnowledgeConflictType>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeConflictType: " + str));
}

// ============================================================
// KnowledgeRevisionDecision
// ============================================================

std::string toString(KnowledgeRevisionDecision decision) {
    switch (decision) {
        case KnowledgeRevisionDecision::Unknown: return "unknown";
        case KnowledgeRevisionDecision::Skipped: return "skipped";
        case KnowledgeRevisionDecision::NoChange: return "no_change";
        case KnowledgeRevisionDecision::ReinforcedClaim: return "reinforced_claim";
        case KnowledgeRevisionDecision::WeakenedClaim: return "weakened_claim";
        case KnowledgeRevisionDecision::CreatedSpecializedClaim: return "created_specialized_claim";
        case KnowledgeRevisionDecision::DeprecatedClaim: return "deprecated_claim";
        case KnowledgeRevisionDecision::DisprovenClaim: return "disproven_claim";
        case KnowledgeRevisionDecision::Rejected: return "rejected";
        case KnowledgeRevisionDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeRevisionDecision> knowledgeRevisionDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::Unknown);
    if (str == "skipped") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::Skipped);
    if (str == "no_change") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::NoChange);
    if (str == "reinforced_claim") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::ReinforcedClaim);
    if (str == "weakened_claim") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::WeakenedClaim);
    if (str == "created_specialized_claim") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::CreatedSpecializedClaim);
    if (str == "deprecated_claim") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::DeprecatedClaim);
    if (str == "disproven_claim") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::DisprovenClaim);
    if (str == "rejected") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::Rejected);
    if (str == "test_only") return Result<KnowledgeRevisionDecision>::ok(KnowledgeRevisionDecision::TestOnly);
    return Result<KnowledgeRevisionDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeRevisionDecision: " + str));
}

// ============================================================
// KnowledgeResolutionStrategy
// ============================================================

std::string toString(KnowledgeResolutionStrategy strategy) {
    switch (strategy) {
        case KnowledgeResolutionStrategy::Unknown: return "unknown";
        case KnowledgeResolutionStrategy::Reinforce: return "reinforce";
        case KnowledgeResolutionStrategy::Weaken: return "weaken";
        case KnowledgeResolutionStrategy::Specialize: return "specialize";
        case KnowledgeResolutionStrategy::Deprecate: return "deprecate";
        case KnowledgeResolutionStrategy::Disprove: return "disprove";
        case KnowledgeResolutionStrategy::MergeDuplicate: return "merge_duplicate";
        case KnowledgeResolutionStrategy::KeepBoth: return "keep_both";
        case KnowledgeResolutionStrategy::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgeResolutionStrategy> knowledgeResolutionStrategyFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Unknown);
    if (str == "reinforce") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Reinforce);
    if (str == "weaken") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Weaken);
    if (str == "specialize") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Specialize);
    if (str == "deprecate") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Deprecate);
    if (str == "disprove") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::Disprove);
    if (str == "merge_duplicate") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::MergeDuplicate);
    if (str == "keep_both") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::KeepBoth);
    if (str == "test_only") return Result<KnowledgeResolutionStrategy>::ok(KnowledgeResolutionStrategy::TestOnly);
    return Result<KnowledgeResolutionStrategy>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgeResolutionStrategy: " + str));
}

// ============================================================
// KnowledgePropagationChannel
// ============================================================

std::string toString(KnowledgePropagationChannel channel) {
    switch (channel) {
        case KnowledgePropagationChannel::Unknown: return "unknown";
        case KnowledgePropagationChannel::DirectTeaching: return "direct_teaching";
        case KnowledgePropagationChannel::Demonstration: return "demonstration";
        case KnowledgePropagationChannel::WarningSignal: return "warning_signal";
        case KnowledgePropagationChannel::SharedObservation: return "shared_observation";
        case KnowledgePropagationChannel::TribeInstruction: return "tribe_instruction";
        case KnowledgePropagationChannel::Correction: return "correction";
        case KnowledgePropagationChannel::ExternalRecord: return "external_record";
        case KnowledgePropagationChannel::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgePropagationChannel> knowledgePropagationChannelFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::Unknown);
    if (str == "direct_teaching") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::DirectTeaching);
    if (str == "demonstration") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::Demonstration);
    if (str == "warning_signal") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::WarningSignal);
    if (str == "shared_observation") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::SharedObservation);
    if (str == "tribe_instruction") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::TribeInstruction);
    if (str == "correction") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::Correction);
    if (str == "external_record") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::ExternalRecord);
    if (str == "test_only") return Result<KnowledgePropagationChannel>::ok(KnowledgePropagationChannel::TestOnly);
    return Result<KnowledgePropagationChannel>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgePropagationChannel: " + str));
}

// ============================================================
// KnowledgePropagationDecision
// ============================================================

std::string toString(KnowledgePropagationDecision decision) {
    switch (decision) {
        case KnowledgePropagationDecision::Unknown: return "unknown";
        case KnowledgePropagationDecision::Skipped: return "skipped";
        case KnowledgePropagationDecision::Rejected: return "rejected";
        case KnowledgePropagationDecision::Failed: return "failed";
        case KnowledgePropagationDecision::CreatedRecipientClaim: return "created_recipient_claim";
        case KnowledgePropagationDecision::UpdatedRecipientClaim: return "updated_recipient_claim";
        case KnowledgePropagationDecision::ReinforcedRecipientClaim: return "reinforced_recipient_claim";
        case KnowledgePropagationDecision::WeakenedRecipientClaim: return "weakened_recipient_claim";
        case KnowledgePropagationDecision::CorrectionDelivered: return "correction_delivered";
        case KnowledgePropagationDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgePropagationDecision> knowledgePropagationDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::Unknown);
    if (str == "skipped") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::Skipped);
    if (str == "rejected") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::Rejected);
    if (str == "failed") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::Failed);
    if (str == "created_recipient_claim") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::CreatedRecipientClaim);
    if (str == "updated_recipient_claim") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::UpdatedRecipientClaim);
    if (str == "reinforced_recipient_claim") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::ReinforcedRecipientClaim);
    if (str == "weakened_recipient_claim") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::WeakenedRecipientClaim);
    if (str == "correction_delivered") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::CorrectionDelivered);
    if (str == "test_only") return Result<KnowledgePropagationDecision>::ok(KnowledgePropagationDecision::TestOnly);
    return Result<KnowledgePropagationDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgePropagationDecision: " + str));
}

// ============================================================
// KnowledgePropagationTrustBand
// ============================================================

std::string toString(KnowledgePropagationTrustBand band) {
    switch (band) {
        case KnowledgePropagationTrustBand::Unknown: return "unknown";
        case KnowledgePropagationTrustBand::Distrusted: return "distrusted";
        case KnowledgePropagationTrustBand::Low: return "low";
        case KnowledgePropagationTrustBand::Medium: return "medium";
        case KnowledgePropagationTrustBand::High: return "high";
        case KnowledgePropagationTrustBand::Authority: return "authority";
        case KnowledgePropagationTrustBand::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgePropagationTrustBand> knowledgePropagationTrustBandFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::Unknown);
    if (str == "distrusted") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::Distrusted);
    if (str == "low") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::Low);
    if (str == "medium") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::Medium);
    if (str == "high") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::High);
    if (str == "authority") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::Authority);
    if (str == "test_only") return Result<KnowledgePropagationTrustBand>::ok(KnowledgePropagationTrustBand::TestOnly);
    return Result<KnowledgePropagationTrustBand>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgePropagationTrustBand: " + str));
}

// ============================================================
// KnowledgePropagationFailureReason
// ============================================================

std::string toString(KnowledgePropagationFailureReason reason) {
    switch (reason) {
        case KnowledgePropagationFailureReason::Unknown: return "unknown";
        case KnowledgePropagationFailureReason::NoTransferableClaim: return "no_transferable_claim";
        case KnowledgePropagationFailureReason::SourceInvalid: return "source_invalid";
        case KnowledgePropagationFailureReason::TargetInvalid: return "target_invalid";
        case KnowledgePropagationFailureReason::SameOwner: return "same_owner";
        case KnowledgePropagationFailureReason::ClaimNotTeachable: return "claim_not_teachable";
        case KnowledgePropagationFailureReason::ClaimDeprecated: return "claim_deprecated";
        case KnowledgePropagationFailureReason::ClaimDisproven: return "claim_disproven";
        case KnowledgePropagationFailureReason::TrustTooLow: return "trust_too_low";
        case KnowledgePropagationFailureReason::ChannelTooWeak: return "channel_too_weak";
        case KnowledgePropagationFailureReason::ConflictWithRecipient: return "conflict_with_recipient";
        case KnowledgePropagationFailureReason::SecurityRejected: return "security_rejected";
        case KnowledgePropagationFailureReason::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<KnowledgePropagationFailureReason> knowledgePropagationFailureReasonFromString(const std::string& str) {
    if (str == "unknown") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::Unknown);
    if (str == "no_transferable_claim") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::NoTransferableClaim);
    if (str == "source_invalid") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::SourceInvalid);
    if (str == "target_invalid") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::TargetInvalid);
    if (str == "same_owner") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::SameOwner);
    if (str == "claim_not_teachable") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::ClaimNotTeachable);
    if (str == "claim_deprecated") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::ClaimDeprecated);
    if (str == "claim_disproven") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::ClaimDisproven);
    if (str == "trust_too_low") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::TrustTooLow);
    if (str == "channel_too_weak") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::ChannelTooWeak);
    if (str == "conflict_with_recipient") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::ConflictWithRecipient);
    if (str == "security_rejected") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::SecurityRejected);
    if (str == "test_only") return Result<KnowledgePropagationFailureReason>::ok(KnowledgePropagationFailureReason::TestOnly);
    return Result<KnowledgePropagationFailureReason>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid KnowledgePropagationFailureReason: " + str));
}

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> knowledgeForbiddenKeys() {
    return {
        "objectdefinition hidden",
        "edible_profile",
        "hunger_delta",
        "health_delta",
        "effect_kind",
        "gamestate",
        "agenttickrecord",
        "reward_value",
        "done =",
        "is_done",
        "savegame",
        "savemanager",
        "raw_state",
        "hidden_truth",
        "true_trait",
        "real_effect",
        "object_truth"
    };
}

bool containsKnowledgeForbiddenKey(const std::string& text) {
    if (text.empty()) return false;
    std::string lower;
    lower.reserve(text.size());
    for (char c : text) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    for (const auto& key : knowledgeForbiddenKeys()) {
        if (lower.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsKnowledgeForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsKnowledgeForbiddenKey(v)) return true;
    }
    return false;
}

// ============================================================
// Helpers
// ============================================================

KnowledgeConfidenceBand confidenceToBand(double confidence) {
    if (confidence < 0.10) return KnowledgeConfidenceBand::Unknown;
    if (confidence < 0.35) return KnowledgeConfidenceBand::Weak;
    if (confidence < 0.55) return KnowledgeConfidenceBand::Plausible;
    if (confidence < 0.75) return KnowledgeConfidenceBand::Reliable;
    if (confidence < 0.90) return KnowledgeConfidenceBand::Strong;
    return KnowledgeConfidenceBand::Established;
}

} // namespace pathfinder::knowledge
