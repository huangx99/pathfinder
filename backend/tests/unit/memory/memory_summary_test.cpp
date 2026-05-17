#include "pathfinder/memory/memory_summary.h"
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

void run_memory_summary_tests() {
    // ============================================================
    // Enum roundtrips
    // ============================================================
    {
        assert(toString(MemorySummaryKind::OwnerSubjectPattern) == "owner_subject_pattern");
        assert(memorySummaryKindFromString("owner_subject_pattern").value() == MemorySummaryKind::OwnerSubjectPattern);
        assert(memorySummaryKindFromString("invalid").is_error());

        assert(toString(MemoryCompressionLevel::LightSummary) == "light_summary");
        assert(memoryCompressionLevelFromString("light_summary").value() == MemoryCompressionLevel::LightSummary);
        assert(memoryCompressionLevelFromString("invalid").is_error());

        assert(toString(MemoryCompressionDecision::CreatedSummary) == "created_summary");
        assert(memoryCompressionDecisionFromString("created_summary").value() == MemoryCompressionDecision::CreatedSummary);
        assert(memoryCompressionDecisionFromString("invalid").is_error());

        assert(toString(MemoryRecallMode::ExactSubject) == "exact_subject");
        assert(memoryRecallModeFromString("exact_subject").value() == MemoryRecallMode::ExactSubject);
        assert(memoryRecallModeFromString("invalid").is_error());

        assert(toString(MemoryRecallSort::StrengthDesc) == "strength_desc");
        assert(memoryRecallSortFromString("strength_desc").value() == MemoryRecallSort::StrengthDesc);
        assert(memoryRecallSortFromString("invalid").is_error());

        assert(toString(MemoryRecallItemKind::Record) == "record");
        assert(memoryRecallItemKindFromString("record").value() == MemoryRecallItemKind::Record);
        assert(memoryRecallItemKindFromString("invalid").is_error());
    }

    // ============================================================
    // MemorySummaryKey validateBasic
    // ============================================================
    {
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
        assert(key.validateBasic().is_ok());

        key.summary_kind = MemorySummaryKind::Unknown;
        assert(key.validateBasic().is_error());

        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
        key.memory_kinds = {MemoryKind::Unknown};
        assert(key.validateBasic().is_error());
    }

    // ============================================================
    // MemorySummary validateBasic
    // ============================================================
    {
        auto summary = makeValidSummary("sum_001");
        assert(summary.validateBasic().is_ok());

        summary.source_memory_ids.clear();
        assert(summary.validateBasic().is_error()); // empty source_memory_ids

        summary = makeValidSummary("sum_002");
        summary.representative_memory_ids = {MemoryId("mem_other")};
        assert(summary.validateBasic().is_error()); // rep not in source

        summary = makeValidSummary("sum_003");
        summary.schema_version = "v2";
        assert(summary.validateBasic().is_error());

        summary = makeValidSummary("sum_004");
        summary.compression_level = MemoryCompressionLevel::Unknown;
        assert(summary.validateBasic().is_error());

        summary = makeValidSummary("sum_005");
        summary.highest_importance = MemoryImportance::Unknown;
        assert(summary.validateBasic().is_error());

        summary = makeValidSummary("sum_006");
        summary.aggregate_strength = 1.5;
        assert(summary.validateBasic().is_error());

        summary = makeValidSummary("sum_007");
        summary.reason_keys = {"edible_profile"};
        assert(summary.validateBasic().is_error());
    }

    // ============================================================
    // MemorySummaryIdFactory deterministic
    // ============================================================
    {
        MemorySummaryIdFactory factory;
        MemorySummaryKey key;
        key.owner.kind = MemoryOwnerKind::Agent;
        key.owner.entity_id = EntityId("agent_001");
        key.subject.kind = MemorySubjectKind::ObjectDefinition;
        key.subject.subject_id = "berry_red";
        key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;

        auto id1 = factory.makeSummaryId(key, Tick(10), 0);
        auto id2 = factory.makeSummaryId(key, Tick(10), 0);
        assert(id1.is_ok());
        assert(id2.is_ok());
        assert(id1.value().value() == id2.value().value());

        auto id3 = factory.makeSummaryId(key, Tick(10), 1);
        assert(id3.is_ok());
        assert(id1.value().value() != id3.value().value());

        MemorySummaryKey bad_key;
        bad_key.summary_kind = MemorySummaryKind::Unknown;
        auto bad = factory.makeSummaryId(bad_key, Tick(0), 0);
        assert(bad.is_error());
    }

    // ============================================================
    // MemorySummaryStore
    // ============================================================
    {
        MemorySummaryStore store;
        auto s1 = makeValidSummary("sum_store_001");
        assert(store.put(s1).is_ok());
        assert(store.size() == 1);

        auto found = store.find(MemorySummaryId("sum_store_001"));
        assert(found.is_ok());
        assert(found.value().has_value());

        auto missing = store.find(MemorySummaryId("sum_missing"));
        assert(missing.is_ok());
        assert(!missing.value().has_value());

        auto list = store.listByOwner(s1.key.owner);
        assert(list.is_ok());
        assert(list.value().size() == 1);

        assert(store.remove(MemorySummaryId("sum_store_001")).is_ok());
        assert(store.size() == 0);

        store.clear();
    }

    // ============================================================
    // MemoryCompressionOptions validateBasic
    // ============================================================
    {
        MemoryCompressionOptions opts;
        assert(opts.validateBasic().is_ok());

        opts.min_records_to_summarize = 0;
        assert(opts.validateBasic().is_error());

        opts.min_records_to_summarize = 3;
        opts.max_source_records = 0;
        assert(opts.validateBasic().is_error());

        opts.max_source_records = 1000;
        opts.max_representative_records = 0;
        assert(opts.validateBasic().is_error());

        opts.max_representative_records = 5;
        opts.target_level = MemoryCompressionLevel::Unknown;
        assert(opts.validateBasic().is_error());

        opts.target_level = MemoryCompressionLevel::LightSummary;
        opts.max_source_records = 5;
        opts.max_representative_records = 10;
        assert(opts.validateBasic().is_error()); // representative > source
    }

    // ============================================================
    // MemoryCompressionPlan validateBasic
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
        assert(plan.validateBasic().is_ok());

        plan.source_memory_ids.clear();
        assert(plan.validateBasic().is_error());

        plan.source_memory_ids = {MemoryId("mem_001")};
        plan.representative_memory_ids = {MemoryId("mem_002")};
        assert(plan.validateBasic().is_error()); // representative not in source
    }

    // ============================================================
    // MemoryRecallQuery validateBasic
    // ============================================================
    {
        MemoryRecallQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        query.subject = MemorySubject{MemorySubjectKind::ObjectDefinition, "berry_red", {}, {}};
        query.mode = MemoryRecallMode::ExactSubject;
        assert(query.validateBasic().is_ok());

        query.mode = MemoryRecallMode::Unknown;
        assert(query.validateBasic().is_error());

        query.mode = MemoryRecallMode::ExactSubject;
        query.subject = std::nullopt;
        assert(query.validateBasic().is_error()); // ExactSubject requires subject

        query.subject = MemorySubject{MemorySubjectKind::ObjectDefinition, "berry_red", {}, {}};
        query.limit = 0;
        assert(query.validateBasic().is_error());

        query.limit = 20;
        query.include_records = false;
        query.include_summaries = false;
        assert(query.validateBasic().is_error());

        query.include_records = true;
        query.memory_kinds = {MemoryKind::Unknown};
        assert(query.validateBasic().is_error());
    }

    // ============================================================
    // MemorySummary validateBasic summary_id format
    // ============================================================
    {
        auto summary = makeValidSummary("sum_001");
        assert(summary.validateBasic().is_ok());

        summary.summary_id = MemorySummaryId("invalid id with spaces");
        assert(summary.validateBasic().is_error());
    }

    // ============================================================
    // MemoryRecallItem validateBasic
    // ============================================================
    {
        MemoryRecallItem item;
        item.item_kind = MemoryRecallItemKind::Record;
        item.record = makeValidRecord("mem_001", MemoryKind::Success, MemoryScope::MidTerm, MemoryLifecycle::Active, MemoryImportance::Normal, 0.5, Tick(10));
        item.relevance_score = 0.5;
        assert(item.validateBasic().is_ok());

        item.item_kind = MemoryRecallItemKind::Summary;
        assert(item.validateBasic().is_error()); // has record but kind is Summary

        item.record = std::nullopt;
        item.summary = makeValidSummary("sum_001");
        assert(item.validateBasic().is_ok());

        item.relevance_score = 1.5;
        assert(item.validateBasic().is_error());
    }

    std::cout << "memory_summary tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_summary_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "summary" || mode == "validate_basic") {
        run_memory_summary_tests();
        return 0;
    }
    return 1;
}
