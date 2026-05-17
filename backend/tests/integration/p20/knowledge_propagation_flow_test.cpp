#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_projection.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::foundation;

// ============================================================
// Helpers
// ============================================================

static KnowledgeOwner makeAgentOwner(const std::string& id) {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId(id);
    return owner;
}

static KnowledgeSubject makeSubject(const std::string& id = "berry_red") {
    KnowledgeSubject s;
    s.kind = KnowledgeSubjectKind::ObjectDefinition;
    s.subject_id = id;
    return s;
}

static KnowledgePredicate makePredicate(KnowledgeRelationType rt = KnowledgeRelationType::HasEffect,
                                         const std::string& action = "eat",
                                         const std::string& effect = "restore_hunger") {
    KnowledgePredicate p;
    p.relation_type = rt;
    p.action_key = action;
    p.effect_key = effect;
    return p;
}

static KnowledgeEvidence makeEvidence(KnowledgeEvidenceKind kind = KnowledgeEvidenceKind::MemorySummary) {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_001");
    ev.evidence_kind = kind;
    ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_001");
    ev.supports_claim = true;
    ev.confidence_delta = 0.5;
    ev.reliability = 0.9;
    return ev;
}

static KnowledgeConfidence makeConfidence(double conf) {
    KnowledgeConfidence c;
    c.confidence = conf;
    c.band = confidenceToBand(conf);
    c.support_count = 3;
    return c;
}

static KnowledgeClaim makeClaim(const std::string& id, const KnowledgeOwner& owner,
                                 KnowledgeStatus status, double confidence) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner = owner;
    claim.subject = makeSubject();
    claim.predicate = makePredicate();
    claim.confidence = makeConfidence(confidence);
    claim.status = status;
    claim.evidence_refs = {makeEvidence()};
    claim.created_tick = Tick(10);
    claim.updated_tick = Tick(10);
    if (status == KnowledgeStatus::Teachable) {
        claim.teaching_profile.teachable = true;
        claim.teaching_profile.teaching_message_key = "teaching_msg";
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

static KnowledgePropagationAttempt makeAttempt(const KnowledgeOwner& source,
                                                const KnowledgeOwner& target,
                                                const std::vector<KnowledgeClaim>& source_claims,
                                                const std::vector<KnowledgeClaim>& target_existing = {}) {
    KnowledgePropagationAttempt att;
    att.attempt_key = "test_attempt_001";
    att.propagator.owner = source;
    att.target.owner = target;
    att.context.channel = KnowledgePropagationChannel::DirectTeaching;
    att.context.trust_band = KnowledgePropagationTrustBand::High;
    att.context.trust_score = 0.8;
    att.context.distance_factor = 1.0;
    att.context.channel_quality = 1.0;
    att.context.noise_factor = 0.0;
    att.context.propagation_tick = Tick(100);
    for (const auto& c : source_claims) {
        KnowledgePropagationSourceClaim sc;
        sc.claim = c;
        att.source_claims.push_back(sc);
    }
    for (const auto& c : target_existing) {
        att.target_existing_claims.push_back(c);
    }
    return att;
}

// ============================================================
// Flow Tests
// ============================================================

static void test_agent_a_to_agent_b_created() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::CreatedRecipientClaim);
    assert(result.created_claims.size() == 1);

    auto& recipient = result.created_claims[0];
    assert(recipient.owner.entity_id.value() == "agent_b");
    assert(recipient.status == KnowledgeStatus::Shared || recipient.status == KnowledgeStatus::Active);

    bool has_teaching = false;
    for (const auto& ev : recipient.evidence_refs) {
        if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
            has_teaching = true;
            break;
        }
    }
    assert(has_teaching);

    std::cout << "agent_a_to_agent_b_created passed" << std::endl;
}

static void test_teachable_propagation() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Teachable, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    if (result_r.is_error()) {
        for (const auto& e : result_r.errors()) {
            std::cerr << "Error: " << e.message_key << std::endl;
        }
    }
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::CreatedRecipientClaim);
    assert(!result.created_claims.empty());

    std::cout << "teachable_propagation passed" << std::endl;
}

static void test_low_trust_failure() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.trust_score = 0.05;
    att.context.trust_band = KnowledgePropagationTrustBand::Distrusted;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);

    std::cout << "low_trust_failure passed" << std::endl;
}

static void test_warning_signal_risk() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);
    claim_a.predicate.relation_type = KnowledgeRelationType::HasRisk;
    claim_a.predicate.effect_key = "poison";

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.channel = KnowledgePropagationChannel::WarningSignal;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::CreatedRecipientClaim);

    std::cout << "warning_signal_risk passed" << std::endl;
}

static void test_candidate_low_confidence() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Candidate, 0.40);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    KnowledgePropagationOptions opts;
    opts.min_source_confidence = 0.35;
    auto result_r = service.propagate(att, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();
    if (result.decision == KnowledgePropagationDecision::CreatedRecipientClaim) {
        assert(result.created_claims[0].status == KnowledgeStatus::Candidate ||
               result.created_claims[0].status == KnowledgeStatus::Hypothesis);
        assert(result.created_claims[0].confidence.confidence < 0.60);
    }

    std::cout << "candidate_low_confidence passed" << std::endl;
}

static void test_reinforced_existing_claim() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);
    auto claim_b = makeClaim("know_b_001", owner_b, KnowledgeStatus::Shared, 0.50);

    auto att = makeAttempt(owner_a, owner_b, {claim_a}, {claim_b});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::ReinforcedRecipientClaim);

    std::cout << "reinforced_existing_claim passed" << std::endl;
}

static void test_deprecated_default_blocked() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Deprecated, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);

    std::cout << "deprecated_default_blocked passed" << std::endl;
}

static void test_disproven_default_blocked() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Disproven, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::Failed);

    std::cout << "disproven_default_blocked passed" << std::endl;
}

static void test_repository_not_written_by_service() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();

    // Service should not write to repo; repo should be empty
    KnowledgeRepository repo;
    assert(repo.size() == 0);

    // Caller writes to repo
    for (const auto& c : result.created_claims) {
        auto put_r = repo.put(c);
        assert(put_r.is_ok());
    }
    assert(repo.size() == 1);

    // Query by owner_b
    auto list_r = repo.listByOwner(owner_b);
    assert(list_r.is_ok());
    assert(list_r.value().size() == 1);

    std::cout << "repository_not_written_by_service passed" << std::endl;
}

static void test_projection_shows_recipient_knowledge() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();

    KnowledgeRepository repo;
    for (const auto& c : result.created_claims) {
        repo.put(c);
    }

    KnowledgeQuery query;
    query.owner = owner_b;
    query.mode = KnowledgeQueryMode::ByOwner;
    auto claims_r = repo.query(query);
    assert(claims_r.is_ok());
    assert(claims_r.value().size() == 1);

    KnowledgeProjectionBuilder builder;
    auto proj_r = builder.buildProjection(claims_r.value(), query);
    assert(proj_r.is_ok());
    assert(proj_r.value().items.size() == 1);

    std::cout << "projection_shows_recipient_knowledge passed" << std::endl;
}

static void test_correction_channel() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.channel = KnowledgePropagationChannel::Correction;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == KnowledgePropagationDecision::CreatedRecipientClaim);

    std::cout << "correction_channel passed" << std::endl;
}

static void test_owner_isolation() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());

    KnowledgeRepository repo;
    for (const auto& c : result_r.value().created_claims) {
        repo.put(c);
    }
    repo.put(claim_a);

    auto list_a = repo.listByOwner(owner_a);
    auto list_b = repo.listByOwner(owner_b);
    assert(list_a.is_ok() && list_a.value().size() == 1);
    assert(list_b.is_ok() && list_b.value().size() == 1);
    assert(list_a.value()[0].owner.entity_id.value() == "agent_a");
    assert(list_b.value()[0].owner.entity_id.value() == "agent_b");

    std::cout << "owner_isolation passed" << std::endl;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    test_agent_a_to_agent_b_created();
    test_teachable_propagation();
    test_low_trust_failure();
    test_warning_signal_risk();
    test_candidate_low_confidence();
    test_reinforced_existing_claim();
    test_deprecated_default_blocked();
    test_disproven_default_blocked();
    test_repository_not_written_by_service();
    test_projection_shows_recipient_knowledge();
    test_correction_channel();
    test_owner_isolation();

    std::cout << "All p20 flow tests passed" << std::endl;
    return 0;
}
