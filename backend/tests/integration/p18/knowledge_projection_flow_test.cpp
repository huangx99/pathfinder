#include "pathfinder/knowledge/knowledge_formation.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_projection.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::memory;
using namespace pathfinder::foundation;

static MemorySummary makeValidSummary() {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId("sum_001");
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = "berry_red";
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = MemoryImportance::Normal;
    summary.aggregate_strength = 0.75;
    summary.source_memory_count = 5;
    summary.source_memory_ids = {
        MemoryId("mem_001"), MemoryId("mem_002"), MemoryId("mem_003"),
        MemoryId("mem_004"), MemoryId("mem_005")
    };
    summary.representative_memory_ids = {MemoryId("mem_001"), MemoryId("mem_002")};
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    summary.evidence_refs = {ref};
    summary.has_long_term_source = true;
    summary.teaching_candidate_count = 2;
    summary.summarized_tick = Tick(100);
    return summary;
}

static KnowledgeOwner makeValidOwner() {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId("agent_001");
    return owner;
}

void run_knowledge_projection_flow_tests() {
    // ============================================================
    // Formation -> Repository -> Projection flow
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        input.effect_key = "restore_hunger";

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        assert(result.value().claim.has_value());
        auto claim = result.value().claim.value();

        KnowledgeRepository repo;
        assert(repo.put(claim).is_ok());

        // Query all
        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 10;

        auto repo_result = repo.query(query);
        assert(repo_result.is_ok());

        KnowledgeProjectionBuilder builder;
        auto proj = builder.buildProjection(repo_result.value(), query);
        assert(proj.is_ok());
        assert(proj.value().items.size() == 1);
        assert(proj.value().items[0].knowledge_id == claim.knowledge_id);

        // Projection does not expose evidence internals
        // KnowledgeProjectionItem has no evidence_refs field
        assert(proj.value().validateBasic().is_ok());

        std::cout << "formation_repository_projection_flow passed" << std::endl;
    }

    // ============================================================
    // Multiple claims projection with deterministic order
    // ============================================================
    {
        KnowledgeRepository repo;

        auto make_claim = [](const std::string& id, KnowledgeStatus st, double conf, const std::string& subject_id) {
            KnowledgeClaim claim;
            claim.knowledge_id = KnowledgeId(id);
            claim.owner.kind = KnowledgeOwnerKind::Agent;
            claim.owner.entity_id = EntityId("agent_001");
            claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
            claim.subject.subject_id = subject_id;
            claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
            claim.predicate.action_key = "eat";
            claim.predicate.effect_key = "restore_hunger";
            claim.confidence.confidence = conf;
            claim.confidence.band = confidenceToBand(conf);
            claim.status = st;
            {
                KnowledgeEvidence ev;
                ev.evidence_id = KnowledgeEvidenceId("kevd_" + id);
                ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
                ev.source_summary_id = MemorySummaryId("sum_" + id);
                ev.supports_claim = true;
                claim.evidence_refs = std::vector<KnowledgeEvidence>{ev};
            }
            claim.created_tick = Tick(10);
            claim.updated_tick = Tick(10);
            if (st == KnowledgeStatus::Teachable) {
                claim.teaching_profile.teachable = true;
                claim.teaching_profile.teaching_message_key = "msg";
                claim.projection_flags.usable_for_teaching = true;
            }
            if (st == KnowledgeStatus::Active) {
                claim.projection_flags.usable_for_action = true;
            }
            return claim;
        };

        assert(repo.put(make_claim("know_c", KnowledgeStatus::Active, 0.60, "berry_red")).is_ok());
        assert(repo.put(make_claim("know_b", KnowledgeStatus::Teachable, 0.80, "berry_blue")).is_ok());
        assert(repo.put(make_claim("know_a", KnowledgeStatus::Hypothesis, 0.40, "mushroom")).is_ok());

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.include_inactive = true;
        query.limit = 10;

        auto repo_result = repo.query(query);
        assert(repo_result.is_ok());

        KnowledgeProjectionBuilder builder;
        auto proj = builder.buildProjection(repo_result.value(), query);
        assert(proj.is_ok());
        assert(proj.value().items.size() == 3);

        // Deterministic: Teachable > Active > Hypothesis
        assert(proj.value().items[0].knowledge_id.value() == "know_b");
        assert(proj.value().items[1].knowledge_id.value() == "know_c");
        assert(proj.value().items[2].knowledge_id.value() == "know_a");

        std::cout << "multiple_claims_projection_order passed" << std::endl;
    }

    std::cout << "knowledge_projection_flow tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_projection_flow_tests();
    return 0;
}
