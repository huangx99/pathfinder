#pragma once

#include "pathfinder/memory/types.h"
#include <optional>

namespace pathfinder::memory {

// ============================================================
// DTOs
// ============================================================

struct MemoryOwner {
    MemoryOwnerKind kind = MemoryOwnerKind::Unknown;
    pathfinder::foundation::EntityId entity_id;
    pathfinder::foundation::TribeId tribe_id;
    std::string external_key;

    bool operator==(const MemoryOwner& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemorySubject {
    MemorySubjectKind kind = MemorySubjectKind::Unknown;
    std::string subject_id;
    std::string subject_type_key;
    std::vector<std::string> safe_tags;

    bool operator==(const MemorySubject& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryEvidenceRef {
    MemorySourceKind source_kind = MemorySourceKind::Unknown;
    pathfinder::cognition::CognitionEvidenceRecordId cognition_evidence_id;
    pathfinder::cognition::CognitionClaimV2Id cognition_claim_id;
    pathfinder::foundation::EventId source_event_id;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;

    bool operator==(const MemoryEvidenceRef& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryCandidate {
    std::string candidate_id;
    MemoryOwner owner;
    MemorySubject subject;
    std::vector<MemoryKind> memory_kinds;
    MemorySourceKind source_kind = MemorySourceKind::Unknown;
    MemoryImportance importance = MemoryImportance::Unknown;
    double initial_strength = 0.0;
    bool protect_from_decay = false;
    bool protect_from_forgetting = false;
    bool teaching_candidate = false;
    MemoryEvidenceRef evidence_ref;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool operator==(const MemoryCandidate& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryRecord {
    pathfinder::foundation::MemoryId memory_id;
    std::string schema_version = "memory_record.v1";
    MemoryOwner owner;
    MemorySubject subject;
    std::vector<MemoryKind> memory_kinds;
    MemoryScope scope = MemoryScope::Unknown;
    MemoryLifecycle lifecycle = MemoryLifecycle::Unknown;
    MemoryImportance importance = MemoryImportance::Unknown;
    double strength = 0.0;
    MemoryStrengthBand strength_band = MemoryStrengthBand::Unknown;
    size_t reinforcement_count = 0;
    size_t contradiction_count = 0;
    bool protect_from_decay = false;
    bool protect_from_forgetting = false;
    bool teaching_candidate = false;
    pathfinder::foundation::Tick created_tick;
    pathfinder::foundation::Tick last_touched_tick;
    std::vector<MemoryEvidenceRef> evidence_refs;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool operator==(const MemoryRecord& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryWriteOptions {
    bool allow_test_only = false;
    size_t max_candidates = 100;
    size_t max_evidence_refs_per_record = 20;
    double duplicate_match_threshold = 0.80;
    double short_to_mid_strength_threshold = 0.60;
    double mid_to_long_strength_threshold = 0.85;
    size_t short_to_mid_reinforcement_count = 3;
    size_t mid_to_long_reinforcement_count = 5;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryDecayOptions {
    size_t max_records = 1000;
    double short_term_decay_per_tick = 0.02;
    double mid_term_decay_per_tick = 0.005;
    double long_term_decay_per_tick = 0.001;
    double fading_threshold = 0.20;
    double forgotten_threshold = 0.05;
    bool decay_long_term = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryWriteIssue {
    MemoryRejectReason reason = MemoryRejectReason::Unknown;
    std::string issue_key;
    std::optional<std::string> field_key;
    std::vector<std::string> detail_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryEventDraft {
    std::string event_key;
    pathfinder::foundation::MemoryId memory_id;
    MemoryOwner owner;
    MemorySubject subject;
    MemoryScope scope = MemoryScope::Unknown;
    MemoryLifecycle lifecycle = MemoryLifecycle::Unknown;
    MemoryWriteDecision write_decision = MemoryWriteDecision::Unknown;
    MemoryDecayDecision decay_decision = MemoryDecayDecision::Unknown;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryStateChangeDraft {
    std::string change_key;
    pathfinder::foundation::MemoryId memory_id;
    std::optional<MemoryRecord> before_record;
    std::optional<MemoryRecord> after_record;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryWriteResult {
    MemoryWriteDecision decision = MemoryWriteDecision::Unknown;
    std::optional<MemoryRecord> before_record;
    std::optional<MemoryRecord> after_record;
    std::vector<MemoryRecord> created_records;
    std::vector<MemoryRecord> updated_records;
    std::vector<MemoryWriteIssue> issues;
    std::vector<MemoryEventDraft> event_drafts;
    std::vector<MemoryStateChangeDraft> state_changes;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryDecayResult {
    MemoryDecayDecision decision = MemoryDecayDecision::Unknown;
    std::vector<MemoryRecord> updated_records;
    std::vector<pathfinder::foundation::MemoryId> forgotten_memory_ids;
    std::vector<MemoryEventDraft> event_drafts;
    std::vector<MemoryStateChangeDraft> state_changes;
    std::vector<MemoryWriteIssue> issues;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryQuery {
    MemoryOwner owner;
    std::optional<MemorySubject> subject;
    std::optional<MemoryScope> scope;
    std::optional<MemoryLifecycle> lifecycle;
    bool include_forgotten = false;
    size_t limit = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryQueryResult {
    MemoryQuery query;
    std::vector<MemoryRecord> records;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::memory
