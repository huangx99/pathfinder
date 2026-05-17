#include "pathfinder/memory/memory_writer.h"
#include "pathfinder/memory/memory_store.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;
using namespace pathfinder::cognition;

MemoryCandidate make_test_candidate(const std::string& candidate_id, MemoryImportance importance = MemoryImportance::Normal, bool protect = false) {
    MemoryCandidate candidate;
    candidate.candidate_id = candidate_id;
    candidate.owner.kind = MemoryOwnerKind::Agent;
    candidate.owner.entity_id = EntityId("agent_001");
    candidate.subject.kind = MemorySubjectKind::ObjectDefinition;
    candidate.subject.subject_id = "berry_red";
    candidate.memory_kinds = {MemoryKind::Experiment};
    candidate.source_kind = MemorySourceKind::CognitionEvidence;
    candidate.importance = importance;
    candidate.initial_strength = 0.35;
    candidate.protect_from_decay = protect;
    candidate.protect_from_forgetting = protect;
    candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
    candidate.evidence_ref.source_event_id = EventId("evt_001");
    candidate.evidence_ref.observed_tick = Tick(1);
    return candidate;
}

void run_memory_writer_tests() {
    // ============================================================
    // MemoryIdFactory deterministic (ISSUE-3: Result<MemoryId>)
    // ============================================================
    {
        MemoryIdFactory factory;
        MemoryOwner owner;
        owner.kind = MemoryOwnerKind::Agent;
        owner.entity_id = EntityId("agent_001");
        MemorySubject subject;
        subject.kind = MemorySubjectKind::ObjectDefinition;
        subject.subject_id = "berry_red";
        MemoryEvidenceRef ref;
        ref.source_kind = MemorySourceKind::CognitionEvidence;
        ref.source_event_id = EventId("evt_001");
        ref.observed_tick = Tick(10);

        auto id1 = factory.makeMemoryId(owner, subject, ref, 0);
        auto id2 = factory.makeMemoryId(owner, subject, ref, 0);
        assert(id1.is_ok());
        assert(id2.is_ok());
        assert(id1.value().value() == id2.value().value());
        assert(id1.value().value().find("mem_") == 0);
    }

    // ============================================================
    // MemoryIdFactory rejects invalid owner
    // ============================================================
    {
        MemoryIdFactory factory;
        MemoryOwner owner;
        owner.kind = MemoryOwnerKind::Unknown;
        MemorySubject subject;
        subject.kind = MemorySubjectKind::ObjectDefinition;
        subject.subject_id = "berry_red";
        MemoryEvidenceRef ref;
        ref.observed_tick = Tick(10);
        auto id = factory.makeMemoryId(owner, subject, ref, 0);
        assert(id.is_error());
    }

    // ============================================================
    // MemoryWriteService creates short term memory
    // ============================================================
    {
        MemoryWriteService writer;
        auto candidate = make_test_candidate("cand_001");
        auto result = writer.writeCandidate(candidate, {}, MemoryWriteOptions{});
        assert(result.is_ok());
        assert(result.value().decision == MemoryWriteDecision::Created);
        assert(result.value().created_records.size() == 1);
        assert(result.value().created_records[0].scope == MemoryScope::ShortTerm);
    }

    // ============================================================
    // MemoryWriteService reinforces existing memory
    // ============================================================
    {
        MemoryWriteService writer;
        auto candidate1 = make_test_candidate("cand_001");
        auto result1 = writer.writeCandidate(candidate1, {}, MemoryWriteOptions{});
        assert(result1.is_ok());

        auto existing = result1.value().created_records;
        auto candidate2 = make_test_candidate("cand_002");
        candidate2.evidence_ref.observed_tick = Tick(2);
        auto result2 = writer.writeCandidate(candidate2, existing, MemoryWriteOptions{});
        assert(result2.is_ok());
        assert(result2.value().decision == MemoryWriteDecision::Promoted || result2.value().decision == MemoryWriteDecision::Reinforced);
        assert(result2.value().updated_records[0].reinforcement_count == 1);
        assert(result2.value().updated_records[0].strength > 0.35);
    }

    // ============================================================
    // MemoryWriteService promotes short to mid
    // ============================================================
    {
        MemoryWriteService writer;
        MemoryWriteOptions opts;
        opts.short_to_mid_reinforcement_count = 2;

        auto candidate1 = make_test_candidate("cand_001");
        auto result1 = writer.writeCandidate(candidate1, {}, opts);
        auto existing = result1.value().created_records;

        auto candidate2 = make_test_candidate("cand_002");
        candidate2.evidence_ref.observed_tick = Tick(2);
        auto result2 = writer.writeCandidate(candidate2, existing, opts);
        existing = result2.value().updated_records;

        auto candidate3 = make_test_candidate("cand_003");
        candidate3.evidence_ref.observed_tick = Tick(3);
        auto result3 = writer.writeCandidate(candidate3, existing, opts);
        assert(result3.is_ok());
        assert(result3.value().decision == MemoryWriteDecision::Promoted);
        assert(result3.value().updated_records[0].scope == MemoryScope::MidTerm);
    }

    // ============================================================
    // MemoryWriteService promotes critical to long
    // ============================================================
    {
        MemoryWriteService writer;
        auto candidate = make_test_candidate("cand_crit", MemoryImportance::Critical, true);
        auto result = writer.writeCandidate(candidate, {}, MemoryWriteOptions{});
        assert(result.is_ok());
        assert(result.value().decision == MemoryWriteDecision::Created);
        assert(result.value().created_records[0].scope == MemoryScope::LongTerm);
        assert(result.value().created_records[0].protect_from_decay);
    }

    // ============================================================
    // BLOCKER-5: High-risk reinforcement protects existing memory
    // ============================================================
    {
        MemoryWriteService writer;
        auto normal_candidate = make_test_candidate("cand_normal", MemoryImportance::Normal, false);
        auto result1 = writer.writeCandidate(normal_candidate, {}, MemoryWriteOptions{});
        auto existing = result1.value().created_records;
        assert(!existing[0].protect_from_decay);

        auto critical_candidate = make_test_candidate("cand_crit", MemoryImportance::Critical, true);
        critical_candidate.evidence_ref.observed_tick = Tick(2);
        auto result2 = writer.writeCandidate(critical_candidate, existing, MemoryWriteOptions{});
        assert(result2.is_ok());
        assert(result2.value().updated_records[0].protect_from_decay);
        assert(result2.value().updated_records[0].lifecycle == MemoryLifecycle::Protected);
    }

    // ============================================================
    // MemoryCandidateFactory fromCognitionEvidence
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_001");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(5);

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionEvidence(evidence);
        assert(result.is_ok());
        assert(result.value().owner.kind == MemoryOwnerKind::Agent);
        assert(result.value().subject.kind == MemorySubjectKind::ObjectDefinition);
        assert(result.value().importance == MemoryImportance::Normal);
    }

    // ============================================================
    // BLOCKER-1/2: MemoryCandidateFactory rejects Unknown/TestOnly
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_bad");
        evidence.key.subject.kind = CognitionSubjectKind::Unknown;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionEvidence(evidence);
        assert(result.is_error()); // BLOCKER-1 fix: factory rejects Unknown
    }

    // ============================================================
    // BLOCKER-6: fromCognitionUpdate Created -> Discovery
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_create");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        CognitionUpdateResult update;
        update.decision = CognitionUpdateDecision::Created;
        update.after_claim.claim_id = CognitionClaimV2Id("claim_001");
        update.after_claim.key = evidence.key;
        update.after_claim.polarity = CognitionBeliefPolarity::Positive;
        update.after_claim.confidence = 0.5;
        update.after_claim.confidence_band = CognitionConfidenceBand::Actionable;

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionUpdate(update, evidence);
        assert(result.is_ok());
        bool has_discovery = false;
        for (const auto& mk : result.value().memory_kinds) {
            if (mk == MemoryKind::Discovery) has_discovery = true;
        }
        assert(has_discovery);
    }

    // ============================================================
    // BLOCKER-6: fromCognitionUpdate Teachable -> TeachingRelated
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_teach");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.9;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        CognitionUpdateResult update;
        update.decision = CognitionUpdateDecision::Reinforced;
        update.after_claim.claim_id = CognitionClaimV2Id("claim_001");
        update.after_claim.key = evidence.key;
        update.after_claim.polarity = CognitionBeliefPolarity::Positive;
        update.after_claim.confidence = 0.9;
        update.after_claim.confidence_band = CognitionConfidenceBand::Teachable;

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionUpdate(update, evidence);
        assert(result.is_ok());
        assert(result.value().teaching_candidate);
        bool has_teaching = false;
        for (const auto& mk : result.value().memory_kinds) {
            if (mk == MemoryKind::TeachingRelated) has_teaching = true;
        }
        assert(has_teaching);
    }

    // ============================================================
    // BLOCKER-RECHECK-1: Contradiction candidate updates old record
    //   Old Success record + new Failure+Contradiction candidate
    //   -> must update old record, not create new
    // ============================================================
    {
        MemoryWriteService writer;
        // Step 1: create old Success record
        auto old_candidate = make_test_candidate("cand_old");
        old_candidate.memory_kinds = {MemoryKind::Success};
        old_candidate.evidence_ref.observed_tick = Tick(1);
        auto result1 = writer.writeCandidate(old_candidate, {}, MemoryWriteOptions{});
        assert(result1.is_ok());
        assert(result1.value().decision == MemoryWriteDecision::Created);
        auto existing = result1.value().created_records;
        assert(existing[0].memory_kinds.size() == 1);
        assert(existing[0].memory_kinds[0] == MemoryKind::Success);
        assert(existing[0].contradiction_count == 0);

        // Step 2: new Failure+Contradiction candidate with same owner+subject
        auto new_candidate = make_test_candidate("cand_new");
        new_candidate.memory_kinds = {MemoryKind::Failure, MemoryKind::Contradiction};
        new_candidate.evidence_ref.observed_tick = Tick(2);
        auto result2 = writer.writeCandidate(new_candidate, existing, MemoryWriteOptions{});
        assert(result2.is_ok());
        assert(result2.value().decision == MemoryWriteDecision::Promoted || result2.value().decision == MemoryWriteDecision::Reinforced);
        assert(result2.value().updated_records.size() == 1);
        const auto& updated = result2.value().updated_records[0];

        // Must update the old record, not create a new one
        assert(updated.memory_id == existing[0].memory_id);
        // Contradiction count must be incremented
        assert(updated.contradiction_count == 1);
        // Contradiction kind must be appended
        bool has_contradiction = false;
        for (const auto& mk : updated.memory_kinds) {
            if (mk == MemoryKind::Contradiction) has_contradiction = true;
        }
        assert(has_contradiction);
        // Reason key must contain memory.contradiction_observed
        bool has_reason = false;
        for (const auto& rk : updated.reason_keys) {
            if (rk == "memory.contradiction_observed") has_reason = true;
        }
        assert(has_reason);
        // Reinforcement count must be incremented
        assert(updated.reinforcement_count == 1);
    }

    // ============================================================
    // BLOCKER-RECHECK-1: CognitionUpdate Conflicted -> contradiction
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_conflict");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Conflicts; // triggers Contradiction in evidence factory
        evidence.outcome_signal = CognitionOutcomeSignal::NeedWorsened; // Failure
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        CognitionUpdateResult update;
        update.decision = CognitionUpdateDecision::Conflicted;
        update.after_claim.claim_id = CognitionClaimV2Id("claim_001");
        update.after_claim.key = evidence.key;
        update.after_claim.polarity = CognitionBeliefPolarity::Negative;
        update.after_claim.confidence = 0.5;
        update.after_claim.confidence_band = CognitionConfidenceBand::Hypothesis;

        MemoryCandidateFactory factory;
        auto result = factory.fromCognitionUpdate(update, evidence);
        assert(result.is_ok());
        bool has_contradiction = false;
        for (const auto& mk : result.value().memory_kinds) {
            if (mk == MemoryKind::Contradiction) has_contradiction = true;
        }
        assert(has_contradiction);
    }

    // ============================================================
    // MemoryWriteService rejects TestOnly when not allowed
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

    std::cout << "memory_writer tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_writer_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "writer" || mode == "creates_short_term" || mode == "reinforces_existing" ||
        mode == "promotes_short_to_mid" || mode == "promotes_critical_to_long") {
        run_memory_writer_tests();
        return 0;
    }
    return 1;
}
