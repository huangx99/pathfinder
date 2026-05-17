#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_formation.h"
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
    // KnowledgeFormationPlan rejects TestOnly/Deprecated/Disproven
    // ============================================================
    {
        KnowledgeFormationPlan plan;
        plan.plan_key = "plan_test";
        plan.input.owner = makeValidOwner();
        plan.input.summary.key.owner.kind = MemoryOwnerKind::Agent;
        plan.input.summary.key.owner.entity_id = EntityId("agent_001");
        plan.input.summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
        plan.input.summary.key.subject.subject_id = "berry_red";
        plan.input.summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
        plan.input.summary.summary_id = MemorySummaryId("sum_001");
        plan.input.summary.schema_version = "memory_summary.v1";
        plan.input.summary.compression_level = MemoryCompressionLevel::LightSummary;
        plan.input.summary.highest_importance = MemoryImportance::Normal;
        plan.input.summary.aggregate_strength = 0.75;
        plan.input.summary.source_memory_count = 3;
        plan.input.summary.source_memory_ids = {MemoryId("mem_001"), MemoryId("mem_002"), MemoryId("mem_003")};
        plan.input.summary.representative_memory_ids = {MemoryId("mem_001")};
        MemoryEvidenceRef ref;
        ref.source_kind = MemorySourceKind::DirectEvent;
        ref.source_event_id = EventId("evt_001");
        plan.input.summary.evidence_refs = {ref};
        plan.input.target_relation = KnowledgeRelationType::HasEffect;
        plan.input.action_key = "eat";
        plan.subject = makeValidSubject();
        plan.predicate = makeValidPredicate();
        plan.evidence_refs = {makeValidEvidence()};
        plan.projected_confidence = makeValidConfidence();
        plan.projected_status = KnowledgeStatus::Active;
        assert(plan.validateBasic().is_ok());

        plan.projected_status = KnowledgeStatus::TestOnly;
        assert(plan.validateBasic().is_error());

        plan.projected_status = KnowledgeStatus::Deprecated;
        assert(plan.validateBasic().is_error());

        plan.projected_status = KnowledgeStatus::Disproven;
        assert(plan.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeEventDraft rejects TestOnly relation and status
    // ============================================================
    {
        KnowledgeEventDraft draft;
        draft.event_key = "test";
        draft.knowledge_id = KnowledgeId("know_test");
        draft.owner = makeValidOwner();
        draft.subject = makeValidSubject();
        draft.relation_type = KnowledgeRelationType::HasEffect;
        draft.status = KnowledgeStatus::Active;
        draft.decision = KnowledgeFormationDecision::CreatedClaim;
        assert(draft.validateBasic().is_ok());

        draft.relation_type = KnowledgeRelationType::TestOnly;
        assert(draft.validateBasic().is_error());

        draft.relation_type = KnowledgeRelationType::HasEffect;
        draft.status = KnowledgeStatus::TestOnly;
        assert(draft.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeQuery rejects TestOnly mode and filters
    // ============================================================
    {
        KnowledgeQuery q;
        q.owner = makeValidOwner();
        q.mode = KnowledgeQueryMode::ByOwner;
        assert(q.validateBasic().is_ok());

        q.mode = KnowledgeQueryMode::TestOnly;
        assert(q.validateBasic().is_error());

        q.mode = KnowledgeQueryMode::ByOwner;
        q.relation_type = KnowledgeRelationType::TestOnly;
        assert(q.validateBasic().is_error());

        q.relation_type = std::nullopt;
        q.min_status = KnowledgeStatus::TestOnly;
        assert(q.validateBasic().is_error());

        q.min_status = KnowledgeStatus::Deprecated;
        assert(q.validateBasic().is_error());

        q.min_status = std::nullopt;
        q.min_confidence_band = KnowledgeConfidenceBand::Unknown;
        assert(q.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeProjectionItem rejects TestOnly/Deprecated/Disproven
    // ============================================================
    {
        KnowledgeProjectionItem item;
        item.knowledge_id = KnowledgeId("know_test");
        item.subject = makeValidSubject();
        item.predicate = makeValidPredicate();
        item.confidence = makeValidConfidence();
        item.status = KnowledgeStatus::Active;
        item.teaching_profile.teachable = false;
        item.projection_flags.usable_by_ai = true;
        assert(item.validateBasic().is_ok());

        item.status = KnowledgeStatus::TestOnly;
        assert(item.validateBasic().is_error());

        item.status = KnowledgeStatus::Deprecated;
        assert(item.validateBasic().is_error());

        item.status = KnowledgeStatus::Disproven;
        assert(item.validateBasic().is_error());
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

    // ============================================================
    // BLOCKER-8: ProjectionBuilder rejects invalid claims
    // ============================================================
    {
        KnowledgeClaim claim;
        claim.knowledge_id = KnowledgeId("know_test");
        claim.owner = makeValidOwner();
        claim.subject = makeValidSubject();
        claim.predicate = makeValidPredicate();
        claim.confidence = makeValidConfidence();
        claim.evidence_refs = {makeValidEvidence()};
        claim.status = KnowledgeStatus::Active;
        claim.created_tick = Tick(10);
        claim.updated_tick = Tick(10);

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;

        // Valid claim -> ok and valid projection
        auto ok_result = builder.buildProjection({claim}, query);
        assert(ok_result.is_ok());
        assert(ok_result.value().validateBasic().is_ok());

        // Invalid claim (Deprecated status) -> error
        claim.status = KnowledgeStatus::Deprecated;
        auto bad_result = builder.buildProjection({claim}, query);
        assert(bad_result.is_error());
    }

    std::cout << "knowledge_boundary_security tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_boundary_security_tests();
    return 0;
}
