#include "pathfinder/memory/memory_decay.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

MemoryRecord make_decay_record(const std::string& id, MemoryScope scope, MemoryLifecycle lifecycle, double strength, bool protect = false) {
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
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::CognitionEvidence;
    ref.source_event_id = EventId("evt_001");
    record.evidence_refs.push_back(ref);
    return record;
}

void run_memory_decay_tests() {
    // ============================================================
    // Short term decays
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.5);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.1;
        opts.fading_threshold = 0.20;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(3), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Decayed);
        assert(result.value().updated_records.size() == 1);
        assert(result.value().updated_records[0].strength < 0.5);
    }

    // ============================================================
    // Protected skipped
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.5, true);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.1;

        auto result = service.applyDecay(records, Tick(10), opts);
        assert(result.is_ok());
        assert(result.value().updated_records.empty());
        assert(!result.value().event_drafts.empty());
        assert(result.value().event_drafts[0].decay_decision == MemoryDecayDecision::ProtectedSkipped);
    }

    // ============================================================
    // LongTerm default skipped
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_record("mem_001", MemoryScope::LongTerm, MemoryLifecycle::Active, 0.5);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.long_term_decay_per_tick = 0.01;

        auto result = service.applyDecay(records, Tick(100), opts);
        assert(result.is_ok());
        assert(result.value().updated_records.empty());
    }

    // ============================================================
    // Forgotten threshold
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.15);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.1;
        opts.fading_threshold = 0.12;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(2), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Forgotten);
        assert(result.value().forgotten_memory_ids.size() == 1);
        assert(result.value().updated_records[0].lifecycle == MemoryLifecycle::Forgotten);
    }

    // ============================================================
    // Fading threshold
    // ============================================================
    {
        MemoryDecayService service;
        auto record = make_decay_record("mem_001", MemoryScope::ShortTerm, MemoryLifecycle::Active, 0.30);
        std::vector<MemoryRecord> records = {record};
        MemoryDecayOptions opts;
        opts.short_term_decay_per_tick = 0.1;
        opts.fading_threshold = 0.20;
        opts.forgotten_threshold = 0.05;

        auto result = service.applyDecay(records, Tick(2), opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryDecayDecision::Faded);
        assert(result.value().updated_records[0].lifecycle == MemoryLifecycle::Fading);
    }

    std::cout << "memory_decay tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_decay_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "decay" || mode == "short_term" || mode == "protected_skipped" || mode == "forgotten") {
        run_memory_decay_tests();
        return 0;
    }
    return 1;
}
