#include "pathfinder/memory/memory_summary.h"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// MemorySummaryKind
// ============================================================

std::string toString(MemorySummaryKind kind) {
    switch (kind) {
        case MemorySummaryKind::Unknown: return "unknown";
        case MemorySummaryKind::OwnerSubjectPattern: return "owner_subject_pattern";
        case MemorySummaryKind::RiskPattern: return "risk_pattern";
        case MemorySummaryKind::TeachingPattern: return "teaching_pattern";
        case MemorySummaryKind::ContradictionPattern: return "contradiction_pattern";
        case MemorySummaryKind::LongTermPattern: return "long_term_pattern";
        case MemorySummaryKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemorySummaryKind> memorySummaryKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemorySummaryKind>::ok(MemorySummaryKind::Unknown);
    if (str == "owner_subject_pattern") return Result<MemorySummaryKind>::ok(MemorySummaryKind::OwnerSubjectPattern);
    if (str == "risk_pattern") return Result<MemorySummaryKind>::ok(MemorySummaryKind::RiskPattern);
    if (str == "teaching_pattern") return Result<MemorySummaryKind>::ok(MemorySummaryKind::TeachingPattern);
    if (str == "contradiction_pattern") return Result<MemorySummaryKind>::ok(MemorySummaryKind::ContradictionPattern);
    if (str == "long_term_pattern") return Result<MemorySummaryKind>::ok(MemorySummaryKind::LongTermPattern);
    if (str == "test_only") return Result<MemorySummaryKind>::ok(MemorySummaryKind::TestOnly);
    return Result<MemorySummaryKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemorySummaryKind: " + str));
}

// ============================================================
// MemoryCompressionLevel
// ============================================================

std::string toString(MemoryCompressionLevel level) {
    switch (level) {
        case MemoryCompressionLevel::Unknown: return "unknown";
        case MemoryCompressionLevel::None: return "none";
        case MemoryCompressionLevel::LightSummary: return "light_summary";
        case MemoryCompressionLevel::StatisticalSummary: return "statistical_summary";
        case MemoryCompressionLevel::ArchivedProjection: return "archived_projection";
        case MemoryCompressionLevel::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemoryCompressionLevel> memoryCompressionLevelFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::Unknown);
    if (str == "none") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::None);
    if (str == "light_summary") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::LightSummary);
    if (str == "statistical_summary") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::StatisticalSummary);
    if (str == "archived_projection") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::ArchivedProjection);
    if (str == "test_only") return Result<MemoryCompressionLevel>::ok(MemoryCompressionLevel::TestOnly);
    return Result<MemoryCompressionLevel>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryCompressionLevel: " + str));
}

// ============================================================
// MemoryCompressionDecision
// ============================================================

std::string toString(MemoryCompressionDecision decision) {
    switch (decision) {
        case MemoryCompressionDecision::Unknown: return "unknown";
        case MemoryCompressionDecision::Skipped: return "skipped";
        case MemoryCompressionDecision::CreatedSummary: return "created_summary";
        case MemoryCompressionDecision::UpdatedSummary: return "updated_summary";
        case MemoryCompressionDecision::Rejected: return "rejected";
    }
    return "unknown";
}

Result<MemoryCompressionDecision> memoryCompressionDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryCompressionDecision>::ok(MemoryCompressionDecision::Unknown);
    if (str == "skipped") return Result<MemoryCompressionDecision>::ok(MemoryCompressionDecision::Skipped);
    if (str == "created_summary") return Result<MemoryCompressionDecision>::ok(MemoryCompressionDecision::CreatedSummary);
    if (str == "updated_summary") return Result<MemoryCompressionDecision>::ok(MemoryCompressionDecision::UpdatedSummary);
    if (str == "rejected") return Result<MemoryCompressionDecision>::ok(MemoryCompressionDecision::Rejected);
    return Result<MemoryCompressionDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryCompressionDecision: " + str));
}

// ============================================================
// MemoryRecallMode
// ============================================================

std::string toString(MemoryRecallMode mode) {
    switch (mode) {
        case MemoryRecallMode::Unknown: return "unknown";
        case MemoryRecallMode::ExactSubject: return "exact_subject";
        case MemoryRecallMode::OwnerRecent: return "owner_recent";
        case MemoryRecallMode::ByMemoryKind: return "by_memory_kind";
        case MemoryRecallMode::ImportantOrCritical: return "important_or_critical";
        case MemoryRecallMode::TeachingCandidates: return "teaching_candidates";
        case MemoryRecallMode::RiskRelated: return "risk_related";
        case MemoryRecallMode::ContradictionRelated: return "contradiction_related";
        case MemoryRecallMode::LongTermOnly: return "long_term_only";
        case MemoryRecallMode::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemoryRecallMode> memoryRecallModeFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryRecallMode>::ok(MemoryRecallMode::Unknown);
    if (str == "exact_subject") return Result<MemoryRecallMode>::ok(MemoryRecallMode::ExactSubject);
    if (str == "owner_recent") return Result<MemoryRecallMode>::ok(MemoryRecallMode::OwnerRecent);
    if (str == "by_memory_kind") return Result<MemoryRecallMode>::ok(MemoryRecallMode::ByMemoryKind);
    if (str == "important_or_critical") return Result<MemoryRecallMode>::ok(MemoryRecallMode::ImportantOrCritical);
    if (str == "teaching_candidates") return Result<MemoryRecallMode>::ok(MemoryRecallMode::TeachingCandidates);
    if (str == "risk_related") return Result<MemoryRecallMode>::ok(MemoryRecallMode::RiskRelated);
    if (str == "contradiction_related") return Result<MemoryRecallMode>::ok(MemoryRecallMode::ContradictionRelated);
    if (str == "long_term_only") return Result<MemoryRecallMode>::ok(MemoryRecallMode::LongTermOnly);
    if (str == "test_only") return Result<MemoryRecallMode>::ok(MemoryRecallMode::TestOnly);
    return Result<MemoryRecallMode>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryRecallMode: " + str));
}

// ============================================================
// MemoryRecallSort
// ============================================================

std::string toString(MemoryRecallSort sort) {
    switch (sort) {
        case MemoryRecallSort::Unknown: return "unknown";
        case MemoryRecallSort::StrengthDesc: return "strength_desc";
        case MemoryRecallSort::LastTouchedDesc: return "last_touched_desc";
        case MemoryRecallSort::CreatedDesc: return "created_desc";
        case MemoryRecallSort::ImportanceDesc: return "importance_desc";
        case MemoryRecallSort::EvidenceCountDesc: return "evidence_count_desc";
        case MemoryRecallSort::DeterministicKeyAsc: return "deterministic_key_asc";
    }
    return "unknown";
}

Result<MemoryRecallSort> memoryRecallSortFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryRecallSort>::ok(MemoryRecallSort::Unknown);
    if (str == "strength_desc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::StrengthDesc);
    if (str == "last_touched_desc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::LastTouchedDesc);
    if (str == "created_desc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::CreatedDesc);
    if (str == "importance_desc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::ImportanceDesc);
    if (str == "evidence_count_desc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::EvidenceCountDesc);
    if (str == "deterministic_key_asc") return Result<MemoryRecallSort>::ok(MemoryRecallSort::DeterministicKeyAsc);
    return Result<MemoryRecallSort>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryRecallSort: " + str));
}

// ============================================================
// MemoryRecallItemKind
// ============================================================

std::string toString(MemoryRecallItemKind kind) {
    switch (kind) {
        case MemoryRecallItemKind::Unknown: return "unknown";
        case MemoryRecallItemKind::Record: return "record";
        case MemoryRecallItemKind::Summary: return "summary";
        case MemoryRecallItemKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<MemoryRecallItemKind> memoryRecallItemKindFromString(const std::string& str) {
    if (str == "unknown") return Result<MemoryRecallItemKind>::ok(MemoryRecallItemKind::Unknown);
    if (str == "record") return Result<MemoryRecallItemKind>::ok(MemoryRecallItemKind::Record);
    if (str == "summary") return Result<MemoryRecallItemKind>::ok(MemoryRecallItemKind::Summary);
    if (str == "test_only") return Result<MemoryRecallItemKind>::ok(MemoryRecallItemKind::TestOnly);
    return Result<MemoryRecallItemKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid MemoryRecallItemKind: " + str));
}

// ============================================================
// MemorySummaryKey
// ============================================================

Result<void> MemorySummaryKey::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    if (summary_kind == MemorySummaryKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummaryKey summary_kind is Unknown"));
    }
    if (summary_kind == MemorySummaryKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummaryKey summary_kind TestOnly not allowed"));
    }
    for (const auto& mk : memory_kinds) {
        if (mk == MemoryKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummaryKey memory_kinds contains Unknown"));
        }
    }
    return Result<void>::ok();
}

// ============================================================
// MemorySummary
// ============================================================

Result<void> MemorySummary::validateBasic() const {
    if (summary_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary summary_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(summary_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary summary_id format invalid"));
    }
    if (schema_version != "memory_summary.v1") {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary schema_version unsupported"));
    }
    auto key_result = key.validateBasic();
    if (key_result.is_error()) return key_result;
    if (compression_level == MemoryCompressionLevel::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary compression_level is Unknown"));
    }
    if (highest_importance == MemoryImportance::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary highest_importance is Unknown"));
    }
    if (source_memory_ids.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary source_memory_ids is empty"));
    }
    if (source_memory_count != source_memory_ids.size()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary source_memory_count mismatch"));
    }
    if (representative_memory_ids.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary representative_memory_ids is empty"));
    }
    for (const auto& rep_id : representative_memory_ids) {
        bool found = false;
        for (const auto& src_id : source_memory_ids) {
            if (src_id == rep_id) { found = true; break; }
        }
        if (!found) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary representative_memory_ids contains id not in source_memory_ids"));
        }
    }
    if (evidence_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary evidence_refs is empty"));
    }
    for (const auto& ref : evidence_refs) {
        auto ref_result = ref.validateBasic();
        if (ref_result.is_error()) return ref_result;
    }
    if (aggregate_strength < 0.0 || aggregate_strength > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemorySummary aggregate_strength out of range"));
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySummary warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryCompressionOptions
// ============================================================

Result<void> MemoryCompressionOptions::validateBasic() const {
    if (min_records_to_summarize == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions min_records_to_summarize is 0"));
    }
    if (max_source_records == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions max_source_records is 0"));
    }
    if (max_representative_records == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions max_representative_records is 0"));
    }
    if (max_representative_records > max_source_records) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions max_representative_records exceeds max_source_records"));
    }
    if (target_level == MemoryCompressionLevel::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions target_level is Unknown"));
    }
    if (!allow_test_only && target_level == MemoryCompressionLevel::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionOptions target_level TestOnly not allowed"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryCompressionPlan
// ============================================================

Result<void> MemoryCompressionPlan::validateBasic() const {
    if (plan_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan plan_key is empty"));
    }
    auto key_result = key.validateBasic();
    if (key_result.is_error()) return key_result;
    if (source_memory_ids.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan source_memory_ids is empty"));
    }
    if (representative_memory_ids.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan representative_memory_ids is empty"));
    }
    for (const auto& rep_id : representative_memory_ids) {
        bool found = false;
        for (const auto& src_id : source_memory_ids) {
            if (src_id == rep_id) { found = true; break; }
        }
        if (!found) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan representative_memory_ids contains id not in source_memory_ids"));
        }
    }
    if (target_level == MemoryCompressionLevel::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan target_level is Unknown"));
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionPlan warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryCompressionResult
// ============================================================

bool MemoryCompressionResult::ok() const {
    return decision == MemoryCompressionDecision::CreatedSummary ||
           decision == MemoryCompressionDecision::UpdatedSummary ||
           decision == MemoryCompressionDecision::Skipped;
}

Result<void> MemoryCompressionResult::validateBasic() const {
    if (decision == MemoryCompressionDecision::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionResult decision is Unknown"));
    }
    if (plan.has_value()) {
        auto r = plan->validateBasic();
        if (r.is_error()) return r;
    }
    if (summary.has_value()) {
        auto r = summary->validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& draft : event_drafts) {
        auto r = draft.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& change : state_changes) {
        auto r = change.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& issue : issues) {
        auto r = issue.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionResult reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCompressionResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryRecallQuery
// ============================================================

Result<void> MemoryRecallQuery::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    if (subject.has_value()) {
        auto subject_result = subject->validateBasic();
        if (subject_result.is_error()) return subject_result;
    }
    if (mode == MemoryRecallMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery mode is Unknown"));
    }
    if (mode == MemoryRecallMode::ExactSubject && !subject.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery ExactSubject requires subject"));
    }
    if (mode == MemoryRecallMode::ByMemoryKind && memory_kinds.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery ByMemoryKind requires memory_kinds"));
    }
    if (sort == MemoryRecallSort::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery sort is Unknown"));
    }
    if (limit == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery limit is 0"));
    }
    if (!include_records && !include_summaries) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery include_records and include_summaries cannot both be false"));
    }
    for (const auto& mk : memory_kinds) {
        if (mk == MemoryKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery memory_kinds contains Unknown"));
        }
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallQuery reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryRecallItem
// ============================================================

Result<void> MemoryRecallItem::validateBasic() const {
    if (item_kind == MemoryRecallItemKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem item_kind is Unknown"));
    }
    if (item_kind == MemoryRecallItemKind::Record) {
        if (!record.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem Record kind missing record"));
        }
        if (summary.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem Record kind must not have summary"));
        }
        auto r = record->validateBasic();
        if (r.is_error()) return r;
    }
    if (item_kind == MemoryRecallItemKind::Summary) {
        if (!summary.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem Summary kind missing summary"));
        }
        if (record.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem Summary kind must not have record"));
        }
        auto r = summary->validateBasic();
        if (r.is_error()) return r;
    }
    if (relevance_score < 0.0 || relevance_score > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryRecallItem relevance_score out of range"));
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallItem reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryRecallResult
// ============================================================

Result<void> MemoryRecallResult::validateBasic() const {
    auto query_result = query.validateBasic();
    if (query_result.is_error()) return query_result;
    for (const auto& item : items) {
        auto r = item.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallResult reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecallResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemorySummaryIdFactory
// ============================================================

Result<MemorySummaryId> MemorySummaryIdFactory::makeSummaryId(
    const MemorySummaryKey& key,
    pathfinder::foundation::Tick first_tick,
    size_t sequence_index) const {

    auto key_result = key.validateBasic();
    if (key_result.is_error()) {
        return Result<MemorySummaryId>::fail(key_result.errors());
    }

    std::string sanitized_subject = key.subject.subject_id;
    for (char& c : sanitized_subject) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        } else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            c = '_';
        }
    }

    std::ostringstream oss;
    oss << "memsum_"
        << toString(key.owner.kind) << "_"
        << sanitized_subject << "_"
        << toString(key.summary_kind) << "_"
        << first_tick.value() << "_"
        << sequence_index;

    std::string id_str = oss.str();
    if (!pathfinder::foundation::isValidIdString(id_str)) {
        return Result<MemorySummaryId>::fail(
            makeError(ErrorCode::validation_failed, "MemorySummaryIdFactory generated invalid id: " + id_str));
    }

    return Result<MemorySummaryId>::ok(MemorySummaryId(id_str));
}

// ============================================================
// MemorySummaryStore
// ============================================================

Result<void> MemorySummaryStore::put(MemorySummary summary) {
    auto r = summary.validateBasic();
    if (r.is_error()) return r;
    summaries_[summary.summary_id.value()] = std::move(summary);
    return Result<void>::ok();
}

Result<std::optional<MemorySummary>> MemorySummaryStore::find(
    const MemorySummaryId& summary_id) const {
    auto it = summaries_.find(summary_id.value());
    if (it == summaries_.end()) {
        return Result<std::optional<MemorySummary>>::ok(std::nullopt);
    }
    return Result<std::optional<MemorySummary>>::ok(it->second);
}

Result<std::vector<MemorySummary>> MemorySummaryStore::listByOwner(
    const MemoryOwner& owner) const {
    std::vector<MemorySummary> result;
    for (const auto& [id, summary] : summaries_) {
        if (summary.key.owner == owner) {
            result.push_back(summary);
        }
    }
    std::sort(result.begin(), result.end(), [](const MemorySummary& a, const MemorySummary& b) {
        return a.summary_id.value() < b.summary_id.value();
    });
    return Result<std::vector<MemorySummary>>::ok(std::move(result));
}

Result<void> MemorySummaryStore::remove(const MemorySummaryId& summary_id) {
    summaries_.erase(summary_id.value());
    return Result<void>::ok();
}

size_t MemorySummaryStore::size() const {
    return summaries_.size();
}

void MemorySummaryStore::clear() {
    summaries_.clear();
}

} // namespace pathfinder::memory
