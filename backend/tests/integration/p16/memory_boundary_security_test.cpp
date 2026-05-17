#include "pathfinder/memory/memory_writer.h"
#include "pathfinder/memory/memory_store.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

void run_p16_memory_boundary_security_tests() {
    // ============================================================
    // subject_id contains GameState -> fail
    // ============================================================
    {
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_001";
        candidate.owner.kind = MemoryOwnerKind::Agent;
        candidate.owner.entity_id = EntityId("agent_001");
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "GameState_raw";
        candidate.memory_kinds = {MemoryKind::Experiment};
        candidate.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
        assert(candidate.validateBasic().is_error());
    }

    // ============================================================
    // reason_keys contains edible_profile -> fail
    // ============================================================
    {
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_001";
        candidate.owner.kind = MemoryOwnerKind::Agent;
        candidate.owner.entity_id = EntityId("agent_001");
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "unknown_fruit";
        candidate.memory_kinds = {MemoryKind::Experiment};
        candidate.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.reason_keys = {"something", "edible_profile"};
        assert(candidate.validateBasic().is_error());
    }

    // ============================================================
    // warning_keys contains hunger_delta -> fail
    // ============================================================
    {
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_001";
        candidate.owner.kind = MemoryOwnerKind::Agent;
        candidate.owner.entity_id = EntityId("agent_001");
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "unknown_fruit";
        candidate.memory_kinds = {MemoryKind::Experiment};
        candidate.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.warning_keys = {"hunger_delta"};
        assert(candidate.validateBasic().is_error());
    }

    // ============================================================
    // safe_tags contains ObjectDefinition hidden -> fail
    // ============================================================
    {
        MemorySubject subject;
        subject.kind = MemorySubjectKind::ObjectDefinition;
        subject.subject_id = "unknown_fruit";
        subject.safe_tags = {"ObjectDefinition hidden truth"};
        assert(subject.validateBasic().is_error());
    }

    // ============================================================
    // MemoryCandidateFactory rejects Unknown/TestOnly default
    // ============================================================
    {
        pathfinder::cognition::CognitionEvidenceRecord evidence;
        evidence.evidence_id = pathfinder::cognition::CognitionEvidenceRecordId("ev_001");
        evidence.key.subject.kind = pathfinder::cognition::CognitionSubjectKind::Unknown;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = pathfinder::cognition::CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = pathfinder::cognition::CognitionActionContextKind::Eat;
        evidence.key.aspect = pathfinder::cognition::CognitionAspect::Edibility;
        evidence.source_kind = pathfinder::cognition::CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = pathfinder::cognition::CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = pathfinder::cognition::CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = pathfinder::cognition::CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionEvidence(evidence);
        assert(result.is_error()); // BLOCKER-1 fix: factory rejects Unknown
    }

    // ============================================================
    // BLOCKER-4: case-insensitive hidden truth rejection
    // ============================================================
    {
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_001";
        candidate.owner.kind = MemoryOwnerKind::Agent;
        candidate.owner.entity_id = EntityId("agent_001");
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "unknown_fruit";
        candidate.memory_kinds = {MemoryKind::Experiment};
        candidate.source_kind = MemorySourceKind::CognitionEvidence;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;

        candidate.evidence_ref.source_event_id = EventId("evt_001"); // satisfy evidence ref validation

        candidate.reason_keys = {"gamestate"};
        assert(candidate.validateBasic().is_error());

        candidate.reason_keys = {"GAMESTATE"};
        assert(candidate.validateBasic().is_error());

        candidate.reason_keys = {"Health_Delta"};
        assert(candidate.validateBasic().is_error());

        candidate.reason_keys = {"HIDDEN_TRUTH"};
        assert(candidate.validateBasic().is_error());
    }

    // ============================================================
    // MemoryWriteOptions allow_test_only=false rejects TestOnly
    // ============================================================
    {
        MemoryWriteService writer;
        MemoryCandidate candidate;
        candidate.candidate_id = "cand_test";
        candidate.owner.kind = MemoryOwnerKind::TestOnly;
        candidate.owner.external_key = "test_key";
        candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
        candidate.subject.subject_id = "test_obj";
        candidate.memory_kinds = {MemoryKind::TestOnly};
        candidate.source_kind = MemorySourceKind::TestOnly;
        candidate.importance = MemoryImportance::Normal;
        candidate.initial_strength = 0.5;
        candidate.evidence_ref.source_kind = MemorySourceKind::TestOnly;

        MemoryWriteOptions opts;
        opts.allow_test_only = false;
        auto result = writer.writeCandidate(candidate, {}, opts);
        assert(result.is_ok());
        assert(result.value().decision == MemoryWriteDecision::Rejected);
    }

    std::cout << "p16_memory_boundary_security tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_p16_memory_boundary_security_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "hidden_truth" || mode == "raw_state" || mode == "test_only_gate") {
        run_p16_memory_boundary_security_tests();
        return 0;
    }
    return 1;
}
