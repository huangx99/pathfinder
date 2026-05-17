#include "pathfinder/knowledge/knowledge_projection.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::memory;
using namespace pathfinder::foundation;

static KnowledgeOwner makeValidOwner() {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId("agent_001");
    return owner;
}

static KnowledgeSubject makeValidSubject(const std::string& id = "berry_red") {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = id;
    return subject;
}

static KnowledgePredicate makeValidPredicate(KnowledgeRelationType rt = KnowledgeRelationType::HasEffect) {
    KnowledgePredicate pred;
    pred.relation_type = rt;
    pred.action_key = "eat";
    pred.effect_key = "restore_hunger";
    return pred;
}

static KnowledgeEvidence makeValidEvidence() {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_test_001");
    ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    ev.source_summary_id = MemorySummaryId("sum_001");
    ev.supports_claim = true;
    ev.confidence_delta = 0.5;
    ev.reliability = 0.9;
    return ev;
}

static KnowledgeProjectionFlags makeValidProjectionFlags() {
    KnowledgeProjectionFlags pf;
    pf.visible_to_player = true;
    pf.usable_by_ai = true;
    pf.usable_for_action = true;
    return pf;
}

static KnowledgeConfidence makeValidConfidence(double conf = 0.75) {
    KnowledgeConfidence c;
    c.confidence = conf;
    c.band = confidenceToBand(conf);
    c.support_count = 3;
    return c;
}

static KnowledgeClaim makeClaim(const std::string& know_id, KnowledgeStatus status, double confidence) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(know_id);
    claim.owner = makeValidOwner();
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
    return claim;
}

void run_knowledge_projection_tests() {
    // ============================================================
    // Projection strips evidence details and sorts deterministically
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        claims.push_back(makeClaim("know_001", KnowledgeStatus::Active, 0.6));
        claims.push_back(makeClaim("know_002", KnowledgeStatus::Teachable, 0.8));
        claims.push_back(makeClaim("know_003", KnowledgeStatus::Hypothesis, 0.4));

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.include_inactive = true;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        auto& proj = result.value();
        assert(proj.items.size() == 3);

        // Verify evidence details are stripped (not exposed in projection)
        for (const auto& item : proj.items) {
            // ProjectionItem does not have evidence_refs field
            assert(item.knowledge_id.value().find("know_") == 0);
        }

        // Deterministic order: Teachable > Active > Hypothesis
        assert(proj.items[0].knowledge_id.value() == "know_002");
        assert(proj.items[1].knowledge_id.value() == "know_001");
        assert(proj.items[2].knowledge_id.value() == "know_003");
    }

    // ============================================================
    // Teachable query filters
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        claims.push_back(makeClaim("know_a", KnowledgeStatus::Active, 0.6));
        claims.push_back(makeClaim("know_t", KnowledgeStatus::Teachable, 0.8));

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::Teachable;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 1);
        assert(result.value().items[0].knowledge_id.value() == "know_t");
    }

    // ============================================================
    // RiskRelated query filters
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        auto c1 = makeClaim("know_safe", KnowledgeStatus::Active, 0.6);
        auto c2 = makeClaim("know_risk", KnowledgeStatus::Active, 0.6);
        c2.predicate.relation_type = KnowledgeRelationType::HasRisk;
        c2.predicate.action_key = "approach";
        claims.push_back(c1);
        claims.push_back(c2);

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::RiskRelated;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 1);
        assert(result.value().items[0].knowledge_id.value() == "know_risk");
    }

    // ============================================================
    // HighConfidence query filters
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        claims.push_back(makeClaim("know_low", KnowledgeStatus::Active, 0.4));
        claims.push_back(makeClaim("know_high", KnowledgeStatus::Active, 0.8));

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::HighConfidence;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 1);
        assert(result.value().items[0].knowledge_id.value() == "know_high");
    }

    // ============================================================
    // Truncation
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        for (int i = 0; i < 10; ++i) {
            claims.push_back(makeClaim("know_" + std::to_string(i), KnowledgeStatus::Active, 0.5 + i * 0.01));
        }

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 5;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 5);
        assert(result.value().truncated);
    }

    // ============================================================
    // Projection validateBasic
    // ============================================================
    {
        KnowledgeProjection proj;
        proj.owner = makeValidOwner();
        assert(proj.validateBasic().is_ok());

        proj.reason_keys = {"edible_profile"};
        assert(proj.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeProjectionItem rejects invalid statuses
    // ============================================================
    {
        KnowledgeProjectionItem item;
        item.knowledge_id = KnowledgeId("know_001");
        item.subject = makeValidSubject();
        item.predicate = makeValidPredicate();
        item.confidence = makeValidConfidence();
        item.status = KnowledgeStatus::Active;
        item.teaching_profile.teachable = false;
        item.projection_flags = makeValidProjectionFlags();
        assert(item.validateBasic().is_ok());

        item.status = KnowledgeStatus::TestOnly;
        assert(item.validateBasic().is_error());

        item.status = KnowledgeStatus::Deprecated;
        assert(item.validateBasic().is_error());

        item.status = KnowledgeStatus::Disproven;
        assert(item.validateBasic().is_error());
    }

    // ============================================================
    // BLOCKER-8: ProjectionBuilder rejects invalid claim input
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        auto bad_claim = makeClaim("know_bad", KnowledgeStatus::Active, 0.6);
        bad_claim.status = KnowledgeStatus::Deprecated; // invalid claim
        claims.push_back(bad_claim);

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.include_inactive = true;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_error());
    }

    // ============================================================
    // BLOCKER-8: ProjectionBuilder never returns invalid projection
    // ============================================================
    {
        std::vector<KnowledgeClaim> claims;
        claims.push_back(makeClaim("know_001", KnowledgeStatus::Active, 0.6));

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto result = builder.buildProjection(claims, query);
        assert(result.is_ok());
        assert(result.value().validateBasic().is_ok());
    }

    std::cout << "knowledge_projection tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_projection_tests();
    return 0;
}
