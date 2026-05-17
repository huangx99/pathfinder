#include "pathfinder/memory/memory_recall.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::MemoryId;

// ============================================================
// Helpers
// ============================================================

static int importanceRank(MemoryImportance importance) {
    switch (importance) {
        case MemoryImportance::Trivial: return 1;
        case MemoryImportance::Normal: return 2;
        case MemoryImportance::Important: return 3;
        case MemoryImportance::Critical: return 4;
        default: return 0;
    }
}

static bool recordMatchesQuery(const MemoryRecord& record, const MemoryRecallQuery& query) {
    if (!(record.owner == query.owner)) return false;
    if (query.subject.has_value() && !(record.subject == *query.subject)) return false;
    if (!query.include_forgotten && record.lifecycle == MemoryLifecycle::Forgotten) return false;
    if (query.scope.has_value() && record.scope != *query.scope) return false;
    if (query.min_importance.has_value() && importanceRank(record.importance) < importanceRank(*query.min_importance)) return false;

    if (!query.memory_kinds.empty()) {
        bool kind_match = false;
        for (const auto& qk : query.memory_kinds) {
            for (const auto& rk : record.memory_kinds) {
                if (qk == rk) { kind_match = true; break; }
            }
            if (kind_match) break;
        }
        if (!kind_match) return false;
    }

    // Mode-specific filters
    switch (query.mode) {
        case MemoryRecallMode::ExactSubject:
            if (!query.subject.has_value()) return false;
            if (!(record.subject == *query.subject)) return false;
            break;
        case MemoryRecallMode::OwnerRecent:
            // owner already matched, accept all non-forgotten
            break;
        case MemoryRecallMode::ByMemoryKind:
            if (query.memory_kinds.empty()) return false; // must have memory_kinds
            break;
        case MemoryRecallMode::ImportantOrCritical:
            if (importanceRank(record.importance) < importanceRank(MemoryImportance::Important)) return false;
            break;
        case MemoryRecallMode::TeachingCandidates: {
            bool is_teaching = record.teaching_candidate;
            if (!is_teaching) {
                for (const auto& mk : record.memory_kinds) {
                    if (mk == MemoryKind::TeachingRelated) { is_teaching = true; break; }
                }
            }
            if (!is_teaching) return false;
            break;
        }
        case MemoryRecallMode::RiskRelated: {
            bool is_risk = importanceRank(record.importance) >= importanceRank(MemoryImportance::Critical);
            if (!is_risk) {
                for (const auto& mk : record.memory_kinds) {
                    if (mk == MemoryKind::Warning || mk == MemoryKind::Accident) { is_risk = true; break; }
                }
            }
            if (!is_risk) return false;
            break;
        }
        case MemoryRecallMode::ContradictionRelated: {
            bool is_contra = record.contradiction_count > 0;
            if (!is_contra) {
                for (const auto& mk : record.memory_kinds) {
                    if (mk == MemoryKind::Contradiction) { is_contra = true; break; }
                }
            }
            if (!is_contra) return false;
            break;
        }
        case MemoryRecallMode::LongTermOnly:
            if (record.scope != MemoryScope::LongTerm) return false;
            break;
        default:
            break;
    }

    if (query.teaching_only) {
        bool is_teaching = record.teaching_candidate;
        if (!is_teaching) {
            for (const auto& mk : record.memory_kinds) {
                if (mk == MemoryKind::TeachingRelated) { is_teaching = true; break; }
            }
        }
        if (!is_teaching) return false;
    }

    if (query.contradiction_only) {
        bool is_contra = record.contradiction_count > 0;
        if (!is_contra) {
            for (const auto& mk : record.memory_kinds) {
                if (mk == MemoryKind::Contradiction) { is_contra = true; break; }
            }
        }
        if (!is_contra) return false;
    }

    return true;
}

static bool summaryMatchesQuery(const MemorySummary& summary, const MemoryRecallQuery& query) {
    if (!(summary.key.owner == query.owner)) return false;
    if (query.subject.has_value() && !(summary.key.subject == *query.subject)) return false;
    if (query.scope.has_value()) {
        // Summary doesn't have direct scope, but we can infer from has_long_term_source
        if (*query.scope == MemoryScope::LongTerm && !summary.has_long_term_source) return false;
    }
    if (query.min_importance.has_value() && importanceRank(summary.highest_importance) < importanceRank(*query.min_importance)) return false;

    if (!query.memory_kinds.empty()) {
        bool kind_match = false;
        for (const auto& qk : query.memory_kinds) {
            for (const auto& sk : summary.key.memory_kinds) {
                if (qk == sk) { kind_match = true; break; }
            }
            if (kind_match) break;
        }
        if (!kind_match) return false;
    }

    switch (query.mode) {
        case MemoryRecallMode::ExactSubject:
            if (!query.subject.has_value()) return false;
            if (!(summary.key.subject == *query.subject)) return false;
            break;
        case MemoryRecallMode::OwnerRecent:
            break;
        case MemoryRecallMode::ByMemoryKind:
            if (query.memory_kinds.empty()) return false;
            break;
        case MemoryRecallMode::ImportantOrCritical:
            if (importanceRank(summary.highest_importance) < importanceRank(MemoryImportance::Important)) return false;
            break;
        case MemoryRecallMode::TeachingCandidates:
            if (summary.teaching_candidate_count == 0) return false;
            break;
        case MemoryRecallMode::RiskRelated:
            if (summary.warning_count == 0 && summary.accident_count == 0 && importanceRank(summary.highest_importance) < importanceRank(MemoryImportance::Critical)) return false;
            break;
        case MemoryRecallMode::ContradictionRelated:
            if (summary.contradiction_count == 0) return false;
            break;
        case MemoryRecallMode::LongTermOnly:
            if (!summary.has_long_term_source) return false;
            break;
        default:
            break;
    }

    if (query.teaching_only && summary.teaching_candidate_count == 0) return false;
    if (query.contradiction_only && summary.contradiction_count == 0) return false;

    return true;
}

static double calculateRecordRelevance(const MemoryRecord& record, const MemoryRecallQuery& query, Tick max_tick) {
    double strength_score = record.strength;
    double importance_score = importanceRank(record.importance) / 4.0;
    double recency_score = 0.0;
    if (max_tick.value() > 0) {
        recency_score = static_cast<double>(record.last_touched_tick.value()) / static_cast<double>(max_tick.value());
    }
    recency_score = std::max(0.0, std::min(1.0, recency_score));

    double mode_match_score = 0.0;
    switch (query.mode) {
        case MemoryRecallMode::ExactSubject:
            mode_match_score = query.subject.has_value() && (record.subject == *query.subject) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::ByMemoryKind:
            mode_match_score = 1.0; // already filtered
            break;
        case MemoryRecallMode::ImportantOrCritical:
            mode_match_score = importanceRank(record.importance) >= importanceRank(MemoryImportance::Important) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::TeachingCandidates:
            mode_match_score = record.teaching_candidate ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::RiskRelated:
            mode_match_score = (record.importance == MemoryImportance::Critical) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::ContradictionRelated:
            mode_match_score = (record.contradiction_count > 0) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::LongTermOnly:
            mode_match_score = (record.scope == MemoryScope::LongTerm) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::OwnerRecent:
            mode_match_score = 1.0;
            break;
        default:
            mode_match_score = 0.0;
            break;
    }

    double teaching_or_risk_bonus = 0.0;
    if (record.teaching_candidate) teaching_or_risk_bonus = 0.05;
    for (const auto& mk : record.memory_kinds) {
        if (mk == MemoryKind::TeachingRelated) { teaching_or_risk_bonus = 0.05; break; }
        if (mk == MemoryKind::Warning || mk == MemoryKind::Accident) { teaching_or_risk_bonus = 0.05; break; }
    }

    double score = strength_score * 0.35
                 + importance_score * 0.25
                 + recency_score * 0.15
                 + mode_match_score * 0.20
                 + teaching_or_risk_bonus;
    return std::max(0.0, std::min(1.0, score));
}

static double calculateSummaryRelevance(const MemorySummary& summary, const MemoryRecallQuery& query, Tick max_tick) {
    double strength_score = summary.aggregate_strength;
    double importance_score = importanceRank(summary.highest_importance) / 4.0;
    double recency_score = 0.0;
    if (max_tick.value() > 0) {
        recency_score = static_cast<double>(summary.last_observed_tick.value()) / static_cast<double>(max_tick.value());
    }
    recency_score = std::max(0.0, std::min(1.0, recency_score));

    double mode_match_score = 0.0;
    switch (query.mode) {
        case MemoryRecallMode::ExactSubject:
            mode_match_score = query.subject.has_value() && (summary.key.subject == *query.subject) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::ByMemoryKind:
            mode_match_score = 1.0;
            break;
        case MemoryRecallMode::ImportantOrCritical:
            mode_match_score = importanceRank(summary.highest_importance) >= importanceRank(MemoryImportance::Important) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::TeachingCandidates:
            mode_match_score = summary.teaching_candidate_count > 0 ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::RiskRelated:
            mode_match_score = (summary.warning_count > 0 || summary.accident_count > 0) ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::ContradictionRelated:
            mode_match_score = summary.contradiction_count > 0 ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::LongTermOnly:
            mode_match_score = summary.has_long_term_source ? 1.0 : 0.0;
            break;
        case MemoryRecallMode::OwnerRecent:
            mode_match_score = 1.0;
            break;
        default:
            mode_match_score = 0.0;
            break;
    }

    double teaching_or_risk_bonus = 0.0;
    if (summary.teaching_candidate_count > 0) teaching_or_risk_bonus = 0.05;
    else if (summary.warning_count > 0 || summary.accident_count > 0) teaching_or_risk_bonus = 0.05;

    double score = strength_score * 0.35
                 + importance_score * 0.25
                 + recency_score * 0.15
                 + mode_match_score * 0.20
                 + teaching_or_risk_bonus;
    return std::max(0.0, std::min(1.0, score));
}

// ============================================================
// MemoryRecallService
// ============================================================

Result<MemoryRecallResult> MemoryRecallService::recall(
    const std::vector<MemoryRecord>& records,
    const std::vector<MemorySummary>& summaries,
    const MemoryRecallQuery& query) const {

    auto query_result = query.validateBasic();
    if (query_result.is_error()) {
        return Result<MemoryRecallResult>::fail(query_result.errors());
    }

    // Compute max tick for recency normalization
    Tick max_tick;
    for (const auto& r : records) {
        if (r.last_touched_tick.value() > max_tick.value()) max_tick = r.last_touched_tick;
    }
    for (const auto& s : summaries) {
        if (s.last_observed_tick.value() > max_tick.value()) max_tick = s.last_observed_tick;
    }

    MemoryRecallResult result;
    result.query = query;

    // Collect record items
    if (query.include_records) {
        for (const auto& record : records) {
            if (recordMatchesQuery(record, query)) {
                MemoryRecallItem item;
                item.item_kind = MemoryRecallItemKind::Record;
                item.record = record;
                item.relevance_score = calculateRecordRelevance(record, query, max_tick);
                result.items.push_back(std::move(item));
                ++result.scanned_record_count;
            }
        }
    }

    // Collect summary items
    if (query.include_summaries) {
        for (const auto& summary : summaries) {
            if (summaryMatchesQuery(summary, query)) {
                MemoryRecallItem item;
                item.item_kind = MemoryRecallItemKind::Summary;
                item.summary = summary;
                item.relevance_score = calculateSummaryRelevance(summary, query, max_tick);
                result.items.push_back(std::move(item));
                ++result.scanned_summary_count;
            }
        }
    }

    // Stable sort
    auto sort_key = query.sort;
    std::sort(result.items.begin(), result.items.end(),
        [sort_key](const MemoryRecallItem& a, const MemoryRecallItem& b) {
            // Primary: query.sort
            bool a_less = false;
            bool b_less = false;
            bool is_ascending = false;
            switch (sort_key) {
                case MemoryRecallSort::StrengthDesc:
                    if (a.relevance_score != b.relevance_score) {
                        a_less = a.relevance_score < b.relevance_score;
                        b_less = b.relevance_score < a.relevance_score;
                    }
                    break;
                case MemoryRecallSort::LastTouchedDesc: {
                    Tick a_tick = a.record.has_value() ? a.record->last_touched_tick
                                     : (a.summary.has_value() ? a.summary->last_observed_tick : Tick(0));
                    Tick b_tick = b.record.has_value() ? b.record->last_touched_tick
                                     : (b.summary.has_value() ? b.summary->last_observed_tick : Tick(0));
                    if (a_tick.value() != b_tick.value()) {
                        a_less = a_tick.value() < b_tick.value();
                        b_less = b_tick.value() < a_tick.value();
                    }
                    break;
                }
                case MemoryRecallSort::CreatedDesc: {
                    Tick a_tick = a.record.has_value() ? a.record->created_tick : Tick(0);
                    Tick b_tick = b.record.has_value() ? b.record->created_tick : Tick(0);
                    if (a_tick.value() != b_tick.value()) {
                        a_less = a_tick.value() < b_tick.value();
                        b_less = b_tick.value() < a_tick.value();
                    }
                    break;
                }
                case MemoryRecallSort::ImportanceDesc: {
                    int a_imp = a.record.has_value() ? importanceRank(a.record->importance)
                                : (a.summary.has_value() ? importanceRank(a.summary->highest_importance) : 0);
                    int b_imp = b.record.has_value() ? importanceRank(b.record->importance)
                                : (b.summary.has_value() ? importanceRank(b.summary->highest_importance) : 0);
                    if (a_imp != b_imp) {
                        a_less = a_imp < b_imp;
                        b_less = b_imp < a_imp;
                    }
                    break;
                }
                case MemoryRecallSort::EvidenceCountDesc: {
                    size_t a_ev = a.record.has_value() ? a.record->evidence_refs.size()
                                  : (a.summary.has_value() ? a.summary->evidence_refs.size() : 0);
                    size_t b_ev = b.record.has_value() ? b.record->evidence_refs.size()
                                  : (b.summary.has_value() ? b.summary->evidence_refs.size() : 0);
                    if (a_ev != b_ev) {
                        a_less = a_ev < b_ev;
                        b_less = b_ev < a_ev;
                    }
                    break;
                }
                case MemoryRecallSort::DeterministicKeyAsc: {
                    std::string a_id = a.record.has_value() ? a.record->memory_id.value()
                                       : (a.summary.has_value() ? a.summary->summary_id.value() : "");
                    std::string b_id = b.record.has_value() ? b.record->memory_id.value()
                                       : (b.summary.has_value() ? b.summary->summary_id.value() : "");
                    a_less = a_id < b_id;
                    b_less = b_id < a_id;
                    is_ascending = true;
                    break;
                }
                default:
                    break;
            }
            if (a_less != b_less) return is_ascending ? a_less : b_less;

            // Secondary: relevance_score desc
            if (a.relevance_score != b.relevance_score) {
                return a.relevance_score > b.relevance_score;
            }

            // Tertiary: last touched / summarized desc
            {
                Tick a_tick = a.record.has_value() ? a.record->last_touched_tick
                                 : (a.summary.has_value() ? a.summary->last_observed_tick : Tick(0));
                Tick b_tick = b.record.has_value() ? b.record->last_touched_tick
                                 : (b.summary.has_value() ? b.summary->last_observed_tick : Tick(0));
                if (a_tick.value() != b_tick.value()) return a_tick.value() > b_tick.value();
            }

            // Final tie-break: ID asc
            std::string a_id = a.record.has_value() ? a.record->memory_id.value()
                               : (a.summary.has_value() ? a.summary->summary_id.value() : "");
            std::string b_id = b.record.has_value() ? b.record->memory_id.value()
                               : (b.summary.has_value() ? b.summary->summary_id.value() : "");
            return a_id < b_id;
        });

    // Limit truncation
    if (result.items.size() > query.limit) {
        result.items.resize(query.limit);
        result.truncated = true;
    }

    return Result<MemoryRecallResult>::ok(std::move(result));
}

} // namespace pathfinder::memory
