#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::memory {

// ============================================================
// Enums
// ============================================================

enum class MemoryOwnerKind {
    Unknown,
    Actor,
    Agent,
    Group,
    Tribe,
    Civilization,
    TestOnly
};

std::string toString(MemoryOwnerKind kind);
pathfinder::foundation::Result<MemoryOwnerKind> memoryOwnerKindFromString(const std::string& str);

enum class MemorySubjectKind {
    Unknown,
    ObjectDefinition,
    WorldObject,
    ObjectCategory,
    AgentSpecies,
    EnvironmentFeature,
    ActionOutcome,
    TestOnly
};

std::string toString(MemorySubjectKind kind);
pathfinder::foundation::Result<MemorySubjectKind> memorySubjectKindFromString(const std::string& str);

enum class MemoryKind {
    Unknown,
    Experiment,
    Success,
    Failure,
    Warning,
    Accident,
    Discovery,
    Contradiction,
    TeachingRelated,
    TestOnly
};

std::string toString(MemoryKind kind);
pathfinder::foundation::Result<MemoryKind> memoryKindFromString(const std::string& str);

enum class MemoryScope {
    Unknown,
    ShortTerm,
    MidTerm,
    LongTerm
};

std::string toString(MemoryScope scope);
pathfinder::foundation::Result<MemoryScope> memoryScopeFromString(const std::string& str);

enum class MemoryLifecycle {
    Unknown,
    Active,
    Fading,
    Forgotten,
    Protected
};

std::string toString(MemoryLifecycle lifecycle);
pathfinder::foundation::Result<MemoryLifecycle> memoryLifecycleFromString(const std::string& str);

enum class MemoryStrengthBand {
    Unknown,
    Trace,
    Weak,
    Stable,
    Strong,
    Core
};

std::string toString(MemoryStrengthBand band);
pathfinder::foundation::Result<MemoryStrengthBand> memoryStrengthBandFromString(const std::string& str);

enum class MemoryImportance {
    Unknown,
    Trivial,
    Normal,
    Important,
    Critical
};

std::string toString(MemoryImportance importance);
pathfinder::foundation::Result<MemoryImportance> memoryImportanceFromString(const std::string& str);

enum class MemorySourceKind {
    Unknown,
    CognitionEvidence,
    CognitionUpdate,
    DirectEvent,
    ImportedSafeProjection,
    TestOnly
};

std::string toString(MemorySourceKind kind);
pathfinder::foundation::Result<MemorySourceKind> memorySourceKindFromString(const std::string& str);

enum class MemoryWriteDecision {
    Unknown,
    Created,
    Reinforced,
    Promoted,
    Weakened,
    Ignored,
    Rejected
};

std::string toString(MemoryWriteDecision decision);
pathfinder::foundation::Result<MemoryWriteDecision> memoryWriteDecisionFromString(const std::string& str);

enum class MemoryDecayDecision {
    Unknown,
    Unchanged,
    Decayed,
    Faded,
    Forgotten,
    ProtectedSkipped
};

std::string toString(MemoryDecayDecision decision);
pathfinder::foundation::Result<MemoryDecayDecision> memoryDecayDecisionFromString(const std::string& str);

enum class MemoryRejectReason {
    Unknown,
    EmptyMemoryId,
    EmptyOwner,
    EmptySubject,
    EmptyCandidateId,
    EmptySource,
    UnknownOwnerKind,
    UnknownSubjectKind,
    UnknownMemoryKind,
    UnknownScope,
    UnknownLifecycle,
    StrengthOutOfRange,
    ImportanceUnknown,
    MissingSourceEvent,
    HiddenTruthDetected,
    RawStateDetected,
    TooManyRecords,
    SchemaVersionUnsupported
};

std::string toString(MemoryRejectReason reason);
pathfinder::foundation::Result<MemoryRejectReason> memoryRejectReasonFromString(const std::string& str);

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> memoryForbiddenKeys();
bool containsMemoryForbiddenKey(const std::string& text);
bool containsMemoryForbiddenKey(const std::vector<std::string>& texts);

// ============================================================
// Helpers
// ============================================================

MemoryStrengthBand strengthToBand(double strength);
double importanceToInitialStrength(MemoryImportance importance);
double importanceToReinforceGain(MemoryImportance importance);

} // namespace pathfinder::memory
