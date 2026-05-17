#include "pathfinder/cognition/cognition_query.h"
#include <algorithm>

namespace pathfinder::cognition {

using pathfinder::foundation::Result;

// ============================================================
// Helpers
// ============================================================

static bool matchesKey(const CognitionClaimV2& claim, const CognitionQuery& query) {
    if (claim.key.subject != query.subject) return false;
    if (query.target && claim.key.target != *query.target) return false;
    if (query.action_id && claim.key.action_id != *query.action_id) return false;
    if (query.action_context && claim.key.action_context != *query.action_context) return false;
    if (query.aspect && claim.key.aspect != *query.aspect) return false;
    return true;
}

static bool isBetterClaim(const CognitionClaimV2& a, const CognitionClaimV2& b) {
    // Prefer non-conflicted
    if (a.conflicted != b.conflicted) return !a.conflicted;
    // Then higher confidence
    if (a.confidence != b.confidence) return a.confidence > b.confidence;
    // Then higher evidence count
    return a.evidence_count > b.evidence_count;
}

// ============================================================
// CognitionQueryService
// ============================================================

Result<CognitionQueryResult> CognitionQueryService::query(
    const CognitionStateV2& state,
    const CognitionQuery& query) const {

    auto vr = query.validateBasic();
    if (vr.is_error()) {
        return Result<CognitionQueryResult>::fail(vr.errors()[0]);
    }

    CognitionQueryResult result;
    result.query = query;

    // Collect matching claims
    std::vector<CognitionClaimV2> matched;
    for (const auto& claim : state.claims()) {
        if (!matchesKey(claim, query)) continue;
        if (claim.confidence < query.min_confidence) continue;
        if (!query.include_conflicted && claim.conflicted) continue;
        if (query.mode == CognitionQueryMode::TeachableCandidates &&
            claim.confidence_band != CognitionConfidenceBand::Teachable) {
            continue;
        }
        matched.push_back(claim);
    }

    // Sort by quality
    std::sort(matched.begin(), matched.end(), isBetterClaim);

    // Apply max_results
    if (matched.size() > query.max_results) {
        matched.resize(query.max_results);
    }

    result.claims = std::move(matched);

    // Determine best claim
    if (!result.claims.empty()) {
        result.best_claim = result.claims[0];
    }

    return Result<CognitionQueryResult>::ok(std::move(result));
}

Result<std::optional<CognitionClaimV2>> CognitionQueryService::findBestClaim(
    const CognitionStateV2& state,
    const CognitionSubject& subject,
    const CognitionTarget& target,
    CognitionAspect aspect) const {

    auto sr = subject.validateBasic();
    if (sr.is_error()) {
        return Result<std::optional<CognitionClaimV2>>::fail(sr.errors()[0]);
    }
    auto tr = target.validateBasic();
    if (tr.is_error()) {
        return Result<std::optional<CognitionClaimV2>>::fail(tr.errors()[0]);
    }

    CognitionQuery q;
    q.subject = subject;
    q.target = target;
    q.aspect = aspect;
    q.mode = CognitionQueryMode::ExactTarget;

    auto qr = query(state, q);
    if (qr.is_error()) {
        return Result<std::optional<CognitionClaimV2>>::fail(qr.errors()[0]);
    }

    return Result<std::optional<CognitionClaimV2>>::ok(qr.value().best_claim);
}

Result<std::optional<CognitionClaimV2>> CognitionQueryService::findEdibility(
    const CognitionStateV2& state,
    const CognitionSubject& subject,
    const CognitionTarget& target) const {
    return findBestClaim(state, subject, target, CognitionAspect::Edibility);
}

Result<std::optional<CognitionClaimV2>> CognitionQueryService::findUsability(
    const CognitionStateV2& state,
    const CognitionSubject& subject,
    const CognitionTarget& target) const {
    return findBestClaim(state, subject, target, CognitionAspect::Usability);
}

Result<std::optional<CognitionClaimV2>> CognitionQueryService::findRisk(
    const CognitionStateV2& state,
    const CognitionSubject& subject,
    const CognitionTarget& target) const {
    return findBestClaim(state, subject, target, CognitionAspect::Risk);
}

} // namespace pathfinder::cognition
