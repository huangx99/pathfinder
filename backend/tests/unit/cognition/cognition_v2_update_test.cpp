#include "pathfinder/cognition/cognition_confidence.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include "pathfinder/cognition/cognition_evidence_builder.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_v2_update_tests() {
    // ============================================================
    // applyEvidence creates claim and saves evidence
    // ============================================================
    {
        CognitionStateV2 state;
        CognitionUpdaterV2 updater;

        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_001");
        evidence.key.subject.kind = CognitionSubjectKind::Actor;
        evidence.key.subject.subject_id = EntityId("actor_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.source_event_id = EventId("evt_001");
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;

        auto result = updater.applyEvidence(state, evidence);
        assert(result.is_ok());
        assert(result.value().decision == CognitionUpdateDecision::Created);
        assert(state.claims().size() == 1);
        assert(state.evidence_records().size() == 1);
    }

    // ============================================================
    // Repeated evidence updates same claim
    // ============================================================
    {
        CognitionStateV2 state;
        CognitionUpdaterV2 updater;

        CognitionEvidenceRecord e1;
        e1.evidence_id = CognitionEvidenceRecordId("ev_001");
        e1.key.subject.kind = CognitionSubjectKind::Actor;
        e1.key.subject.subject_id = EntityId("actor_001");
        e1.key.target.kind = CognitionTargetKind::ObjectDefinition;
        e1.key.target.target_id = "unknown_fruit";
        e1.key.action_id = ActionId("eat");
        e1.key.action_context = CognitionActionContextKind::Eat;
        e1.key.aspect = CognitionAspect::Edibility;
        e1.support = CognitionEvidenceSupport::Supports;
        e1.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        e1.source_event_id = EventId("evt_001");
        e1.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        updater.applyEvidence(state, e1);

        CognitionEvidenceRecord e2;
        e2.evidence_id = CognitionEvidenceRecordId("ev_002");
        e2.key = e1.key;
        e2.support = CognitionEvidenceSupport::Supports;
        e2.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        e2.source_event_id = EventId("evt_002");
        e2.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        auto result = updater.applyEvidence(state, e2);
        assert(result.is_ok());
        assert(result.value().decision == CognitionUpdateDecision::Reinforced);
        assert(state.claims().size() == 1);
        assert(state.evidence_records().size() == 2);
        assert(state.claims()[0].evidence_count == 2);
    }

    // ============================================================
    // Different aspect generates different claim
    // ============================================================
    {
        CognitionStateV2 state;
        CognitionUpdaterV2 updater;

        CognitionEvidenceRecord e1;
        e1.evidence_id = CognitionEvidenceRecordId("ev_001");
        e1.key.subject.kind = CognitionSubjectKind::Actor;
        e1.key.subject.subject_id = EntityId("actor_001");
        e1.key.target.kind = CognitionTargetKind::ObjectDefinition;
        e1.key.target.target_id = "unknown_fruit";
        e1.key.action_id = ActionId("eat");
        e1.key.action_context = CognitionActionContextKind::Eat;
        e1.key.aspect = CognitionAspect::Edibility;
        e1.support = CognitionEvidenceSupport::Supports;
        e1.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        e1.source_event_id = EventId("evt_001");
        e1.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        updater.applyEvidence(state, e1);

        CognitionEvidenceRecord e2;
        e2.evidence_id = CognitionEvidenceRecordId("ev_002");
        e2.key = e1.key;
        e2.key.aspect = CognitionAspect::Risk;
        e2.support = CognitionEvidenceSupport::Supports;
        e2.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        e2.source_event_id = EventId("evt_002");
        e2.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        updater.applyEvidence(state, e2);

        assert(state.claims().size() == 2);
    }

    // ============================================================
    // Invalid evidence does not write state
    // ============================================================
    {
        CognitionStateV2 state;
        CognitionUpdaterV2 updater;

        CognitionEvidenceRecord evidence;
        evidence.support = CognitionEvidenceSupport::Supports;
        // Missing required fields -> invalid

        auto result = updater.applyEvidence(state, evidence);
        assert(result.is_error());
        assert(state.claims().empty());
        assert(state.evidence_records().empty());
    }

    // ============================================================
    // fromFeedbackSignal(Eat + NeedImproved) generates Edibility/ConsumptionEffect/Risk/Utility
    // ============================================================
    {
        CognitionFeedbackSignal signal;
        signal.subject.kind = CognitionSubjectKind::Actor;
        signal.subject.subject_id = EntityId("actor_001");
        signal.target.kind = CognitionTargetKind::ObjectDefinition;
        signal.target.target_id = "unknown_fruit";
        signal.action_id = ActionId("eat");
        signal.action_context = CognitionActionContextKind::Eat;
        signal.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        signal.source_event_id = EventId("evt_001");

        CognitionEvidenceBuilder builder;
        auto result = builder.fromFeedbackSignal(signal);
        assert(result.is_ok());
        const auto& records = result.value();
        assert(!records.empty());

        bool has_edibility = false;
        bool has_consumption_effect = false;
        bool has_risk = false;
        bool has_utility = false;
        for (const auto& rec : records) {
            if (rec.key.aspect == CognitionAspect::Edibility) has_edibility = true;
            if (rec.key.aspect == CognitionAspect::ConsumptionEffect) has_consumption_effect = true;
            if (rec.key.aspect == CognitionAspect::Risk) has_risk = true;
            if (rec.key.aspect == CognitionAspect::Utility) has_utility = true;
        }
        assert(has_edibility);
        assert(has_consumption_effect);
        assert(has_risk);
        assert(has_utility);
    }

    // ============================================================
    // fromFeedbackSignal(Use + ToolActivated) generates Usability/UseEffect/Utility
    // ============================================================
    {
        CognitionFeedbackSignal signal;
        signal.subject.kind = CognitionSubjectKind::Actor;
        signal.subject.subject_id = EntityId("actor_001");
        signal.target.kind = CognitionTargetKind::ObjectDefinition;
        signal.target.target_id = "unknown_tool";
        signal.action_id = ActionId("use");
        signal.action_context = CognitionActionContextKind::Use;
        signal.outcome_signal = CognitionOutcomeSignal::ToolActivated;
        signal.source_event_id = EventId("evt_002");

        CognitionEvidenceBuilder builder;
        auto result = builder.fromFeedbackSignal(signal);
        assert(result.is_ok());
        const auto& records = result.value();

        bool has_usability = false;
        bool has_use_effect = false;
        bool has_utility = false;
        for (const auto& rec : records) {
            if (rec.key.aspect == CognitionAspect::Usability) has_usability = true;
            if (rec.key.aspect == CognitionAspect::UseEffect) has_use_effect = true;
            if (rec.key.aspect == CognitionAspect::Utility) has_utility = true;
        }
        assert(has_usability);
        assert(has_use_effect);
        assert(has_utility);
    }

    // ============================================================
    // fromLegacyEvidence does not leak hidden field names
    // ============================================================
    {
        CognitionEvidence legacy;
        legacy.key.subject_id = EntityId("actor_001");
        legacy.key.object_definition_id = ObjectDefinitionId("unknown_fruit");
        legacy.key.action_id = ActionId("eat");
        legacy.key.effect_kind = CognitionEffectKind::Edible;
        legacy.source_event_id = EventId("evt_001");
        legacy.confidence_delta = 0.3;
        legacy.observed_hunger_delta = -20;
        legacy.observed_health_delta = 0;

        CognitionEvidenceBuilder builder;
        auto result = builder.fromLegacyEvidence(legacy);
        assert(result.is_ok());
        const auto& records = result.value();
        assert(!records.empty());

        for (const auto& rec : records) {
            for (const auto& reason : rec.reason_keys) {
                assert(reason.find("hunger_delta") == std::string::npos);
                assert(reason.find("health_delta") == std::string::npos);
                assert(reason.find("edible_profile") == std::string::npos);
                assert(reason.find("effect_kind") == std::string::npos);
            }
        }
    }

    std::cout << "cognition_v2_update tests passed" << std::endl;
}
