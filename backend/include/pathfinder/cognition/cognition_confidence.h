#pragma once

#include "pathfinder/cognition/cognition_v2_types.h"

namespace pathfinder::cognition {

// Forward declaration
class CognitionStateV2;

// CognitionConfidenceModel: computes confidence bands and applies evidence to claims
class CognitionConfidenceModel {
public:
    CognitionConfidenceBand bandFor(double confidence, const CognitionConfidenceModelOptions& options = {}) const;

    pathfinder::foundation::Result<CognitionUpdateResult> applyEvidence(
        const CognitionUpdateRequest& request) const;
};

// CognitionUpdaterV2: applies evidence to CognitionStateV2
class CognitionUpdaterV2 {
public:
    pathfinder::foundation::Result<CognitionUpdateResult> applyEvidence(
        CognitionStateV2& state,
        const CognitionEvidenceRecord& evidence,
        const CognitionConfidenceModelOptions& options = {}) const;
};

} // namespace pathfinder::cognition
