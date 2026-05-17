#include "pathfinder/cognition/cognition_confidence.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_confidence_model_tests() {
    CognitionConfidenceModel model;
    CognitionConfidenceModelOptions options;

    // ============================================================
    // bandFor thresholds
    // ============================================================
    {
        assert(model.bandFor(0.0, options) == CognitionConfidenceBand::Untrusted);
        assert(model.bandFor(0.10, options) == CognitionConfidenceBand::Untrusted);
        assert(model.bandFor(0.20, options) == CognitionConfidenceBand::Hypothesis);
        assert(model.bandFor(0.49, options) == CognitionConfidenceBand::Hypothesis);
        assert(model.bandFor(0.50, options) == CognitionConfidenceBand::Actionable);
        assert(model.bandFor(0.69, options) == CognitionConfidenceBand::Actionable);
        assert(model.bandFor(0.70, options) == CognitionConfidenceBand::Reliable);
        assert(model.bandFor(0.84, options) == CognitionConfidenceBand::Reliable);
        assert(model.bandFor(0.85, options) == CognitionConfidenceBand::Teachable);
        assert(model.bandFor(1.0, options) == CognitionConfidenceBand::Teachable);
    }

    // ============================================================
    // First Supports creates claim
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.key.subject.kind = CognitionSubjectKind::Actor;
        evidence.key.subject.subject_id = EntityId("actor_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_001");
        evidence.source_event_id = EventId("evt_001");
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;

        CognitionUpdateRequest request;
        request.evidence = evidence;
        request.options = options;

        auto result = model.applyEvidence(request);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == CognitionUpdateDecision::Created);
        assert(res.after_claim.confidence == options.initial_direct_feedback_confidence);
        assert(res.after_claim.polarity == CognitionBeliefPolarity::Positive);
        assert(res.after_claim.confidence_band == CognitionConfidenceBand::Hypothesis);
        assert(res.after_claim.evidence_count == 1);
        assert(res.after_claim.supporting_evidence_count == 1);
    }

    // ============================================================
    // Repeated Supports reinforces
    // ============================================================
    {
        CognitionClaimV2 existing;
        existing.key.subject.kind = CognitionSubjectKind::Actor;
        existing.key.subject.subject_id = EntityId("actor_001");
        existing.key.target.kind = CognitionTargetKind::ObjectDefinition;
        existing.key.target.target_id = "unknown_fruit";
        existing.key.action_id = ActionId("eat");
        existing.key.action_context = CognitionActionContextKind::Eat;
        existing.key.aspect = CognitionAspect::Edibility;
        existing.confidence = 0.35;
        existing.polarity = CognitionBeliefPolarity::Positive;
        existing.evidence_count = 1;
        existing.supporting_evidence_count = 1;

        CognitionEvidenceRecord evidence;
        evidence.key = existing.key;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_002");
        evidence.source_event_id = EventId("evt_002");
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;

        CognitionUpdateRequest request;
        request.existing_claim = existing;
        request.evidence = evidence;
        request.options = options;

        auto result = model.applyEvidence(request);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == CognitionUpdateDecision::Reinforced);
        assert(res.after_claim.confidence > existing.confidence);
        assert(res.after_claim.polarity == CognitionBeliefPolarity::Positive);
        assert(res.after_claim.evidence_count == 2);
    }

    // ============================================================
    // Refutes weakens
    // ============================================================
    {
        CognitionClaimV2 existing;
        existing.key.subject.kind = CognitionSubjectKind::Actor;
        existing.key.subject.subject_id = EntityId("actor_001");
        existing.key.target.kind = CognitionTargetKind::ObjectDefinition;
        existing.key.target.target_id = "unknown_fruit";
        existing.key.action_id = ActionId("eat");
        existing.key.action_context = CognitionActionContextKind::Eat;
        existing.key.aspect = CognitionAspect::Edibility;
        existing.confidence = 0.60;
        existing.polarity = CognitionBeliefPolarity::Positive;
        existing.evidence_count = 2;

        CognitionEvidenceRecord evidence;
        evidence.key = existing.key;
        evidence.support = CognitionEvidenceSupport::Refutes;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_003");
        evidence.source_event_id = EventId("evt_003");
        evidence.outcome_signal = CognitionOutcomeSignal::NeedWorsened;

        CognitionUpdateRequest request;
        request.existing_claim = existing;
        request.evidence = evidence;
        request.options = options;

        auto result = model.applyEvidence(request);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == CognitionUpdateDecision::Weakened);
        assert(res.after_claim.confidence < existing.confidence);
    }

    // ============================================================
    // Conflicts sets conflicted=true / polarity=Mixed
    // ============================================================
    {
        CognitionClaimV2 existing;
        existing.key.subject.kind = CognitionSubjectKind::Actor;
        existing.key.subject.subject_id = EntityId("actor_001");
        existing.key.target.kind = CognitionTargetKind::ObjectDefinition;
        existing.key.target.target_id = "unknown_fruit";
        existing.key.action_id = ActionId("eat");
        existing.key.action_context = CognitionActionContextKind::Eat;
        existing.key.aspect = CognitionAspect::Edibility;
        existing.confidence = 0.60;
        existing.polarity = CognitionBeliefPolarity::Positive;
        existing.evidence_count = 1;

        CognitionEvidenceRecord evidence;
        evidence.key = existing.key;
        evidence.support = CognitionEvidenceSupport::Conflicts;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_004");
        evidence.source_event_id = EventId("evt_004");
        evidence.outcome_signal = CognitionOutcomeSignal::HealthWorsened;

        CognitionUpdateRequest request;
        request.existing_claim = existing;
        request.evidence = evidence;
        request.options = options;

        auto result = model.applyEvidence(request);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == CognitionUpdateDecision::Conflicted);
        assert(res.after_claim.conflicted == true);
        assert(res.after_claim.polarity == CognitionBeliefPolarity::Mixed);
        assert(res.after_claim.confidence < existing.confidence);
    }

    // ============================================================
    // Confidence clamped to [0,1]
    // ============================================================
    {
        CognitionClaimV2 existing;
        existing.key.subject.kind = CognitionSubjectKind::Actor;
        existing.key.subject.subject_id = EntityId("actor_001");
        existing.key.target.kind = CognitionTargetKind::ObjectDefinition;
        existing.key.target.target_id = "unknown_fruit";
        existing.key.action_id = ActionId("eat");
        existing.key.action_context = CognitionActionContextKind::Eat;
        existing.key.aspect = CognitionAspect::Edibility;
        existing.confidence = 0.95;
        existing.polarity = CognitionBeliefPolarity::Positive;
        existing.evidence_count = 5;

        // Many reinforcements should not exceed 1.0
        CognitionEvidenceRecord evidence;
        evidence.key = existing.key;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_005");
        evidence.source_event_id = EventId("evt_005");
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;

        CognitionUpdateRequest request;
        request.existing_claim = existing;
        request.evidence = evidence;
        request.options = options;

        auto result = model.applyEvidence(request);
        assert(result.is_ok());
        assert(result.value().after_claim.confidence <= 1.0);
    }

    std::cout << "cognition_confidence_model tests passed" << std::endl;
}
