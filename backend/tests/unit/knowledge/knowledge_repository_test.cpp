#include "pathfinder/knowledge/knowledge_repository.h"
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

void run_knowledge_repository_tests() {
    // ============================================================
    // put / find / remove / clear
    // ============================================================
    {
        KnowledgeRepository repo;
        auto c1 = makeClaim("know_001", KnowledgeStatus::Active, 0.6);
        assert(repo.put(c1).is_ok());
        assert(repo.size() == 1);

        auto found = repo.find(KnowledgeId("know_001"));
        assert(found.is_ok());
        assert(found.value().has_value());
        assert(found.value()->status == KnowledgeStatus::Active);

        auto missing = repo.find(KnowledgeId("know_missing"));
        assert(missing.is_ok());
        assert(!missing.value().has_value());

        assert(repo.remove(KnowledgeId("know_001")).is_ok());
        assert(repo.size() == 0);

        repo.clear();
    }

    // ============================================================
    // put rejects invalid claim
    // ============================================================
    {
        KnowledgeRepository repo;
        auto bad = makeClaim("know_bad", KnowledgeStatus::Unknown, 0.6);
        assert(repo.put(bad).is_error());
        assert(repo.size() == 0);
    }

    // ============================================================
    // query deterministic order
    // ============================================================
    {
        KnowledgeRepository repo;
        auto c1 = makeClaim("know_001", KnowledgeStatus::Active, 0.6);
        c1.updated_tick = Tick(5);
        auto c2 = makeClaim("know_002", KnowledgeStatus::Teachable, 0.8);
        c2.updated_tick = Tick(10);
        auto c3 = makeClaim("know_003", KnowledgeStatus::Hypothesis, 0.4);
        c3.updated_tick = Tick(15);
        auto c4 = makeClaim("know_004", KnowledgeStatus::Active, 0.7);
        c4.updated_tick = Tick(8);

        assert(repo.put(c1).is_ok());
        assert(repo.put(c2).is_ok());
        assert(repo.put(c3).is_ok());
        assert(repo.put(c4).is_ok());

        KnowledgeQuery q;
        q.owner = makeValidOwner();
        q.mode = KnowledgeQueryMode::ByOwner;
        q.include_inactive = true;
        q.limit = 10;

        auto result = repo.query(q);
        assert(result.is_ok());
        auto& claims = result.value();
        assert(claims.size() == 4);

        // Order: Teachable(5) > Active(4) > Hypothesis(3)
        // Within Active: confidence desc, then updated_tick desc
        assert(claims[0].knowledge_id.value() == "know_002"); // Teachable
        assert(claims[1].knowledge_id.value() == "know_004"); // Active 0.7
        assert(claims[2].knowledge_id.value() == "know_001"); // Active 0.6
        assert(claims[3].knowledge_id.value() == "know_003"); // Hypothesis
    }

    // ============================================================
    // query by mode filters
    // ============================================================
    {
        KnowledgeRepository repo;
        auto c1 = makeClaim("know_a", KnowledgeStatus::Active, 0.6);
        auto c2 = makeClaim("know_t", KnowledgeStatus::Teachable, 0.8);
        auto c3 = makeClaim("know_r", KnowledgeStatus::Active, 0.6);
        c3.predicate.relation_type = KnowledgeRelationType::HasRisk;
        c3.predicate.action_key = "approach";

        assert(repo.put(c1).is_ok());
        assert(repo.put(c2).is_ok());
        assert(repo.put(c3).is_ok());

        KnowledgeQuery q;
        q.owner = makeValidOwner();
        q.mode = KnowledgeQueryMode::Teachable;
        q.limit = 10;
        auto r = repo.query(q);
        assert(r.is_ok());
        assert(r.value().size() == 1);
        assert(r.value()[0].knowledge_id.value() == "know_t");

        q.mode = KnowledgeQueryMode::RiskRelated;
        r = repo.query(q);
        assert(r.is_ok());
        assert(r.value().size() == 1);
        assert(r.value()[0].knowledge_id.value() == "know_r");

        q.mode = KnowledgeQueryMode::HighConfidence;
        r = repo.query(q);
        assert(r.is_ok());
        assert(r.value().size() == 1);
        assert(r.value()[0].knowledge_id.value() == "know_t");
    }

    // ============================================================
    // listByOwner
    // ============================================================
    {
        KnowledgeRepository repo;
        auto c1 = makeClaim("know_001", KnowledgeStatus::Active, 0.6);
        assert(repo.put(c1).is_ok());

        auto list = repo.listByOwner(makeValidOwner());
        assert(list.is_ok());
        assert(list.value().size() == 1);

        KnowledgeOwner other;
        other.kind = KnowledgeOwnerKind::Agent;
        other.entity_id = EntityId("agent_999");
        list = repo.listByOwner(other);
        assert(list.is_ok());
        assert(list.value().empty());
    }

    std::cout << "knowledge_repository tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_repository_tests();
    return 0;
}
