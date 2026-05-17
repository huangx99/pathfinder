#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include <string>
#include <vector>

namespace pathfinder::knowledge {

// ============================================================
// ID Types
// ============================================================

struct KnowledgeEvidenceIdTag {};
using KnowledgeEvidenceId = pathfinder::foundation::StrongId<KnowledgeEvidenceIdTag>;

struct KnowledgeTraceIdTag {};
using KnowledgeTraceId = pathfinder::foundation::StrongId<KnowledgeTraceIdTag>;

// ============================================================
// Enums
// ============================================================

enum class KnowledgeOwnerKind {
    Unknown,
    Agent,
    Actor,
    Tribe,
    Region,
    Civilization,
    ExternalRecord,
    TestOnly
};

std::string toString(KnowledgeOwnerKind kind);
pathfinder::foundation::Result<KnowledgeOwnerKind> knowledgeOwnerKindFromString(const std::string& str);

enum class KnowledgeSubjectKind {
    Unknown,
    ObjectDefinition,
    ObjectInstance,
    Trait,
    Region,
    Action,
    Combination,
    Agent,
    Tribe,
    TestOnly
};

std::string toString(KnowledgeSubjectKind kind);
pathfinder::foundation::Result<KnowledgeSubjectKind> knowledgeSubjectKindFromString(const std::string& str);

enum class KnowledgeRelationType {
    Unknown,
    HasEffect,
    HasRisk,
    IsEdibleUnder,
    IsDangerousUnder,
    IsUsableFor,
    ReactsWith,
    TransformsInto,
    Produces,
    Prevents,
    Requires,
    FailsUnder,
    LocatedAt,
    Indicates,
    TestOnly
};

std::string toString(KnowledgeRelationType type);
pathfinder::foundation::Result<KnowledgeRelationType> knowledgeRelationTypeFromString(const std::string& str);

enum class KnowledgeConfidenceBand {
    Unknown,
    Weak,
    Plausible,
    Reliable,
    Strong,
    Established
};

std::string toString(KnowledgeConfidenceBand band);
pathfinder::foundation::Result<KnowledgeConfidenceBand> knowledgeConfidenceBandFromString(const std::string& str);

enum class KnowledgeStatus {
    Unknown,
    Candidate,
    Hypothesis,
    Active,
    Teachable,
    Shared,
    Operational,
    Deprecated,
    Disproven,
    TestOnly
};

std::string toString(KnowledgeStatus status);
pathfinder::foundation::Result<KnowledgeStatus> knowledgeStatusFromString(const std::string& str);

enum class KnowledgeEvidenceKind {
    Unknown,
    MemoryRecord,
    MemorySummary,
    CognitionEvidence,
    DirectExperience,
    RepeatedExperience,
    Teaching,
    ExternalRecord,
    TestOnly
};

std::string toString(KnowledgeEvidenceKind kind);
pathfinder::foundation::Result<KnowledgeEvidenceKind> knowledgeEvidenceKindFromString(const std::string& str);

enum class KnowledgeFormationDecision {
    Unknown,
    Skipped,
    CreatedClaim,
    UpdatedClaim,
    Rejected
};

std::string toString(KnowledgeFormationDecision decision);
pathfinder::foundation::Result<KnowledgeFormationDecision> knowledgeFormationDecisionFromString(const std::string& str);

enum class KnowledgeQueryMode {
    Unknown,
    ByOwner,
    ExactSubject,
    ByRelation,
    Actionable,
    Teachable,
    RiskRelated,
    HighConfidence,
    TestOnly
};

std::string toString(KnowledgeQueryMode mode);
pathfinder::foundation::Result<KnowledgeQueryMode> knowledgeQueryModeFromString(const std::string& str);

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> knowledgeForbiddenKeys();
bool containsKnowledgeForbiddenKey(const std::string& text);
bool containsKnowledgeForbiddenKey(const std::vector<std::string>& values);

// ============================================================
// Helpers
// ============================================================

KnowledgeConfidenceBand confidenceToBand(double confidence);

} // namespace pathfinder::knowledge
