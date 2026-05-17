#include "pathfinder/memory/memory_compression.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

static MemoryRecord makeValidRecord(const std::string& mem_id, MemoryKind kind, MemoryScope scope, MemoryLifecycle lifecycle, MemoryImportance importance, double strength, Tick tick) {
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
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    record.evidence_refs.push_back(ref);
    return record;
}

void run_memory_compression_flow_tests() {
    // ============================================================
    // 3 MidTerm/LongTerm memories -> MemorySummary
    // ============================================================
    {
        MemoryCompressionService service;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.6, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Failure, MemoryScope::LongTerm, MemoryLifecycle::Active, MemoryImportance::Important, 0.7, Tick(12)));

        auto result = service.compress(records, key);
        assert(result.is_ok());
        assert(result.value().decision == MemoryCompressionDecision::CreatedSummary);
        assert(result.value().summary.has_value());
        assert(result.value().summary->source_memory_count == 3);
        assert(result.value().summary->source_memory_ids.size() == 3);
        assert(!result.value().summary->representative_memory_ids.empty());
        assert(result.value().summary->success_count == 2);
        assert(result.value().summary->failure_count == 1);
    }

    // ============================================================
    // Original MemoryRecord not deleted
    // ============================================================
    {
        MemoryCompressionService service;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.6, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.7, Tick(12)));

        auto result = service.compress(records, key);
        assert(result.is_ok());
        // Input records still present and unchanged
        assert(records.size() == 3);
        assert(records[0].memory_id.value() == "mem_001");
        assert(records[1].memory_id.value() == "mem_002");
        assert(records[2].memory_id.value() == "mem_003");
    }

    // ============================================================
    // Protected records participate but not deleted
    // ============================================================
    {
        MemoryCompressionService service;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.6, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Warning, MemoryScope::LongTerm, MemoryLifecycle::Protected, MemoryImportance::Critical, 0.8, Tick(12)));

        auto result = service.compress(records, key);
        assert(result.is_ok());
        assert(result.value().summary->has_protected_source == true);
        assert(records[2].lifecycle == MemoryLifecycle::Protected);
    }

    std::cout << "memory_compression_flow tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_compression_flow_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "compression_flow" || mode == "integration") {
        run_memory_compression_flow_tests();
        return 0;
    }
    return 1;
}
