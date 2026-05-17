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

static KnowledgeOwner makeValidOwner() {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId("agent_001");
    return owner;
}

void run_knowledge_from_memory_summary_flow_tests() {
    // ============================================================
    // 3 stable memories -> P17 MemorySummary -> P18 KnowledgeClaim
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        input.effect_key = "restore_hunger";
        input.representative_records = {
            makeValidRecord("mem_001"),
            makeValidRecord("mem_002"),
            makeValidRecord("mem_003")
        };

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeFormationDecision::CreatedClaim);
        assert(res.claim.has_value());

        // Store in repository
        KnowledgeRepository repo;
        assert(repo.put(res.claim.value()).is_ok());

        auto found = repo.find(res.claim->knowledge_id);
        assert(found.is_ok());
        assert(found.value().has_value());

        std::cout << "knowledge_from_memory_summary_flow passed" << std::endl;
    }

    // ============================================================
    // KnowledgeClaim retains source_summary_id, source_memory_ids, memory_evidence_refs
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        input.representative_records = {makeValidRecord("mem_001")};

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        auto& claim = result.value().claim.value();

        bool found_summary_id = false;
        bool found_memory_ids = false;
        bool found_evidence_refs = false;

        for (const auto& ev : claim.evidence_refs) {
            if (ev.evidence_kind == KnowledgeEvidenceKind::MemorySummary) {
                assert(ev.source_summary_id.value() == "sum_001");
                found_summary_id = true;
            }
            if (ev.evidence_kind == KnowledgeEvidenceKind::MemoryRecord) {
                for (const auto& mid : ev.source_memory_ids) {
                    if (mid.value() == "mem_001") {
                        found_memory_ids = true;
                    }
                }
            }
            if (!ev.memory_evidence_refs.empty()) {
                found_evidence_refs = true;
            }
        }

        assert(found_summary_id);
        assert(found_memory_ids);
        assert(found_evidence_refs);

        std::cout << "knowledge_retains_evidence_chain passed" << std::endl;
    }

    // ============================================================
    // Teachable knowledge -> projection query
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

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::Teachable;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto proj_result = builder.buildProjection(repo.query(query).value(), query);
        assert(proj_result.is_ok());

        std::cout << "knowledge_teachable_projection passed" << std::endl;
    }

    // ============================================================
    // Risk knowledge teachable must have risk_warning_required
    // ============================================================
    {
        KnowledgeFormationService service;
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasRisk;
        input.action_key = "approach";
        input.effect_key = "poison";

        auto result = service.formFromMemorySummary(input);
        assert(result.is_ok());
        if (result.value().claim.has_value()) {
            auto& claim = result.value().claim.value();
            if (claim.status == KnowledgeStatus::Teachable) {
                assert(claim.teaching_profile.risk_warning_required);
            }
        }

        std::cout << "risk_knowledge_warning_required passed" << std::endl;
    }

    std::cout << "knowledge_from_memory_summary_flow tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_from_memory_summary_flow_tests();
    return 0;
}
