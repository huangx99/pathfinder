#pragma once

#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include <optional>

namespace pathfinder::cognition {

// CognitionQueryService: read-only query service for V2 cognition state
class CognitionQueryService {
public:
    pathfinder::foundation::Result<CognitionQueryResult> query(
        const CognitionStateV2& state,
        const CognitionQuery& query) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findBestClaim(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target,
        CognitionAspect aspect) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findEdibility(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findUsability(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findRisk(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;
};

} // namespace pathfinder::cognition
