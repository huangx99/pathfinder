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

static MemorySummary makeValidSummary(const std::string& sum_id, const std::string& subject_id, size_t teaching_count, size_t warning_count, size_t accident_count, size_t contradiction_count, bool has_long_term, double strength) {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId(sum_id);
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = subject_id;
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

void run_memory_recall_flow_tests() {
    // ============================================================
    // ExactSubject can recall records and summaries
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));

        std::vector<MemorySummary> summaries;
        summaries.push_back(makeValidSummary("sum_001", "berry_red", 0, 0, 0, 0, false, 0.6));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.subject = MemorySubject{MemorySubjectKind::ObjectDefinition, "berry_red", {}, {}};
        query.mode = MemoryRecallMode::ExactSubject;

        auto result = service.recall(records, summaries, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 3);
        bool has_record = false;
        bool has_summary = false;
        for (const auto& item : result.value().items) {
            if (item.item_kind == MemoryRecallItemKind::Record) has_record = true;
            if (item.item_kind == MemoryRecallItemKind::Summary) has_summary = true;
        }
        assert(has_record);
        assert(has_summary);
    }

    // ============================================================
    // RiskRelated prioritizes Warning/Accident/Critical
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
    // TeachingCandidates recalls only teaching candidates
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10), false));
        records.push_back(makeValidRecord("mem_002", MemoryKind::TeachingRelated, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11), true));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12), false));

        std::vector<MemorySummary> summaries;
        summaries.push_back(makeValidSummary("sum_001", "berry_red", 1, 0, 0, 0, false, 0.6));
        summaries.push_back(makeValidSummary("sum_002", "berry_red", 0, 0, 0, 0, false, 0.5));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::TeachingCandidates;

        auto result = service.recall(records, summaries, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 2);
    }

    // ============================================================
    // ContradictionRelated recalls contradiction_count > 0
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        MemoryRecord contra = makeValidRecord("mem_002", MemoryKind::Contradiction, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11));
        contra.contradiction_count = 1;
        records.push_back(contra);

        std::vector<MemorySummary> summaries;
        summaries.push_back(makeValidSummary("sum_001", "berry_red", 0, 0, 0, 1, false, 0.6));
        summaries.push_back(makeValidSummary("sum_002", "berry_red", 0, 0, 0, 0, false, 0.5));

        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::ContradictionRelated;

        auto result = service.recall(records, summaries, query);
        assert(result.is_ok());
        assert(result.value().items.size() == 2);
    }

    // ============================================================
    // Sorting is stable and deterministic
    // ============================================================
    {
        MemoryRecallService service;
        std::vector<MemoryRecord> records;
        for (int i = 0; i < 5; ++i) {
            records.push_back(makeValidRecord("mem_00" + std::to_string(i), MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.1 * (i + 1), Tick(i)));
        }

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
    }

    std::cout << "memory_recall_flow tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_recall_flow_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "recall_flow" || mode == "integration") {
        run_memory_recall_flow_tests();
        return 0;
    }
    return 1;
}
