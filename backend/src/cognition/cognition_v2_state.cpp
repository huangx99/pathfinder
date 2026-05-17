#include "pathfinder/cognition/cognition_v2_state.h"
#include <algorithm>

namespace pathfinder::cognition {

const CognitionClaimV2* CognitionStateV2::findClaim(const CognitionClaimKeyV2& key) const {
    for (const auto& c : claims_) {
        if (c.key == key) return &c;
    }
    return nullptr;
}

const CognitionClaimV2* CognitionStateV2::findById(const CognitionClaimV2Id& claim_id) const {
    for (const auto& c : claims_) {
        if (c.claim_id == claim_id) return &c;
    }
    return nullptr;
}

pathfinder::foundation::Result<void> CognitionStateV2::upsertClaim(CognitionClaimV2 claim) {
    auto vr = claim.validateBasic();
    if (vr.is_error()) {
        return vr;
    }
    for (auto& c : claims_) {
        if (c.key == claim.key) {
            c = std::move(claim);
            return pathfinder::foundation::Result<void>::ok();
        }
    }
    claims_.push_back(std::move(claim));
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> CognitionStateV2::appendEvidence(CognitionEvidenceRecord evidence) {
    auto vr = evidence.validateBasic();
    if (vr.is_error()) {
        return vr;
    }
    evidence_records_.push_back(std::move(evidence));
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> CognitionStateV2::validateBasic() const {
    for (const auto& claim : claims_) {
        auto cr = claim.validateBasic();
        if (cr.is_error()) return cr;
    }
    for (const auto& evidence : evidence_records_) {
        auto er = evidence.validateBasic();
        if (er.is_error()) return er;
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::cognition
