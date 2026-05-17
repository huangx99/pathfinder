#pragma once

#include "pathfinder/memory/memory_record.h"
#include "pathfinder/foundation/id.h"
#include <optional>
#include <unordered_map>

namespace pathfinder::memory {

// ============================================================
// ID
// ============================================================

struct MemorySummaryIdTag {};
using MemorySummaryId = pathfinder::foundation::StrongId<MemorySummaryIdTag>;

// ============================================================
// Enums
// ============================================================

enum class MemorySummaryKind {
    Unknown,
    OwnerSubjectPattern,
    RiskPattern,
    TeachingPattern,
    ContradictionPattern,
    LongTermPattern,
    TestOnly
};

std::string toString(MemorySummaryKind kind);
pathfinder::foundation::Result<MemorySummaryKind> memorySummaryKindFromString(const std::string& str);

enum class MemoryCompressionLevel {
    Unknown,
    None,
    LightSummary,
    StatisticalSummary,
    ArchivedProjection,
    TestOnly
};

std::string toString(MemoryCompressionLevel level);
pathfinder::foundation::Result<MemoryCompressionLevel> memoryCompressionLevelFromString(const std::string& str);

enum class MemoryCompressionDecision {
    Unknown,
    Skipped,
    CreatedSummary,
    UpdatedSummary,
    Rejected
};

std::string toString(MemoryCompressionDecision decision);
pathfinder::foundation::Result<MemoryCompressionDecision> memoryCompressionDecisionFromString(const std::string& str);

enum class MemoryRecallMode {
    Unknown,
    ExactSubject,
    OwnerRecent,
    ByMemoryKind,
    ImportantOrCritical,
    TeachingCandidates,
    RiskRelated,
    ContradictionRelated,
    LongTermOnly,
    TestOnly
};

std::string toString(MemoryRecallMode mode);
pathfinder::foundation::Result<MemoryRecallMode> memoryRecallModeFromString(const std::string& str);

enum class MemoryRecallSort {
    Unknown,
    StrengthDesc,
    LastTouchedDesc,
    CreatedDesc,
    ImportanceDesc,
    EvidenceCountDesc,
    DeterministicKeyAsc
};

std::string toString(MemoryRecallSort sort);
pathfinder::foundation::Result<MemoryRecallSort> memoryRecallSortFromString(const std::string& str);

enum class MemoryRecallItemKind {
    Unknown,
    Record,
    Summary,
    TestOnly
};

std::string toString(MemoryRecallItemKind kind);
pathfinder::foundation::Result<MemoryRecallItemKind> memoryRecallItemKindFromString(const std::string& str);

// ============================================================
// DTOs
// ============================================================

struct MemorySummaryKey {
    MemoryOwner owner;
    MemorySubject subject;
    MemorySummaryKind summary_kind = MemorySummaryKind::Unknown;
    std::vector<MemoryKind> memory_kinds;

    bool operator==(const MemorySummaryKey& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemorySummary {
    MemorySummaryId summary_id;
    std::string schema_version = "memory_summary.v1";
    MemorySummaryKey key;
    MemoryCompressionLevel compression_level = MemoryCompressionLevel::Unknown;
    MemoryImportance highest_importance = MemoryImportance::Unknown;
    MemoryStrengthBand aggregate_strength_band = MemoryStrengthBand::Unknown;
    double aggregate_strength = 0.0;
    size_t source_memory_count = 0;
    size_t success_count = 0;
    size_t failure_count = 0;
    size_t warning_count = 0;
    size_t accident_count = 0;
    size_t contradiction_count = 0;
    size_t teaching_candidate_count = 0;
    bool has_protected_source = false;
    bool has_long_term_source = false;
    pathfinder::foundation::Tick first_observed_tick;
    pathfinder::foundation::Tick last_observed_tick;
    pathfinder::foundation::Tick summarized_tick;
    std::vector<pathfinder::foundation::MemoryId> source_memory_ids;
    std::vector<pathfinder::foundation::MemoryId> representative_memory_ids;
    std::vector<MemoryEvidenceRef> evidence_refs;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool operator==(const MemorySummary& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryCompressionOptions {
    bool allow_test_only = false;
    size_t min_records_to_summarize = 3;
    size_t max_source_records = 1000;
    size_t max_representative_records = 5;
    bool include_short_term = false;
    bool include_fading = false;
    bool include_forgotten = false;
    bool include_protected = true;
    MemoryCompressionLevel target_level = MemoryCompressionLevel::LightSummary;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryCompressionPlan {
    std::string plan_key;
    MemorySummaryKey key;
    std::vector<pathfinder::foundation::MemoryId> source_memory_ids;
    std::vector<pathfinder::foundation::MemoryId> representative_memory_ids;
    MemoryCompressionLevel target_level = MemoryCompressionLevel::Unknown;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryCompressionResult {
    MemoryCompressionDecision decision = MemoryCompressionDecision::Unknown;
    std::optional<MemoryCompressionPlan> plan;
    std::optional<MemorySummary> summary;
    std::vector<MemoryEventDraft> event_drafts;
    std::vector<MemoryStateChangeDraft> state_changes;
    std::vector<MemoryWriteIssue> issues;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryRecallQuery {
    MemoryOwner owner;
    std::optional<MemorySubject> subject;
    MemoryRecallMode mode = MemoryRecallMode::Unknown;
    std::vector<MemoryKind> memory_kinds;
    std::optional<MemoryScope> scope;
    std::optional<MemoryImportance> min_importance;
    bool include_records = true;
    bool include_summaries = true;
    bool include_forgotten = false;
    bool teaching_only = false;
    bool contradiction_only = false;
    size_t limit = 20;
    MemoryRecallSort sort = MemoryRecallSort::StrengthDesc;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryRecallItem {
    MemoryRecallItemKind item_kind = MemoryRecallItemKind::Unknown;
    std::optional<MemoryRecord> record;
    std::optional<MemorySummary> summary;
    double relevance_score = 0.0;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemoryRecallResult {
    MemoryRecallQuery query;
    std::vector<MemoryRecallItem> items;
    size_t scanned_record_count = 0;
    size_t scanned_summary_count = 0;
    bool truncated = false;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Services
// ============================================================

class MemorySummaryIdFactory {
public:
    pathfinder::foundation::Result<MemorySummaryId> makeSummaryId(
        const MemorySummaryKey& key,
        pathfinder::foundation::Tick first_tick,
        size_t sequence_index) const;
};

class MemorySummaryStore {
public:
    pathfinder::foundation::Result<void> put(MemorySummary summary);
    pathfinder::foundation::Result<std::optional<MemorySummary>> find(
        const MemorySummaryId& summary_id) const;
    pathfinder::foundation::Result<std::vector<MemorySummary>> listByOwner(
        const MemoryOwner& owner) const;
    pathfinder::foundation::Result<void> remove(const MemorySummaryId& summary_id);
    size_t size() const;
    void clear();

private:
    std::unordered_map<std::string, MemorySummary> summaries_;
};

} // namespace pathfinder::memory
