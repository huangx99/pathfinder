#include "pathfinder/cognition/cognition_v2_types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_v2_types_tests() {
    // ============================================================
    // Enum roundtrips: CognitionSubjectKind
    // ============================================================
    {
        assert(toString(CognitionSubjectKind::Unknown) == "unknown");
        assert(toString(CognitionSubjectKind::Actor) == "actor");
        assert(toString(CognitionSubjectKind::Agent) == "agent");
        assert(toString(CognitionSubjectKind::Group) == "group");
        assert(toString(CognitionSubjectKind::Tribe) == "tribe");
        assert(toString(CognitionSubjectKind::Civilization) == "civilization");
        assert(toString(CognitionSubjectKind::TestOnly) == "test_only");

        assert(cognitionSubjectKindFromString("actor").value() == CognitionSubjectKind::Actor);
        assert(cognitionSubjectKindFromString("invalid").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionTargetKind
    // ============================================================
    {
        assert(toString(CognitionTargetKind::ObjectDefinition) == "object_definition");
        assert(toString(CognitionTargetKind::WorldObject) == "world_object");
        assert(cognitionTargetKindFromString("object_definition").value() == CognitionTargetKind::ObjectDefinition);
        assert(cognitionTargetKindFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionAspect
    // ============================================================
    {
        assert(toString(CognitionAspect::Edibility) == "edibility");
        assert(toString(CognitionAspect::Usability) == "usability");
        assert(toString(CognitionAspect::ConsumptionEffect) == "consumption_effect");
        assert(toString(CognitionAspect::UseEffect) == "use_effect");
        assert(toString(CognitionAspect::Risk) == "risk");
        assert(toString(CognitionAspect::Utility) == "utility");
        assert(cognitionAspectFromString("risk").value() == CognitionAspect::Risk);
        assert(cognitionAspectFromString("invalid").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionBeliefPolarity
    // ============================================================
    {
        assert(toString(CognitionBeliefPolarity::Positive) == "positive");
        assert(toString(CognitionBeliefPolarity::Negative) == "negative");
        assert(toString(CognitionBeliefPolarity::Mixed) == "mixed");
        assert(cognitionBeliefPolarityFromString("mixed").value() == CognitionBeliefPolarity::Mixed);
        assert(cognitionBeliefPolarityFromString("xxx").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionEvidenceSupport
    // ============================================================
    {
        assert(toString(CognitionEvidenceSupport::Supports) == "supports");
        assert(toString(CognitionEvidenceSupport::Refutes) == "refutes");
        assert(toString(CognitionEvidenceSupport::Neutral) == "neutral");
        assert(toString(CognitionEvidenceSupport::Conflicts) == "conflicts");
        assert(cognitionEvidenceSupportFromString("conflicts").value() == CognitionEvidenceSupport::Conflicts);
        assert(cognitionEvidenceSupportFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionEvidenceSourceKind
    // ============================================================
    {
        assert(toString(CognitionEvidenceSourceKind::DirectActionFeedback) == "direct_action_feedback");
        assert(toString(CognitionEvidenceSourceKind::TestOnly) == "test_only");
        assert(cognitionEvidenceSourceKindFromString("direct_action_feedback").value() == CognitionEvidenceSourceKind::DirectActionFeedback);
        assert(cognitionEvidenceSourceKindFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionActionContextKind
    // ============================================================
    {
        assert(toString(CognitionActionContextKind::Eat) == "eat");
        assert(toString(CognitionActionContextKind::Use) == "use");
        assert(toString(CognitionActionContextKind::Observe) == "observe");
        assert(cognitionActionContextKindFromString("use").value() == CognitionActionContextKind::Use);
        assert(cognitionActionContextKindFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionOutcomeSignal
    // ============================================================
    {
        assert(toString(CognitionOutcomeSignal::NeedImproved) == "need_improved");
        assert(toString(CognitionOutcomeSignal::HealthWorsened) == "health_worsened");
        assert(toString(CognitionOutcomeSignal::DamageTaken) == "damage_taken");
        assert(toString(CognitionOutcomeSignal::ToolActivated) == "tool_activated");
        assert(cognitionOutcomeSignalFromString("tool_activated").value() == CognitionOutcomeSignal::ToolActivated);
        assert(cognitionOutcomeSignalFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionRiskLevel
    // ============================================================
    {
        assert(toString(CognitionRiskLevel::Unknown) == "unknown");
        assert(toString(CognitionRiskLevel::None) == "none");
        assert(toString(CognitionRiskLevel::Critical) == "critical");
        assert(cognitionRiskLevelFromString("high").value() == CognitionRiskLevel::High);
        assert(cognitionRiskLevelFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionConfidenceBand
    // ============================================================
    {
        assert(toString(CognitionConfidenceBand::Untrusted) == "untrusted");
        assert(toString(CognitionConfidenceBand::Hypothesis) == "hypothesis");
        assert(toString(CognitionConfidenceBand::Actionable) == "actionable");
        assert(toString(CognitionConfidenceBand::Reliable) == "reliable");
        assert(toString(CognitionConfidenceBand::Teachable) == "teachable");
        assert(cognitionConfidenceBandFromString("reliable").value() == CognitionConfidenceBand::Reliable);
        assert(cognitionConfidenceBandFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionUpdateDecision
    // ============================================================
    {
        assert(toString(CognitionUpdateDecision::Created) == "created");
        assert(toString(CognitionUpdateDecision::Reinforced) == "reinforced");
        assert(toString(CognitionUpdateDecision::Weakened) == "weakened");
        assert(toString(CognitionUpdateDecision::Conflicted) == "conflicted");
        assert(cognitionUpdateDecisionFromString("conflicted").value() == CognitionUpdateDecision::Conflicted);
        assert(cognitionUpdateDecisionFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: CognitionQueryMode
    // ============================================================
    {
        assert(toString(CognitionQueryMode::ExactTarget) == "exact_target");
        assert(toString(CognitionQueryMode::BestActionable) == "best_actionable");
        assert(toString(CognitionQueryMode::TeachableCandidates) == "teachable_candidates");
        assert(cognitionQueryModeFromString("teachable_candidates").value() == CognitionQueryMode::TeachableCandidates);
        assert(cognitionQueryModeFromString("bad").is_error());
    }

    // ============================================================
    // CognitionSubject validateBasic
    // ============================================================
    {
        CognitionSubject s;
        assert(s.validateBasic().is_error()); // Unknown kind
        s.kind = CognitionSubjectKind::Actor;
        assert(s.validateBasic().is_error()); // empty subject_id
        s.subject_id = EntityId("actor_001");
        assert(s.validateBasic().is_ok());
    }

    // ============================================================
    // CognitionTarget validateBasic
    // ============================================================
    {
        CognitionTarget t;
        assert(t.validateBasic().is_error()); // Unknown kind
        t.kind = CognitionTargetKind::ObjectDefinition;
        assert(t.validateBasic().is_error()); // empty target_id
        t.target_id = "unknown_fruit";
        assert(t.validateBasic().is_ok());

        // Hidden truth in public_label_key rejected
        t.public_label_key = "edible_profile";
        assert(t.validateBasic().is_error());
        t.public_label_key = "safe_label";
        assert(t.validateBasic().is_ok());
    }

    // ============================================================
    // CognitionClaimKeyV2 validateBasic
    // ============================================================
    {
        CognitionClaimKeyV2 key;
        assert(key.validateBasic().is_error()); // subject invalid

        key.subject.kind = CognitionSubjectKind::Actor;
        key.subject.subject_id = EntityId("actor_001");
        key.target.kind = CognitionTargetKind::ObjectDefinition;
        key.target.target_id = "unknown_fruit";
        key.action_id = ActionId("eat");
        key.action_context = CognitionActionContextKind::Eat;
        key.aspect = CognitionAspect::Edibility;
        assert(key.validateBasic().is_ok());
    }

    // ============================================================
    // CognitionEvidenceRecord validateBasic
    // ============================================================
    {
        CognitionEvidenceRecord rec;
        assert(rec.validateBasic().is_error()); // key invalid

        rec.key.subject.kind = CognitionSubjectKind::Actor;
        rec.key.subject.subject_id = EntityId("actor_001");
        rec.key.target.kind = CognitionTargetKind::ObjectDefinition;
        rec.key.target.target_id = "unknown_fruit";
        rec.key.action_id = ActionId("eat");
        rec.key.action_context = CognitionActionContextKind::Eat;
        rec.key.aspect = CognitionAspect::Edibility;
        rec.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        rec.support = CognitionEvidenceSupport::Supports;
        rec.evidence_id = CognitionEvidenceRecordId("ev_001");
        rec.source_event_id = EventId("evt_001");
        rec.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        assert(rec.validateBasic().is_ok());

        rec.confidence_weight = 1.5;
        assert(rec.validateBasic().is_error()); // out of range
        rec.confidence_weight = 0.5;
        assert(rec.validateBasic().is_ok());

        rec.utility_signal = -1.5;
        assert(rec.validateBasic().is_error()); // out of range
        rec.utility_signal = 0.0;
        assert(rec.validateBasic().is_ok());

        // Hidden truth in reason_keys rejected
        rec.reason_keys = {"something", "hunger_delta"};
        assert(rec.validateBasic().is_error());
        rec.reason_keys.clear();
        assert(rec.validateBasic().is_ok());
    }

    // ============================================================
    // CognitionClaimV2 validateBasic
    // ============================================================
    {
        CognitionClaimV2 claim;
        assert(claim.validateBasic().is_error()); // key invalid

        claim.key.subject.kind = CognitionSubjectKind::Actor;
        claim.key.subject.subject_id = EntityId("actor_001");
        claim.key.target.kind = CognitionTargetKind::ObjectDefinition;
        claim.key.target.target_id = "unknown_fruit";
        claim.key.action_id = ActionId("eat");
        claim.key.action_context = CognitionActionContextKind::Eat;
        claim.key.aspect = CognitionAspect::Edibility;
        claim.claim_id = CognitionClaimV2Id("test_claim_001");
        assert(claim.validateBasic().is_ok());

        claim.confidence = 1.5;
        assert(claim.validateBasic().is_error());
        claim.confidence = 0.5;
        assert(claim.validateBasic().is_ok());

        claim.utility_score = 2.0;
        assert(claim.validateBasic().is_error());
        claim.utility_score = 0.0;
        assert(claim.validateBasic().is_ok());

        // conflicted requires Mixed
        claim.conflicted = true;
        assert(claim.validateBasic().is_error());
        claim.polarity = CognitionBeliefPolarity::Mixed;
        assert(claim.validateBasic().is_ok());
    }

    // ============================================================
    // CognitionFeedbackSignal validateBasic
    // ============================================================
    {
        CognitionFeedbackSignal sig;
        assert(sig.validateBasic().is_error());

        sig.subject.kind = CognitionSubjectKind::Actor;
        sig.subject.subject_id = EntityId("actor_001");
        sig.target.kind = CognitionTargetKind::ObjectDefinition;
        sig.target.target_id = "unknown_fruit";
        sig.action_id = ActionId("eat");
        sig.action_context = CognitionActionContextKind::Eat;
        assert(sig.validateBasic().is_ok());

        sig.utility_signal = -2.0;
        assert(sig.validateBasic().is_error());
        sig.utility_signal = 0.0;
        assert(sig.validateBasic().is_ok());
    }

    // ============================================================
    // ID generation helpers
    // ============================================================
    {
        auto eid = makeEvidenceRecordId(EventId("evt_001"), CognitionAspect::Edibility, 0);
        assert(eid.value().find("cognition_evidence_evt_001_edibility_0") != std::string::npos);

        auto cid = makeClaimV2Id(EntityId("actor_001"), "unknown_fruit", ActionId("eat"), CognitionAspect::Edibility);
        assert(cid.value().find("cognition_claim_actor_001_unknown_fruit_eat_edibility") != std::string::npos);
    }

    // ============================================================
    // BLOCKER-1: evidence_count consistency
    // ============================================================
    {
        CognitionClaimV2 claim;
        claim.key.subject.kind = CognitionSubjectKind::Actor;
        claim.key.subject.subject_id = EntityId("actor_001");
        claim.key.target.kind = CognitionTargetKind::ObjectDefinition;
        claim.key.target.target_id = "unknown_fruit";
        claim.key.action_id = ActionId("eat");
        claim.key.action_context = CognitionActionContextKind::Eat;
        claim.key.aspect = CognitionAspect::Edibility;
        claim.claim_id = CognitionClaimV2Id("test_claim_001");
        claim.evidence_count = 1;
        claim.supporting_evidence_count = 1;
        claim.refuting_evidence_count = 1;
        // 1 < 1 + 1 = 2 -> must fail
        assert(claim.validateBasic().is_error());

        claim.evidence_count = 2;
        assert(claim.validateBasic().is_ok());
    }

    // ============================================================
    // SUGGESTION-1: claim_id must not be empty
    // ============================================================
    {
        CognitionClaimV2 claim;
        claim.key.subject.kind = CognitionSubjectKind::Actor;
        claim.key.subject.subject_id = EntityId("actor_001");
        claim.key.target.kind = CognitionTargetKind::ObjectDefinition;
        claim.key.target.target_id = "unknown_fruit";
        claim.key.action_id = ActionId("eat");
        claim.key.action_context = CognitionActionContextKind::Eat;
        claim.key.aspect = CognitionAspect::Edibility;
        // claim_id empty -> must fail
        assert(claim.validateBasic().is_error());

        claim.claim_id = CognitionClaimV2Id("test_claim_001");
        claim.evidence_count = 1;
        claim.supporting_evidence_count = 1;
        assert(claim.validateBasic().is_ok());
    }

    // ============================================================
    // SUGGESTION-2: evidence_id / source_event_id non-empty, outcome_signal not Unknown
    // ============================================================
    {
        CognitionEvidenceRecord rec;
        rec.key.subject.kind = CognitionSubjectKind::Actor;
        rec.key.subject.subject_id = EntityId("actor_001");
        rec.key.target.kind = CognitionTargetKind::ObjectDefinition;
        rec.key.target.target_id = "unknown_fruit";
        rec.key.action_id = ActionId("eat");
        rec.key.action_context = CognitionActionContextKind::Eat;
        rec.key.aspect = CognitionAspect::Edibility;
        rec.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        rec.support = CognitionEvidenceSupport::Supports;
        // evidence_id empty -> fail
        assert(rec.validateBasic().is_error());

        rec.evidence_id = CognitionEvidenceRecordId("ev_001");
        // source_event_id empty -> fail
        assert(rec.validateBasic().is_error());

        rec.source_event_id = EventId("evt_001");
        // outcome_signal Unknown -> fail
        assert(rec.validateBasic().is_error());

        rec.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        assert(rec.validateBasic().is_ok());
    }

    // ============================================================
    // Hidden truth detection
    // ============================================================
    {
        assert(containsHiddenTruthKey("edible_profile") == true);
        assert(containsHiddenTruthKey("hunger_delta") == true);
        assert(containsHiddenTruthKey("health_delta") == true);
        assert(containsHiddenTruthKey("effect_kind") == true);
        assert(containsHiddenTruthKey("Edible_Profile") == true); // case insensitive
        assert(containsHiddenTruthKey("safe_reason") == false);

        std::vector<std::string> bad_keys = {"ok", "edible_profile"};
        assert(containsHiddenTruthKey(bad_keys) == true);
        std::vector<std::string> good_keys = {"ok", "safe"};
        assert(containsHiddenTruthKey(good_keys) == false);
    }

    std::cout << "cognition_v2_types tests passed" << std::endl;
}
