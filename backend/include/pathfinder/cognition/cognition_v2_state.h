#pragma once

#include "pathfinder/cognition/cognition_v2_types.h"
#include <vector>
#include <optional>

namespace pathfinder::cognition {

// CognitionStateV2: formal collection of claims and evidence records
class CognitionStateV2 {
public:
    const std::vector<CognitionClaimV2>& claims() const { return claims_; }
    const std::vector<CognitionEvidenceRecord>& evidence_records() const { return evidence_records_; }

    const CognitionClaimV2* findClaim(const CognitionClaimKeyV2& key) const;
    const CognitionClaimV2* findById(const CognitionClaimV2Id& claim_id) const;

    pathfinder::foundation::Result<void> upsertClaim(CognitionClaimV2 claim);
    pathfinder::foundation::Result<void> appendEvidence(CognitionEvidenceRecord evidence);

    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<CognitionClaimV2> claims_;
    std::vector<CognitionEvidenceRecord> evidence_records_;
};

} // namespace pathfinder::cognition
