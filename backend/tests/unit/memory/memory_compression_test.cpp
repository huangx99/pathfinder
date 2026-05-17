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

void run_memory_compression_tests() {
    // ============================================================
    // MemoryCompressionPlanner fails on insufficient records
    // ============================================================
    {
        MemoryCompressionPlanner planner;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));

        MemoryCompressionOptions opts;
        opts.min_records_to_summarize = 3;
        auto plan_result = planner.planForOwnerSubject(records, key, opts);
        assert(plan_result.is_error());
    }

    // ============================================================
    // MemoryCompressionPlanner empty records does not return invalid ok plan
    // ============================================================
    {
        MemoryCompressionPlanner planner;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> empty_records;
        auto plan_result = planner.planForOwnerSubject(empty_records, key);
        assert(plan_result.is_error());
    }

    // ============================================================
    // MemoryCompressionPlanner filters owner+subject
    // ============================================================
    {
        MemoryCompressionPlanner planner;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12)));
        // Different subject
        MemoryRecord other = makeValidRecord("mem_004", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(13));
        other.subject.subject_id = "berry_blue";
        records.push_back(other);
        // Different owner
        MemoryRecord other2 = makeValidRecord("mem_005", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(14));
        other2.owner.entity_id = EntityId("agent_002");
        records.push_back(other2);

        auto plan_result = planner.planForOwnerSubject(records, key);
        assert(plan_result.is_ok());
        assert(plan_result.value().source_memory_ids.size() == 3);
    }

    // ============================================================
    // MemoryCompressionPlanner skips ShortTerm by default
    // ============================================================
    {
        MemoryCompressionPlanner planner;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        std::vector<MemoryRecord> records;
        records.push_back(makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::ShortTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10)));
        records.push_back(makeValidRecord("mem_002", MemoryKind::Success, MemoryScope::ShortTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(11)));
        records.push_back(makeValidRecord("mem_003", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(12)));

        auto plan_result = planner.planForOwnerSubject(records, key);
        assert(plan_result.is_error()); // Only 1 MidTerm record remains, < default min (3)

        MemoryCompressionOptions opts;
        opts.include_short_term = true;
        auto plan_result2 = planner.planForOwnerSubject(records, key, opts);
        assert(plan_result2.is_ok());
        assert(plan_result2.value().source_memory_ids.size() == 3);
    }

    // ============================================================
    // MemoryCompressionService creates summary
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
        records.push_back(makeValidRecord("mem_003", MemoryKind::Failure, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Important, 0.7, Tick(12)));
        records.push_back(makeValidRecord("mem_004", MemoryKind::Warning, MemoryScope::LongTerm, MemoryLifecycle::Protected, MemoryImportance::Critical, 0.8, Tick(13)));

        auto result = service.compress(records, key);
        assert(result.is_ok());
        assert(result.value().decision == MemoryCompressionDecision::CreatedSummary);
        assert(result.value().summary.has_value());
        assert(result.value().summary->source_memory_count == 4);
        assert(result.value().summary->success_count == 2);
        assert(result.value().summary->failure_count == 1);
        assert(result.value().summary->warning_count == 1);
        assert(result.value().summary->has_protected_source == true);
        assert(result.value().summary->has_long_term_source == true);
        assert(result.value().summary->representative_memory_ids.size() <= 5);
        assert(!result.value().event_drafts.empty());
        assert(!result.value().state_changes.empty());
    }

    // ============================================================
    // MemoryCompressionService skips with insufficient records
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

        MemoryCompressionOptions opts;
        opts.min_records_to_summarize = 3;
        auto result = service.compress(records, key, opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryCompressionDecision::Skipped);
        assert(!result.value().plan.has_value());
        assert(result.value().validateBasic().is_ok());
    }

    // ============================================================
    // MemoryCompressionService preserves distinct cognition evidence
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
        for (int i = 0; i < 3; ++i) {
            MemoryRecord record;
            record.memory_id = MemoryId("mem_00" + std::to_string(i + 1));
            record.owner.kind = MemoryOwnerKind::Agent;
            record.owner.entity_id = EntityId("agent_001");
            record.subject.kind = MemorySubjectKind::ObjectDefinition;
            record.subject.subject_id = "berry_red";
            record.memory_kinds = {MemoryKind::Success};
            record.scope = MemoryScope::MidTerm;
            record.lifecycle = MemoryLifecycle::Active;
            record.importance = MemoryImportance::Normal;
            record.strength = 0.5;
            record.created_tick = Tick(10 + i);
            record.last_touched_tick = Tick(10 + i);
            MemoryEvidenceRef ref;
            ref.source_kind = MemorySourceKind::CognitionEvidence;
            ref.cognition_evidence_id = pathfinder::cognition::CognitionEvidenceRecordId("cog_ev_00" + std::to_string(i + 1));
            record.evidence_refs.push_back(ref);
            records.push_back(record);
        }

        auto result = service.compress(records, key);
        assert(result.is_ok());
        assert(result.value().decision == MemoryCompressionDecision::CreatedSummary);
        assert(result.value().summary->evidence_refs.size() == 3);
    }

    // ============================================================
    // MemoryCompressionService skips when all evidence refs missing
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
        for (int i = 0; i < 3; ++i) {
            MemoryRecord record;
            record.memory_id = MemoryId("mem_00" + std::to_string(i + 1));
            record.owner.kind = MemoryOwnerKind::Agent;
            record.owner.entity_id = EntityId("agent_001");
            record.subject.kind = MemorySubjectKind::ObjectDefinition;
            record.subject.subject_id = "berry_red";
            record.memory_kinds = {MemoryKind::Success};
            record.scope = MemoryScope::MidTerm;
            record.lifecycle = MemoryLifecycle::Active;
            record.importance = MemoryImportance::Normal;
            record.strength = 0.5;
            record.created_tick = Tick(10 + i);
            record.last_touched_tick = Tick(10 + i);
            // no evidence_refs
            records.push_back(record);
        }

        auto result = service.compress(records, key);
        assert(result.is_ok());
        assert(result.value().decision == MemoryCompressionDecision::Skipped);
        assert(result.value().validateBasic().is_ok());
        assert(result.value().reason_keys[1] == "missing_evidence_refs");
    }

    std::cout << "memory_compression tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_compression_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "compression" || mode == "validate_basic") {
        run_memory_compression_tests();
        return 0;
    }
    return 1;
}
