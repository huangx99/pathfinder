#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/memory/memory_compression.h"
#include "pathfinder/memory/memory_recall.h"
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

static MemorySummary makeValidSummary(const std::string& sum_id) {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId(sum_id);
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = "berry_red";
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = MemoryImportance::Normal;
    summary.aggregate_strength = 0.5;
    summary.source_memory_count = 1;
    summary.source_memory_ids = {MemoryId("mem_001")};
    summary.representative_memory_ids = {MemoryId("mem_001")};
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    summary.evidence_refs = {ref};
    return summary;
}

void run_p17_memory_boundary_security_tests() {
    // ============================================================
    // MemorySummary reason_keys contains edible_profile -> fail
    // ============================================================
    {
        auto summary = makeValidSummary("sum_001");
        summary.reason_keys = {"edible_profile"};
        assert(summary.validateBasic().is_error());
    }

    // ============================================================
    // MemorySummaryKey subject_type_key contains GameState -> fail
    // ============================================================
    {
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.subject.subject_type_key = "GameState";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
        assert(key.validateBasic().is_error());
    }

    // ============================================================
    // MemoryRecallQuery reason_keys contains raw_state -> fail
    // ============================================================
    {
        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.mode = MemoryRecallMode::ExactSubject;
        query.reason_keys = {"raw_state"};
        assert(query.validateBasic().is_error());
    }

    // ============================================================
    // MemoryCompressionPlan warning_keys contains SaveManager -> fail
    // ============================================================
    {
        MemoryCompressionPlan plan;
        plan.plan_key = "plan_001";
        plan.key.owner.kind = MemoryOwnerKind::Agent;
        plan.key.owner.entity_id = EntityId("agent_001");
        plan.key.subject.kind = MemorySubjectKind::ObjectDefinition;
        plan.key.subject.subject_id = "berry_red";
        plan.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
        plan.source_memory_ids = {MemoryId("mem_001")};
        plan.representative_memory_ids = {MemoryId("mem_001")};
        plan.target_level = MemoryCompressionLevel::LightSummary;
        plan.warning_keys = {"SaveManager"};
        assert(plan.validateBasic().is_error());
    }

    // ============================================================
    // TestOnly default not allowed
    // ============================================================
    {
        MemoryCompressionOptions opts;
        opts.allow_test_only = false;
        opts.target_level = MemoryCompressionLevel::TestOnly;
        assert(opts.validateBasic().is_error());

        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::TestOnly;
        assert(key.validateBasic().is_error());
    }

    // ============================================================
    // MemoryRecallItem reason_keys contains hidden truth -> fail
    // ============================================================
    {
        MemoryRecallItem item;
        item.item_kind = MemoryRecallItemKind::Record;
        item.record = makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10));
        item.relevance_score = 0.5;
        item.reason_keys = {"hunger_delta"};
        assert(item.validateBasic().is_error());
    }

    // ============================================================
    // MemoryRecallResult reason_keys / warning_keys hidden truth
    // ============================================================
    {
        MemoryRecallResult result;
        result.query.owner.kind = MemoryOwnerKind::Agent;
        result.query.owner.entity_id = EntityId("agent_001");
        result.query.mode = MemoryRecallMode::ExactSubject;
        result.reason_keys = {"hidden_truth"};
        assert(result.validateBasic().is_error());

        result.reason_keys.clear();
        result.warning_keys = {"GameState_raw"};
        assert(result.validateBasic().is_error());
    }

    std::cout << "p17_memory_boundary_security tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_p17_memory_boundary_security_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "hidden_truth" || mode == "raw_state" || mode == "test_only_gate") {
        run_p17_memory_boundary_security_tests();
        return 0;
    }
    return 1;
}
