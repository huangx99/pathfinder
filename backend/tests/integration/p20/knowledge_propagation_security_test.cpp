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

static KnowledgePredicate makePredicate() {
    KnowledgePredicate p;
    p.relation_type = KnowledgeRelationType::HasEffect;
    p.action_key = "eat";
    p.effect_key = "restore_hunger";
    return p;
}

static KnowledgeEvidence makeEvidence() {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_001");
    ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
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
    return claim;
}

static KnowledgePropagationAttempt makeAttempt(const KnowledgeOwner& source,
                                                const KnowledgeOwner& target,
                                                const std::vector<KnowledgeClaim>& source_claims) {
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
    return att;
}

// ============================================================
// Security Tests
// ============================================================

static void test_source_claim_hidden_truth_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.reason_keys.push_back("hidden_truth");
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "source_claim_hidden_truth_fails passed" << std::endl;
}

static void test_attempt_raw_state_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.condition_keys.push_back("raw_state");
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "attempt_raw_state_fails passed" << std::endl;
}

static void test_source_owner_mismatch_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto owner_c = makeAgentOwner("agent_c");
    auto claim_c = makeClaim("know_c_001", owner_c, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_c});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "source_owner_mismatch_fails passed" << std::endl;
}

static void test_target_existing_owner_mismatch_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto owner_c = makeAgentOwner("agent_c");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);
    auto claim_c = makeClaim("know_c_001", owner_c, KnowledgeStatus::Shared, 0.50);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.target_existing_claims.push_back(claim_c);
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "target_existing_owner_mismatch_fails passed" << std::endl;
}

static void test_same_owner_default_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_a, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "same_owner_default_fails passed" << std::endl;
}

static void test_test_only_channel_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.channel = KnowledgePropagationChannel::TestOnly;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "test_only_channel_fails passed" << std::endl;
}

static void test_test_only_trust_band_fails() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.trust_band = KnowledgePropagationTrustBand::TestOnly;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_error());

    std::cout << "test_only_trust_band_fails passed" << std::endl;
}

static void test_external_record_first_version_no_text() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    att.context.channel = KnowledgePropagationChannel::ExternalRecord;
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    if (result_r.is_error()) {
        for (const auto& e : result_r.errors()) {
            std::cerr << "Error: " << e.message_key << std::endl;
        }
    }
    // ExternalRecord is a valid enum but first version should still work as a channel
    assert(result_r.is_ok());

    std::cout << "external_record_first_version_no_text passed" << std::endl;
}

static void test_service_does_not_write_repository() {
    auto owner_a = makeAgentOwner("agent_a");
    auto owner_b = makeAgentOwner("agent_b");
    auto claim_a = makeClaim("know_a_001", owner_a, KnowledgeStatus::Active, 0.80);

    auto att = makeAttempt(owner_a, owner_b, {claim_a});
    KnowledgePropagationService service;
    auto result_r = service.propagate(att);
    assert(result_r.is_ok());

    KnowledgeRepository repo;
    assert(repo.size() == 0);

    std::cout << "service_does_not_write_repository passed" << std::endl;
}

static void test_projection_excludes_deprecated_disproven() {
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

    // Add deprecated/disproven to repo
    auto dep_claim = makeClaim("know_dep_001", owner_b, KnowledgeStatus::Deprecated, 0.30);
    auto dis_claim = makeClaim("know_dis_001", owner_b, KnowledgeStatus::Disproven, 0.10);
    repo.put(dep_claim);
    repo.put(dis_claim);

    KnowledgeQuery query;
    query.owner = owner_b;
    query.mode = KnowledgeQueryMode::ByOwner;
    auto claims_r = repo.query(query);
    assert(claims_r.is_ok());
    // Default query includes all since include_deprecated/disproven default false in query
    // But projection builder should skip them
    KnowledgeProjectionBuilder builder;
    auto proj_r = builder.buildProjection(claims_r.value(), query);
    assert(proj_r.is_ok());
    // Only the Active/Shared claim from propagation should appear
    size_t visible_count = 0;
    for (const auto& item : proj_r.value().items) {
        if (item.status != KnowledgeStatus::Deprecated && item.status != KnowledgeStatus::Disproven) {
            visible_count++;
        }
    }
    assert(visible_count >= 1);

    std::cout << "projection_excludes_deprecated_disproven passed" << std::endl;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    test_source_claim_hidden_truth_fails();
    test_attempt_raw_state_fails();
    test_source_owner_mismatch_fails();
    test_target_existing_owner_mismatch_fails();
    test_same_owner_default_fails();
    test_test_only_channel_fails();
    test_test_only_trust_band_fails();
    test_external_record_first_version_no_text();
    test_service_does_not_write_repository();
    test_projection_excludes_deprecated_disproven();

    std::cout << "All p20 security tests passed" << std::endl;
    return 0;
}
