#include "pathfinder/memory/memory_decay.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

MemoryRecord make_decay_flow_record(const std::string& id, MemoryScope scope, MemoryLifecycle lifecycle, double strength, bool protect = false) {
    MemoryRecord record;
    record.memory_id = MemoryId(id);
    record.owner.kind = MemoryOwnerKind::Agent;
    record.owner.entity_id = EntityId("agent_001");
    record.subject.kind = MemorySubjectKind::ObjectDefinition;
    record.subject.subject_id = "berry_red";
    record.memory_kinds = {MemoryKind::Experiment};
    record.scope = scope;
    record.lifecycle = lifecycle;
    record.importance = MemoryImportance::Normal;
    record.strength = strength;
    record.protect_from_decay = protect;
    record.created_tick = Tick(1);
    record.last_touched_tick = Tick(1);
    return record;
}

void run_p16_memory_decay_flow_tests() {
    // ============================================================
    // ShortTerm decays over ticks
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_flow_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.5);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.05;
        opts.fading_threshold = 0.20;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(5), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Decayed);
        assert(result.value().updated_records[0].strength < 0.5);
    }

    // ============================================================
    // Fading lifecycle
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_flow_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.30);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.05;
        opts.fading_threshold = 0.20;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(5), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Faded);
        assert(result.value().updated_records[0].lifecycle == MemoryLifecycle::Fading);
    }

    // ============================================================
    // Forgotten lifecycle
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_flow_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.15);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.05;
        opts.fading_threshold = 0.12;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(5), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Forgotten);
        assert(result.value().updated_records[0].lifecycle == MemoryLifecycle::Forgotten);
        assert(result.value().forgotten_memory_ids.size() == 1);
    }

    // ============================================================
    // Protected memory not decayed
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_flow_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.5, true);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.5;
        opts.fading_threshold = 0.20;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(10), opts);
        assert(result.is_ok());
        assert(result.value().updated_records.empty());
    }

    // ============================================================
    // LongTerm default not decayed
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_flow_record("mem_001", MemoryScope::LongTerm, MemoryLifecycle::Active, 0.5);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.long_term_decay_per_tick = 0.01;

        auto result = service.applyDecay(records, Tick(100), opts);
        assert(result.is_ok());
        assert(result.value().updated_records.empty());
    }

    std::cout << "p16_memory_decay_flow tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_p16_memory_decay_flow_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "decay_flow") {
        run_p16_memory_decay_flow_tests();
        return 0;
    }
    return 1;
}
