#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::foundation;

// ============================================================
// Helpers
// ============================================================

static KnowledgeOwner makeValidOwner(const std::string& id = "agent_001") {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId(id);
    return owner;
}

static KnowledgeSubject makeValidSubject(const std::string& id = "berry_red") {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = id;
    return subject;
}

static KnowledgePredicate makeValidPredicate(const std::string& action = "eat",
                                              const std::string& effect = "restore_hunger",
                                              KnowledgeRelationType rt = KnowledgeRelationType::HasEffect) {
    KnowledgePredicate pred;
    pred.relation_type = rt;
    pred.action_key = action;
    pred.effect_key = effect;
    return pred;
}

static KnowledgeEvidence makeValidEvidence(KnowledgeEvidenceKind kind = KnowledgeEvidenceKind::MemorySummary) {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_test_001");
    ev.evidence_kind = kind;
    ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_001");
    ev.supports_claim = true;
    ev.confidence_delta = 0.5;
    ev.reliability = 0.9;
    return ev;
}

static KnowledgeConfidence makeValidConfidence(double conf = 0.75) {
    KnowledgeConfidence c;
    c.confidence = conf;
    c.band = confidenceToBand(conf);
    c.support_count = 3;
    c.conflict_count = 0;
    return c;
}

static KnowledgeClaim makeValidClaim(const std::string& know_id,
                                      const std::string& owner_id = "agent_001",
                                      KnowledgeStatus status = KnowledgeStatus::Active,
                                      double confidence = 0.75) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(know_id);
    claim.owner = makeValidOwner(owner_id);
    claim.subject = makeValidSubject();
    claim.predicate = makeValidPredicate();
    claim.confidence = makeValidConfidence(confidence);
    claim.status = status;
    claim.evidence_refs = {makeValidEvidence()};
    claim.created_tick = Tick(10);
    claim.updated_tick = Tick(10);
    if (status == KnowledgeStatus::Teachable) {
        claim.teaching_profile.teachable = true;
        claim.teaching_profile.teaching_message_key = "msg";
        claim.projection_flags.usable_for_teaching = true;
    }
    if (status == KnowledgeStatus::Active) {
        claim.projection_flags.usable_for_action = true;
    }
    if (status == KnowledgeStatus::Deprecated) {
        claim.superseded_by_knowledge_id = KnowledgeId("know_newer_001");
        claim.reason_keys.push_back("deprecated_by_revision");
    }
    if (status == KnowledgeStatus::Disproven) {
        claim.confidence.conflict_count = 1;
        claim.reason_keys.push_back("disproven_by_contradiction");
        KnowledgeEvidence neg_ev;
        neg_ev.evidence_id = KnowledgeEvidenceId("kevd_neg_001");
        neg_ev.evidence_kind = KnowledgeEvidenceKind::DirectExperience;
        neg_ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_neg_001");
        neg_ev.supports_claim = false;
        neg_ev.confidence_delta = -0.5;
        neg_ev.reliability = 0.9;
        claim.evidence_refs.push_back(neg_ev);
    }
    return claim;
}

static KnowledgePropagator makeValidPropagator(const std::string& id = "agent_001") {
    KnowledgePropagator p;
    p.owner = makeValidOwner(id);
    return p;
}

static KnowledgePropagationTarget makeValidTarget(const std::string& id = "agent_002") {
    KnowledgePropagationTarget t;
    t.owner = makeValidOwner(id);
    return t;
}

static KnowledgePropagationContext makeValidContext(KnowledgePropagationChannel channel = KnowledgePropagationChannel::DirectTeaching) {
    KnowledgePropagationContext ctx;
    ctx.channel = channel;
    ctx.trust_band = KnowledgePropagationTrustBand::High;
    ctx.trust_score = 0.8;
    ctx.distance_factor = 1.0;
    ctx.channel_quality = 1.0;
    ctx.noise_factor = 0.0;
    ctx.propagation_tick = Tick(100);
    return ctx;
}

static KnowledgePropagationAttempt makeValidAttempt() {
    KnowledgePropagationAttempt att;
    att.attempt_key = "attempt_001";
    att.propagator = makeValidPropagator("agent_001");
    att.target = makeValidTarget("agent_002");
    att.context = makeValidContext();
    KnowledgePropagationSourceClaim sc;
    sc.claim = makeValidClaim("know_agent_001_berry_red_has_effect_10_0", "agent_001");
    att.source_claims.push_back(sc);
    return att;
}

// ============================================================
// Enum Tests
// ============================================================

static void test_enum_roundtrip() {
    assert(toString(KnowledgePropagationChannel::DirectTeaching) == "direct_teaching");
    assert(knowledgePropagationChannelFromString("direct_teaching").value() == KnowledgePropagationChannel::DirectTeaching);
    assert(knowledgePropagationChannelFromString("invalid").is_error());

    assert(toString(KnowledgePropagationDecision::CreatedRecipientClaim) == "created_recipient_claim");
    assert(knowledgePropagationDecisionFromString("created_recipient_claim").value() == KnowledgePropagationDecision::CreatedRecipientClaim);
    assert(knowledgePropagationDecisionFromString("invalid").is_error());

    assert(toString(KnowledgePropagationTrustBand::High) == "high");
    assert(knowledgePropagationTrustBandFromString("high").value() == KnowledgePropagationTrustBand::High);
    assert(knowledgePropagationTrustBandFromString("invalid").is_error());

    assert(toString(KnowledgePropagationFailureReason::TrustTooLow) == "trust_too_low");
    assert(knowledgePropagationFailureReasonFromString("trust_too_low").value() == KnowledgePropagationFailureReason::TrustTooLow);
    assert(knowledgePropagationFailureReasonFromString("invalid").is_error());

    std::cout << "enum_roundtrip passed" << std::endl;
}

// ============================================================
// DTO Validate Tests
// ============================================================

static void test_options_validate() {
    KnowledgePropagationOptions opts;
    assert(opts.validateBasic().is_ok());

    opts.min_source_confidence = 1.5;
    assert(opts.validateBasic().is_error());
    opts.min_source_confidence = 0.35;

    opts.max_claims_per_attempt = 0;
    assert(opts.validateBasic().is_error());
    opts.max_claims_per_attempt = 8;

    std::cout << "options_validate passed" << std::endl;
}

static void test_propagator_validate() {
    auto p = makeValidPropagator();
    assert(p.validateBasic().is_ok());

    p.owner.kind = KnowledgeOwnerKind::Unknown;
    assert(p.validateBasic().is_error());

    std::cout << "propagator_validate passed" << std::endl;
}

static void test_target_validate() {
    auto t = makeValidTarget();
    assert(t.validateBasic().is_ok());

    t.owner.kind = KnowledgeOwnerKind::Unknown;
    assert(t.validateBasic().is_error());

    std::cout << "target_validate passed" << std::endl;
}

static void test_context_validate() {
    auto ctx = makeValidContext();
    assert(ctx.validateBasic().is_ok());

    ctx.channel = KnowledgePropagationChannel::Unknown;
    assert(ctx.validateBasic().is_error());
    ctx.channel = KnowledgePropagationChannel::DirectTeaching;

    ctx.trust_band = KnowledgePropagationTrustBand::TestOnly;
    assert(ctx.validateBasic().is_error());
    ctx.trust_band = KnowledgePropagationTrustBand::High;

    ctx.trust_score = 1.5;
    assert(ctx.validateBasic().is_error());
    ctx.trust_score = 0.8;

    std::cout << "context_validate passed" << std::endl;
}

static void test_attempt_validate() {
    auto att = makeValidAttempt();
    assert(att.validateBasic().is_ok());

    att.attempt_key = "";
    assert(att.validateBasic().is_error());
    att.attempt_key = "attempt_001";

    // target_existing_claims owner mismatch
    KnowledgeClaim bad_claim = makeValidClaim("know_bad", "agent_003");
    att.target_existing_claims.push_back(bad_claim);
    assert(att.validateBasic().is_error());
    att.target_existing_claims.clear();

    std::cout << "attempt_validate passed" << std::endl;
}

static void test_transfer_assessment_validate() {
    KnowledgeTransferAssessment a;
    a.source_knowledge_id = KnowledgeId("know_001");
    a.recommended_decision = KnowledgePropagationDecision::CreatedRecipientClaim;
    a.transfer_score = 0.5;
    a.created_confidence = 0.5;
    a.can_transfer = true;
    assert(a.validateBasic().is_ok());

    a.source_knowledge_id = KnowledgeId("");
    assert(a.validateBasic().is_error());
    a.source_knowledge_id = KnowledgeId("know_001");

    a.recommended_decision = KnowledgePropagationDecision::Unknown;
    assert(a.validateBasic().is_error());
    a.recommended_decision = KnowledgePropagationDecision::CreatedRecipientClaim;

    a.can_transfer = false;
    a.failure_reason = KnowledgePropagationFailureReason::Unknown;
    assert(a.validateBasic().is_error());
    a.failure_reason = KnowledgePropagationFailureReason::TrustTooLow;
    assert(a.validateBasic().is_ok());

    std::cout << "transfer_assessment_validate passed" << std::endl;
}

static void test_recipient_claim_draft_validate() {
    KnowledgeRecipientClaimDraft draft;
    draft.claim = makeValidClaim("know_001", "agent_002");
    draft.source_knowledge_id = KnowledgeId("know_src_001");
    draft.source_owner = makeValidOwner("agent_001");
    draft.channel = KnowledgePropagationChannel::DirectTeaching;
    draft.transfer_score = 0.5;

    // Must have Teaching evidence
    draft.claim.evidence_refs.clear();
    assert(draft.validateBasic().is_error());

    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_teaching_001");
    ev.evidence_kind = KnowledgeEvidenceKind::Teaching;
    ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_teaching_001");
    ev.supports_claim = true;
    ev.reliability = 0.8;
    ev.confidence_delta = 0.5;
    draft.claim.evidence_refs.push_back(ev);
    assert(draft.validateBasic().is_ok());

    std::cout << "recipient_claim_draft_validate passed" << std::endl;
}

static void test_plan_validate() {
    auto plan_r = KnowledgePropagationPlanner().planPropagation(makeValidAttempt());
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(plan.validateBasic().is_ok());

    std::cout << "plan_validate passed" << std::endl;
}

static void test_result_validate() {
    KnowledgePropagationResult result;
    result.decision = KnowledgePropagationDecision::Skipped;
    assert(result.validateBasic().is_ok());

    result.decision = KnowledgePropagationDecision::Unknown;
    assert(result.validateBasic().is_error());
    result.decision = KnowledgePropagationDecision::Rejected;
    assert(result.validateBasic().is_error());

    result.decision = KnowledgePropagationDecision::CreatedRecipientClaim;
    result.created_claims.push_back(makeValidClaim("know_001", "agent_002"));
    assert(result.validateBasic().is_ok());

    result.created_claims.clear();
    assert(result.validateBasic().is_error());

    std::cout << "result_validate passed" << std::endl;
}

// ============================================================
// Hidden Truth / Security Tests
// ============================================================

static void test_hidden_truth_rejected() {
    KnowledgePropagationAttempt att = makeValidAttempt();
    att.reason_keys.push_back("edible_profile");
    assert(att.validateBasic().is_error());

    att.reason_keys.clear();
    att.context.condition_keys.push_back("raw_state");
    assert(att.validateBasic().is_error());

    std::cout << "hidden_truth_rejected passed" << std::endl;
}

static void test_test_only_rejected() {
    KnowledgePropagationAttempt att = makeValidAttempt();
    att.context.channel = KnowledgePropagationChannel::TestOnly;
    assert(att.validateBasic().is_error());

    att.context.channel = KnowledgePropagationChannel::DirectTeaching;
    att.context.trust_band = KnowledgePropagationTrustBand::TestOnly;
    assert(att.validateBasic().is_error());

    std::cout << "test_only_rejected passed" << std::endl;
}

// ============================================================
// Planner Tests
// ============================================================

static void test_planner_success() {
    auto att = makeValidAttempt();
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].can_transfer);
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::CreatedRecipientClaim);
    assert(!plan.recipient_claim_drafts.empty());

    // Check recipient claim has Teaching evidence
    const auto& draft = plan.recipient_claim_drafts[0];
    bool has_teaching = false;
    for (const auto& ev : draft.claim.evidence_refs) {
        if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
            has_teaching = true;
            break;
        }
    }
    assert(has_teaching);
    assert(draft.claim.owner.entity_id.value() == "agent_002");

    std::cout << "planner_success passed" << std::endl;
}

static void test_planner_low_trust_fails() {
    auto att = makeValidAttempt();
    att.context.trust_score = 0.1;
    att.context.trust_band = KnowledgePropagationTrustBand::Distrusted;
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(!plan.assessments[0].can_transfer);
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::Failed);

    std::cout << "planner_low_trust_fails passed" << std::endl;
}

static void test_planner_deprecated_blocked() {
    auto att = makeValidAttempt();
    att.source_claims[0].claim = makeValidClaim("know_dep_001", "agent_001", KnowledgeStatus::Deprecated, 0.80);
    KnowledgePropagationPlanner planner;
    KnowledgePropagationOptions opts;
    opts.allow_deprecated = false;
    auto plan_r = planner.planPropagation(att, opts);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].failure_reason == KnowledgePropagationFailureReason::ClaimDeprecated);

    std::cout << "planner_deprecated_blocked passed" << std::endl;
}

static void test_planner_disproven_blocked() {
    auto att = makeValidAttempt();
    att.source_claims[0].claim = makeValidClaim("know_dis_001", "agent_001", KnowledgeStatus::Disproven, 0.80);
    KnowledgePropagationPlanner planner;
    KnowledgePropagationOptions opts;
    opts.allow_disproven = false;
    auto plan_r = planner.planPropagation(att, opts);
    if (plan_r.is_error()) {
        for (const auto& e : plan_r.errors()) {
            std::cerr << "Error: " << e.message_key << std::endl;
        }
    }
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].failure_reason == KnowledgePropagationFailureReason::ClaimDisproven);

    std::cout << "planner_disproven_blocked passed" << std::endl;
}

static void test_planner_same_owner_blocked() {
    auto att = makeValidAttempt();
    att.target.owner = makeValidOwner("agent_001");
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_error());

    std::cout << "planner_same_owner_blocked passed" << std::endl;
}

static void test_planner_candidate_hypothesis_penalty() {
    auto att = makeValidAttempt();
    att.source_claims[0].claim.status = KnowledgeStatus::Candidate;
    att.source_claims[0].claim.confidence.confidence = 0.40;
    att.source_claims[0].claim.confidence.band = confidenceToBand(0.40);
    KnowledgePropagationPlanner planner;
    KnowledgePropagationOptions opts;
    opts.min_source_confidence = 0.35;
    auto plan_r = planner.planPropagation(att, opts);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    if (!plan.recipient_claim_drafts.empty()) {
        // Candidate penalty: created_confidence *= 0.75
        double raw = plan.assessments[0].transfer_score;
        double expected = std::min(raw * 0.75, opts.max_created_confidence);
        assert(plan.assessments[0].created_confidence <= expected + 0.001);
        assert(plan.recipient_claim_drafts[0].claim.status == KnowledgeStatus::Candidate ||
               plan.recipient_claim_drafts[0].claim.status == KnowledgeStatus::Hypothesis);
    }

    std::cout << "planner_candidate_hypothesis_penalty passed" << std::endl;
}

static void test_planner_reinforcement() {
    auto att = makeValidAttempt();
    // Add existing matching claim for target
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::ReinforcedRecipientClaim);

    std::cout << "planner_reinforcement passed" << std::endl;
}

static void test_reinforced_existing_claim_updates_same_knowledge_id() {
    auto att = makeValidAttempt();
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.recipient_claim_drafts.empty());
    assert(plan.recipient_claim_drafts[0].claim.knowledge_id == existing.knowledge_id);

    std::cout << "reinforced_existing_claim_updates_same_knowledge_id passed" << std::endl;
}

static void test_propagation_different_effect_routes_to_revision() {
    auto att = makeValidAttempt();
    // Existing claim with same subject/action but different effect
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    existing.predicate.effect_key = "restore_health"; // different effect
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::WeakenedRecipientClaim);
    assert(plan.assessments[0].should_route_to_revision);

    std::cout << "propagation_different_effect_routes_to_revision passed" << std::endl;
}

static void test_propagation_different_effect_does_not_create_plain_claim() {
    auto att = makeValidAttempt();
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    existing.predicate.effect_key = "restore_health";
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::WeakenedRecipientClaim);
    assert(result.created_claims.empty());
    assert(result.updated_claims.empty());

    std::cout << "propagation_different_effect_does_not_create_plain_claim passed" << std::endl;
}

static void test_allow_create_false_does_not_create_claim() {
    auto att = makeValidAttempt();
    KnowledgePropagationOptions opts;
    opts.allow_create_recipient_claim = false;
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att, opts);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::Failed);

    KnowledgePropagationService service;
    auto result_r = service.propagate(att, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);
    assert(result.created_claims.empty());

    std::cout << "allow_create_false_does_not_create_claim passed" << std::endl;
}

static void test_allow_update_false_does_not_update_existing_claim() {
    auto att = makeValidAttempt();
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationOptions opts;
    opts.allow_update_recipient_claim = false;
    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(att, opts);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();
    assert(!plan.assessments.empty());
    assert(plan.assessments[0].recommended_decision == KnowledgePropagationDecision::Failed);

    KnowledgePropagationService service;
    auto result_r = service.propagate(att, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);
    assert(result.updated_claims.empty());

    std::cout << "allow_update_false_does_not_update_existing_claim passed" << std::endl;
}

// ============================================================
// Service Tests
// ============================================================

static void test_service_propagate() {
    auto att = makeValidAttempt();
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.ok());
    assert(result.decision == KnowledgePropagationDecision::CreatedRecipientClaim);
    assert(!result.created_claims.empty());
    assert(!result.event_drafts.empty());

    std::cout << "service_propagate passed" << std::endl;
}

static void test_service_skipped() {
    auto att = makeValidAttempt();
    att.source_claims.clear();
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Skipped);

    std::cout << "service_skipped passed" << std::endl;
}

static void test_failed_result_is_business_ok_and_validate_ok() {
    auto att = makeValidAttempt();
    att.context.trust_score = 0.05; // very low trust -> should fail
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok()); // Result<KnowledgePropagationResult> is ok
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);
    assert(result.ok()); // business ok() must include Failed
    assert(result.validateBasic().is_ok()); // validateBasic must pass

    std::cout << "failed_result_is_business_ok_and_validate_ok passed" << std::endl;
}

// ============================================================
// Applier Tests
// ============================================================

static void test_applier_adds_claim() {
    auto att = makeValidAttempt();
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();

    std::vector<KnowledgeClaim> current;
    KnowledgePropagationApplier applier;
    auto snapshot_r = applier.applyToTargetSnapshot(current, result);
    assert(snapshot_r.is_ok());
    auto snapshot = snapshot_r.value();
    assert(snapshot.size() == 1);

    std::cout << "applier_adds_claim passed" << std::endl;
}

static void test_reinforced_existing_claim_applier_does_not_duplicate() {
    auto att = makeValidAttempt();
    KnowledgeClaim existing = makeValidClaim("know_agent_002_berry_red_has_effect_10_0", "agent_002");
    att.target_existing_claims.push_back(existing);
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::ReinforcedRecipientClaim);
    assert(result.updated_claims.size() == 1);

    std::vector<KnowledgeClaim> current;
    current.push_back(existing);
    KnowledgePropagationApplier applier;
    auto snapshot_r = applier.applyToTargetSnapshot(current, result);
    assert(snapshot_r.is_ok());
    auto snapshot = snapshot_r.value();
    assert(snapshot.size() == 1); // must not duplicate
    assert(snapshot[0].knowledge_id == existing.knowledge_id);
    assert(snapshot[0].confidence.support_count > existing.confidence.support_count); // confidence updated

    std::cout << "reinforced_existing_claim_applier_does_not_duplicate passed" << std::endl;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    test_enum_roundtrip();
    test_options_validate();
    test_propagator_validate();
    test_target_validate();
    test_context_validate();
    test_attempt_validate();
    test_transfer_assessment_validate();
    test_recipient_claim_draft_validate();
    test_plan_validate();
    test_result_validate();
    test_hidden_truth_rejected();
    test_test_only_rejected();
    test_planner_success();
    test_planner_low_trust_fails();
    test_planner_deprecated_blocked();
    test_planner_disproven_blocked();
    test_planner_same_owner_blocked();
    test_planner_candidate_hypothesis_penalty();
    test_planner_reinforcement();
    test_reinforced_existing_claim_updates_same_knowledge_id();
    test_propagation_different_effect_routes_to_revision();
    test_propagation_different_effect_does_not_create_plain_claim();
    test_allow_create_false_does_not_create_claim();
    test_allow_update_false_does_not_update_existing_claim();
    test_service_propagate();
    test_service_skipped();
    test_failed_result_is_business_ok_and_validate_ok();
    test_applier_adds_claim();
    test_reinforced_existing_claim_applier_does_not_duplicate();

    std::cout << "All knowledge_propagation tests passed" << std::endl;
    return 0;
}
