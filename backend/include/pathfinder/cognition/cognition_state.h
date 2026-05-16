#pragma once

#include "pathfinder/cognition/types.h"
#include <vector>
#include <optional>

namespace pathfinder::cognition {

// CognitionClaim: a subject's belief about an object+action+effect
struct CognitionClaim {
    CognitionKey key;
    double confidence = 0.0;
    int evidence_count = 0;
    EventId last_event_id;

    bool operator==(const CognitionClaim& other) const = default;
};

// CognitionState: collection of claims for a subject
class CognitionState {
public:
    const std::vector<CognitionClaim>& claims() const { return claims_; }

    const CognitionClaim* findClaim(const CognitionKey& key) const;

    // Upsert: create or update a claim
    void upsertClaim(CognitionClaim claim);

private:
    std::vector<CognitionClaim> claims_;
};

// CognitionUpdater: applies evidence to update cognition state
class CognitionUpdater {
public:
    // Apply a single evidence to the state. Returns the updated claim or error.
    static pathfinder::foundation::Result<CognitionClaim> applyEvidence(CognitionState& state, const CognitionEvidence& evidence);
};

} // namespace pathfinder::cognition
