#include "pathfinder/memory/memory_record.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

void run_memory_record_tests() {
    // ============================================================
    // MemoryOwner validateBasic
    // ============================================================
    {
        MemoryOwner owner;
        owner.kind = MemoryOwnerKind::Unknown;
        assert(owner.validateBasic().is_error());

        owner.kind = MemoryOwnerKind::Actor;
        owner.entity_id = EntityId("actor_001");
        assert(owner.validateBasic().is_ok());

        owner.kind = MemoryOwnerKind::Tribe;
        owner.entity_id = EntityId();
        assert(owner.validateBasic().is_error());

        owner.tribe_id = TribeId("tribe_001");
        assert(owner.validateBasic().is_ok());
    }

    // ============================================================
    // MemorySubject validateBasic
    // ============================================================
    {
        MemorySubject subject;
        subject.kind = MemorySubjectKind::Unknown;
        assert(subject.validateBasic().is_error());

        subject.kind = MemorySubjectKind::ObjectDefinition;
        subject.subject_id = "unknown_fruit";
        assert(subject.validateBasic().is_ok());

        subject.subject_id = "edible_profile";
        assert(subject.validateBasic().is_error());

        subject.subject_id = "unknown_fruit";
        subject.safe_tags = {"hunger_delta"};
        assert(subject.validateBasic().is_error());
    }

    // ============================================================
    // MemoryCandidate validateBasic
    // ============================================================
    {
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_001";
        candidate.owner.kind = MemoryOwnerKind::Agent;
        candidate.owner.entity_id = EntityId("agent_001");
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "berry_red";
        candidate.memory_kinds = {MemoryKind::Experiment};
        candidate.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.evidence_ref.source_event_id = EventId("evt_001");
        assert(candidate.validateBasic().is_ok());

        candidate.initial_strength = 1.5;
        assert(candidate.validateBasic().is_error());

        candidate.initial_strength = 0.5;
        candidate.memory_kinds = {MemoryKind::Unknown};
        assert(candidate.validateBasic().is_error());
    }

    // ============================================================
    // MemoryRecord validateBasic
    // ============================================================
    {
        MemoryRecord record;
        record.memory_id = MemoryId("mem_001");
        record.owner.kind = MemoryOwnerKind::Agent;
        record.owner.entity_id = EntityId("agent_001");
        record.subject.kind = MemorySubjectKind::ObjectDefinition;
        record.subject.subject_id = "berry_red";
        record.memory_kinds = {MemoryKind::Experiment};
        record.scope = MemoryScope::ShortTerm;
        record.lifecycle = MemoryLifecycle::Active;
        record.importance = MemoryImportance::Normal;
        record.strength = 0.5;
        record.created_tick = Tick(10);
        record.last_touched_tick = Tick(10);
        MemoryEvidenceRef ref;
        ref.source_kind = MemorySourceKind::CognitionEvidence;
        ref.source_event_id = EventId("evt_001");
        record.evidence_refs.push_back(ref);
        assert(record.validateBasic().is_ok());

        record.strength = -0.1;
        assert(record.validateBasic().is_error());

        record.strength = 0.5;
        record.schema_version = "v2";
        assert(record.validateBasic().is_error());
    }

    // ============================================================
    // MemoryWriteOptions validateBasic
    // ============================================================
    {
        MemoryWriteOptions opts;
        assert(opts.validateBasic().is_ok());

        opts.max_candidates = 0;
        assert(opts.validateBasic().is_error());
    }

    // ============================================================
    // MemoryDecayOptions validateBasic
    // ============================================================
    {
        MemoryDecayOptions opts;
        assert(opts.validateBasic().is_ok());

        opts.fading_threshold = 1.5;
        assert(opts.validateBasic().is_error());

        opts.fading_threshold = 0.2;
        opts.forgotten_threshold = 0.3;
        assert(opts.validateBasic().is_error()); // forgotten > fading
    }

    // ============================================================
    // MemoryWriteOptions threshold ordering
    // ============================================================
    {
        MemoryWriteOptions opts;
        opts.short_to_mid_strength_threshold = 0.7;
        opts.mid_to_long_strength_threshold = 0.6;
        assert(opts.validateBasic().is_error()); // short_to_mid > mid_to_long
    }

    // ============================================================
    // MemoryRecord empty evidence_refs fails
    // ============================================================
    {
        MemoryRecord record;
        record.memory_id = MemoryId("mem_001");
        record.owner.kind = MemoryOwnerKind::Agent;
        record.owner.entity_id = EntityId("agent_001");
        record.subject.kind = MemorySubjectKind::ObjectDefinition;
        record.subject.subject_id = "berry_red";
        record.memory_kinds = {MemoryKind::Experiment};
        record.scope = MemoryScope::ShortTerm;
        record.lifecycle = MemoryLifecycle::Active;
        record.importance = MemoryImportance::Normal;
        record.strength = 0.5;
        record.created_tick = Tick(10);
        record.last_touched_tick = Tick(10);
        // No evidence_refs
        assert(record.validateBasic().is_error());
    }

    // ============================================================
    // MemoryEvidenceRef missing traceable source fails
    // ============================================================
    {
        MemoryEvidenceRef ref;
        ref.source_kind = MemorySourceKind::CognitionEvidence;
        // Missing both cognition_evidence_id and source_event_id
        assert(ref.validateBasic().is_error());
    }

    // ============================================================
    // MemoryQuery validateBasic
    // ============================================================
    {
        MemoryQuery query;
        query.owner.kind = MemoryOwnerKind::Agent;
        query.owner.entity_id = EntityId("agent_001");
        assert(query.validateBasic().is_ok());

        query.limit = 0;
        assert(query.validateBasic().is_error());
    }

    std::cout << "memory_record tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_record_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "record" || mode == "validate_basic") {
        run_memory_record_tests();
        return 0;
    }
    return 1;
}
