#include "pathfinder/memory/memory_recall.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

static MemoryRecord makeValidRecord(const std::string& mem_id, MemoryKind kind, MemoryScope scope, MemoryLifecycle lifecycle, MemoryImportance importance, double strength, Tick tick, bool teaching = false) {
    MemoryRecord record;
    record.memory_id = MemoryId(mem_id);
    record.owner.kind = MemoryOwnerKind::Agent;
    record.owner.entity_id = EntityId("agent_001");
    record.subject.kind = MemorySubjectKind::ObjectDefinition;
    record.subject.subject_id = "berry_red";
    record.memory_kinds = {kind};
    record.scope = scope;
    record.lifecycle = lifecycle;
    record.importance = importance;
    record.strength = strength;
    record.created_tick = tick;
    record.last_touched_tick = tick;
    record.teaching_candidate = teaching;
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    record.evidence_refs.push_back(ref);
    return record;
}

static MemorySummary makeValidSummary(const std::string& sum_id, size_t teaching_count, size_t warning_count, size_t accident_count, size_t contradiction_count, bool has_long_term, double strength) {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId(sum_id);
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = "berry_red";
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = MemoryImportance::Normal;
    summary.aggregate_strength = strength;
    summary.source_memory_count = 1;
    summary.source_memory_ids = {MemoryId("mem_001")};
    summary.representative_memory_ids = {MemoryId("mem_001")};
    summary.teaching_candidate_count = teaching_count;
    summary.warning_count = warning_count;
    summary.accident_count = accident_count;
    summary.contradiction_count = contradiction_count;
    summary.has_long_term_source = has_long_term;
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    summary.evidence_refs = {ref};
    return summary;
}

void run_memory_recall_tests() {
    // ============================================================
    // ExactSubject recall
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));

        std::vector<MemorySummary> summaries;
        summaries.push_back(makeValidSummary("sum_001", 0, 0, 0, 0, false, 0.6));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.subject = MemorySubject{MemorySubjectKind::ObjectDefinition, "berry_red", {}, {}};
        query.mode = MemoryRecallMode::ExactSubject;

        auto result = service.recall(records, summaries, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
    }

    // ============================================================
    // TeachingCandidates recall
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10), false));
        records.push_back(makeValidRecord("mem_002", MemoryKind::TeachingRelated, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11), true));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12), false));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::TeachingCandidates;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 1);
        assert(result.value().items[0].record->memory_id.value() == "mem_002");
    }

    // ============================================================
    // ContradictionRelated recall
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        MemoryRecord contra = makeValidRecord("mem_002", MemoryKind::Contradiction, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11));
        contra.contradiction_count = 1;
        records.push_back(contra);

        std::vector<MemorySummary> summaries;
        summaries.push_back(makeValidSummary("sum_001", 0, 0, 0, 1, false, 0.6));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::ContradictionRelated;

        auto result = service.recall(records, summaries, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 2);
    }

    // ============================================================
    // RiskRelated recall
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Warning, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Critical, 0.5, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Accident, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Important, 0.5, Tick(12)));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::RiskRelated;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 2);
    }

    // ============================================================
    // Deterministic sorting
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.3, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.4, Tick(12)));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.sort = MemoryRecallSort::StrengthDesc;

        auto result1 = service.recall(records, {}, query);
        auto result2 = service.recall(records, {}, query);
        assert(result1.is_ok());
        assert(result2.is_ok());
        assert(result1.value().items.size() == result2.value().items.size());
        for (size_t i = 0; i < result1.value().items.size(); ++i) {
            assert(result1.value().items[i].record->memory_id.value() == result2.value().items[i].record->memory_id.value());
        }
        // StrengthDesc: mem_002 (0.5) first
        assert(result1.value().items[0].record->memory_id.value() == "mem_002");
    }

    // ============================================================
    // DeterministicKeyAsc orders IDs ascending
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_b", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));
        records.push_back(makeValidRecord("mem_a", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_c", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12)));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.sort = MemoryRecallSort::DeterministicKeyAsc;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
        assert(result.value().items[0].record->memory_id.value() == "mem_a");
        assert(result.value().items[1].record->memory_id.value() == "mem_b");
        assert(result.value().items[2].record->memory_id.value() == "mem_c");
    }

    // ============================================================
    // LastTouchedDesc orders ticks descending with ID tie-break
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12)));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.sort = MemoryRecallSort::LastTouchedDesc;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
        // Tick 12 first (desc), tie-break by ID asc
        assert(result.value().items[0].record->memory_id.value() == "mem_001");
        assert(result.value().items[1].record->memory_id.value() == "mem_003");
        assert(result.value().items[2].record->memory_id.value() == "mem_002");
    }

    // ============================================================
    // ImportanceDesc orders importance descending with ID tie-break
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        // Same tick to ensure same relevance_score, so tie-break by ID applies
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Critical, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Critical, 0.5, Tick(10)));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.sort = MemoryRecallSort::ImportanceDesc;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
        // Critical first (desc), tie-break by ID asc
        assert(result.value().items[0].record->memory_id.value() == "mem_001");
        assert(result.value().items[1].record->memory_id.value() == "mem_003");
        assert(result.value().items[2].record->memory_id.value() == "mem_002");
    }

    // ============================================================
    // EvidenceCountDesc orders count descending with ID tie-break
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        // Same tick to ensure same relevance_score, so tie-break by ID applies
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        MemoryRecord r1 = makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10));
        MemoryEvidenceRef extra;
        extra.source_kind = MemorySourceKind::DirectEvent;
        extra.source_event_id = EventId("evt_extra");
        r1.evidence_refs.push_back(extra);
        records.push_back(r1);
        MemoryRecord r3 = makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10));
        r3.evidence_refs.push_back(extra);
        records.push_back(r3);

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.sort = MemoryRecallSort::EvidenceCountDesc;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
        // 2 evidence refs first (desc), tie-break by ID asc
        assert(result.value().items[0].record->memory_id.value() == "mem_001");
        assert(result.value().items[1].record->memory_id.value() == "mem_003");
        assert(result.value().items[2].record->memory_id.value() == "mem_002");
    }

    // ============================================================
    // Limit truncation
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        for (int i = 0; i < 10; ++i) {
            records.push_back(makeValidRecord("mem_0" + std::to_string(i), MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(i)));
        }

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::OwnerRecent;
        query.limit = 5;

        auto result = service.recall(records, {}, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 5);
        assert(result.value().truncated == true);
    }

    std::cout << "memory_recall tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_recall_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "recall" || mode == "validate_basic") {
        run_memory_recall_tests();
        return 0;
    }
    return 1;
}
