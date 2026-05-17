#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/cognition/cognition_evidence_builder.h"
#include "pathfinder/cognition/cognition_query.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_p15_cognition_hidden_truth_boundary_tests() {
    // ============================================================
    // reason_keys with edible_profile -> rejected
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
        rec.reason_keys = {"something", "edible_profile"};
        assert(rec.validateBasic().is_error());
    }

    // ============================================================
    // reason_keys with hunger_delta -> rejected
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
        rec.reason_keys = {"hunger_delta"};
        assert(rec.validateBasic().is_error());
    }

    // ============================================================
    // reason_keys with health_delta -> rejected
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
        rec.reason_keys = {"health_delta"};
        assert(rec.validateBasic().is_error());
    }

    // ============================================================
    // reason_keys with effect_kind -> rejected
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
        rec.reason_keys = {"effect_kind"};
        assert(rec.validateBasic().is_error());
    }

    // ============================================================
    // target public_label_key with ObjectDefinition hidden truth -> rejected
    // ============================================================
    {
        CognitionTarget target;
        target.kind = CognitionTargetKind::ObjectDefinition;
        target.target_id = "unknown_fruit";
        target.public_label_key = "edible_profile_info";
        assert(target.validateBasic().is_error());
    }

    // ============================================================
    // EvidenceBuilder fromLegacyEvidence does not leak hidden field names
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
        legacy.observed_health_delta = -5;

        CognitionEvidenceBuilder builder;
        auto result = builder.fromLegacyEvidence(legacy);
        assert(result.is_ok());
        for (const auto& rec : result.value()) {
            for (const auto& reason : rec.reason_keys) {
                assert(reason.find("hunger_delta") == std::string::npos);
                assert(reason.find("health_delta") == std::string::npos);
                assert(reason.find("edible_profile") == std::string::npos);
                assert(reason.find("effect_kind") == std::string::npos);
            }
        }
    }

    // ============================================================
    // QueryService is read-only and does not access ObjectDefinition
    // (Verified by compilation: it only takes CognitionStateV2)
    // ============================================================
    {
        CognitionStateV2 state;
        CognitionQueryService service;
        CognitionSubject subject{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
        CognitionTarget target{CognitionTargetKind::ObjectDefinition, "unknown_fruit", std::nullopt, "safe_label"};
        auto result = service.findEdibility(state, subject, target);
        assert(result.is_ok());
        // No ObjectDefinition access, just empty result
        assert(!result.value().has_value());
    }

    std::cout << "p15_cognition_hidden_truth_boundary tests passed" << std::endl;
}
