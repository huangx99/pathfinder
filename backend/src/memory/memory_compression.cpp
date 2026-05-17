#include "pathfinder/memory/memory_compression.h"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::MemoryId;

// ============================================================
// Helpers
// ============================================================

static bool ownerMatches(const MemoryOwner& a, const MemoryOwner& b) {
    return a == b;
}

static bool subjectMatches(const MemorySubject& a, const MemorySubject& b) {
    return a == b;
}

static bool isCompressibleRecord(const MemoryRecord& record, const MemoryCompressionOptions& options) {
    if (!options.allow_test_only) {
        bool has_test_only = false;
        for (const auto& mk : record.memory_kinds) {
            if (mk == MemoryKind::TestOnly) {
                has_test_only = true;
                break;
            }
        }
        if (has_test_only) return false;
    }

    if (record.scope == MemoryScope::Unknown) return false;
    if (record.lifecycle == MemoryLifecycle::Unknown) return false;

    if (!options.include_forgotten && record.lifecycle == MemoryLifecycle::Forgotten) return false;
    if (!options.include_short_term && record.scope == MemoryScope::ShortTerm) return false;
    if (!options.include_fading && record.lifecycle == MemoryLifecycle::Fading) return false;

    // Protected can participate but must not be deleted (handled by not deleting records)
    if (record.lifecycle == MemoryLifecycle::Protected && !options.include_protected) return false;

    if (record.strength < 0.20) return false;

    return true;
}

static int importanceRank(MemoryImportance importance) {
    switch (importance) {
        case MemoryImportance::Trivial: return 1;
        case MemoryImportance::Normal: return 2;
        case MemoryImportance::Important: return 3;
        case MemoryImportance::Critical: return 4;
        default: return 0;
    }
}

static std::vector<MemoryId> selectRepresentativeMemoryIds(
    const std::vector<MemoryRecord>& records,
    size_t max_count) {

    std::vector<const MemoryRecord*> ptrs;
    ptrs.reserve(records.size());
    for (const auto& r : records) ptrs.push_back(&r);

    std::sort(ptrs.begin(), ptrs.end(), [](const MemoryRecord* a, const MemoryRecord* b) {
        if (a->protect_from_forgetting != b->protect_from_forgetting)
            return a->protect_from_forgetting > b->protect_from_forgetting;
        int rank_a = importanceRank(a->importance);
        int rank_b = importanceRank(b->importance);
        if (rank_a != rank_b) return rank_a > rank_b;
        if (a->strength != b->strength) return a->strength > b->strength;
        if (a->last_touched_tick.value() != b->last_touched_tick.value())
            return a->last_touched_tick.value() > b->last_touched_tick.value();
        return a->memory_id.value() < b->memory_id.value();
    });

    std::vector<MemoryId> result;
    result.reserve(std::min(max_count, ptrs.size()));
    for (size_t i = 0; i < std::min(max_count, ptrs.size()); ++i) {
        result.push_back(ptrs[i]->memory_id);
    }
    return result;
}

// ============================================================
// MemoryCompressionPlanner
// ============================================================

Result<MemoryCompressionPlan> MemoryCompressionPlanner::planForOwnerSubject(
    const std::vector<MemoryRecord>& records,
    const MemorySummaryKey& key,
    const MemoryCompressionOptions& options) const {

    auto opts_result = options.validateBasic();
    if (opts_result.is_error()) return Result<MemoryCompressionPlan>::fail(opts_result.errors());
    auto key_result = key.validateBasic();
    if (key_result.is_error()) return Result<MemoryCompressionPlan>::fail(key_result.errors());

    std::vector<MemoryRecord> filtered;
    filtered.reserve(records.size());
    for (const auto& record : records) {
        if (!ownerMatches(record.owner, key.owner)) continue;
        if (!subjectMatches(record.subject, key.subject)) continue;
        if (!isCompressibleRecord(record, options)) continue;
        filtered.push_back(record);
    }

    if (filtered.size() < options.min_records_to_summarize) {
        return Result<MemoryCompressionPlan>::fail(makeError(ErrorCode::validation_failed,
            "insufficient_records", "filtered count " + std::to_string(filtered.size()) +
            " < min_records_to_summarize " + std::to_string(options.min_records_to_summarize)));
    }

    if (filtered.size() > options.max_source_records) {
        // Sort by last_touched_tick desc, then memory_id asc for determinism
        std::sort(filtered.begin(), filtered.end(), [](const MemoryRecord& a, const MemoryRecord& b) {
            if (a.last_touched_tick.value() != b.last_touched_tick.value())
                return a.last_touched_tick.value() > b.last_touched_tick.value();
            return a.memory_id.value() < b.memory_id.value();
        });
        filtered.resize(options.max_source_records);
    }

    MemoryCompressionPlan plan;
    plan.plan_key = "compress_owner_subject_" + key.subject.subject_id;
    plan.key = key;
    plan.target_level = options.target_level;

    for (const auto& record : filtered) {
        plan.source_memory_ids.push_back(record.memory_id);
    }

    plan.representative_memory_ids = selectRepresentativeMemoryIds(filtered, options.max_representative_records);
    plan.reason_keys = {"owner_subject_match", "record_count:" + std::to_string(filtered.size())};

    return Result<MemoryCompressionPlan>::ok(std::move(plan));
}

// ============================================================
// MemoryCompressionService
// ============================================================

Result<MemoryCompressionResult> MemoryCompressionService::compress(
    const std::vector<MemoryRecord>& records,
    const MemorySummaryKey& key,
    const MemoryCompressionOptions& options) const {

    MemoryCompressionPlanner planner;
    auto plan_result = planner.planForOwnerSubject(records, key, options);
    if (plan_result.is_error()) {
        // If planner failed due to insufficient records, convert to Skipped result
        bool is_insufficient = false;
        size_t filtered_count = 0;
        for (const auto& err : plan_result.errors()) {
            if (err.message_key.find("insufficient_records") != std::string::npos) {
                is_insufficient = true;
                // Try to extract count from debug message if present
                if (err.debug_message.has_value()) {
                    auto pos = err.debug_message->find("filtered count ");
                    if (pos != std::string::npos) {
                        filtered_count = std::stoull(err.debug_message->substr(pos + 15));
                    }
                }
                break;
            }
        }
        if (is_insufficient) {
            MemoryCompressionResult result;
            result.decision = MemoryCompressionDecision::Skipped;
            result.reason_keys = {"skipped", "insufficient_records", "count:" + std::to_string(filtered_count)};
            return Result<MemoryCompressionResult>::ok(std::move(result));
        }
        return Result<MemoryCompressionResult>::fail(plan_result.errors());
    }

    MemoryCompressionPlan plan = plan_result.value();
    MemoryCompressionResult result;

    if (plan.source_memory_ids.size() < options.min_records_to_summarize) {
        result.decision = MemoryCompressionDecision::Skipped;
        result.reason_keys = {"skipped", "insufficient_records", "count:" + std::to_string(plan.source_memory_ids.size())};
        return Result<MemoryCompressionResult>::ok(std::move(result));
    }

    // Gather source records
    std::vector<MemoryRecord> source_records;
    source_records.reserve(plan.source_memory_ids.size());
    for (const auto& record : records) {
        for (const auto& id : plan.source_memory_ids) {
            if (record.memory_id == id) {
                source_records.push_back(record);
                break;
            }
        }
    }

    if (source_records.empty()) {
        result.decision = MemoryCompressionDecision::Skipped;
        result.reason_keys = {"skipped", "no_matching_records"};
        return Result<MemoryCompressionResult>::ok(std::move(result));
    }

    result.plan = plan;

    // Determine first/last tick
    Tick first_tick = source_records[0].created_tick;
    Tick last_tick = source_records[0].last_touched_tick;
    for (const auto& r : source_records) {
        if (r.created_tick.value() < first_tick.value()) first_tick = r.created_tick;
        if (r.last_touched_tick.value() > last_tick.value()) last_tick = r.last_touched_tick;
    }

    // Statistics
    size_t success_count = 0;
    size_t failure_count = 0;
    size_t warning_count = 0;
    size_t accident_count = 0;
    size_t contradiction_count = 0;
    size_t teaching_count = 0;
    double max_strength = 0.0;
    double sum_strength = 0.0;
    MemoryImportance highest_importance = MemoryImportance::Trivial;
    bool has_protected = false;
    bool has_long_term = false;
    std::vector<MemoryEvidenceRef> all_evidence;

    for (const auto& r : source_records) {
        for (const auto& mk : r.memory_kinds) {
            switch (mk) {
                case MemoryKind::Success: ++success_count; break;
                case MemoryKind::Failure: ++failure_count; break;
                case MemoryKind::Warning: ++warning_count; break;
                case MemoryKind::Accident: ++accident_count; break;
                case MemoryKind::Contradiction: ++contradiction_count; break;
                default: break;
            }
        }
        bool is_teaching = r.teaching_candidate;
        if (!is_teaching) {
            for (const auto& mk : r.memory_kinds) {
                if (mk == MemoryKind::TeachingRelated) { is_teaching = true; break; }
            }
        }
        if (is_teaching) ++teaching_count;
        if (r.strength > max_strength) max_strength = r.strength;
        sum_strength += r.strength;
        if (importanceRank(r.importance) > importanceRank(highest_importance)) {
            highest_importance = r.importance;
        }
        if (r.lifecycle == MemoryLifecycle::Protected) has_protected = true;
        if (r.scope == MemoryScope::LongTerm) has_long_term = true;
        for (const auto& ref : r.evidence_refs) {
            all_evidence.push_back(ref);
        }
    }

    double avg_strength = source_records.empty() ? 0.0 : sum_strength / static_cast<double>(source_records.size());
    double repetition_bonus = std::min(static_cast<double>(source_records.size()) / 10.0, 1.0);
    double aggregate_strength = max_strength * 0.50 + avg_strength * 0.30 + repetition_bonus * 0.20;
    aggregate_strength = std::max(0.0, std::min(1.0, aggregate_strength));

    // Build summary
    MemorySummary summary;
    MemorySummaryIdFactory id_factory;
    auto id_result = id_factory.makeSummaryId(key, first_tick, 0);
    if (id_result.is_error()) {
        return Result<MemoryCompressionResult>::fail(id_result.errors());
    }
    summary.summary_id = id_result.value();
    summary.key = key;
    summary.compression_level = options.target_level;
    summary.highest_importance = highest_importance;
    summary.aggregate_strength_band = strengthToBand(aggregate_strength);
    summary.aggregate_strength = aggregate_strength;
    summary.source_memory_count = source_records.size();
    summary.success_count = success_count;
    summary.failure_count = failure_count;
    summary.warning_count = warning_count;
    summary.accident_count = accident_count;
    summary.contradiction_count = contradiction_count;
    summary.teaching_candidate_count = teaching_count;
    summary.has_protected_source = has_protected;
    summary.has_long_term_source = has_long_term;
    summary.first_observed_tick = first_tick;
    summary.last_observed_tick = last_tick;
    summary.summarized_tick = Tick(last_tick.value()); // use last tick as summary tick
    summary.source_memory_ids = plan.source_memory_ids;
    summary.representative_memory_ids = plan.representative_memory_ids;

    // Deduplicate evidence refs by comprehensive key
    std::vector<MemoryEvidenceRef> deduped_evidence;
    std::vector<std::string> seen_evidence_keys;
    for (const auto& ref : all_evidence) {
        std::string key_str = "sk:" + std::to_string(static_cast<int>(ref.source_kind))
                            + ":cei:" + ref.cognition_evidence_id.value()
                            + ":cci:" + ref.cognition_claim_id.value()
                            + ":sei:" + ref.source_event_id.value()
                            + ":ot:" + std::to_string(ref.observed_tick.value());
        if (std::find(seen_evidence_keys.begin(), seen_evidence_keys.end(), key_str) == seen_evidence_keys.end()) {
            seen_evidence_keys.push_back(key_str);
            deduped_evidence.push_back(ref);
        }
    }
    if (deduped_evidence.empty()) {
        result.decision = MemoryCompressionDecision::Skipped;
        result.reason_keys = {"skipped", "missing_evidence_refs"};
        return Result<MemoryCompressionResult>::ok(std::move(result));
    }
    summary.evidence_refs = std::move(deduped_evidence);
    summary.reason_keys = {"compression_created", "owner_subject", "record_count:" + std::to_string(source_records.size())};

    auto summary_validate = summary.validateBasic();
    if (summary_validate.is_error()) {
        return Result<MemoryCompressionResult>::fail(summary_validate.errors());
    }

    result.decision = MemoryCompressionDecision::CreatedSummary;
    result.summary = std::move(summary);

    // Event draft
    MemoryEventDraft event_draft;
    event_draft.event_key = "memory.summary_created";
    event_draft.memory_id = result.summary->source_memory_ids.empty() ? MemoryId() : result.summary->source_memory_ids[0];
    event_draft.owner = key.owner;
    event_draft.subject = key.subject;
    event_draft.scope = MemoryScope::MidTerm;
    event_draft.lifecycle = MemoryLifecycle::Active;
    event_draft.write_decision = MemoryWriteDecision::Created;
    event_draft.reason_keys = {"memory.summary_created"};
    result.event_drafts.push_back(std::move(event_draft));

    // State change draft
    MemoryStateChangeDraft state_change;
    state_change.change_key = "memory.summary.create";
    state_change.memory_id = result.summary->source_memory_ids.empty() ? MemoryId() : result.summary->source_memory_ids[0];
    result.state_changes.push_back(std::move(state_change));

    result.reason_keys = {"created_summary", "record_count:" + std::to_string(source_records.size())};

    return Result<MemoryCompressionResult>::ok(std::move(result));
}

} // namespace pathfinder::memory
