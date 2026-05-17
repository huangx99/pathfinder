#include "pathfinder/memory/memory_store.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

MemoryRecord make_test_record(const std::string& id, MemoryLifecycle lifecycle = MemoryLifecycle::Active) {
    MemoryRecord record;
    record.memory_id = MemoryId(id);
    record.owner.kind = MemoryOwnerKind::Agent;
    record.owner.entity_id = EntityId("agent_001");
    record.subject.kind = MemorySubjectKind::ObjectDefinition;
    record.subject.subject_id = "berry_red";
    record.memory_kinds = {MemoryKind::Experiment};
    record.scope = MemoryScope::ShortTerm;
    record.lifecycle = lifecycle;
    record.importance = MemoryImportance::Normal;
    record.strength = 0.5;
    record.created_tick = Tick(1);
    record.last_touched_tick = Tick(1);
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::CognitionEvidence;
    ref.source_event_id = EventId("evt_001");
    record.evidence_refs.push_back(ref);
    return record;
}

void run_memory_store_tests() {
    // ============================================================
    // put / find
    // ============================================================
    {
        MemoryStore store;
        auto record = make_test_record("mem_001");
        assert(store.put(record).is_ok());
        auto found = store.find(MemoryId("mem_001"));
        assert(found.is_ok());
        assert(found.value().has_value());
        assert(found.value()->memory_id.value() == "mem_001");
    }

    // ============================================================
    // query default excludes Forgotten
    // ============================================================
    {
        MemoryStore store;
        store.put(make_test_record("mem_001", MemoryLifecycle::Active));
        store.put(make_test_record("mem_002", MemoryLifecycle::Forgotten));

        MemoryQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.limit = 50;

        auto result = store.query(query);
        assert(result.is_ok());
        assert(result.value().records.size() == 1);
        assert(result.value().records[0].memory_id.value() == "mem_001");
    }

    // ============================================================
    // query include_forgotten
    // ============================================================
    {
        MemoryStore store;
        store.put(make_test_record("mem_001", MemoryLifecycle::Active));
        store.put(make_test_record("mem_002", MemoryLifecycle::Forgotten));

        MemoryQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.include_forgotten = true;
        query.limit = 50;

        auto result = store.query(query);
        assert(result.is_ok());
        assert(result.value().records.size() == 2);
    }

    // ============================================================
    // remove
    // ============================================================
    {
        MemoryStore store;
        store.put(make_test_record("mem_001"));
        assert(store.remove(MemoryId("mem_001")).is_ok());
        auto found = store.find(MemoryId("mem_001"));
        assert(found.is_ok());
        assert(!found.value().has_value());
    }

    // ============================================================
    // size / clear
    // ============================================================
    {
        MemoryStore store;
        assert(store.size() == 0);
        store.put(make_test_record("mem_001"));
        assert(store.size() == 1);
        store.clear();
        assert(store.size() == 0);
    }

    std::cout << "memory_store tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_store_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "store" || mode == "put_find_query") {
        run_memory_store_tests();
        return 0;
    }
    return 1;
}
