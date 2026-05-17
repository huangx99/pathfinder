#pragma once

#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/cognition/types.h"
#include <vector>

namespace pathfinder::cognition {

// CognitionEvidenceBuilder: constructs V2 evidence from safe feedback signals or legacy evidence
class CognitionEvidenceBuilder {
public:
    pathfinder::foundation::Result<std::vector<CognitionEvidenceRecord>> fromFeedbackSignal(
        const CognitionFeedbackSignal& signal) const;

    pathfinder::foundation::Result<std::vector<CognitionEvidenceRecord>> fromLegacyEvidence(
        const CognitionEvidence& legacy_evidence) const;
};

} // namespace pathfinder::cognition
