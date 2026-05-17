#include "pathfinder/knowledge/knowledge_projection.h"
#include <algorithm>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// KnowledgeProjectionItem
// ============================================================

Result<void> KnowledgeProjectionItem::validateBasic() const {
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjectionItem knowledge_id is empty"));
    }
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    auto predicate_result = predicate.validateBasic();
    if (predicate_result.is_error()) return predicate_result;
    auto confidence_result = confidence.validateBasic();
    if (confidence_result.is_error()) return confidence_result;
    if (status == KnowledgeStatus::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjectionItem status is Unknown"));
    }
    auto tp_result = teaching_profile.validateBasic();
    if (tp_result.is_error()) return tp_result;
    auto pf_result = projection_flags.validateBasic();
    if (pf_result.is_error()) return pf_result;
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjectionItem reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeProjection
// ============================================================

Result<void> KnowledgeProjection::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    for (const auto& item : items) {
        auto r = item.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjection reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjection warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeProjectionBuilder
// ============================================================

Result<KnowledgeProjection> KnowledgeProjectionBuilder::buildProjection(
    const std::vector<KnowledgeClaim>& claims,
    const KnowledgeQuery& query) const {

    auto query_result = query.validateBasic();
    if (query_result.is_error()) {
        return Result<KnowledgeProjection>::fail(query_result.errors());
    }

    KnowledgeProjection projection;
    projection.owner = query.owner;

    // Filter and transform claims
    std::vector<KnowledgeProjectionItem> items;
    for (const auto& claim : claims) {
        // Owner filter
        if (claim.owner != query.owner) continue;

        // Subject filter
        if (query.subject.has_value()) {
            if (claim.subject != query.subject.value()) continue;
        }

        // Relation type filter
        if (query.relation_type.has_value()) {
            if (claim.predicate.relation_type != query.relation_type.value()) continue;
        }

        // Status filter
        if (!query.include_inactive) {
            if (claim.status == KnowledgeStatus::Candidate || claim.status == KnowledgeStatus::Hypothesis) continue;
        }

        // Teachable filter
        if (!query.include_teachable) {
            if (claim.status == KnowledgeStatus::Teachable) continue;
        }

        // Min confidence band filter
        if (query.min_confidence_band.has_value()) {
            auto required = static_cast<int>(query.min_confidence_band.value());
            auto actual = static_cast<int>(claim.confidence.band);
            if (actual < required) continue;
        }

        // Mode-specific filters
        if (query.mode == KnowledgeQueryMode::Actionable) {
            if (!claim.projection_flags.usable_for_action) continue;
        }
        if (query.mode == KnowledgeQueryMode::Teachable) {
            if (!claim.teaching_profile.teachable) continue;
        }
        if (query.mode == KnowledgeQueryMode::RiskRelated) {
            if (claim.predicate.relation_type != KnowledgeRelationType::HasRisk &&
                claim.predicate.relation_type != KnowledgeRelationType::IsDangerousUnder) continue;
        }
        if (query.mode == KnowledgeQueryMode::HighConfidence) {
            if (claim.confidence.confidence < 0.75) continue;
        }

        // Projection must not expose full evidence internals by default
        KnowledgeProjectionItem item;
        item.knowledge_id = claim.knowledge_id;
        item.subject = claim.subject;
        item.predicate = claim.predicate;
        item.conditions = claim.conditions;
        item.confidence = claim.confidence;
        item.status = claim.status;
        item.teaching_profile = claim.teaching_profile;
        item.projection_flags = claim.projection_flags;
        items.push_back(item);
    }

    // Deterministic sort: status rank desc, confidence desc, updated_tick desc, knowledge_id asc
    std::sort(items.begin(), items.end(), [](const KnowledgeProjectionItem& a, const KnowledgeProjectionItem& b) {
        auto getRank = [](KnowledgeStatus status) -> int {
            switch (status) {
                case KnowledgeStatus::Teachable: return 5;
                case KnowledgeStatus::Active: return 4;
                case KnowledgeStatus::Hypothesis: return 3;
                case KnowledgeStatus::Candidate: return 2;
                case KnowledgeStatus::Shared: return 1;
                case KnowledgeStatus::Operational: return 1;
                case KnowledgeStatus::Deprecated: return 0;
                case KnowledgeStatus::Disproven: return 0;
                default: return -1;
            }
        };
        int rank_a = getRank(a.status);
        int rank_b = getRank(b.status);
        if (rank_a != rank_b) return rank_a > rank_b;
        if (a.confidence.confidence != b.confidence.confidence) return a.confidence.confidence > b.confidence.confidence;
        return a.knowledge_id.value() < b.knowledge_id.value();
    });

    if (items.size() > query.limit) {
        projection.truncated = true;
        items.resize(query.limit);
    }

    projection.items = std::move(items);
    return Result<KnowledgeProjection>::ok(std::move(projection));
}

} // namespace pathfinder::knowledge
