#include "pathfinder/cognition/cognition_evidence_builder.h"
#include "pathfinder/cognition/cognition_confidence.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include "pathfinder/cognition/cognition_query.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_p15_cognition_use_feedback_flow_tests() {
    // Construct safe Use feedback signal without full UseObjectResolver
    CognitionFeedbackSignal signal;
    signal.subject.kind = CognitionSubjectKind::Actor;
    signal.subject.subject_id = EntityId("actor_001");
    signal.target.kind = CognitionTargetKind::ObjectDefinition; // hidden_truth: safe enum
    signal.target.target_id = "unknown_tool";
    signal.action_id = ActionId("use");
    signal.action_context = CognitionActionContextKind::Use;
    signal.outcome_signal = CognitionOutcomeSignal::ToolActivated;
    signal.source_event_id = EventId("evt_use_001");
    signal.observed_tick = Tick(1);

    // Build evidence
    CognitionEvidenceBuilder builder;
    auto build_result = builder.fromFeedbackSignal(signal);
    assert(build_result.is_ok());
    const auto& evidence_records = build_result.value();
    assert(!evidence_records.empty());

    // Apply to state
    CognitionStateV2 state;
    CognitionUpdaterV2 updater;
    for (const auto& evidence : evidence_records) {
        auto update_result = updater.applyEvidence(state, evidence);
        assert(update_result.is_ok());
    }

    // Query Usability claim
    CognitionQueryService query_service;
    CognitionSubject subject{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
    CognitionTarget target{CognitionTargetKind::ObjectDefinition, "unknown_tool", std::nullopt, ""}; // hidden_truth: safe enum

    auto usability_result = query_service.findUsability(state, subject, target);
    assert(usability_result.is_ok());
    assert(usability_result.value().has_value());
    auto& usability_claim = usability_result.value().value();
    assert(usability_claim.key.aspect == CognitionAspect::Usability);
    assert(usability_claim.confidence > 0.0);

    // UseEffect claim exists
    {
        CognitionQuery q;
        q.subject = subject;
        q.target = target;
        q.aspect = CognitionAspect::UseEffect;
        q.mode = CognitionQueryMode::ExactTarget;
        auto ue_result = query_service.query(state, q);
        assert(ue_result.is_ok());
        assert(!ue_result.value().claims.empty());
    }

    std::cout << "p15_cognition_use_feedback_flow tests passed" << std::endl;
}
