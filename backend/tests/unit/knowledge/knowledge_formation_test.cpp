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
    summary.success_count = 4;
    summary.failure_count = 0;
    summary.warning_count = 0;
    summary.accident_count = 0;
    summary.contradiction_count = 0;
    summary.summarized_tick = Tick(100);
    return summary;
}

static MemoryRecord makeValidRecord(const std::string& mem_id) {
    MemoryRecord record;
    record.memory_id = MemoryId(mem_id);
    record.owner.kind = MemoryOwnerKind::Agent;
    record.owner.entity_id = EntityId("agent_001");
    record.subject.kind = MemorySubjectKind::ObjectDefinition;
    record.subject.subject_id = "berry_red";
    record.memory_kinds = {MemoryKind::Success};
    record.scope = MemoryScope::LongTerm;
    record.lifecycle = MemoryLifecycle::Active;
    record.importance = MemoryImportance::Normal;
    record.strength = 0.8;
    record.created_tick = Tick(10);
    record.last_touched_tick = Tick(20);
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    record.evidence_refs = {ref};
    return record;
}

void run_knowledge_formation_tests() {
    // ============================================================
    // Planner insufficient evidence -> fail
    // ============================================================
    {
        KnowledgeFormationPlanner planner;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.summary.source_memory_count = 1; // too few
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";

        auto result = planner.planFromMemorySummary(input);
        assert(result.is_error());
    }

    // ============================================================
    // Planner valid summary -> valid plan
    // ============================================================
    {
        KnowledgeFormationPlanner planner;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        input.effect_key = "restore_hunger";
        input.representative_records = {makeValidRecord("mem_001"), makeValidRecord("mem_002")};

        auto result = planner.planFromMemorySummary(input);
        assert(result.is_ok());
        auto& plan = result.value();
        assert(plan.evidence_refs.size() > 0);
        assert(plan.projected_status != KnowledgeStatus::Unknown);
        assert(plan.projected_confidence.confidence > 0.0);
    }

    // ============================================================
    // Service insufficient evidence -> Skipped valid result
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.summary.source_memory_count = 1;
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        assert(result.value().decision == KnowledgeFormationDecision::Skipped);
        assert(result.value().ok());
    }

    // ============================================================
    // Service creates valid claim from MemorySummary
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        input.effect_key = "restore_hunger";
        input.representative_records = {makeValidRecord("mem_001"), makeValidRecord("mem_002")};

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeFormationDecision::CreatedClaim);
        assert(res.claim.has_value());
        assert(res.claim->validateBasic().is_ok());

        // Check evidence chain
        assert(!res.claim->evidence_refs.empty());
        bool has_summary_evidence = false;
        bool has_record_evidence = false;
        for (const auto& ev : res.claim->evidence_refs) {
            if (ev.evidence_kind == KnowledgeEvidenceKind::MemorySummary) {
                has_summary_evidence = true;
                assert(!ev.source_summary_id.empty());
            }
            if (ev.evidence_kind == KnowledgeEvidenceKind::MemoryRecord) {
                has_record_evidence = true;
                assert(!ev.source_memory_ids.empty());
            }
        }
        assert(has_summary_evidence);
        assert(has_record_evidence);

        // Check status mapping
        assert(res.claim->status == KnowledgeStatus::Teachable ||
               res.claim->status == KnowledgeStatus::Active);

        // Check event drafts
        assert(!res.event_drafts.empty());
        assert(!res.state_changes.empty());
    }

    // ============================================================
    // Contradiction summary -> skipped by default
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.summary.contradiction_count = 1;
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        assert(result.value().decision == KnowledgeFormationDecision::Skipped);
    }

    // ============================================================
    // Risk knowledge with warnings -> penalty avoided for risk relation
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.summary.warning_count = 2;
        input.summary.accident_count = 1;
        input.target_relation = KnowledgeRelationType::HasRisk;
        input.action_key = "approach";

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        if (result.value().claim.has_value()) {
            // For risk relations, warnings don't penalize as heavily
            assert(result.value().claim->validateBasic().is_ok());
        }
    }

    std::cout << "knowledge_formation tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_formation_tests();
    return 0;
}
