#include "pathfinder/knowledge/knowledge_repository.h"
#include <algorithm>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// KnowledgeQuery
// ============================================================

Result<void> KnowledgeQuery::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    if (mode == KnowledgeQueryMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery mode is Unknown"));
    }
    if (mode == KnowledgeQueryMode::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery mode TestOnly not allowed"));
    }
    if (subject.has_value()) {
        auto subject_result = subject->validateBasic();
        if (subject_result.is_error()) return subject_result;
    }
    if (relation_type.has_value()) {
        if (relation_type.value() == KnowledgeRelationType::Unknown ||
            relation_type.value() == KnowledgeRelationType::TestOnly) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery relation_type is invalid"));
        }
    }
    if (min_status.has_value()) {
        auto s = min_status.value();
        if (s == KnowledgeStatus::Unknown || s == KnowledgeStatus::TestOnly ||
            s == KnowledgeStatus::Deprecated || s == KnowledgeStatus::Disproven) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery min_status is invalid"));
        }
    }
    if (min_confidence_band.has_value()) {
        if (min_confidence_band.value() == KnowledgeConfidenceBand::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery min_confidence_band is Unknown"));
        }
    }
    if (limit == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery limit is 0"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeQuery reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRepository
// ============================================================

static int statusRank(KnowledgeStatus status) {
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
}

Result<void> KnowledgeRepository::put(KnowledgeClaim claim) {
    auto r = claim.validateBasic();
    if (r.is_error()) return r;
    claims_[claim.knowledge_id.value()] = std::move(claim);
    return Result<void>::ok();
}

Result<std::optional<KnowledgeClaim>> KnowledgeRepository::find(
    const pathfinder::foundation::KnowledgeId& knowledge_id) const {
    auto it = claims_.find(knowledge_id.value());
    if (it == claims_.end()) {
        return Result<std::optional<KnowledgeClaim>>::ok(std::nullopt);
    }
    return Result<std::optional<KnowledgeClaim>>::ok(it->second);
}

Result<std::vector<KnowledgeClaim>> KnowledgeRepository::query(
    const KnowledgeQuery& query) const {
    auto query_result = query.validateBasic();
    if (query_result.is_error()) {
        return Result<std::vector<KnowledgeClaim>>::fail(query_result.errors());
    }

    std::vector<KnowledgeClaim> result;
    for (const auto& [id, claim] : claims_) {
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

        // Min status filter
        if (query.min_status.has_value()) {
            if (statusRank(claim.status) < statusRank(query.min_status.value())) continue;
        }

        // Min confidence band filter
        if (query.min_confidence_band.has_value()) {
            auto required = static_cast<int>(query.min_confidence_band.value());
            auto actual = static_cast<int>(claim.confidence.band);
            if (actual < required) continue;
        }

        // Teachable filter
        if (!query.include_teachable) {
            if (claim.status == KnowledgeStatus::Teachable) continue;
        }

        // Inactive filter
        if (!query.include_inactive) {
            if (claim.status == KnowledgeStatus::Candidate || claim.status == KnowledgeStatus::Hypothesis) continue;
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

        result.push_back(claim);
    }

    // Deterministic sort: status rank desc, confidence desc, updated_tick desc, knowledge_id asc
    std::sort(result.begin(), result.end(), [](const KnowledgeClaim& a, const KnowledgeClaim& b) {
        int rank_a = statusRank(a.status);
        int rank_b = statusRank(b.status);
        if (rank_a != rank_b) return rank_a > rank_b;
        if (a.confidence.confidence != b.confidence.confidence) return a.confidence.confidence > b.confidence.confidence;
        if (a.updated_tick.value() != b.updated_tick.value()) return a.updated_tick.value() > b.updated_tick.value();
        return a.knowledge_id.value() < b.knowledge_id.value();
    });

    if (result.size() > query.limit) {
        result.resize(query.limit);
    }

    return Result<std::vector<KnowledgeClaim>>::ok(std::move(result));
}

Result<std::vector<KnowledgeClaim>> KnowledgeRepository::listByOwner(
    const KnowledgeOwner& owner) const {
    std::vector<KnowledgeClaim> result;
    for (const auto& [id, claim] : claims_) {
        if (claim.owner == owner) {
            result.push_back(claim);
        }
    }
    std::sort(result.begin(), result.end(), [](const KnowledgeClaim& a, const KnowledgeClaim& b) {
        int rank_a = statusRank(a.status);
        int rank_b = statusRank(b.status);
        if (rank_a != rank_b) return rank_a > rank_b;
        if (a.confidence.confidence != b.confidence.confidence) return a.confidence.confidence > b.confidence.confidence;
        if (a.updated_tick.value() != b.updated_tick.value()) return a.updated_tick.value() > b.updated_tick.value();
        return a.knowledge_id.value() < b.knowledge_id.value();
    });
    return Result<std::vector<KnowledgeClaim>>::ok(std::move(result));
}

Result<void> KnowledgeRepository::remove(const pathfinder::foundation::KnowledgeId& knowledge_id) {
    claims_.erase(knowledge_id.value());
    return Result<void>::ok();
}

size_t KnowledgeRepository::size() const {
    return claims_.size();
}

void KnowledgeRepository::clear() {
    claims_.clear();
}

} // namespace pathfinder::knowledge
