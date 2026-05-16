#include "pathfinder/cognition/cognition_state.h"
#include <algorithm>

namespace pathfinder::cognition {

const CognitionClaim* CognitionState::findClaim(const CognitionKey& key) const {
    for (const auto& c : claims_) {
        if (c.key == key) return &c;
    }
    return nullptr;
}

const CognitionClaim* CognitionState::findEdibleClaim(
    const EntityId& subject_id,
    const ObjectDefinitionId& object_definition_id,
    const ActionId& action_id) const {
    CognitionKey key;
    key.subject_id = subject_id;
    key.object_definition_id = object_definition_id;
    key.action_id = action_id;
    key.effect_kind = CognitionEffectKind::Edible;
    return findClaim(key);
}

const CognitionClaim* CognitionState::findHarmfulClaim(
    const EntityId& subject_id,
    const ObjectDefinitionId& object_definition_id,
    const ActionId& action_id) const {
    CognitionKey key;
    key.subject_id = subject_id;
    key.object_definition_id = object_definition_id;
    key.action_id = action_id;
    key.effect_kind = CognitionEffectKind::Harmful;
    return findClaim(key);
}

void CognitionState::upsertClaim(CognitionClaim claim) {
    for (auto& c : claims_) {
        if (c.key == claim.key) {
            c = std::move(claim);
            return;
        }
    }
    claims_.push_back(std::move(claim));
}

pathfinder::foundation::Result<CognitionClaim> CognitionUpdater::applyEvidence(CognitionState& state, const CognitionEvidence& evidence) {
    // Validate evidence first
    auto validation = evidence.validateBasic();
    if (validation.is_error()) {
        return pathfinder::foundation::Result<CognitionClaim>::fail(validation.errors()[0]);
    }

    const auto* existing = state.findClaim(evidence.key);

    CognitionClaim claim;
    if (existing) {
        claim = *existing;
        claim.confidence = std::clamp(claim.confidence + evidence.confidence_delta, 0.0, 1.0);
        claim.evidence_count += 1;
    } else {
        claim.key = evidence.key;
        claim.confidence = std::clamp(evidence.confidence_delta, 0.0, 1.0);
        claim.evidence_count = 1;
    }
    claim.last_event_id = evidence.source_event_id;

    state.upsertClaim(claim);
    return pathfinder::foundation::Result<CognitionClaim>::ok(std::move(claim));
}

} // namespace pathfinder::cognition
