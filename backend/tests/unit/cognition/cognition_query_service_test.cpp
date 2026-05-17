#include "pathfinder/cognition/cognition_query.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_query_service_tests() {
    CognitionQueryService service;
    CognitionStateV2 state;

    // Helper to build a claim
    auto make_claim = [&](const std::string& subject, const std::string& target, CognitionAspect aspect,
                          double confidence, bool conflicted = false) -> CognitionClaimV2 {
        CognitionClaimV2 claim;
        claim.claim_id = makeClaimV2Id(EntityId(subject), target, ActionId("eat"), aspect);
        claim.key.subject.kind = CognitionSubjectKind::Actor;
        claim.key.subject.subject_id = EntityId(subject);
        claim.key.target.kind = CognitionTargetKind::ObjectDefinition;
        claim.key.target.target_id = target;
        claim.key.action_id = ActionId("eat");
        claim.key.action_context = CognitionActionContextKind::Eat;
        claim.key.aspect = aspect;
        claim.confidence = confidence;
        claim.polarity = conflicted ? CognitionBeliefPolarity::Mixed : CognitionBeliefPolarity::Positive;
        claim.conflicted = conflicted;
        claim.evidence_count = 1;
        claim.supporting_evidence_count = 1;
        claim.confidence_band = confidence >= 0.85 ? CognitionConfidenceBand::Teachable :
                                 confidence >= 0.70 ? CognitionConfidenceBand::Reliable :
                                 confidence >= 0.50 ? CognitionConfidenceBand::Actionable :
                                 confidence >= 0.20 ? CognitionConfidenceBand::Hypothesis :
                                 CognitionConfidenceBand::Untrusted;
        return claim;
    };

    // Populate state
    assert(state.upsertClaim(make_claim("actor_001", "unknown_fruit", CognitionAspect::Edibility, 0.35)).is_ok());
    assert(state.upsertClaim(make_claim("actor_001", "unknown_fruit", CognitionAspect::Risk, 0.25)).is_ok());
    assert(state.upsertClaim(make_claim("actor_001", "unknown_fruit", CognitionAspect::ConsumptionEffect, 0.60)).is_ok());
    assert(state.upsertClaim(make_claim("actor_001", "unknown_tool", CognitionAspect::Usability, 0.75)).is_ok());
    assert(state.upsertClaim(make_claim("actor_001", "unknown_tool", CognitionAspect::UseEffect, 0.90)).is_ok());
    assert(state.upsertClaim(make_claim("actor_001", "unknown_fruit", CognitionAspect::Edibility, 0.55,
                                 true)).is_ok()); // conflicted duplicate key would replace

    // ============================================================
    // ExactTarget returns matching claim
    // ============================================================
    {
        CognitionQuery q;
        q.subject.kind = CognitionSubjectKind::Actor;
        q.subject.subject_id = EntityId("actor_001");
        q.target = CognitionTarget{CognitionTargetKind::ObjectDefinition, "unknown_fruit", std::nullopt, ""};
        q.aspect = CognitionAspect::Edibility;
        q.mode = CognitionQueryMode::ExactTarget;

        auto result = service.query(state, q);
        assert(result.is_ok());
        assert(!result.value().claims.empty());
        assert(result.value().claims[0].key.aspect == CognitionAspect::Edibility);
    }

    // ============================================================
    // findEdibility
    // ============================================================
    {
        CognitionSubject sub{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
        CognitionTarget tgt{CognitionTargetKind::ObjectDefinition, "unknown_fruit", std::nullopt, ""};
        auto result = service.findEdibility(state, sub, tgt);
        assert(result.is_ok());
        assert(result.value().has_value());
        assert(result.value()->key.aspect == CognitionAspect::Edibility);
    }

    // ============================================================
    // findUsability
    // ============================================================
    {
        CognitionSubject sub{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
        CognitionTarget tgt{CognitionTargetKind::ObjectDefinition, "unknown_tool", std::nullopt, ""};
        auto result = service.findUsability(state, sub, tgt);
        assert(result.is_ok());
        assert(result.value().has_value());
        assert(result.value()->key.aspect == CognitionAspect::Usability);
    }

    // ============================================================
    // findRisk
    // ============================================================
    {
        CognitionSubject sub{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
        CognitionTarget tgt{CognitionTargetKind::ObjectDefinition, "unknown_fruit", std::nullopt, ""};
        auto result = service.findRisk(state, sub, tgt);
        assert(result.is_ok());
        assert(result.value().has_value());
        assert(result.value()->key.aspect == CognitionAspect::Risk);
    }

    // ============================================================
    // BestActionable sorts by confidence and filters conflicted
    // ============================================================
    {
        CognitionQuery q;
        q.subject.kind = CognitionSubjectKind::Actor;
        q.subject.subject_id = EntityId("actor_001");
        q.mode = CognitionQueryMode::BestActionable;
        q.include_conflicted = false;

        auto result = service.query(state, q);
        assert(result.is_ok());
        // Should return claims sorted by confidence desc, no conflicted
        const auto& claims = result.value().claims;
        for (size_t i = 1; i < claims.size(); ++i) {
            assert(claims[i - 1].confidence >= claims[i].confidence);
            assert(!claims[i - 1].conflicted);
        }
    }

    // ============================================================
    // TeachableCandidates only returns Teachable band
    // ============================================================
    {
        CognitionQuery q;
        q.subject.kind = CognitionSubjectKind::Actor;
        q.subject.subject_id = EntityId("actor_001");
        q.mode = CognitionQueryMode::TeachableCandidates;

        auto result = service.query(state, q);
        assert(result.is_ok());
        for (const auto& claim : result.value().claims) {
            assert(claim.confidence_band == CognitionConfidenceBand::Teachable);
        }
    }

    // ============================================================
    // max_results limit
    // ============================================================
    {
        CognitionQuery q;
        q.subject.kind = CognitionSubjectKind::Actor;
        q.subject.subject_id = EntityId("actor_001");
        q.mode = CognitionQueryMode::ExactTarget;
        q.max_results = 2;

        auto result = service.query(state, q);
        assert(result.is_ok());
        assert(result.value().claims.size() <= 2);
    }

    std::cout << "cognition_query_service tests passed" << std::endl;
}
