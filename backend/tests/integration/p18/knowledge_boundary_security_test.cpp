#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_formation.h"
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

static KnowledgeSubject makeValidSubject() {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = "berry_red";
    return subject;
}

static KnowledgePredicate makeValidPredicate() {
    KnowledgePredicate pred;
    pred.relation_type = KnowledgeRelationType::HasEffect;
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

static KnowledgeConfidence makeValidConfidence() {
    KnowledgeConfidence conf;
    conf.confidence = 0.75;
    conf.band = KnowledgeConfidenceBand::Strong;
    conf.support_count = 3;
    return conf;
}

void run_knowledge_boundary_security_tests() {
    // ============================================================
    // hidden truth guard case-insensitive
    // ============================================================
    {
        assert(containsKnowledgeForbiddenKey("edible_profile"));
        assert(containsKnowledgeForbiddenKey("Edible_Profile"));
        assert(containsKnowledgeForbiddenKey("EDIBLE_PROFILE"));
        assert(containsKnowledgeForbiddenKey("gamestate"));
        assert(containsKnowledgeForbiddenKey("GAMESTATE"));
        assert(containsKnowledgeForbiddenKey("Health_Delta"));
        assert(containsKnowledgeForbiddenKey("HIDDEN_TRUTH"));
    }

    // ============================================================
    // hidden truth in DTOs rejected
    // ============================================================
    {
        KnowledgeSubject subject = makeValidSubject();
        subject.subject_id = "edible_profile";
        assert(subject.validateBasic().is_error());

        subject = makeValidSubject();
        subject.safe_tags = {"gamestate"};
        assert(subject.validateBasic().is_error());

        KnowledgePredicate pred = makeValidPredicate();
        pred.effect_key = "hunger_delta";
        assert(pred.validateBasic().is_error());

        pred = makeValidPredicate();
        pred.value_summary_key = "health_delta";
        assert(pred.validateBasic().is_error());
    }

    // ============================================================
    // TestOnly default rejected
    // ============================================================
    {
        KnowledgeOwner owner = makeValidOwner();
        owner.kind = KnowledgeOwnerKind::TestOnly;
        owner.external_key = "";
        assert(owner.validateBasic().is_error());
    }

    // ============================================================
    // P18 does not create forbidden statuses
    // ============================================================
    {
        KnowledgeClaim claim;
        claim.knowledge_id = KnowledgeId("know_test");
        claim.owner = makeValidOwner();
        claim.subject = makeValidSubject();
        claim.predicate = makeValidPredicate();
        claim.confidence = makeValidConfidence();
        claim.evidence_refs = {makeValidEvidence()};
        claim.status = KnowledgeStatus::Deprecated;
        assert(claim.validateBasic().is_error());

        claim.status = KnowledgeStatus::Disproven;
        assert(claim.validateBasic().is_error());
    }

    // ============================================================
    // P18 does not read ObjectDefinition hidden truth
    // ============================================================
    {
        // Verify knowledgeForbiddenKeys includes hidden truth fields
        auto keys = knowledgeForbiddenKeys();
        bool has_hidden = false;
        for (const auto& k : keys) {
            if (k == "hidden_truth" || k == "object_truth") {
                has_hidden = true;
            }
        }
        assert(has_hidden);
    }

    std::cout << "knowledge_boundary_security tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_boundary_security_tests();
    return 0;
}
